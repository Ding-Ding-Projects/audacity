/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <deque>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "inotificationcenter.h"
#include "imessagestyler.h"
#include "iexperienceconfiguration.h"
#include "narratorqueue.h"
#include "narratorservice.h"

namespace au::experience {
class NotificationCenter : public INotificationCenter, public muse::async::Asyncable
{
    muse::GlobalInject<IMessageStyler> styler;
    muse::GlobalInject<IExperienceConfiguration> configuration;

public:
    NotificationCenter();
    ~NotificationCenter() override = default;

    //! The notification centre keeps the most recent notifications only.
    static constexpr size_t HISTORY_LIMIT = 200;

    int push(NotificationType type, const QString& title, const QString& body,
             const QString& actionText = QString(), const QString& actionCode = QString()) override;
    int pushLocalized(NotificationType type, const QString& title, const QString& body,
                      const LocalizedNarrationText& narration,
                      const QString& actionText = QString(), const QString& actionCode = QString()) override;

    void dismiss(int id) override;
    void dismissAll() override;
    void clearHistory() override;

    std::vector<Notification> active() const override;
    std::vector<Notification> history() const override;

    muse::async::Notification changed() const override;
    muse::async::Channel<QString> actionRequested() const override;

    //! Called by the view when the reader presses a notification action.
    void requestAction(const QString& actionCode);

private:
    MessageKind kindOf(NotificationType type) const;
    void narrate(const Notification& notification, const LocalizedNarrationText& narration);
    void speakNext();

    std::deque<Notification> m_notifications;
    int m_nextId = 1;
    muse::async::Notification m_changed;
    muse::async::Channel<QString> m_actionRequested;
    NarratorQueue m_narratorQueue;
    NarratorEngine m_narratorEngine;
    bool m_narratorSpeaking = false;
};
}
