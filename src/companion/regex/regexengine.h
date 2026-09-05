/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace au::companion {
/*!
 * \brief The engine behind the regular expression builder workbench.
 *
 * One instance backs one search field. It owns the pattern, the flags, the
 * sample text, the saved test cases and every derived view of them: the
 * token by token explanation, the parse tree, the match list with its
 * capture table, the replacement preview, the measured run time and the
 * backtracking risk report.
 *
 * The engine never throws. An invalid pattern reports itself through
 * \c valid, \c errorString and \c errorOffset and leaves every derived view
 * empty.
 */
class RegexEngine : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString pattern READ pattern WRITE setPattern NOTIFY patternChanged)
    Q_PROPERTY(QString sampleText READ sampleText WRITE setSampleText NOTIFY sampleTextChanged)
    Q_PROPERTY(QString replacement READ replacement WRITE setReplacement NOTIFY replacementChanged)

    Q_PROPERTY(bool caseInsensitive READ caseInsensitive WRITE setCaseInsensitive NOTIFY flagsChanged)
    Q_PROPERTY(bool multiline READ multiline WRITE setMultiline NOTIFY flagsChanged)
    Q_PROPERTY(bool dotAll READ dotAll WRITE setDotAll NOTIFY flagsChanged)
    Q_PROPERTY(bool extended READ extended WRITE setExtended NOTIFY flagsChanged)
    Q_PROPERTY(bool unicodeProperties READ unicodeProperties WRITE setUnicodeProperties NOTIFY flagsChanged)

    Q_PROPERTY(bool valid READ valid NOTIFY resultsChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY resultsChanged)
    Q_PROPERTY(int errorOffset READ errorOffset NOTIFY resultsChanged)

    Q_PROPERTY(QVariantList matches READ matches NOTIFY resultsChanged)
    Q_PROPERTY(int matchCount READ matchCount NOTIFY resultsChanged)
    Q_PROPERTY(QVariantList explanation READ explanation NOTIFY resultsChanged)
    Q_PROPERTY(QVariantMap parseTree READ parseTree NOTIFY resultsChanged)
    Q_PROPERTY(QString replacementPreview READ replacementPreview NOTIFY resultsChanged)
    Q_PROPERTY(double lastRunMilliseconds READ lastRunMilliseconds NOTIFY resultsChanged)
    Q_PROPERTY(bool truncated READ truncated NOTIFY resultsChanged)

    Q_PROPERTY(QVariantList risks READ risks NOTIFY resultsChanged)
    Q_PROPERTY(int riskLevel READ riskLevel NOTIFY resultsChanged)

    Q_PROPERTY(QString dialect READ dialect CONSTANT)
    Q_PROPERTY(QVariantList capabilities READ capabilities CONSTANT)

    Q_PROPERTY(QVariantList testCases READ testCases NOTIFY testCasesChanged)
    Q_PROPERTY(QString storeName READ storeName WRITE setStoreName NOTIFY storeNameChanged)

public:
    //! The longest sample the engine will run a pattern against. Longer
    //! samples are truncated and reported through \c truncated, which keeps a
    //! catastrophic pattern from freezing the user interface.
    static constexpr int MAX_SAMPLE_CHARS = 20000;
    //! The largest number of matches collected for the match table.
    static constexpr int MAX_MATCHES = 500;

    enum RiskLevel {
        RiskNone = 0,
        RiskLow,
        RiskModerate,
        RiskHigh
    };
    Q_ENUM(RiskLevel)

    explicit RegexEngine(QObject* parent = nullptr);

    QString pattern() const;
    void setPattern(const QString& pattern);

    QString sampleText() const;
    void setSampleText(const QString& text);

    QString replacement() const;
    void setReplacement(const QString& replacement);

    bool caseInsensitive() const;
    void setCaseInsensitive(bool value);
    bool multiline() const;
    void setMultiline(bool value);
    bool dotAll() const;
    void setDotAll(bool value);
    bool extended() const;
    void setExtended(bool value);
    bool unicodeProperties() const;
    void setUnicodeProperties(bool value);

    bool valid() const;
    QString errorString() const;
    int errorOffset() const;

    QVariantList matches() const;
    int matchCount() const;
    QVariantList explanation() const;
    QVariantMap parseTree() const;
    QString replacementPreview() const;
    double lastRunMilliseconds() const;
    bool truncated() const;

    QVariantList risks() const;
    int riskLevel() const;

    QString dialect() const;
    QVariantList capabilities() const;

    QVariantList testCases() const;
    QString storeName() const;
    void setStoreName(const QString& name);

    //! Guided construction. Each of these inserts a fragment at \a position
    //! (or appends when \a position is negative) and keeps the raw pattern in
    //! sync with the guided view.
    Q_INVOKABLE void insertFragment(const QString& fragment, int position = -1);
    Q_INVOKABLE QString wrapSelection(const QString& kind, const QString& name, int start, int end);
    Q_INVOKABLE static QString escapeLiteral(const QString& literal);
    Q_INVOKABLE QVariantList tokenCatalog() const;

    //! Saved test cases, persisted as JSON under the application data
    //! directory in \c companion/regex/<storeName>.json.
    Q_INVOKABLE void saveTestCase(const QString& name);
    Q_INVOKABLE void loadTestCase(int index);
    Q_INVOKABLE void removeTestCase(int index);
    Q_INVOKABLE QString exportJson() const;
    Q_INVOKABLE bool importJson(const QString& json);
    Q_INVOKABLE QString storePath() const;

signals:
    void patternChanged();
    void sampleTextChanged();
    void replacementChanged();
    void flagsChanged();
    void resultsChanged();
    void testCasesChanged();
    void storeNameChanged();

private:
    void recompute();
    void loadTestCases();
    void writeTestCases() const;
    QRegularExpression::PatternOptions patternOptions() const;
    void buildExplanation();
    void buildRisks();

    QString m_pattern;
    QString m_sampleText;
    QString m_replacement;
    QString m_storeName = QStringLiteral("default");

    bool m_caseInsensitive = false;
    bool m_multiline = false;
    bool m_dotAll = false;
    bool m_extended = false;
    bool m_unicodeProperties = false;

    bool m_valid = true;
    QString m_errorString;
    int m_errorOffset = -1;

    QVariantList m_matches;
    QVariantList m_explanation;
    QVariantMap m_parseTree;
    QString m_replacementPreview;
    double m_lastRunMs = 0.0;
    bool m_truncated = false;

    QVariantList m_risks;
    int m_riskLevel = RiskNone;

    QVariantList m_testCases;
    bool m_loaded = false;
};
}
