//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_util.hpp>

class GFXCon {
    public:
    static GFXCon *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPopulateDeviceInfo {0};
    mach_vm_address_t orgGetFamilyId {0};
    mach_vm_address_t orgGetPllClock {0};
    mach_vm_address_t orgGetVRAMBaseOffset {0};

    static IOReturn wrapPopulateDeviceInfo(void *that);
    static UInt16 wrapGetFamilyId(void);
    static UInt32 wrapGetPllClock(void *that, uint8_t pll, void *clockparams);
    static IOReturn wrapPopulateFbLocation(void *that);
    static UInt64 wrapGetVRAMBaseOffset(void *that);
};
