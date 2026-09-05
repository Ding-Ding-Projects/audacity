/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QList>
#include <QTranslator>

#include "personalvocabulary.h"
#include "experiencetypes.h"

namespace au::experience {
//! An extra QTranslator that sits on top of the ones the muse language
//! service installs.
//!
//! It does two jobs:
//!
//! 1. Bilingual mode. It loads the Cantonese (Hong Kong) catalogues itself and
//!    returns "English / 廣東話" for every string whose Cantonese translation
//!    differs from the English source.
//! 2. Personal vocabulary. It applies whole-word substitutions to the text
//!    that would otherwise be shown.
//!
//! Returning an empty string from translate() makes Qt fall through to the
//! translator underneath, which is how the module stays out of the way when it
//! has nothing to add.
class ExperienceTranslator : public QTranslator
{
    Q_OBJECT

public:
    explicit ExperienceTranslator(QObject* parent = nullptr);

    //! Loads the Cantonese catalogues from the given .qm files. Returns the
    //! number of catalogues that loaded.
    int loadCantoneseCatalogues(const QStringList& filePaths);

    void setLanguageMode(LanguageMode mode);
    LanguageMode languageMode() const;

    void setVocabulary(const PersonalVocabulary::Table& entries);
    const PersonalVocabulary::Table& vocabulary() const;

    //! True when the translator currently has something to contribute.
    bool isEmpty() const override;

    QString translate(const char* context, const char* sourceText, const char* disambiguation, int n) const override;

    //! The composition rule, exposed so that it can be tested directly.
    static QString compose(const QString& english, const QString& cantonese);

private:
    QString cantoneseFor(const char* context, const char* sourceText, const char* disambiguation, int n) const;

    QList<QTranslator*> m_cantonese;
    LanguageMode m_mode = LanguageMode::English;
    PersonalVocabulary::Table m_vocabulary;
};
}
