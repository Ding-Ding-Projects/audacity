/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <memory>
#include <utility>

namespace au::experience {
//! Reads and applies a personal vocabulary file.
//!
//! The accepted document is {"schemaVersion":1,"entries":{"source":"replacement"}}
//! with at most 256 KB of strict UTF-8 text and at most 4096 entries.
class PersonalVocabulary
{
public:
    static constexpr int MAX_BYTES = 256 * 1024;
    static constexpr int MAX_ENTRIES = 4096;
    static constexpr int MAX_SOURCE_LENGTH = 160;
    static constexpr int MAX_REPLACEMENT_LENGTH = 1000;

    using Table = QVector<std::pair<QString, QString> >;

    //! An immutable compiled lookup table. Build it when the vocabulary changes
    //! and reuse it for each translation.
    class Matcher;
    using MatcherPtr = std::shared_ptr<const Matcher>;

    struct ParseResult
    {
        bool ok = false;
        //! A short, already translated reason when ok is false.
        QString error;
        Table entries;
        bool migratedLegacy = false;
    };

    //! Parses the document. The contents are never logged.
    static ParseResult parse(const QByteArray& data);

    //! Parses a local cache document. This accepts the former array-shaped
    //! cache only after validating it, and marks the result for migration.
    static ParseResult parseStoredCache(const QByteArray& data);

    //! Serialises a parsed table back to the stored form.
    static QByteArray serialize(const Table& entries);

    //! Compiles a bounded, immutable matcher for a validated table.
    static MatcherPtr compile(const Table& entries);

    //! Applies whole-word substitutions to one piece of visible text.
    //! Returns text unchanged when nothing matches.
    static QString apply(const QString& text, const Table& entries);
    static QString apply(const QString& text, const MatcherPtr& matcher);
};
}
