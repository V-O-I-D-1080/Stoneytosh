//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_util.hpp>

using t_XPowerTuneConstructor = void (*)(void *that, void *ppInstance, void *ppCallbacks);
using t_sendMsgToSmc = UInt32 (*)(void *smum, UInt32 msgId);

class HWLibs {
    public:
    static HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_XPowerTuneConstructor orgPowerTuneConstructor {nullptr};
    mach_vm_address_t orgAmdCailServicesConstructor {0};
    mach_vm_address_t orgBonairePerformSrbmReset {0};

    static const char *forceX4000HWLibs();

    static void wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static void wrapBonairePerformSrbmReset(void *param1, UInt32 bit);
};

/* ---- Patterns ---- */

static const UInt8 kCailAsicCapsTableHWLibsPattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kCailBonaireLoadUcodeMontereyPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x31, 0xC0};

// ----- Patch ----- //

//! Force allow all log levels.
static const UInt8 AtiPowerPlayServicesCOriginal[] = {0x8B, 0x40, 0x60, 0x48, 0x8D};
static const UInt8 AtiPowerPlayServicesCPatched[] = {0x6A, 0xFF, 0x58, 0x48, 0x8D};
