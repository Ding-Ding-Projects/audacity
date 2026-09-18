/*
 * Audacity: A Digital Audio Editor
 */
#include "notificationcenter.h"

#include <QDateTime>

namespace au::experience {
namespace {
NarratorCategory narratorCategory(NotificationType type)
{
    switch (type) {
    case NotificationType::Success:
        return NarratorCategory::Success;
    case NotificationType::Warning:
        return NarratorCategory::Warning;
    case NotificationType::Error:
        return NarratorCategory::Error;
    case NotificationType::Info:
    default:
        return NarratorCategory::General;
    }
}
}

NotificationCenter::NotificationCenter()
{
    QObject::connect(&m_narratorEngine, &NarratorEngine::speechFinished, &m_narratorEngine, [this]() {
        m_narratorSpeaking = false;
        speakNext();
    });
}

MessageKind NotificationCenter::kindOf(NotificationType type) const
{
    switch (type) {
    case NotificationType::Success:
        return MessageKind::Success;
    case NotificationType::Warning:
        return MessageKind::Warning;
    case NotificationType::Error:
        return MessageKind::Error;
    case NotificationType::Info:
        break;
    }
    return MessageKind::Info;
}

int NotificationCenter::push(NotificationType type, const QString& title, const QString& body, const QString& actionText,
                             const QString& actionCode)
{
    LocalizedNarrationText narration;
    narration.english = body.isEmpty() ? title : body;
    return pushLocalized(type, title, body, narration, actionText, actionCode);
}

int NotificationCenter::pushLocalized(NotificationType type, const QString& title, const QString& body,
                                      const LocalizedNarrationText& narration, const QString& actionText,
                                      const QString& actionCode)
{
    Notification notification;
    notification.id = m_nextId++;
    notification.type = type;
    //! The title names the thing that happened and stays plain. Only the body
    //! carries tone.
    notification.title = title;
    notification.body = styler() ? styler()->style(kindOf(type), body) : body;
    notification.actionText = actionText;
    notification.actionCode = actionCode;
    notification.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_notifications.push_back(notification);
    while (m_notifications.size() > HISTORY_LIMIT) {
        m_notifications.pop_front();
    }

    m_changed.notify();
    narrate(notification, narration);
    return notification.id;
}

void NotificationCenter::narrate(const Notification& notification, const LocalizedNarrationText& narration)
{
    if (!configuration() || !configuration()->narratorEnabled()) {
        return;
    }

    const NarratorCategory category = narratorCategory(notification.type);
    const int configuredLanguage = configuration()->narratorLanguage();
    const NarratorLanguage language = configuredLanguage >= static_cast<int>(NarratorLanguage::English)
                                     && configuredLanguage <= static_cast<int>(NarratorLanguage::Both)
                                     ? static_cast<NarratorLanguage>(configuredLanguage) : NarratorLanguage::English;
    const QString baseKey = notification.type == NotificationType::Info ? QStringLiteral("notification-info")
                                                                         : QStringLiteral("notification-state");
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_narratorQueue.enqueueLocalized(narration.english, narration.cantonese, language, category, baseKey, now)) {
        speakNext();
    }
}

void NotificationCenter::speakNext()
{
    if (m_narratorSpeaking || m_narratorQueue.isEmpty() || !configuration() || !configuration()->narratorEnabled()) {
        return;
    }

    const NarratorUtterance utterance = m_narratorQueue.popNext();
    if (utterance.text.isEmpty()) {
        return;
    }

    const QString voiceId = utterance.spokenIn == NarratorLanguage::Cantonese
                            ? configuration()->narratorCantoneseVoiceId() : configuration()->narratorEnglishVoiceId();
    m_narratorSpeaking = true;
    m_narratorEngine.speak(utterance.text, utterance.spokenIn, voiceId, configuration()->narratorRate(),
                           configuration()->narratorPitch(), configuration()->quietModeEnabled());
}

void NotificationCenter::dismiss(int id)
{
    for (Notification& notification : m_notifications) {
        if (notification.id == id && !notification.dismissed) {
            notification.dismissed = true;
            m_changed.notify();
            return;
        }
    }
}

void NotificationCenter::dismissAll()
{
    bool changed = false;
    for (Notification& notification : m_notifications) {
        if (!notification.dismissed) {
            notification.dismissed = true;
            changed = true;
        }
    }

    if (changed) {
        m_changed.notify();
    }
}

void NotificationCenter::clearHistory()
{
    if (m_notifications.empty()) {
        return;
    }
    m_notifications.clear();
    m_changed.notify();
}

std::vector<Notification> NotificationCenter::active() const
{
    std::vector<Notification> result;
    for (const Notification& notification : m_notifications) {
        if (!notification.dismissed) {
            result.push_back(notification);
        }
    }
    return result;
}

std::vector<Notification> NotificationCenter::history() const
{
    std::vector<Notification> result(m_notifications.rbegin(), m_notifications.rend());
    return result;
}

muse::async::Notification NotificationCenter::changed() const
{
    return m_changed;
}

muse::async::Channel<QString> NotificationCenter::actionRequested() const
{
    return m_actionRequested;
}

void NotificationCenter::requestAction(const QString& actionCode)
{
    if (actionCode.isEmpty()) {
        return;
    }
    m_actionRequested.send(actionCode);
}
}
