/*
* Audacity: A Digital Audio Editor
*/
#include "chronicledebughooks.h"

#include <QByteArray>

using namespace au::chronicle;

bool ChronicleDebugHooks::startOnVersions() const
{
    return qgetenv("AU_OPEN_HISTORY") == QByteArray("versions");
}
