/*
* Audacity: A Digital Audio Editor
*/
#pragma once

namespace au::squirrelupdate {
//! The one visible state a user needs to see. Kept as a plain, dependency
//! free enum so the transition logic below is directly testable.
enum class UpdateBannerState {
    Hidden,
    NoUpdate,
    Checking,
    Available,
    Downloading,
    Ready,
    Failed,
    Offline,
    InvalidMetadata,
    CorruptAsset,
    Cancelled,
    Rollback,
    NotApplicable
};

//! Pure computation of the banner state from the service's observable facts.
//! No Qt event loop, no settings and no injected dependencies, so this is
//! exercised directly by the state machine tests.
UpdateBannerState computeUpdateBannerState(bool isWindowsPlatform, bool isSupportedPlatform, bool checking, bool available,
                                           bool lastErrorPresent);
}
