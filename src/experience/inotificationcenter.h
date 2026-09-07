/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <vector>

#include "framework/global/modularity/imoduleinterface.h"
#include "framework/global/async/channel.h"
#include "framework/global/async/notification.h"

#include "experiencetypes.h"

namespace au::experience {
//! Narration content for a notification. These strings are deliberately kept
//! separate from the rendered body so the narrator never treats styled text or
//! English text as a Cantonese translation.
struct LocalizedNarrationText
{
    QString english;
    QString cantonese;
};

//! Collects application notifications, shows them as toasts and keeps the
//! dismissed ones for the notification centre.
class INotificationCenter : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(au::experience::INotificationCenter)

public:
    virtual ~INotificationCenter() = default;

    //! Adds a notification and returns its id. The body is passed through the
    //! message styler before it is shown.
    virtual int push(NotificationType type, const QString& title, const QString& body,
                     const QString& actionText = QString(), const QString& actionCode = QString()) = 0;

    //! Adds a notification with explicit source text for narration. Existing
    //! push() calls remain English-only because they do not carry a reliable
    //! Cantonese translation.
    virtual int pushLocalized(NotificationType type, const QString& title, const QString& body,
                              const LocalizedNarrationText& narration,
                              const QString& actionText = QString(), const QString& actionCode = QString()) = 0;

    virtual void dismiss(int id) = 0;
    virtual void dismissAll() = 0;
    virtual void clearHistory() = 0;

    //! The notifications that are currently on screen.
    virtual std::vector<Notification> active() const = 0;
    //! Every notification, including the dismissed ones, newest first.
    virtual std::vector<Notification> history() const = 0;

    virtual muse::async::Notification changed() const = 0;
    virtual muse::async::Channel<QString> actionRequested() const = 0;
};
}
