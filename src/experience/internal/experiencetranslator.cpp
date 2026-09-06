/*
 * Audacity: A Digital Audio Editor
 */
#include "experiencetranslator.h"

#include <QStringList>

#include <atomic>

namespace au::experience {
ExperienceTranslator::ExperienceTranslator(QObject* parent)
    : QTranslator(parent)
{
}

int ExperienceTranslator::loadCantoneseCatalogues(const QStringList& filePaths)
{
    qDeleteAll(m_cantonese);
    m_cantonese.clear();

    for (const QString& path : filePaths) {
        auto* translator = new QTranslator(this);
        if (translator->load(path)) {
            m_cantonese.append(translator);
        } else {
            delete translator;
        }
    }

    return m_cantonese.size();
}

void ExperienceTranslator::setLanguageMode(LanguageMode mode)
{
    m_mode = mode;
}

LanguageMode ExperienceTranslator::languageMode() const
{
    return m_mode;
}

void ExperienceTranslator::setVocabulary(const PersonalVocabulary::Table& entries)
{
    const PersonalVocabulary::MatcherPtr matcher = PersonalVocabulary::compile(entries);
    m_vocabulary = entries;
    std::atomic_store_explicit(&m_vocabularyMatcher, matcher, std::memory_order_release);
}

const PersonalVocabulary::Table& ExperienceTranslator::vocabulary() const
{
    return m_vocabulary;
}

bool ExperienceTranslator::isEmpty() const
{
    const bool bilingual = m_mode == LanguageMode::Bilingual && !m_cantonese.isEmpty();
    return !bilingual && !std::atomic_load_explicit(&m_vocabularyMatcher, std::memory_order_acquire);
}

QString ExperienceTranslator::compose(const QString& english, const QString& cantonese)
{
    if (cantonese.isEmpty() || cantonese == english) {
        return english;
    }
    return english + QStringLiteral(" / ") + cantonese;
}

QString ExperienceTranslator::cantoneseFor(const char* context, const char* sourceText, const char* disambiguation, int n) const
{
    for (const QTranslator* translator : m_cantonese) {
        const QString candidate = translator->translate(context, sourceText, disambiguation, n);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }
    return QString();
}

QString ExperienceTranslator::translate(const char* context, const char* sourceText, const char* disambiguation, int n) const
{
    if (!sourceText) {
        return QString();
    }

    const QString english = QString::fromUtf8(sourceText);
    QString result;

    if (m_mode == LanguageMode::Bilingual && !m_cantonese.isEmpty()) {
        result = compose(english, cantoneseFor(context, sourceText, disambiguation, n));
    } else if (m_mode == LanguageMode::Cantonese && !m_cantonese.isEmpty()) {
        const QString cantonese = cantoneseFor(context, sourceText, disambiguation, n);
        if (!cantonese.isEmpty()) {
            result = cantonese;
        }
    }

    const PersonalVocabulary::MatcherPtr matcher = std::atomic_load_explicit(&m_vocabularyMatcher, std::memory_order_acquire);
    if (matcher) {
        const QString base = result.isEmpty() ? english : result;
        const QString substituted = PersonalVocabulary::apply(base, matcher);
        if (substituted != base) {
            result = substituted;
        }
    }

    if (result == english) {
        //! Nothing was added, so let the translator underneath answer.
        return QString();
    }

    return result;
}
}
