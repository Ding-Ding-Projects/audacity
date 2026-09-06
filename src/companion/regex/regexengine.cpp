#include "shared/profilepaths.h"
/*
* Audacity: A Digital Audio Editor
*/

#include "regexengine.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QStandardPaths>

#include <algorithm>

using namespace au::companion;

namespace {
struct Token
{
    QString text;
    QString kind;
    QString description;
    int start = 0;
    int length = 0;
    int depth = 0;
    QVector<Token> children;
    // Set on a group or a literal that a quantifier applies to.
    QString quantifier;
    bool unboundedQuantifier = false;
};

bool isQuantifierStart(QChar c)
{
    return c == u'*' || c == u'+' || c == u'?' || c == u'{';
}

QString describeEscape(QChar c)
{
    switch (c.unicode()) {
    case u'd': return QStringLiteral("any digit");
    case u'D': return QStringLiteral("any character that is not a digit");
    case u'w': return QStringLiteral("any word character (letter, digit or underscore)");
    case u'W': return QStringLiteral("any character that is not a word character");
    case u's': return QStringLiteral("any whitespace character");
    case u'S': return QStringLiteral("any character that is not whitespace");
    case u'b': return QStringLiteral("a word boundary");
    case u'B': return QStringLiteral("a position that is not a word boundary");
    case u'A': return QStringLiteral("the start of the subject");
    case u'z': return QStringLiteral("the end of the subject");
    case u'Z': return QStringLiteral("the end of the subject, before a final newline");
    case u'G': return QStringLiteral("the end of the previous match");
    case u'n': return QStringLiteral("a newline");
    case u'r': return QStringLiteral("a carriage return");
    case u't': return QStringLiteral("a tab");
    case u'f': return QStringLiteral("a form feed");
    case u'v': return QStringLiteral("a vertical tab");
    case u'0': return QStringLiteral("a null character");
    case u'R': return QStringLiteral("any line break sequence");
    case u'K': return QStringLiteral("resets the reported start of the match");
    default: break;
    }
    if (c.isDigit()) {
        return QStringLiteral("a back reference to capture group %1").arg(c);
    }
    return QStringLiteral("the literal character %1").arg(c);
}

QString describeQuantifier(const QString& q)
{
    if (q.isEmpty()) {
        return QString();
    }
    const bool lazy = q.endsWith(u'?') && q.size() > 1;
    const bool possessive = q.endsWith(u'+') && q.size() > 1 && q != QStringLiteral("+");
    QString core = q;
    if (lazy || possessive) {
        core.chop(1);
    }

    QString base;
    if (core == QStringLiteral("*")) {
        base = QStringLiteral("repeated zero or more times");
    } else if (core == QStringLiteral("+")) {
        base = QStringLiteral("repeated one or more times");
    } else if (core == QStringLiteral("?")) {
        base = QStringLiteral("optional");
    } else if (core.startsWith(u'{')) {
        const QString inner = core.mid(1, core.size() - 2);
        const QStringList parts = inner.split(u',');
        if (parts.size() == 1) {
            base = QStringLiteral("repeated exactly %1 times").arg(parts.at(0));
        } else if (parts.size() == 2 && parts.at(1).isEmpty()) {
            base = QStringLiteral("repeated %1 or more times").arg(parts.at(0));
        } else if (parts.size() == 2) {
            base = QStringLiteral("repeated between %1 and %2 times").arg(parts.at(0), parts.at(1));
        } else {
            base = QStringLiteral("repeated");
        }
    }

    if (lazy) {
        return base + QStringLiteral(", as few times as possible");
    }
    if (possessive) {
        return base + QStringLiteral(", without giving anything back on backtracking");
    }
    return base;
}

bool quantifierIsUnbounded(const QString& q)
{
    if (q.isEmpty()) {
        return false;
    }
    if (q.startsWith(u'*') || q.startsWith(u'+')) {
        return true;
    }
    if (q.startsWith(u'{')) {
        const int comma = q.indexOf(u',');
        if (comma < 0) {
            return false;
        }
        const QString upper = q.mid(comma + 1, q.indexOf(u'}') - comma - 1);
        return upper.isEmpty();
    }
    return false;
}

//! Reads a quantifier, if any, starting at \a i. Advances \a i past it.
QString readQuantifier(const QString& p, int& i)
{
    if (i >= p.size() || !isQuantifierStart(p.at(i))) {
        return QString();
    }
    QString q;
    if (p.at(i) == u'{') {
        const int close = p.indexOf(u'}', i);
        if (close < 0) {
            return QString();
        }
        const QString inner = p.mid(i + 1, close - i - 1);
        static const QRegularExpression braceForm(QStringLiteral("^\\d+(,\\d*)?$"));
        if (!braceForm.match(inner).hasMatch()) {
            return QString();
        }
        q = p.mid(i, close - i + 1);
        i = close + 1;
    } else {
        q = p.mid(i, 1);
        ++i;
    }
    if (i < p.size() && (p.at(i) == u'?' || p.at(i) == u'+')) {
        q += p.at(i);
        ++i;
    }
    return q;
}

QString describeGroupOpen(const QString& open, QString* name)
{
    if (open == QStringLiteral("(")) {
        return QStringLiteral("a capture group");
    }
    if (open == QStringLiteral("(?:")) {
        return QStringLiteral("a group that does not capture");
    }
    if (open == QStringLiteral("(?=")) {
        return QStringLiteral("a lookahead: what follows must match, without consuming it");
    }
    if (open == QStringLiteral("(?!")) {
        return QStringLiteral("a negative lookahead: what follows must not match");
    }
    if (open == QStringLiteral("(?<=")) {
        return QStringLiteral("a lookbehind: what precedes must match, without consuming it");
    }
    if (open == QStringLiteral("(?<!")) {
        return QStringLiteral("a negative lookbehind: what precedes must not match");
    }
    if (open == QStringLiteral("(?>")) {
        return QStringLiteral("an atomic group, which never backtracks into itself");
    }
    if (open.startsWith(QStringLiteral("(?<")) || open.startsWith(QStringLiteral("(?P<"))
        || open.startsWith(QStringLiteral("(?'"))) {
        QString n = open;
        n.remove(0, open.startsWith(QStringLiteral("(?P<")) ? 4 : 3);
        n.chop(1);
        if (name) {
            *name = n;
        }
        return QStringLiteral("a capture group named %1").arg(n);
    }
    if (open.startsWith(QStringLiteral("(?#"))) {
        return QStringLiteral("a comment, which matches nothing");
    }
    // Inline modifiers, for example (?i) or (?i-s:
    return QStringLiteral("an inline modifier group: %1").arg(open);
}

//! Reads the opening delimiter of a group starting at \a i (which points at
//! the "("). Advances \a i past the delimiter.
QString readGroupOpen(const QString& p, int& i)
{
    const int start = i;
    ++i; // "("
    if (i >= p.size() || p.at(i) != u'?') {
        return p.mid(start, 1);
    }
    ++i; // "?"
    if (i >= p.size()) {
        return p.mid(start, i - start);
    }
    const QChar c = p.at(i);
    if (c == u':' || c == u'=' || c == u'!' || c == u'>') {
        ++i;
        return p.mid(start, i - start);
    }
    if (c == u'#') {
        const int close = p.indexOf(u')', i);
        i = close < 0 ? p.size() : close;
        return p.mid(start, i - start);
    }
    if (c == u'<') {
        if (i + 1 < p.size() && (p.at(i + 1) == u'=' || p.at(i + 1) == u'!')) {
            i += 2;
            return p.mid(start, i - start);
        }
        const int close = p.indexOf(u'>', i);
        i = close < 0 ? p.size() : close + 1;
        return p.mid(start, i - start);
    }
    if (c == u'\'') {
        const int close = p.indexOf(u'\'', i + 1);
        i = close < 0 ? p.size() : close + 1;
        return p.mid(start, i - start);
    }
    if (c == u'P' && i + 1 < p.size() && p.at(i + 1) == u'<') {
        const int close = p.indexOf(u'>', i);
        i = close < 0 ? p.size() : close + 1;
        return p.mid(start, i - start);
    }
    // Inline modifiers such as (?i), (?im-sx:
    int j = i;
    while (j < p.size() && p.at(j) != u')' && p.at(j) != u':') {
        ++j;
    }
    i = j < p.size() ? j + 1 : p.size();
    return p.mid(start, i - start);
}

QVector<Token> tokenize(const QString& p, int& i, int depth, bool& ok);

Token readCharacterClass(const QString& p, int& i, int depth)
{
    Token t;
    t.kind = QStringLiteral("class");
    t.depth = depth;
    t.start = i;
    ++i; // "["
    bool negated = false;
    if (i < p.size() && p.at(i) == u'^') {
        negated = true;
        ++i;
    }
    if (i < p.size() && p.at(i) == u']') {
        ++i; // a "]" straight after the opening bracket is a literal
    }
    while (i < p.size() && p.at(i) != u']') {
        if (p.at(i) == u'\\' && i + 1 < p.size()) {
            i += 2;
        } else {
            ++i;
        }
    }
    if (i < p.size()) {
        ++i; // "]"
    }
    t.length = i - t.start;
    t.text = p.mid(t.start, t.length);
    t.quantifier = readQuantifier(p, i);
    t.unboundedQuantifier = quantifierIsUnbounded(t.quantifier);
    t.length = i - t.start;
    t.description = negated
                    ? QStringLiteral("any character that is not in the set %1").arg(t.text)
                    : QStringLiteral("any one character from the set %1").arg(t.text);
    const QString q = describeQuantifier(t.quantifier);
    if (!q.isEmpty()) {
        t.description += QStringLiteral(", ") + q;
    }
    t.text = p.mid(t.start, t.length);
    return t;
}

QVector<Token> tokenize(const QString& p, int& i, int depth, bool& ok)
{
    QVector<Token> tokens;
    while (i < p.size()) {
        const QChar c = p.at(i);

        if (c == u')') {
            if (depth == 0) {
                ok = false;
                ++i;
                continue;
            }
            return tokens;
        }

        Token t;
        t.depth = depth;
        t.start = i;

        if (c == u'(') {
            QString name;
            const QString open = readGroupOpen(p, i);
            t.kind = QStringLiteral("group");
            t.description = describeGroupOpen(open, &name);
            t.children = tokenize(p, i, depth + 1, ok);
            if (i < p.size() && p.at(i) == u')') {
                ++i;
            } else {
                ok = false;
            }
            t.quantifier = readQuantifier(p, i);
            t.unboundedQuantifier = quantifierIsUnbounded(t.quantifier);
            t.length = i - t.start;
            t.text = p.mid(t.start, t.length);
            const QString q = describeQuantifier(t.quantifier);
            if (!q.isEmpty()) {
                t.description += QStringLiteral(", ") + q;
            }
            tokens.append(t);
            continue;
        }

        if (c == u'[') {
            tokens.append(readCharacterClass(p, i, depth));
            continue;
        }

        if (c == u'|') {
            ++i;
            t.kind = QStringLiteral("alternation");
            t.text = QStringLiteral("|");
            t.length = 1;
            t.description = QStringLiteral("or: either the part before or the part after this");
            tokens.append(t);
            continue;
        }

        if (c == u'^') {
            ++i;
            t.kind = QStringLiteral("anchor");
            t.text = QStringLiteral("^");
            t.length = 1;
            t.description = QStringLiteral("the start of the subject, or of a line in multiline mode");
            tokens.append(t);
            continue;
        }

        if (c == u'$') {
            ++i;
            t.kind = QStringLiteral("anchor");
            t.text = QStringLiteral("$");
            t.length = 1;
            t.description = QStringLiteral("the end of the subject, or of a line in multiline mode");
            tokens.append(t);
            continue;
        }

        if (c == u'.') {
            ++i;
            t.kind = QStringLiteral("any");
            t.description = QStringLiteral("any character except a newline, or any character at all in dot-all mode");
            t.quantifier = readQuantifier(p, i);
            t.unboundedQuantifier = quantifierIsUnbounded(t.quantifier);
            t.length = i - t.start;
            t.text = p.mid(t.start, t.length);
            const QString q = describeQuantifier(t.quantifier);
            if (!q.isEmpty()) {
                t.description += QStringLiteral(", ") + q;
            }
            tokens.append(t);
            continue;
        }

        if (c == u'\\') {
            ++i;
            if (i >= p.size()) {
                ok = false;
                break;
            }
            const QChar e = p.at(i);
            t.kind = QStringLiteral("escape");
            if ((e == u'p' || e == u'P') && i + 1 < p.size() && p.at(i + 1) == u'{') {
                const int close = p.indexOf(u'}', i);
                i = close < 0 ? p.size() : close + 1;
                const QString prop = p.mid(t.start, i - t.start);
                t.description = e == u'p'
                                ? QStringLiteral("any character in the Unicode property %1").arg(prop.mid(3, prop.size() - 4))
                                : QStringLiteral("any character not in the Unicode property %1").arg(prop.mid(3, prop.size() - 4));
            } else if (e == u'k' && i + 1 < p.size() && (p.at(i + 1) == u'<' || p.at(i + 1) == u'{')) {
                const QChar closeChar = p.at(i + 1) == u'<' ? QChar(u'>') : QChar(u'}');
                const int close = p.indexOf(closeChar, i);
                i = close < 0 ? p.size() : close + 1;
                t.description = QStringLiteral("a back reference to the named group %1")
                                .arg(p.mid(t.start + 3, i - t.start - 4));
            } else {
                ++i;
                t.description = describeEscape(e);
            }
            t.quantifier = readQuantifier(p, i);
            t.unboundedQuantifier = quantifierIsUnbounded(t.quantifier);
            t.length = i - t.start;
            t.text = p.mid(t.start, t.length);
            const QString q = describeQuantifier(t.quantifier);
            if (!q.isEmpty()) {
                t.description += QStringLiteral(", ") + q;
            }
            tokens.append(t);
            continue;
        }

        if (isQuantifierStart(c)) {
            const QString q = readQuantifier(p, i);
            if (q.isEmpty()) {
                ++i;
                t.kind = QStringLiteral("literal");
                t.text = QString(c);
                t.length = 1;
                t.description = QStringLiteral("the literal character %1").arg(c);
                tokens.append(t);
                continue;
            }
            // A quantifier with nothing to repeat.
            ok = false;
            t.kind = QStringLiteral("quantifier");
            t.text = q;
            t.length = q.size();
            t.description = QStringLiteral("a quantifier with nothing before it to repeat");
            tokens.append(t);
            continue;
        }

        // A run of plain literal characters, stopping before a character that
        // a quantifier would apply to on its own.
        int j = i;
        while (j < p.size()) {
            const QChar lc = p.at(j);
            if (lc == u'(' || lc == u')' || lc == u'[' || lc == u'|' || lc == u'^'
                || lc == u'$' || lc == u'.' || lc == u'\\' || isQuantifierStart(lc)) {
                break;
            }
            ++j;
        }
        if (j > i + 1 && j < p.size() && isQuantifierStart(p.at(j))) {
            // The last character belongs to the quantifier that follows.
            --j;
        }
        if (j == i) {
            ++j;
        }
        t.kind = QStringLiteral("literal");
        i = j;
        t.quantifier = readQuantifier(p, i);
        t.unboundedQuantifier = quantifierIsUnbounded(t.quantifier);
        t.length = i - t.start;
        t.text = p.mid(t.start, t.length);
        t.description = QStringLiteral("the literal text %1").arg(p.mid(t.start, j - t.start));
        const QString q = describeQuantifier(t.quantifier);
        if (!q.isEmpty()) {
            t.description += QStringLiteral(", ") + q;
        }
        tokens.append(t);
    }
    return tokens;
}

void flatten(const QVector<Token>& tokens, QVariantList& out)
{
    for (const Token& t : tokens) {
        QVariantMap row;
        row[QStringLiteral("text")] = t.text;
        row[QStringLiteral("kind")] = t.kind;
        row[QStringLiteral("description")] = t.description;
        row[QStringLiteral("start")] = t.start;
        row[QStringLiteral("length")] = t.length;
        row[QStringLiteral("depth")] = t.depth;
        out.append(row);
        flatten(t.children, out);
    }
}

QVariantList toTreeNodes(const QVector<Token>& tokens)
{
    QVariantList nodes;
    for (const Token& t : tokens) {
        QVariantMap node;
        node[QStringLiteral("text")] = t.text;
        node[QStringLiteral("kind")] = t.kind;
        node[QStringLiteral("description")] = t.description;
        node[QStringLiteral("quantifier")] = t.quantifier;
        node[QStringLiteral("children")] = toTreeNodes(t.children);
        nodes.append(node);
    }
    return nodes;
}

bool containsQuantifier(const QVector<Token>& tokens)
{
    for (const Token& t : tokens) {
        if (!t.quantifier.isEmpty()) {
            return true;
        }
        if (containsQuantifier(t.children)) {
            return true;
        }
    }
    return false;
}

//! Splits a token list on its top level alternation markers.
QVector<QVector<Token> > splitAlternatives(const QVector<Token>& tokens)
{
    QVector<QVector<Token> > branches;
    QVector<Token> current;
    for (const Token& t : tokens) {
        if (t.kind == QStringLiteral("alternation")) {
            branches.append(current);
            current.clear();
            continue;
        }
        current.append(t);
    }
    branches.append(current);
    return branches;
}

//! A cheap approximation of the set of characters a branch can start with.
//! Used only to spot alternatives that clearly overlap.
QString firstSetOf(const QVector<Token>& branch)
{
    if (branch.isEmpty()) {
        return QString();
    }
    const Token& t = branch.first();
    if (t.kind == QStringLiteral("group")) {
        return firstSetOf(t.children);
    }
    if (t.kind == QStringLiteral("any")) {
        return QStringLiteral("<any>");
    }
    return t.text;
}

void collectRisks(const QVector<Token>& tokens, QVariantList& risks, int& level)
{
    auto add = [&risks, &level](const QString& severity, const QString& title,
                                const QString& detail, const QString& fragment, int weight) {
        QVariantMap r;
        r[QStringLiteral("severity")] = severity;
        r[QStringLiteral("title")] = title;
        r[QStringLiteral("detail")] = detail;
        r[QStringLiteral("fragment")] = fragment;
        risks.append(r);
        level = std::max(level, weight);
    };

    for (int idx = 0; idx < tokens.size(); ++idx) {
        const Token& t = tokens.at(idx);

        if (t.kind == QStringLiteral("group") && t.unboundedQuantifier) {
            if (containsQuantifier(t.children)) {
                add(QStringLiteral("high"),
                    QStringLiteral("Nested quantifier"),
                    QStringLiteral("A group that is itself repeated without an upper bound contains another "
                                   "quantifier. On input that almost matches, the number of ways to split the "
                                   "subject grows exponentially and the match can take effectively forever. "
                                   "Add an upper bound, make the inner quantifier possessive, or use an atomic group."),
                    t.text, RegexEngine::RiskHigh);
            }
            const QVector<QVector<Token> > branches = splitAlternatives(t.children);
            if (branches.size() > 1) {
                QStringList firsts;
                bool overlap = false;
                for (const QVector<Token>& b : branches) {
                    const QString f = firstSetOf(b);
                    if (f.isEmpty()) {
                        continue;
                    }
                    if (f == QStringLiteral("<any>") || firsts.contains(f)) {
                        overlap = true;
                    }
                    firsts.append(f);
                }
                if (overlap) {
                    add(QStringLiteral("high"),
                        QStringLiteral("Overlapping alternation under a quantifier"),
                        QStringLiteral("Two or more branches of this repeated alternation can match the same "
                                       "starting text, so the engine has to try every combination before it "
                                       "reports a failure. Make the branches mutually exclusive, or wrap the "
                                       "group in an atomic group."),
                        t.text, RegexEngine::RiskHigh);
                }
            }
        }

        if (t.kind == QStringLiteral("any") && t.unboundedQuantifier
            && idx + 1 < tokens.size()) {
            const Token& next = tokens.at(idx + 1);
            if (next.kind == QStringLiteral("any") && next.unboundedQuantifier) {
                add(QStringLiteral("moderate"),
                    QStringLiteral("Adjacent unbounded wildcards"),
                    QStringLiteral("Two unbounded wildcards in a row give the engine many equivalent ways to "
                                   "split the same text. Keep one of them, or bound one with a repetition count."),
                    t.text + next.text, RegexEngine::RiskModerate);
            }
        }

        if (t.kind == QStringLiteral("class") && t.unboundedQuantifier && idx + 1 < tokens.size()) {
            const Token& next = tokens.at(idx + 1);
            if (next.kind == QStringLiteral("class") && next.unboundedQuantifier
                && next.text.startsWith(t.text.left(t.text.indexOf(u']') + 1))) {
                add(QStringLiteral("moderate"),
                    QStringLiteral("Adjacent overlapping character classes"),
                    QStringLiteral("Two repeated character classes in a row accept the same characters, so the "
                                   "boundary between them is ambiguous and the engine has to try every split."),
                    t.text + next.text, RegexEngine::RiskModerate);
            }
        }

        collectRisks(t.children, risks, level);
    }
}
}

RegexEngine::RegexEngine(QObject* parent)
    : QObject(parent)
{
    recompute();
}

QString RegexEngine::pattern() const
{
    return m_pattern;
}

void RegexEngine::setPattern(const QString& pattern)
{
    if (m_pattern == pattern) {
        return;
    }
    m_pattern = pattern;
    emit patternChanged();
    recompute();
}

QString RegexEngine::sampleText() const
{
    return m_sampleText;
}

void RegexEngine::setSampleText(const QString& text)
{
    if (m_sampleText == text) {
        return;
    }
    m_sampleText = text;
    emit sampleTextChanged();
    recompute();
}

QString RegexEngine::replacement() const
{
    return m_replacement;
}

void RegexEngine::setReplacement(const QString& replacement)
{
    if (m_replacement == replacement) {
        return;
    }
    m_replacement = replacement;
    emit replacementChanged();
    recompute();
}

#define AU_REGEX_FLAG(getter, setter, member)  \
    bool RegexEngine::getter() const { return member; } \
    void RegexEngine::setter(bool value) {              \
        if (member == value) { return; }                \
        member = value;                                 \
        emit flagsChanged();                            \
        recompute();                                    \
    }

AU_REGEX_FLAG(caseInsensitive, setCaseInsensitive, m_caseInsensitive)
AU_REGEX_FLAG(multiline, setMultiline, m_multiline)
AU_REGEX_FLAG(dotAll, setDotAll, m_dotAll)
AU_REGEX_FLAG(extended, setExtended, m_extended)
AU_REGEX_FLAG(unicodeProperties, setUnicodeProperties, m_unicodeProperties)

#undef AU_REGEX_FLAG

bool RegexEngine::valid() const { return m_valid; }
QString RegexEngine::errorString() const { return m_errorString; }
int RegexEngine::errorOffset() const { return m_errorOffset; }
QVariantList RegexEngine::matches() const { return m_matches; }
int RegexEngine::matchCount() const { return m_matches.size(); }
QVariantList RegexEngine::explanation() const { return m_explanation; }
QVariantMap RegexEngine::parseTree() const { return m_parseTree; }
QString RegexEngine::replacementPreview() const { return m_replacementPreview; }
double RegexEngine::lastRunMilliseconds() const { return m_lastRunMs; }
bool RegexEngine::truncated() const { return m_truncated; }
QVariantList RegexEngine::risks() const { return m_risks; }
int RegexEngine::riskLevel() const { return m_riskLevel; }

QString RegexEngine::dialect() const
{
    return QStringLiteral("PCRE2 via QRegularExpression %1").arg(QLibraryInfo::version().toString());
}

QVariantList RegexEngine::capabilities() const
{
    struct Row {
        const char* name;
        bool supported;
        const char* note;
    };
    static const Row rows[] = {
        { "Character classes and ranges", true, "[a-z], [^0-9], POSIX classes such as [[:alpha:]]" },
        { "Shorthand classes", true, "\\d \\D \\w \\W \\s \\S \\h \\v \\R" },
        { "Anchors", true, "^ $ \\A \\z \\Z \\b \\B \\G" },
        { "Numbered capture groups", true, "( ... ), referenced as \\1 and $1" },
        { "Named capture groups", true, "(?<name> ... ), (?'name' ... ) and (?P<name> ... )" },
        { "Non-capturing groups", true, "(?: ... )" },
        { "Atomic groups", true, "(?> ... ), which never backtrack into themselves" },
        { "Greedy, lazy and possessive quantifiers", true, "a* a*? a*+ and the braced forms" },
        { "Alternation", true, "a|b, tried left to right" },
        { "Lookahead", true, "(?= ... ) and (?! ... )" },
        { "Lookbehind", true, "(?<= ... ) and (?<! ... ), fixed width in PCRE2" },
        { "Back references", true, "\\1 and \\k<name>" },
        { "Inline modifiers", true, "(?i) (?m) (?s) (?x) and the scoped form (?i: ... )" },
        { "Unicode properties", true, "\\p{L}, \\p{Han}, enable the Unicode flag for full property support" },
        { "Conditional patterns", true, "(?(1)yes|no)" },
        { "Recursion and subroutine calls", true, "(?R), (?1), (?&name)" },
        { "Variable length lookbehind", false, "PCRE2 requires a fixed width lookbehind; use \\K instead" },
        { "Possessive quantifier on a back reference", false, "Not accepted by PCRE2" },
    };
    QVariantList out;
    for (const Row& r : rows) {
        QVariantMap m;
        m[QStringLiteral("name")] = QString::fromLatin1(r.name);
        m[QStringLiteral("supported")] = r.supported;
        m[QStringLiteral("note")] = QString::fromLatin1(r.note);
        out.append(m);
    }
    return out;
}

QRegularExpression::PatternOptions RegexEngine::patternOptions() const
{
    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (m_caseInsensitive) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }
    if (m_multiline) {
        options |= QRegularExpression::MultilineOption;
    }
    if (m_dotAll) {
        options |= QRegularExpression::DotMatchesEverythingOption;
    }
    if (m_extended) {
        options |= QRegularExpression::ExtendedPatternSyntaxOption;
    }
    if (m_unicodeProperties) {
        options |= QRegularExpression::UseUnicodePropertiesOption;
    }
    return options;
}

void RegexEngine::buildExplanation()
{
    m_explanation.clear();
    m_parseTree.clear();

    if (m_pattern.isEmpty()) {
        return;
    }

    int i = 0;
    bool ok = true;
    const QVector<Token> tokens = tokenize(m_pattern, i, 0, ok);

    flatten(tokens, m_explanation);

    m_parseTree[QStringLiteral("kind")] = QStringLiteral("pattern");
    m_parseTree[QStringLiteral("text")] = m_pattern;
    m_parseTree[QStringLiteral("description")] = QStringLiteral("the whole pattern");
    m_parseTree[QStringLiteral("children")] = toTreeNodes(tokens);
    m_parseTree[QStringLiteral("balanced")] = ok;
}

void RegexEngine::buildRisks()
{
    m_risks.clear();
    m_riskLevel = RiskNone;

    if (m_pattern.isEmpty()) {
        return;
    }

    int i = 0;
    bool ok = true;
    const QVector<Token> tokens = tokenize(m_pattern, i, 0, ok);
    collectRisks(tokens, m_risks, m_riskLevel);

    if (m_riskLevel == RiskNone && m_pattern.count(u'*') + m_pattern.count(u'+') >= 4) {
        QVariantMap r;
        r[QStringLiteral("severity")] = QStringLiteral("low");
        r[QStringLiteral("title")] = QStringLiteral("Many unbounded quantifiers");
        r[QStringLiteral("detail")] = QStringLiteral(
            "This pattern uses several unbounded quantifiers. That is not dangerous on its own, "
            "but it is worth timing against a long sample before using it on user input.");
        r[QStringLiteral("fragment")] = m_pattern;
        m_risks.append(r);
        m_riskLevel = RiskLow;
    }
}

void RegexEngine::recompute()
{
    m_matches.clear();
    m_replacementPreview.clear();
    m_lastRunMs = 0.0;
    m_truncated = false;
    m_errorString.clear();
    m_errorOffset = -1;
    m_valid = true;

    buildExplanation();
    buildRisks();

    if (m_pattern.isEmpty()) {
        emit resultsChanged();
        return;
    }

    QRegularExpression expression(m_pattern, patternOptions());
    if (!expression.isValid()) {
        m_valid = false;
        m_errorString = expression.errorString();
        m_errorOffset = expression.patternErrorOffset();
        emit resultsChanged();
        return;
    }
    expression.optimize();

    QString subject = m_sampleText;
    if (subject.size() > MAX_SAMPLE_CHARS) {
        subject.truncate(MAX_SAMPLE_CHARS);
        m_truncated = true;
    }

    const QStringList names = expression.namedCaptureGroups();

    QElapsedTimer timer;
    timer.start();

    QRegularExpressionMatchIterator it = expression.globalMatch(subject);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        if (m_matches.size() >= MAX_MATCHES) {
            m_truncated = true;
            break;
        }

        QVariantMap row;
        row[QStringLiteral("index")] = m_matches.size();
        row[QStringLiteral("start")] = match.capturedStart();
        row[QStringLiteral("length")] = match.capturedLength();
        row[QStringLiteral("text")] = match.captured();

        QVariantList captures;
        for (int g = 1; g <= match.lastCapturedIndex(); ++g) {
            QVariantMap capture;
            capture[QStringLiteral("group")] = g;
            capture[QStringLiteral("name")] = g < names.size() ? names.at(g) : QString();
            capture[QStringLiteral("text")] = match.captured(g);
            capture[QStringLiteral("start")] = match.capturedStart(g);
            capture[QStringLiteral("matched")] = match.hasCaptured(g);
            captures.append(capture);
        }
        row[QStringLiteral("captures")] = captures;
        m_matches.append(row);
    }

    m_lastRunMs = static_cast<double>(timer.nsecsElapsed()) / 1000000.0;

    if (!m_replacement.isEmpty()) {
        m_replacementPreview = subject;
        m_replacementPreview.replace(expression, m_replacement);
    }

    emit resultsChanged();
}

void RegexEngine::insertFragment(const QString& fragment, int position)
{
    QString next = m_pattern;
    if (position < 0 || position > next.size()) {
        next.append(fragment);
    } else {
        next.insert(position, fragment);
    }
    setPattern(next);
}

QString RegexEngine::wrapSelection(const QString& kind, const QString& name, int start, int end)
{
    if (start < 0 || end > m_pattern.size() || start >= end) {
        return m_pattern;
    }

    QString open;
    if (kind == QStringLiteral("capture")) {
        open = QStringLiteral("(");
    } else if (kind == QStringLiteral("named")) {
        open = QStringLiteral("(?<%1>").arg(name.isEmpty() ? QStringLiteral("name") : name);
    } else if (kind == QStringLiteral("nonCapturing")) {
        open = QStringLiteral("(?:");
    } else if (kind == QStringLiteral("atomic")) {
        open = QStringLiteral("(?>");
    } else if (kind == QStringLiteral("lookahead")) {
        open = QStringLiteral("(?=");
    } else if (kind == QStringLiteral("negativeLookahead")) {
        open = QStringLiteral("(?!");
    } else if (kind == QStringLiteral("lookbehind")) {
        open = QStringLiteral("(?<=");
    } else if (kind == QStringLiteral("negativeLookbehind")) {
        open = QStringLiteral("(?<!");
    } else {
        open = QStringLiteral("(?:");
    }

    QString next = m_pattern;
    next.insert(end, QStringLiteral(")"));
    next.insert(start, open);
    setPattern(next);
    return next;
}

QString RegexEngine::escapeLiteral(const QString& literal)
{
    return QRegularExpression::escape(literal);
}

QVariantList RegexEngine::tokenCatalog() const
{
    struct Entry {
        const char* group;
        const char* label;
        const char* fragment;
        const char* help;
    };
    static const Entry entries[] = {
        { "Character classes", "Any digit", "\\d", "Matches 0 to 9" },
        { "Character classes", "Any word character", "\\w", "Letters, digits and underscore" },
        { "Character classes", "Any whitespace", "\\s", "Space, tab, newline and friends" },
        { "Character classes", "Any character", ".", "Any character except a newline" },
        { "Character classes", "Set of characters", "[abc]", "Any one of the listed characters" },
        { "Character classes", "Character range", "[a-z]", "Any one character in the range" },
        { "Character classes", "Negated set", "[^abc]", "Any character that is not listed" },
        { "Character classes", "Unicode letter", "\\p{L}", "Any character with the Unicode letter property" },
        { "Character classes", "Unicode script", "\\p{Han}", "Any character in the named Unicode script" },
        { "Anchors", "Start", "^", "The start of the subject, or of a line in multiline mode" },
        { "Anchors", "End", "$", "The end of the subject, or of a line in multiline mode" },
        { "Anchors", "Subject start", "\\A", "The start of the subject, whatever the flags" },
        { "Anchors", "Subject end", "\\z", "The end of the subject, whatever the flags" },
        { "Anchors", "Word boundary", "\\b", "The edge between a word character and anything else" },
        { "Anchors", "Not a word boundary", "\\B", "Any position that is not a word boundary" },
        { "Groups", "Capture group", "( )", "Captures what it matches for reuse" },
        { "Groups", "Named capture group", "(?<name> )", "Captures under a name" },
        { "Groups", "Non-capturing group", "(?: )", "Groups without capturing" },
        { "Groups", "Atomic group", "(?> )", "Groups and never backtracks into itself" },
        { "Quantifiers", "Zero or more", "*", "Greedy: takes as much as it can" },
        { "Quantifiers", "One or more", "+", "Greedy: takes as much as it can" },
        { "Quantifiers", "Optional", "?", "Zero or one" },
        { "Quantifiers", "Exactly n", "{3}", "Exactly three repetitions" },
        { "Quantifiers", "Between n and m", "{2,5}", "Between two and five repetitions" },
        { "Quantifiers", "n or more", "{2,}", "Two or more repetitions" },
        { "Quantifiers", "Lazy", "*?", "Takes as little as it can" },
        { "Quantifiers", "Possessive", "*+", "Takes as much as it can and never gives it back" },
        { "Alternation", "Either", "|", "Tries the left branch, then the right" },
        { "Lookaround", "Lookahead", "(?= )", "What follows must match" },
        { "Lookaround", "Negative lookahead", "(?! )", "What follows must not match" },
        { "Lookaround", "Lookbehind", "(?<= )", "What precedes must match, fixed width" },
        { "Lookaround", "Negative lookbehind", "(?<! )", "What precedes must not match, fixed width" },
        { "References", "Back reference", "\\1", "Matches the text captured by group 1" },
        { "References", "Named back reference", "\\k<name>", "Matches the text captured by a named group" },
        { "Modifiers", "Case insensitive from here", "(?i)", "Turns case folding on for the rest of the pattern" },
        { "Modifiers", "Multiline from here", "(?m)", "Makes ^ and $ match at line breaks" },
        { "Modifiers", "Dot matches newline from here", "(?s)", "Makes . match a newline" },
        { "Modifiers", "Extended from here", "(?x)", "Ignores unescaped whitespace and # comments" },
        { "Modifiers", "Scoped modifier", "(?i: )", "Applies a modifier to one group only" },
    };
    QVariantList out;
    for (const Entry& e : entries) {
        QVariantMap m;
        m[QStringLiteral("group")] = QString::fromLatin1(e.group);
        m[QStringLiteral("label")] = QString::fromLatin1(e.label);
        m[QStringLiteral("fragment")] = QString::fromLatin1(e.fragment);
        m[QStringLiteral("help")] = QString::fromLatin1(e.help);
        out.append(m);
    }
    return out;
}

QString RegexEngine::storeName() const
{
    return m_storeName;
}

void RegexEngine::setStoreName(const QString& name)
{
    const QString clean = name.isEmpty() ? QStringLiteral("default") : QString(name).replace(QRegularExpression(
                                                                                                 QStringLiteral("[^A-Za-z0-9_-]")),
                                                                                             QStringLiteral("_"));
    if (m_storeName == clean && m_loaded) {
        return;
    }
    m_storeName = clean;
    m_loaded = false;
    emit storeNameChanged();
    loadTestCases();
}

QString RegexEngine::storePath() const
{
    const QString base = au::profile::Paths::writableLocation(QStandardPaths::AppDataLocation);
    return base + QStringLiteral("/companion/regex/") + m_storeName + QStringLiteral(".json");
}

QVariantList RegexEngine::testCases() const
{
    if (!m_loaded) {
        const_cast<RegexEngine*>(this)->loadTestCases();
    }
    return m_testCases;
}

void RegexEngine::loadTestCases()
{
    m_loaded = true;
    m_testCases.clear();

    QFile file(storePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        emit testCasesChanged();
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();

    const QJsonArray array = document.isArray() ? document.array()
                             : document.object().value(QStringLiteral("testCases")).toArray();
    for (const QJsonValue& value : array) {
        m_testCases.append(value.toObject().toVariantMap());
    }
    emit testCasesChanged();
}

void RegexEngine::writeTestCases() const
{
    const QString path = storePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray array;
    for (const QVariant& value : m_testCases) {
        array.append(QJsonObject::fromVariantMap(value.toMap()));
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
}

void RegexEngine::saveTestCase(const QString& name)
{
    if (!m_loaded) {
        loadTestCases();
    }

    QVariantMap entry;
    entry[QStringLiteral("name")] = name.isEmpty() ? m_pattern : name;
    entry[QStringLiteral("pattern")] = m_pattern;
    entry[QStringLiteral("sample")] = m_sampleText;
    entry[QStringLiteral("replacement")] = m_replacement;
    entry[QStringLiteral("caseInsensitive")] = m_caseInsensitive;
    entry[QStringLiteral("multiline")] = m_multiline;
    entry[QStringLiteral("dotAll")] = m_dotAll;
    entry[QStringLiteral("extended")] = m_extended;
    entry[QStringLiteral("unicodeProperties")] = m_unicodeProperties;
    entry[QStringLiteral("expectedMatchCount")] = m_matches.size();

    for (int i = 0; i < m_testCases.size(); ++i) {
        if (m_testCases.at(i).toMap().value(QStringLiteral("name")).toString()
            == entry.value(QStringLiteral("name")).toString()) {
            m_testCases[i] = entry;
            writeTestCases();
            emit testCasesChanged();
            return;
        }
    }

    m_testCases.append(entry);
    writeTestCases();
    emit testCasesChanged();
}

void RegexEngine::loadTestCase(int index)
{
    if (!m_loaded) {
        loadTestCases();
    }
    if (index < 0 || index >= m_testCases.size()) {
        return;
    }

    const QVariantMap entry = m_testCases.at(index).toMap();
    m_pattern = entry.value(QStringLiteral("pattern")).toString();
    m_sampleText = entry.value(QStringLiteral("sample")).toString();
    m_replacement = entry.value(QStringLiteral("replacement")).toString();
    m_caseInsensitive = entry.value(QStringLiteral("caseInsensitive")).toBool();
    m_multiline = entry.value(QStringLiteral("multiline")).toBool();
    m_dotAll = entry.value(QStringLiteral("dotAll")).toBool();
    m_extended = entry.value(QStringLiteral("extended")).toBool();
    m_unicodeProperties = entry.value(QStringLiteral("unicodeProperties")).toBool();

    emit patternChanged();
    emit sampleTextChanged();
    emit replacementChanged();
    emit flagsChanged();
    recompute();
}

void RegexEngine::removeTestCase(int index)
{
    if (!m_loaded) {
        loadTestCases();
    }
    if (index < 0 || index >= m_testCases.size()) {
        return;
    }
    m_testCases.removeAt(index);
    writeTestCases();
    emit testCasesChanged();
}

QString RegexEngine::exportJson() const
{
    if (!m_loaded) {
        const_cast<RegexEngine*>(this)->loadTestCases();
    }

    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("audacity-regex-workbench");
    root[QStringLiteral("version")] = 1;
    root[QStringLiteral("pattern")] = m_pattern;
    root[QStringLiteral("sample")] = m_sampleText;
    root[QStringLiteral("replacement")] = m_replacement;

    QJsonObject flags;
    flags[QStringLiteral("caseInsensitive")] = m_caseInsensitive;
    flags[QStringLiteral("multiline")] = m_multiline;
    flags[QStringLiteral("dotAll")] = m_dotAll;
    flags[QStringLiteral("extended")] = m_extended;
    flags[QStringLiteral("unicodeProperties")] = m_unicodeProperties;
    root[QStringLiteral("flags")] = flags;

    QJsonArray array;
    for (const QVariant& value : m_testCases) {
        array.append(QJsonObject::fromVariantMap(value.toMap()));
    }
    root[QStringLiteral("testCases")] = array;

    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

bool RegexEngine::importJson(const QString& json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    if (root.contains(QStringLiteral("pattern"))) {
        m_pattern = root.value(QStringLiteral("pattern")).toString();
    }
    if (root.contains(QStringLiteral("sample"))) {
        m_sampleText = root.value(QStringLiteral("sample")).toString();
    }
    if (root.contains(QStringLiteral("replacement"))) {
        m_replacement = root.value(QStringLiteral("replacement")).toString();
    }

    const QJsonObject flags = root.value(QStringLiteral("flags")).toObject();
    m_caseInsensitive = flags.value(QStringLiteral("caseInsensitive")).toBool(m_caseInsensitive);
    m_multiline = flags.value(QStringLiteral("multiline")).toBool(m_multiline);
    m_dotAll = flags.value(QStringLiteral("dotAll")).toBool(m_dotAll);
    m_extended = flags.value(QStringLiteral("extended")).toBool(m_extended);
    m_unicodeProperties = flags.value(QStringLiteral("unicodeProperties")).toBool(m_unicodeProperties);

    if (!m_loaded) {
        loadTestCases();
    }
    const QJsonArray array = root.value(QStringLiteral("testCases")).toArray();
    for (const QJsonValue& value : array) {
        const QVariantMap entry = value.toObject().toVariantMap();
        bool replaced = false;
        for (int i = 0; i < m_testCases.size(); ++i) {
            if (m_testCases.at(i).toMap().value(QStringLiteral("name")).toString()
                == entry.value(QStringLiteral("name")).toString()) {
                m_testCases[i] = entry;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            m_testCases.append(entry);
        }
    }
    if (!array.isEmpty()) {
        writeTestCases();
        emit testCasesChanged();
    }

    emit patternChanged();
    emit sampleTextChanged();
    emit replacementChanged();
    emit flagsChanged();
    recompute();
    return true;
}
