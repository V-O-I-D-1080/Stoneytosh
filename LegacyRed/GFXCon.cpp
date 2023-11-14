//!  Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//!  See LICENSE for details.

#include "GFXCon.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>

static const char *pathAMD8000Controller =
    "/System/Library/Extensions/AMD8000Controller.kext/Contents/MacOS/AMD8000Controller";

static const char *pathAMD9000Controller =
    "/System/Library/Extensions/AMD9000Controller.kext/Contents/MacOS/AMD9000Controller";

static const char *pathAMD9500Controller =
    "/System/Library/Extensions/AMD9500Controller.kext/Contents/MacOS/AMD9500Controller";

static KernelPatcher::KextInfo kextAMD8KController = {"com.apple.kext.AMD8000Controller", &pathAMD8000Controller, 1, {},
    {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextAMD9KController = {"com.apple.kext.AMD9000Controller", &pathAMD9000Controller, 1, {},
    {}, KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextAMD95KController = {"com.apple.kext.AMD9500Controller", &pathAMD9500Controller, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

GFXCon *GFXCon::callback = nullptr;

void GFXCon::init() {
    callback = this;
    lilu.onKextLoadForce(&kextAMD8KController);
    lilu.onKextLoadForce(&kextAMD9KController);
    lilu.onKextLoadForce(&kextAMD95KController);
}

bool GFXCon::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextAMD8KController.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZNK18CISharedController11getFamilyIdEv", wrapGetFamilyId, this->orgGetFamilyId},
            {"__ZN13ASIC_INFO__CI18populateDeviceInfoEv", wrapPopulateDeviceInfo, this->orgPopulateDeviceInfo},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    } else if (kextAMD9KController.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZNK18VISharedController11getFamilyIdEv", wrapGetFamilyId, this->orgGetFamilyId},
            {"__ZN13ASIC_INFO__VI18populateDeviceInfoEv", wrapPopulateDeviceInfo, this->orgPopulateDeviceInfo},
            {"__ZN17AMD9000Controller11getPllClockEhP11ClockParams", wrapGetPllClock, orgGetPllClock},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    } else if (kextAMD95KController.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZNK22BaffinSharedController11getFamilyIdEv", wrapGetFamilyId, this->orgGetFamilyId},
            {"__ZN17ASIC_INFO__BAFFIN18populateDeviceInfoEv", wrapPopulateDeviceInfo, this->orgPopulateDeviceInfo},
            {"__ZN17AMD9500Controller11getPllClockEhP11ClockParams", wrapGetPllClock, orgGetPllClock},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    }

    return false;
}

uint16_t GFXCon::wrapGetFamilyId(void) { return LRed::callback->currentFamilyId; }

IOReturn GFXCon::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callback->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x40) = LRed::callback->currentFamilyId;
    getMember<uint32_t>(that, 0x4C) = LRed::callback->currentEmulatedRevisionId;
    return ret;
}

UInt32 GFXCon::wrapGetPllClock(void *that, uint8_t pll, void *clockparams) {
    DBGLOG("GFXCon", "getPllClock: pll: %x", pll);
    auto ret = FunctionCast(wrapGetPllClock, callback->orgGetPllClock)(that, pll, clockparams);
    DBGLOG("GFXCon", "getPllClock: returned %x", ret);
    return ret;
}
