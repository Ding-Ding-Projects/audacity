/*
* Audacity: A Digital Audio Editor
*/
#include "chronicleactionscontroller.h"

using namespace au::chronicle;

void ChronicleActionsController::init()
{
    dispatcher()->reg(this, "whats-new", this, &ChronicleActionsController::openChangelog);
}

void ChronicleActionsController::openChangelog()
{
    interactive()->open("audacity://chronicle/changelog");
}
