/* Bounded local catalog metadata. Consistency never establishes provenance. */
#pragma once
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <cmath>
#include <vector>

namespace au::toolkit::catalog {
inline constexpr qint64 MaxBytes = 16LL * 1024 * 1024;
inline constexpr int MaxModels = 10000, MaxTags = 100000, MaxReceipts = 10000;
inline const QString Origin = QStringLiteral("https://ollama.com/library");
inline bool identifier(const QString& value, int limit = 128) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]*$"));
    return value.size() <= limit && pattern.match(value).hasMatch();
}
inline bool revision(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._:-]{0,127}$"));
    return pattern.match(value).hasMatch();
}
inline bool hash(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{64}$"));
    return pattern.match(value).hasMatch();
}
inline bool integer(const QJsonValue& value, qint64 minimum, qint64 maximum, qint64* result = nullptr) {
    if (!value.isDouble()) return false;
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum) return false;
    if (result) *result = qint64(number);
    return true;
}
inline QString pageUrl(const QString& text) {
    if (text.isEmpty() || text.size() > 2048) return {};
    for (QChar ch : text) if (ch.unicode() < 0x21 || ch.unicode() > 0x7e || ch == QLatin1Char('\\') || ch == QLatin1Char('%')) return {};
    const QUrl url(text, QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != QStringLiteral("https") || url.host() != QStringLiteral("ollama.com")
        || url.port(-1) != -1 || url.authority().contains(QLatin1Char('@')) || url.hasFragment()) return {};
    static const QRegularExpression path(QStringLiteral("^/library(?:/[A-Za-z0-9][A-Za-z0-9._-]{0,127}(?::[A-Za-z0-9][A-Za-z0-9._-]{0,127}|/tags)?)?$"));
    if (!path.match(url.path()).hasMatch() || url.query().size() > 256 || (url.hasQuery() && url.query().isEmpty())) return {};
    auto items = QUrlQuery(url).queryItems(QUrl::FullyDecoded);
    if (items.size() > 2) return {};
    QSet<QString> keys;
    for (const auto& item : items) {
        if (keys.contains(item.first)) return {};
        keys.insert(item.first);
        if (item.first == QStringLiteral("page")) {
            static const QRegularExpression number(QStringLiteral("^[1-9][0-9]{0,3}$"));
            if (!number.match(item.second).hasMatch() || item.second.toInt() > 1000) return {};
        } else if (item.first != QStringLiteral("sort")
                   || (item.second != QStringLiteral("popular") && item.second != QStringLiteral("newest"))) return {};
    }
    std::sort(items.begin(), items.end());
    QUrl normalized(url); QUrlQuery query; query.setQueryItems(items);
    normalized.setQuery(items.isEmpty() ? QString() : query.query());
    return normalized.toString(QUrl::FullyEncoded);
}

// Bound nesting and scalar sizes before Qt allocates a JSON tree. Detect
// duplicate object keys before QJsonObject's last-key-wins representation loses
// that information, including escaped spellings of the same key.
inline bool lexicalBounds(const QByteArray& bytes) {
    if (bytes.isEmpty() || bytes.size() > MaxBytes) return false;
    struct Frame { char kind; QSet<QString> keys; };
    std::vector<Frame> stack;
    int values = 0;
    for (qsizetype i = 0; i < bytes.size();) {
        const char ch = bytes[i];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == ',' || ch == ':') { ++i; continue; }
        if (ch == '{' || ch == '[') {
            if (++values > 300000 || stack.size() >= 12) return false;
            stack.push_back({ch, {}}); ++i; continue;
        }
        if (ch == '}' || ch == ']') {
            if (stack.empty() || stack.back().kind != (ch == '}' ? '{' : '[')) return false;
            stack.pop_back(); ++i; continue;
        }
        if (ch == '"') {
            if (++values > 300000) return false;
            const qsizetype start = i++;
            bool ended = false;
            while (i < bytes.size() && i - start <= 8192) {
                const char value = bytes[i++];
                if (static_cast<unsigned char>(value) < 0x20) return false;
                if (value == '\\') { if (i >= bytes.size()) return false; ++i; }
                else if (value == '"') { ended = true; break; }
            }
            if (!ended || i - start > 8192) return false;
            qsizetype next = i;
            while (next < bytes.size() && (bytes[next] == ' ' || bytes[next] == '\r' || bytes[next] == '\n' || bytes[next] == '\t')) ++next;
            if (next < bytes.size() && bytes[next] == ':') {
                if (stack.empty() || stack.back().kind != '{') return false;
                QJsonParseError error;
                const auto key = QJsonDocument::fromJson('[' + bytes.mid(start, i - start) + ']', &error).array();
                if (error.error != QJsonParseError::NoError || key.size() != 1 || !key.first().isString()
                    || key.first().toString().size() > 128 || stack.back().keys.contains(key.first().toString())) return false;
                stack.back().keys.insert(key.first().toString());
            }
            continue;
        }
        if (++values > 300000) return false;
        const qsizetype start = i;
        while (i < bytes.size() && bytes[i] != ',' && bytes[i] != ']' && bytes[i] != '}'
               && bytes[i] != ' ' && bytes[i] != '\t' && bytes[i] != '\r' && bytes[i] != '\n') ++i;
        if (i == start || i - start > 64) return false;
    }
    return stack.empty();
}

struct Inventory {
    QSet<QString> urls, hashes;
    qint64 bytes = 0;
    int receipts = 0;
};
inline bool pages(const QJsonValue& value, int maximum, const QString& model, bool complete,
                  Inventory& inventory, QJsonArray& output, QSet<QString>& allItems) {
    if (!value.isArray()) return false;
    const auto input = value.toArray();
    if (input.isEmpty() || input.size() > maximum) return false;
    for (int index = 0; index < input.size(); ++index) {
        if (!input[index].isObject()) return false;
        const auto page = input[index].toObject();
        if (!page.value(QStringLiteral("url")).isString() || !page.value(QStringLiteral("sha256")).isString()) return false;
        const QString url = pageUrl(page.value(QStringLiteral("url")).toString()), digest = page.value(QStringLiteral("sha256")).toString();
        if (url.isEmpty() || !hash(digest) || inventory.urls.contains(url) || inventory.hashes.contains(digest)
            || ++inventory.receipts > MaxReceipts) return false;
        inventory.urls.insert(url); inventory.hashes.insert(digest);
        QJsonObject safe {{QStringLiteral("url"), url}, {QStringLiteral("sha256"), digest}};
        const bool hasDetails = page.contains(QStringLiteral("index")) || page.contains(QStringLiteral("items"))
            || page.contains(QStringLiteral("next")) || page.contains(QStringLiteral("terminal")) || page.contains(QStringLiteral("bytes"));
        if (complete || hasDetails) {
            qint64 size = 0;
            if (!integer(page.value(QStringLiteral("index")), index + 1, index + 1)
                || !integer(page.value(QStringLiteral("bytes")), 1, 2 * 1024 * 1024, &size)
                || !page.value(QStringLiteral("terminal")).isBool()
                || page.value(QStringLiteral("terminal")).toBool() != (index == input.size() - 1)
                || !page.value(QStringLiteral("items")).isArray()) return false;
            const auto items = page.value(QStringLiteral("items")).toArray();
            if (items.isEmpty() || items.size() > 10000) return false;
            QJsonArray safeItems;
            for (const auto& item : items) {
                if (!item.isString()) return false;
                const QString text = item.toString();
                const bool valid = model.isEmpty() ? identifier(text)
                    : text.startsWith(model + QLatin1Char(':')) && text.size() <= 256 && identifier(text.mid(model.size() + 1));
                if (!valid || allItems.contains(text)) return false;
                allItems.insert(text); safeItems.append(text);
            }
            const QString requiredPath = model.isEmpty() ? QStringLiteral("/library") : QStringLiteral("/library/") + model + QStringLiteral("/tags");
            const QUrl parsed(url);
            const QUrlQuery query(parsed);
            if (parsed.path() != requiredPath || query.queryItemValue(QStringLiteral("page")).toInt() != (index == 0 && !query.hasQueryItem(QStringLiteral("page")) ? 0 : index + 1)) return false;
            if (index + 1 < input.size()) {
                if (!page.value(QStringLiteral("next")).isString() || !input[index + 1].isObject()) return false;
                const QString next = pageUrl(page.value(QStringLiteral("next")).toString());
                if (next.isEmpty() || next != pageUrl(input[index + 1].toObject().value(QStringLiteral("url")).toString())) return false;
                safe.insert(QStringLiteral("next"), next);
            } else {
                if (!page.value(QStringLiteral("next")).isNull()) return false;
                safe.insert(QStringLiteral("next"), QJsonValue::Null);
            }
            inventory.bytes += size;
            if (inventory.bytes > 256LL * 1024 * 1024) return false;
            safe.insert(QStringLiteral("index"), index + 1); safe.insert(QStringLiteral("bytes"), size);
            safe.insert(QStringLiteral("items"), safeItems); safe.insert(QStringLiteral("terminal"), index == input.size() - 1);
        }
        output.append(safe);
    }
    return true;
}

inline bool utcTime(const QJsonValue& value, QDateTime* result = nullptr) {
    if (!value.isString() || value.toString().size() > 32 || !value.toString().endsWith(QLatin1Char('Z'))) return false;
    const auto date = QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
    if (!date.isValid()) return false;
    if (result) *result = date;
    return true;
}
inline bool normalize(const QJsonObject& input, QVariantMap& output) {
    if (!integer(input.value(QStringLiteral("schemaVersion")), 1, 1)
        || !input.value(QStringLiteral("origin")).isString() || input.value(QStringLiteral("origin")).toString() != Origin
        || !input.value(QStringLiteral("revision")).isString() || !revision(input.value(QStringLiteral("revision")).toString())
        || !input.value(QStringLiteral("models")).isArray() || !input.value(QStringLiteral("pages")).isArray()
        || !integer(input.value(QStringLiteral("pageCount")), 1, 1000)
        || input.value(QStringLiteral("pageCount")).toInt() != input.value(QStringLiteral("pages")).toArray().size()) return false;
    const bool acquired = input.contains(QStringLiteral("acquisition"));
    if (acquired && !input.value(QStringLiteral("acquisition")).isObject()) return false;
    const QJsonArray models = input.value(QStringLiteral("models")).toArray();
    if (models.isEmpty() || models.size() > MaxModels) return false;
    Inventory inventory;
    QJsonArray safePages, safeModels;
    QSet<QString> indexItems, names;
    if (!pages(input.value(QStringLiteral("pages")), 1000, {}, acquired, inventory, safePages, indexItems)) return false;
    int totalTags = 0;
    for (const auto& model : models) {
        if (!model.isObject()) return false;
        const auto object = model.toObject();
        if (!object.value(QStringLiteral("name")).isString() || !object.value(QStringLiteral("tags")).isArray()) return false;
        const QString name = object.value(QStringLiteral("name")).toString();
        const QJsonArray tags = object.value(QStringLiteral("tags")).toArray();
        if (!identifier(name) || names.contains(name) || tags.isEmpty() || tags.size() > 10000) return false;
        names.insert(name); QJsonArray safeTags; QSet<QString> seenTags;
        for (const auto& tag : tags) {
            if (!tag.isString()) return false;
            const QString text = tag.toString();
            if (!text.startsWith(name + QLatin1Char(':')) || text.size() > 256 || !identifier(text.mid(name.size() + 1))
                || seenTags.contains(text) || ++totalTags > MaxTags) return false;
            seenTags.insert(text); safeTags.append(text);
        }
        QJsonObject safe {{QStringLiteral("name"), name}, {QStringLiteral("tags"), safeTags}};
        if (acquired || object.contains(QStringLiteral("tagPages"))) {
            QJsonArray safeTagPages; QSet<QString> tagItems;
            if (!pages(object.value(QStringLiteral("tagPages")), 100, name, acquired, inventory, safeTagPages, tagItems)
                || (acquired && tagItems != seenTags)) return false;
            safe.insert(QStringLiteral("tagPages"), safeTagPages);
        }
        safeModels.append(safe);
    }
    QJsonObject safe {{QStringLiteral("schemaVersion"), 1}, {QStringLiteral("origin"), Origin},
        {QStringLiteral("revision"), input.value(QStringLiteral("revision"))}, {QStringLiteral("pageCount"), safePages.size()},
        {QStringLiteral("pages"), safePages}, {QStringLiteral("models"), safeModels},
        {QStringLiteral("completeness"), QStringLiteral("untrusted-local-import")}, {QStringLiteral("originVerified"), false},
        {QStringLiteral("receiptKind"), acquired ? QStringLiteral("acquisition-receipt-claims") : QStringLiteral("metadata-only")}};
    const QString claimKey = input.contains(QStringLiteral("declaredCompleteness")) ? QStringLiteral("declaredCompleteness") : QStringLiteral("completeness");
    if (input.contains(claimKey)) {
        const QJsonValue claim = input.value(claimKey);
        if (!claim.isString() || claim.toString().size() > 64 || !identifier(claim.toString(), 64)) return false;
        safe.insert(QStringLiteral("declaredCompleteness"), claim);
    }
    if (acquired) {
        const auto receipt = input.value(QStringLiteral("acquisition")).toObject();
        QDateTime start, end;
        if (indexItems != names || receipt.value(QStringLiteral("method")).toString() != QStringLiteral("bounded-https-library-pages")
            || !utcTime(receipt.value(QStringLiteral("startedAt")), &start) || !utcTime(receipt.value(QStringLiteral("completedAt")), &end)
            || end < start || start.secsTo(end) > 3600
            || pageUrl(receipt.value(QStringLiteral("firstUrl")).toString()) != safePages.first().toObject().value(QStringLiteral("url")).toString()
            || pageUrl(receipt.value(QStringLiteral("terminalUrl")).toString()) != safePages.last().toObject().value(QStringLiteral("url")).toString()
            || !integer(receipt.value(QStringLiteral("pageCount")), inventory.receipts, inventory.receipts)
            || !integer(receipt.value(QStringLiteral("responseBytes")), inventory.bytes, inventory.bytes)
            || !integer(receipt.value(QStringLiteral("modelCount")), models.size(), models.size())
            || !integer(receipt.value(QStringLiteral("tagCount")), totalTags, totalTags)) return false;
        const QJsonObject graph {{QStringLiteral("models"), safeModels}, {QStringLiteral("pages"), safePages}};
        const QString digest = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(graph).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
        if (input.value(QStringLiteral("revision")).toString() != digest) return false;
        QJsonObject safeReceipt;
        for (const QString& key : {QStringLiteral("method"), QStringLiteral("startedAt"), QStringLiteral("completedAt"), QStringLiteral("pageCount"),
             QStringLiteral("responseBytes"), QStringLiteral("modelCount"), QStringLiteral("tagCount")}) safeReceipt.insert(key, receipt.value(key));
        safeReceipt.insert(QStringLiteral("firstUrl"), safePages.first().toObject().value(QStringLiteral("url")));
        safeReceipt.insert(QStringLiteral("terminalUrl"), safePages.last().toObject().value(QStringLiteral("url")));
        safe.insert(QStringLiteral("acquisition"), safeReceipt);
    }
    if (input.contains(QStringLiteral("importedAt"))) {
        if (!utcTime(input.value(QStringLiteral("importedAt")))) return false;
        safe.insert(QStringLiteral("importedAt"), input.value(QStringLiteral("importedAt")));
    }
    output = safe.toVariantMap(); return true;
}
inline bool parse(const QByteArray& bytes, QVariantMap& output) {
    if (!lexicalBounds(bytes)) return false;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(bytes, &error);
    return error.error == QJsonParseError::NoError && document.isObject() && normalize(document.object(), output);
}
}
