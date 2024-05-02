//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "GFXCon.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include "Support.hpp"
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
            {"__ZN13ASIC_INFO__CI18populateFbLocationEv", wrapPopulateFbLocation},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    } else if (kextAMD9KController.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        SolveRequestPlus solveRequests[] = {
            {"__ZN18VIInterruptManager16setRBReadPointerEj", this->IHSetRBReadPointer},
            {"__ZN18VIInterruptManager22getWPTRWriteBackOffsetEv", this->IHGetWPTRWriteBackOffset},
            {"__ZN18VIInterruptManager24isUsingVRAMForRingBufferEv", this->IHIsUsingVRAMForRingBuffer},
            {"__ZN18VIInterruptManager31getActiveRingBufferSizeRegValueEv", this->IHGetActiveRingBufferSizeRegValue},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "GFXCon",
            "Failed to solve symbols for IH");

        RouteRequestPlus requests[] = {
            {"__ZNK18VISharedController11getFamilyIdEv", wrapGetFamilyId, this->orgGetFamilyId},
            {"__ZN13ASIC_INFO__VI18populateDeviceInfoEv", wrapPopulateDeviceInfo, this->orgPopulateDeviceInfo},
            {"__ZN18VIInterruptManager18setHardwareEnabledEb", IHSetHardwareEnabled},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    } else if (kextAMD95KController.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        RouteRequestPlus requests[] = {
            {"__ZNK22BaffinSharedController11getFamilyIdEv", wrapGetFamilyId, this->orgGetFamilyId},
            {"__ZN17ASIC_INFO__BAFFIN18populateDeviceInfoEv", wrapPopulateDeviceInfo, this->orgPopulateDeviceInfo},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "GFXCon",
            "Failed to route symbols");
        return true;
    }

    return false;
}

UInt16 GFXCon::wrapGetFamilyId(void) { return LRed::callback->familyId; }

IOReturn GFXCon::wrapPopulateDeviceInfo(void *that) {
    auto ret = FunctionCast(wrapPopulateDeviceInfo, callback->orgPopulateDeviceInfo)(that);
    getMember<uint32_t>(that, 0x40) = LRed::callback->familyId;
    getMember<uint32_t>(that, 0x4C) = LRed::callback->emulatedRevision;
    return ret;
}

IOReturn GFXCon::wrapPopulateFbLocation(void *that) {
    UInt64 base = (LRed::callback->readReg32(mmMC_VM_FB_LOCATION) & 0xFFFF) << 24;

    //! experiment
    //! 0xF400000000
    //!   0x20000000
    base = (USUAL_VRAM_PADDR + base);

    getMember<UInt64>(that, 0x58) = base;

    //! https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_gmc.c#L206
    UInt64 memsize = ((LRed::callback->readReg32(mmCONFIG_MEMSIZE) * 1024ULL) * 1024ULL);
    DBGLOG("GFXCon", "memsize: 0x%llx, addr calc: 0x%llx, res: 0x%llx", memsize, (unsigned long long)(base + memsize), ((base + memsize) - 1));
    getMember<UInt64>(that, 0x60) = ((base + memsize) - 1);

    DBGLOG("GFXCon", "populateFbLocation: base: 0x%llx top: 0x%llx", getMember<UInt64>(that, 0x58),
        getMember<UInt64>(that, 0x60));
    return kIOReturnSuccess;
}

//-------- Carrizo IH fixes !!! WIP DO NOT USE YET !!! --------//

enum InterruptManagerFields {
    Unk1 = 0x32,
    Flags = 0x38,
    Unk2 = 0x3C,
    GPUAddress = 0x48,
    Unk3 = 0x50,
};

enum InterruptManagerFlags {
    UseSRRB = 0x1,
    MSIEnabled = 0x2,
    DontUsePulseBasedInterrupts = 0x8,
    IHEnableClockGating = 0x10,    //! unused in VIIntMgr
};

//! description:
//! Properly power-up the IH, Tonga's OSS 3.0.0 uses a new bit in the RB_CNTL to fully
//! get the IH ready for usage, this bit is not present on the OSS 3.0.1 IH and thus
//! we must override the function here with our own logic
void GFXCon::IHSetHardwareEnabled(void *ihmgr, bool enabled) {
    DBGLOG("GFXCon", "CZ IH @ setHardwareEnabled: enabled = 0x%x", enabled);
    if (enabled) {
        Support::callback->IHAcknowledgeAllOutStandingInterrupts(ihmgr);
        callback->IHSetRBReadPointer(ihmgr, 0);
        LRed::callback->writeReg32(mmIH_RB_WPTR, 0);    //! what

        IODelay(10);    //! give it a lil time to catch up

        UInt32 tmp = LRed::callback->readReg32(mmIH_RB_CNTL);
        UInt32 tmp2 = LRed::callback->readReg32(mmIH_CNTL);

        if (tmp & IH_RB_CNTL__RB_ENABLE || tmp2 & IH_CNTL__ENABLE_INTR) {
            DBGLOG("GFXCon", "CZ IH @ setHardwareEnabled (true): what.");
        }

        tmp |= IH_RB_CNTL__RB_ENABLE;
        tmp2 |= IH_CNTL__ENABLE_INTR;

        LRed::callback->writeReg32(mmIH_RB_CNTL, tmp);
        LRed::callback->writeReg32(mmIH_CNTL, tmp2);
    } else {
        UInt32 tmp = LRed::callback->readReg32(mmIH_RB_CNTL);
        UInt32 tmp2 = LRed::callback->readReg32(mmIH_CNTL);

        //! assume that it's set
        tmp &= ~IH_RB_CNTL__RB_ENABLE;
        tmp2 &= ~IH_CNTL__ENABLE_INTR;

        LRed::callback->writeReg32(mmIH_RB_CNTL, tmp);
        LRed::callback->writeReg32(mmIH_CNTL, tmp2);

        IODelay(10);    //! give it a lil time to catch up

        callback->IHSetRBReadPointer(ihmgr, 0);
        LRed::callback->writeReg32(mmIH_RB_WPTR, 0);    //! what
        Support::callback->IHAcknowledgeAllOutStandingInterrupts(ihmgr);
    }
    getMember<char>(ihmgr, InterruptManagerFields::Unk1) = 0;    //! what
}
