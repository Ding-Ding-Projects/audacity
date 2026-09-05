/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>
#include <utility>

namespace au::experience {
//! Reads and applies a personal vocabulary file.
//!
//! The accepted document is
//! {"version":1,"entries":[{"from":"...","to":"..."}]}
//! with at most 256 KB of text and at most 2000 entries.
class PersonalVocabulary
{
public:
    static constexpr int MAX_BYTES = 256 * 1024;
    static constexpr int MAX_ENTRIES = 2000;
    //! Neither side of one entry may be longer than this, in characters.
    static constexpr int MAX_TERM_LENGTH = 200;

    using Table = QVector<std::pair<QString, QString> >;

    struct ParseResult
    {
        bool ok = false;
        //! A short, already translated reason when ok is false.
        QString error;
        Table entries;
    };

    //! Parses the document. The contents are never logged.
    static ParseResult parse(const QByteArray& data);

    //! Serialises a parsed table back to the stored form.
    static QByteArray serialize(const Table& entries);

    //! Applies whole-word substitutions to one piece of visible text.
    //! Returns text unchanged when nothing matches.
    static QString apply(const QString& text, const Table& entries);
};
}
