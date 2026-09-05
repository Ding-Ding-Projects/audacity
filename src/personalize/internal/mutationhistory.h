/*
* Audacity: A Digital Audio Editor
*/

#pragma once

#include <QObject>
#include <QVariantList>

namespace au::personalize {
/*!
 * \brief An append only, redacted log of every lock, authenticator and
 * display name mutation.
 *
 * The chronicle module's local version history service is the natural home
 * for this record, but its interface has no "append one redacted entry"
 * call, and wiring a new one into it is out of scope for this module. This
 * class keeps the same shape the record would have there: one JSON line per
 * mutation, append only, never rewritten, holding a timestamp, an action
 * name and a redacted description, never a secret value. If chronicle later
 * grows a matching entry point, this store's records are still valid input
 * for it.
 */
class MutationHistory : public QObject
{
    Q_OBJECT

public:
    explicit MutationHistory(QObject* parent = nullptr);

    Q_INVOKABLE void record(const QString& action, const QString& redactedDescription);
    Q_INVOKABLE QVariantList entries() const;

private:
    QString storePath() const;
};
}
