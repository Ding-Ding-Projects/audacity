/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/restartcoordinator.h"

using namespace au::squirrelupdate;
using namespace muse;

TEST(RestartCoordinatorTests, RestartRequestedWithUnsavedWorkUserCancelsStateStaysReady)
{
    bool applyUpdateWasCalled = false;

    // Stands in for IProjectFilesController::closeOpenedProject returning
    // false: there is unsaved work open, and the user cancelled the save or
    // discard prompt instead of choosing one.
    auto closeOpenedProjectsCancelled = []() { return false; };
    auto applyUpdate = [&applyUpdateWasCalled]() {
        applyUpdateWasCalled = true;
        return make_ok();
    };

    const Ret result = RestartCoordinator::attemptRestart(closeOpenedProjectsCancelled, applyUpdate);

    // The restart never happened...
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::Cancel));
    // ...and, critically, the apply step (the one that would tear down the
    // downloaded package's Ready state and quit the application) was never
    // reached at all. The verified package, the Ready state and the banner
    // are all exactly as they were before restart was requested.
    EXPECT_FALSE(applyUpdateWasCalled);
}

TEST(RestartCoordinatorTests, RestartProceedsWhenThereIsNothingUnsavedToProtect)
{
    bool applyUpdateWasCalled = false;

    auto closeOpenedProjectsNothingOpen = []() { return true; };
    auto applyUpdate = [&applyUpdateWasCalled]() {
        applyUpdateWasCalled = true;
        return make_ok();
    };

    const Ret result = RestartCoordinator::attemptRestart(closeOpenedProjectsNothingOpen, applyUpdate);

    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_TRUE(applyUpdateWasCalled);
}

TEST(RestartCoordinatorTests, RestartProceedsAfterTheUserSavesOrDiscards)
{
    // closeOpenedProject(false) returns true both when the user chose Save
    // and when the user chose Discard; the coordinator does not need to
    // know which, only that it is now safe to proceed.
    bool applyUpdateWasCalled = false;

    auto closeOpenedProjectsSavedOrDiscarded = []() { return true; };
    auto applyUpdate = [&applyUpdateWasCalled]() {
        applyUpdateWasCalled = true;
        return make_ok();
    };

    const Ret result = RestartCoordinator::attemptRestart(closeOpenedProjectsSavedOrDiscarded, applyUpdate);

    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_TRUE(applyUpdateWasCalled);
}

TEST(RestartCoordinatorTests, ApplyUpdateFailureIsReturnedUnchanged)
{
    auto closeOpenedProjectsOk = []() { return true; };
    auto applyUpdateFails = []() {
        return make_ret(Ret::Code::UnknownError, std::string("Update.exe did not finish in time"));
    };

    const Ret result = RestartCoordinator::attemptRestart(closeOpenedProjectsOk, applyUpdateFails);

    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.code(), static_cast<int>(Ret::Code::UnknownError));
}

TEST(RestartCoordinatorTests, MissingCloseFunctionSkipsTheGuardRatherThanFailing)
{
    // A missing close function stands for a headless caller with no project
    // concept at all, never for the real application skipping its own
    // unsaved-work protection: SquirrelUpdateModel always supplies one.
    bool applyUpdateWasCalled = false;
    auto applyUpdate = [&applyUpdateWasCalled]() {
        applyUpdateWasCalled = true;
        return make_ok();
    };

    const Ret result = RestartCoordinator::attemptRestart(RestartCoordinator::CloseOpenedProjectsFn(), applyUpdate);

    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_TRUE(applyUpdateWasCalled);
}
