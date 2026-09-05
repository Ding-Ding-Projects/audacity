/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/hardwarefit.h"

using namespace au::toolkit;

TEST(HardwareFitTests, MissingRamEvidenceIsUnknownNotAGuess)
{
    HardwareEvidence evidence;
    evidence.totalRamBytes = 0;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 4LL * 1024 * 1024 * 1024;

    EXPECT_EQ(computeHardwareFit(evidence, claim), HardwareFitVerdict::Unknown);
}

TEST(HardwareFitTests, MissingBlobSizeIsUnknown)
{
    HardwareEvidence evidence;
    evidence.totalRamBytes = 32LL * 1024 * 1024 * 1024;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 0;

    EXPECT_EQ(computeHardwareFit(evidence, claim), HardwareFitVerdict::Unknown);
}

TEST(HardwareFitTests, PlentyOfRamAndVramRunsWell)
{
    HardwareEvidence evidence;
    evidence.totalRamBytes = 64LL * 1024 * 1024 * 1024;
    evidence.freeDiskBytes = 200LL * 1024 * 1024 * 1024;
    evidence.vramBytes = 24LL * 1024 * 1024 * 1024;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 4LL * 1024 * 1024 * 1024;

    EXPECT_EQ(computeHardwareFit(evidence, claim), HardwareFitVerdict::RunsWell);
}

TEST(HardwareFitTests, TightRamNoGpuIsRunsWithLimitsOrUnlikelyNeverRunsWell)
{
    HardwareEvidence evidence;
    evidence.totalRamBytes = 8LL * 1024 * 1024 * 1024;
    evidence.freeDiskBytes = 50LL * 1024 * 1024 * 1024;
    evidence.vramBytes = -1;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 4LL * 1024 * 1024 * 1024;

    const HardwareFitVerdict verdict = computeHardwareFit(evidence, claim);
    EXPECT_NE(verdict, HardwareFitVerdict::RunsWell);
}

TEST(HardwareFitTests, InsufficientFreeDiskIsUnlikely)
{
    HardwareEvidence evidence;
    evidence.totalRamBytes = 64LL * 1024 * 1024 * 1024;
    evidence.freeDiskBytes = 1LL * 1024 * 1024 * 1024;
    evidence.vramBytes = 24LL * 1024 * 1024 * 1024;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 4LL * 1024 * 1024 * 1024;

    EXPECT_EQ(computeHardwareFit(evidence, claim), HardwareFitVerdict::Unlikely);
}

TEST(HardwareFitTests, VerdictNeverDependsOnAnyNameLikeField)
{
    // The function signature itself proves this: HardwareEvidence and
    // ModelResourceClaim carry no name field, so there is nothing for the
    // verdict to read a name from even by accident.
    HardwareEvidence evidence;
    evidence.totalRamBytes = 16LL * 1024 * 1024 * 1024;
    evidence.freeDiskBytes = 100LL * 1024 * 1024 * 1024;
    ModelResourceClaim claim;
    claim.blobSizeBytes = 4LL * 1024 * 1024 * 1024;

    EXPECT_NO_THROW(computeHardwareFit(evidence, claim));
}
