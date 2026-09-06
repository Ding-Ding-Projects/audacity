/*
 * Audacity: A Digital Audio Editor
 */
#include "narratorqueue.h"

using namespace au::experience;

NarratorQueue::NarratorQueue(qint64 debounceMs, qint64 cooldownMs)
    : m_debounceMs(debounceMs), m_cooldownMs(cooldownMs)
{
    m_lastCategoryAtMs.fill(-1000000);
}

bool NarratorQueue::enqueue(const NarratorUtterance& utterance, qint64 nowMs)
{
    const bool isError = utterance.category == NarratorCategory::Error;

    if (!isError) {
        if (utterance.text == m_lastText && (nowMs - m_lastTextAtMs) < m_debounceMs) {
            return false;
        }

        const int categoryIndex = static_cast<int>(utterance.category);
        if (categoryIndex >= 0 && categoryIndex < m_lastCategoryAtMs.size()) {
            if ((nowMs - m_lastCategoryAtMs[categoryIndex]) < m_cooldownMs) {
                return false;
            }
        }
    }

    if (!utterance.supersedeKey.isEmpty()) {
        for (PendingItem& item : m_pending) {
            if (item.utterance.supersedeKey == utterance.supersedeKey) {
                item.utterance = utterance;
                item.queuedAtMs = nowMs;
                return true;
            }
        }
    }

    m_pending.push_back({ utterance, nowMs });

    m_lastText = utterance.text;
    m_lastTextAtMs = nowMs;
    const int categoryIndex = static_cast<int>(utterance.category);
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
