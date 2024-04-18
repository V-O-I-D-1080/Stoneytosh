//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "Support.hpp"
#include "ATOMBIOS.hpp"
#include "LRed.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonSupport = "/System/Library/Extensions/AMDSupport.kext/Contents/MacOS/AMDSupport";

static KernelPatcher::KextInfo kextRadeonSupport = {"com.apple.kext.AMDSupport", &pathRadeonSupport, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

Support *Support::callback = nullptr;

void Support::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonSupport);
}

bool Support::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonSupport.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        const bool agdcon = checkKernelArgument("-LRedAGDCEnable");
        if (agdcon) {
            RouteRequestPlus request {"__ZN16AtiDeviceControl5startEP9IOService", wrapADCStart, this->orgADCStart};
            PANIC_COND(!request.route(patcher, index, address, size), "Support",
                "Failed to route AtiDeviceControl::start");
        }

        if (checkKernelArgument("-LRedVBIOSDebug")) {
            RouteRequestPlus request {
                "__ZN24AtiAtomFirmwareInterface16createAtomParserEP18BiosParserServicesPh11DCE_Version",
                wrapCreateAtomBiosParser, this->orgCreateAtomBiosParser};
            PANIC_COND(!request.route(patcher, index, address, size), "Support", "Failed to route createAtomParser");
        }

        if (LRed::callback->gcn3) {
            SolveRequestPlus solveRequests[] = {
                {"__ZN21AtiFbInterruptManager30initializePulseBasedInterruptsEb", this->IHInitPulseBasedInterrupts},
                {"__ZN21AtiFbInterruptManager32acknowledgeOutstandingInterruptsEv",
                    this->IHAcknowledgeAllOutStandingInterrupts},
            };
            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "Support",
                "Failed to solve symbols for CZ IH");
        }

        RouteRequestPlus requests[] = {
            {"__ZN13ATIController20populateDeviceMemoryE13PCI_REG_INDEX", wrapPopulateDeviceMemory,
                this->orgPopulateDeviceMemory},
            {"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange,
                this->orgNotifyLinkChange},
            {"__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram},
            {"__ZN13ATIController10doGPUPanicEPKcz", wrapDoGPUPanic},
            {"__ZN30AtiObjectInfoTableInterface_V14initERN21AtiDataTableBaseClass17DataTableInitInfoE",
                wrapObjectInfoTableInit, this->orgObjectInfoTableInit},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "Support",
            "Failed to route symbols");

        if (checkKernelArgument("-LRedAGDCPatch")) {
            const LookupPatchPlus patch {&kextRadeonSupport, kAtiDeviceControlGetVendorInfoOriginal,
                kAtiDeviceControlGetVendorInfoMask, kAtiDeviceControlGetVendorInfoPatched,
                kAtiDeviceControlGetVendorInfoMask, 1};
            PANIC_COND(!patch.apply(patcher, address, size), "Support", "Failed to apply getVendorInfo patch");
        }

        if (agdcon) {
            const LookupPatchPlus patch {&kextRadeonSupport, kATIControllerStartAGDCCheckOriginal,
                kATIControllerStartAGDCCheckMask, kATIControllerStartAGDCCheckPatched, kATIControllerStartAGDCCheckMask,
                1};
            PANIC_COND(!patch.apply(patcher, address, size), "Support",
                "Failed to apply ATIController::start AGDC Check patch");
        }

        return true;
    }

    return false;
}

IOReturn Support::wrapPopulateDeviceMemory(void *that, UInt32 reg) {
    FunctionCast(wrapPopulateDeviceMemory, callback->orgPopulateDeviceMemory)(that, reg);
    return kIOReturnSuccess;
}

bool Support::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
    UInt32 eventFlags) {
    LRed::callback->signalFBDumpDeviceInfo();
    auto ret = FunctionCast(wrapNotifyLinkChange, callback->orgNotifyLinkChange)(atiDeviceControl, event, eventData,
        eventFlags);

    DBGLOG("Support", "FB Link has changed! Event: %d, Data: %p, Flags: 0x%x", event, eventData, eventFlags);

    if (event == kAGDCValidateDetailedTiming) {
        auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
        DBGLOG("Support", "AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
        if (ret == false || cmd->modeStatus < 1 || cmd->modeStatus > 3) {
            cmd->modeStatus = 2;
            ret = true;
        }
    }

    return ret;
}

bool Support::doNotTestVram([[maybe_unused]] IOService *ctrl, [[maybe_unused]] UInt32 reg,
    [[maybe_unused]] bool retryOnFail) {
    LRed::callback->signalFBDumpDeviceInfo();
    return true;
}

void *Support::wrapCreateAtomBiosParser(void *that, void *param1, unsigned char *param2, UInt32 dceVersion) {
    DBGLOG("Support", "wrapCreateAtomBiosParser: DCE_Version: %d", dceVersion);
    getMember<UInt32>(param1, 0x4) = 0xFF;
    auto ret =
        FunctionCast(wrapCreateAtomBiosParser, callback->orgCreateAtomBiosParser)(that, param1, param2, dceVersion);
    return ret;
}

void Support::wrapDoGPUPanic() {
    DBGLOG("Support", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}

bool Support::wrapObjectInfoTableInit(void *that, void *initdata) {
    auto ret = FunctionCast(wrapObjectInfoTableInit, callback->orgObjectInfoTableInit)(that, initdata);
    struct ATOMObjTable *conInfoTbl = getMember<ATOMObjTable *>(that, 0x38);
    auto n = conInfoTbl->numberOfObjects;
    DBGLOG("Support", "Fixing VBIOS connectors");
    for (size_t i = 0, j = 0; i < n; i++) {
        UInt8 conObjType = (conInfoTbl->objects[i].objectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;
        //! Block out all invalid entries (ones that don't have `GRAPH_OBJECT_TYPE_CONNECTOR`)
        if (conObjType == GRAPH_OBJECT_TYPE_CONNECTOR) {
            conInfoTbl->objects[j] = conInfoTbl->objects[i];
            j++;
        } else {
            SYSLOG("Support", "Invalid Connector Info Table entry at index 0x%zx, with object type 0x%x", i,
                conObjType);
            conInfoTbl->numberOfObjects--;
        }
    }
    return ret;
}

void *Support::wrapADCStart(void *that, IOService *provider) {
    SYSLOG("Support", "Enabling AGDC, be warned that this is untested");
    auto ret = FunctionCast(wrapADCStart, callback->orgADCStart)(that, provider);
    getMember<bool>(that, 0x22A) = true;
    return ret;
}
