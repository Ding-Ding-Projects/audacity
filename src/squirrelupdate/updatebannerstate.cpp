/*
* Audacity: A Digital Audio Editor
*/
#include "updatebannerstate.h"

namespace au::squirrelupdate {
UpdateBannerState computeUpdateBannerState(bool isWindowsPlatform, bool isSupportedPlatform, bool checking,
                                           bool available, bool lastErrorPresent)
{
    if (!isWindowsPlatform || !isSupportedPlatform) {
        return UpdateBannerState::NotApplicable;
    }
    if (checking) {
        return UpdateBannerState::Checking;
    }
    if (available) {
        return UpdateBannerState::Ready;
    }
    if (lastErrorPresent) {
        return UpdateBannerState::Failed;
    }
    return UpdateBannerState::NoUpdate;
}
}
