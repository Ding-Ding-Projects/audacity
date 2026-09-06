/*
* Audacity: A Digital Audio Editor
*/
#pragma once

#include <functional>

#include "framework/global/types/ret.h"

namespace au::squirrelupdate {
//! Orchestrates the one decision that has to happen before Update.exe is
//! ever run: does the user still have unsaved work open. Kept free of Qt and
//! of the modularity container so the decision itself, and every outcome of
//! it, is directly testable without a real project or a real window.
class RestartCoordinator
{
public:
    //! Runs the application's normal unsaved-work protection, the same path
    //! quit and restart already use elsewhere in the application (see
    //! ApplicationActionController::quit and ::restart, which both call
    //! IProjectFilesController::closeOpenedProject before doing anything
    //! irreversible). Returns true when it is safe to proceed: either there
    //! was nothing open, or the user saved or explicitly discarded it.
    //! Returns false when the user cancelled that prompt, in which case
    //! nothing else here runs and the caller must be left exactly as it was.
    using CloseOpenedProjectsFn = std::function<bool ()>;

    //! Performs the actual Squirrel apply and relaunch. Only ever called
    //! once closeOpenedProjects above has returned true.
    using ApplyUpdateFn = std::function<muse::Ret ()>;

    //! A cancelled unsaved-work prompt is reported as Ret::Code::Cancel, so a
    //! caller can tell "the user said not now" apart from "something failed"
    //! and, in particular, never has to guess from a message string.
    static muse::Ret attemptRestart(const CloseOpenedProjectsFn& closeOpenedProjects, const ApplyUpdateFn& applyUpdate);
};
}
