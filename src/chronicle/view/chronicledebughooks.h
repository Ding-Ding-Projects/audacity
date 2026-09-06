/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <QObject>

namespace au::chronicle {
/*!
 * A single, narrow debug entry point, registered as the QML singleton
 * ChronicleDebugHooks: setting AU_OPEN_HISTORY=versions before launch makes
 * the History panel start on its Versions segment instead of the undo
 * history, so a screenshot or a manual check can reach the version history
 * panel directly rather than clicking through the panel's own segmented
 * control every time.
 *
 * Read once at process start and never written to, so it carries no state a
 * test or a screenshot run has to reset between launches.
 */
class ChronicleDebugHooks : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool startOnVersions READ startOnVersions CONSTANT)

public:
    explicit ChronicleDebugHooks(QObject* parent = nullptr)
        : QObject(parent) {}

    bool startOnVersions() const;
};
}
