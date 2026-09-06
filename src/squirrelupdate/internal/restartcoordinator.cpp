/*
* Audacity: A Digital Audio Editor
*/
#include "restartcoordinator.h"

#include <string>

using namespace muse;

namespace au::squirrelupdate {
Ret RestartCoordinator::attemptRestart(const CloseOpenedProjectsFn& closeOpenedProjects, const ApplyUpdateFn& applyUpdate)
{
    // A missing close function means the caller has no unsaved-work concept
    // to guard (a headless run, a test double), not that it is safe to skip
    // the guard silently in the real application. The real application
    // always supplies one; see SquirrelUpdateModel::restartToUpdate.
    if (closeOpenedProjects && !closeOpenedProjects()) {
        return make_ret(Ret::Code::Cancel,
                        std::string("The restart was cancelled while there was unsaved work open"));
    }

    if (!applyUpdate) {
        return make_ret(Ret::Code::UnknownError, std::string("No update apply function was supplied"));
    }

    return applyUpdate();
}
}
