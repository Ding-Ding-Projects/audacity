/*
 * Audacity: A Digital Audio Editor
 */
#include "narratorqueue.h"

using namespace au::experience;

namespace {
QString boundedNarrationText(const QString& text)
{
    return text.trimmed().left(NarratorQueue::MAX_TEXT_LENGTH);
}

QString eventFingerprint(const QVector<NarratorUtterance>& utterances)
{
    QString result;
    for (const NarratorUtterance& utterance : utterances) {
        result += QString::number(static_cast<int>(utterance.spokenIn));
        result += QLatin1Char(':');
        result += utterance.text;
        result += QLatin1Char('\x1e');
    }
    return result;
}
}

NarratorQueue::NarratorQueue(qint64 debounceMs, qint64 cooldownMs)
    : m_debounceMs(debounceMs), m_cooldownMs(cooldownMs)
{
    m_lastCategoryAtMs.fill(-1000000);
}

bool NarratorQueue::enqueue(const NarratorUtterance& utterance, qint64 nowMs)
{
    return enqueueLocalized(utterance.spokenIn == NarratorLanguage::Cantonese ? QString() : utterance.text,
                            utterance.spokenIn == NarratorLanguage::Cantonese ? utterance.text : QString(),
                            utterance.spokenIn, utterance.category, utterance.supersedeKey, nowMs);
}

bool NarratorQueue::enqueueLocalized(const QString& englishText, const QString& cantoneseText,
                                     NarratorLanguage requestedLanguage, NarratorCategory category,
                                     const QString& supersedeKey, qint64 nowMs)
{
    const QString english = boundedNarrationText(englishText);
    const QString cantonese = boundedNarrationText(cantoneseText);
    QVector<NarratorUtterance> utterances;
    const auto append = [&utterances, category, &supersedeKey](const QString& text, NarratorLanguage spokenIn) {
        if (!text.isEmpty()) {
            utterances.push_back({ text, spokenIn, category, supersedeKey });
        }
    };

    switch (requestedLanguage) {
    case NarratorLanguage::Cantonese:
        append(cantonese, NarratorLanguage::Cantonese);
        break;
    case NarratorLanguage::Both:
        append(english, NarratorLanguage::English);
        append(cantonese, NarratorLanguage::Cantonese);
        break;
    case NarratorLanguage::English:
    default:
        append(english, NarratorLanguage::English);
        break;
    }

    if (utterances.isEmpty()) {
        return false;
    }

    int replacementIndex = -1;
    if (!supersedeKey.isEmpty()) {
        for (int index = 0; index < m_pending.size(); ++index) {
            if (m_pending[index].utterance.supersedeKey == supersedeKey) {
                replacementIndex = index;
                break;
            }
        }
    }

    const bool isError = category == NarratorCategory::Error;
    const QString fingerprint = eventFingerprint(utterances);
    if (replacementIndex < 0 && !isError) {
        if (fingerprint == m_lastEventFingerprint && (nowMs - m_lastEventAtMs) < m_debounceMs) {
            return false;
        }

        const int categoryIndex = static_cast<int>(category);
        if (categoryIndex >= 0 && categoryIndex < m_lastCategoryAtMs.size()
            && (nowMs - m_lastCategoryAtMs[categoryIndex]) < m_cooldownMs) {
            return false;
        }
    }

    int insertionIndex = m_pending.size();
    if (replacementIndex >= 0) {
        for (int index = m_pending.size() - 1; index >= 0; --index) {
            if (m_pending[index].utterance.supersedeKey == supersedeKey) {
                m_pending.removeAt(index);
            }
        }
        insertionIndex = qMin(replacementIndex, m_pending.size());
    }

    while (m_pending.size() + utterances.size() > MAX_PENDING) {
        m_pending.removeFirst();
        insertionIndex = qMax(0, insertionIndex - 1);
    }
    for (int index = 0; index < utterances.size(); ++index) {
        m_pending.insert(insertionIndex + index, PendingItem { utterances[index], nowMs });
    }

    m_lastEventFingerprint = fingerprint;
    m_lastEventAtMs = nowMs;
    const int categoryIndex = static_cast<int>(category);
    if (categoryIndex >= 0 && categoryIndex < m_lastCategoryAtMs.size()) {
        m_lastCategoryAtMs[categoryIndex] = nowMs;
    }

    return true;
}

NarratorUtterance NarratorQueue::popNext()
{
    if (m_pending.isEmpty()) {
        return NarratorUtterance();
    }

    const NarratorUtterance next = m_pending.front().utterance;
    m_pending.removeFirst();
    return next;
}
