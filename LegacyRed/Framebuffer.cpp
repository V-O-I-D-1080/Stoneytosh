//! Copyright © 2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "Framebuffer.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

static const char *pathAMDFramebuffer = "/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/AMDFramebuffer";

static KernelPatcher::KextInfo kextAMDFramebuffer = {"com.apple.kext.AMDFramebuffer", &pathAMDFramebuffer, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

Framebuffer *Framebuffer::callback = nullptr;

void Framebuffer::init() {
    callback = this;
    this->fbPtrs[0] = nullptr;
    this->fbPtrs[1] = nullptr;
    this->fbPtrs[2] = nullptr;
    this->fbPtrs[3] = nullptr;
    this->fbPtrs[4] = nullptr;
    this->fbPtrs[5] = nullptr;
    lilu.onKextLoadForce(&kextAMDFramebuffer);
}

bool Framebuffer::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextAMDFramebuffer.loadIndex == index) {
        // if (PE_parse_boot_argn("AMDBitsPerComponent", &this->bitsPerComponent, sizeof(this->bitsPerComponent))) {
        // DBGLOG("Framebuffer", "Bits Per Component: %d", this->bitsPerComponent);
        //}
        SolveRequestPlus solveRequests[] = {
            {"__ZN14AMDFramebuffer32getDevicePropertiesForUserClientEP12OSDictionary", this->orgGetDevPropsForUC},
            {"__ZN14AMDFramebuffer26getPropertiesForUserClientEP12OSDictionary", this->orgGetPropsForUC},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "Framebuffer", "Failed to solve symbols");

        RouteRequestPlus requests[] {
            {"__ZN14AMDFramebuffer30populateDisplayModeInformationEP28AtiDetailedTimingInformationP6WindowS3_S3_"
              "P24IODisplayModeInformation", wrapPopulateDisplayModeInfo, this->orgPopulateDisplayModeInfo},
            {"__ZN14AMDFramebuffer5startEP9IOService", wrapStart, this->orgStart},
        };
        PANIC_COND(!request.route(patcher, index, address, size), "Framebuffer",
            "Failed to route populateDisplayModeInformation!");
        return true;
    }
    return false;
}

IOReturn Framebuffer::dumpAllFramebuffers() {
    OSDictionary *upperDict = OSDictionary::withCapacity(6);
    if (upperDict == nullptr) {
        DBGLOG("Framebuffer", "Failed to create dictionary");
        return kIOReturnNoMemory;
    } else if (fbPtrs[0] == nullptr) {
        DBGLOG("Framebuffer", "Cannot dump at this time, FB 0 pointer is null.");
        return kIOReturnNoDevice;
    }
    for (int i = 0; i < MAX_FRAMEBUFFER_COUNT; i++) {
        if (callback->fbPtrs[i] == nullptr) {
            break;
        } else {
            OSDictionary *dict = OSDictionary::withCapacity(1);
            if (dict == nullptr) {
                DBGLOG("Framebuffer", "Failed to create dictionary");
                OSSafeReleaseNULL(upperDict);
                return kIOReturnNoMemory;
            }
            callback->orgGetPropsForUC(callback->fbPtrs[i], dict);
            char name[128];
            snprintf(name, 128, "Framebuffer %d", i);
            upperDict->setObject(name, dict);
        }
    }
    LRed::callback->iGPU->setProperty("iGPU Framebuffer Config", upperDict);
    OSSafeReleaseNULL(upperDict);
    return kIOReturnSuccess;
}

IOReturn Framebuffer::fbDumpDevProps() {
    if (this->fbPtrs[0] == nullptr) {
        DBGLOG("Framebuffer", "Cannot dump at this time, FB 0 pointer is null.");
        return kIOReturnNoDevice;
    }

    OSDictionary *dict = OSDictionary::withCapacity(1);
    if (dict == nullptr) {
        DBGLOG("Framebuffer", "Failed to create dictionary");
        return kIOReturnNoMemory;
    }

    //! funnily enough theres one where we can dump the FB itself
    callback->orgGetDevPropsForUC(callback->fbPtrs[0], dict);    //! attrocity #1

    LRed::callback->iGPU->setProperty("iGPU Device Config", dict);
    OSSafeReleaseNULL(dict);

    return kIOReturnSuccess;
}

IOReturn Framebuffer::wrapPopulateDisplayModeInfo(void *that, void *detailedTiming, void *param2, void *param3,
    void *param4, void *modeInfo) {
    DBGLOG("Framebuffer",
        "populateDisplayModeInfo: that %p, detailedTming %p, param2 %p, param3 %p, param5 %p, modeInfo %p", that,
        detailedTiming, param2, param3, param4, modeInfo);
    IOReturn ret = FunctionCast(wrapPopulateDisplayModeInfo, callback->orgPopulateDisplayModeInfo)(that, detailedTiming,
        param2, param3, param4, modeInfo);
    //! BROKEN
    /*
    switch (callback->bitsPerComponent) {
            case 0:
                    break;
            case 5:
                    getMember<UInt32>(modeInfo, 0xC) = 0;
                    break;
            case 8:
                    getMember<UInt32>(modeInfo, 0xC) = 1;
                    break;
            case 10:
                    getMember<UInt32>(modeInfo, 0xC) = 2;
                    break;
            default:
                    PANIC("Framebuffer", "Unsupported bits per component value!");
    }
    */
    DBGLOG("FB", "populateDisplayModeInfo: ret: 0x%x, maxDepthIndex: 0x%x (%d)", ret,
        getMember<UInt32>(modeInfo, 0xC), getMember<UInt32>(modeInfo, 0xC));
    return ret;
}

bool Framebuffer::wrapStart(void *that, void *provider) {
    DBGLOG("FB", "<%p>::start(%p)", that, provider);
    for (int i = 0; i < MAX_FRAMEBUFFER_COUNT; i++) {
        if (callback->fbPtrs[i] == nullptr) {
            callback->fbPtrs[i] = that;
        }
    }
    return FunctionCast(wrapStart, callback->orgStart)(that, provider);
}
