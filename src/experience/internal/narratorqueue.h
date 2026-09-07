/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QString>
#include <QVector>

namespace au::experience {
//! The language a narrated line is spoken in.
enum class NarratorLanguage {
    English = 0,
    Cantonese = 1,
    Both = 2
};

//! A broad category of narrated event, used for per category cooldown.
//! Error narration is exempt from every rate limit.
enum class NarratorCategory {
    General = 0,
    Success = 1,
    Warning = 2,
    Error = 3
};

//! One line ready to speak, after language expansion. A "Both" request
//! produces two of these (English, then Cantonese), always in that order.
struct NarratorUtterance
{
    QString text;
    NarratorLanguage spokenIn = NarratorLanguage::English;
    NarratorCategory category = NarratorCategory::General;
    //! A stable identifier for the logical event this utterance narrates.
    //! Queuing a new utterance with the same key as one still pending
    //! replaces it rather than stacking a second one.
    QString supersedeKey;
};

//! Orders narrated lines into a single, non-overlapping, serialized queue.
//!
//! Rules enforced here, and only here:
//! - at most one utterance is ever "current" at a time;
//! - queuing an utterance whose supersedeKey matches one already waiting
//!   replaces it in place rather than appending a duplicate;
//! - a debounce window suppresses a second push of the exact same text
//!   arriving within a short interval;
//! - a per category cooldown suppresses further non-error narration in
//!   that category until the cooldown elapses, but never suppresses
//!   NarratorCategory::Error.
class NarratorQueue
{
public:
    static constexpr int MAX_PENDING = 64;
    explicit NarratorQueue(qint64 debounceMs = 400, qint64 cooldownMs = 4000);

    //! Attempts to enqueue at the given monotonic time (milliseconds).
    //! Returns true if the utterance was queued (or replaced a pending
    //! one), false if it was suppressed by debounce or cooldown.
    bool enqueue(const NarratorUtterance& utterance, qint64 nowMs);

    //! Removes and returns the next utterance to speak, or nullptr-like
    //! (empty text) if the queue is empty. Call popNext() only when the
    //! previous utterance has finished speaking, so only one is ever in
    //! flight.
    NarratorUtterance popNext();

    bool isEmpty() const { return m_pending.isEmpty(); }
    int size() const { return m_pending.size(); }

private:
    struct PendingItem
    {
        NarratorUtterance utterance;
        qint64 queuedAtMs = 0;
    };

    qint64 m_debounceMs;
    qint64 m_cooldownMs;
    QVector<PendingItem> m_pending;
    QString m_lastText;
    NarratorLanguage m_lastLanguage = NarratorLanguage::English;
    qint64 m_lastTextAtMs = -1000000;
    QVector<qint64> m_lastCategoryAtMs { 0, 0, 0, 0 };
};
}
