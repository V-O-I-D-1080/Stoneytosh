//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_support.hpp"
#include "kern_lred.hpp"
#include "kern_model.hpp"
#include "kern_patches.hpp"
#include "kern_vbios.hpp"
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
        auto vbiosdbg = checkKernelArgument("-lredvbiosdbg");
        auto adcpatch = checkKernelArgument("-lredadcpatch");
        auto gpiodbg = checkKernelArgument("-lredgpiodbg");
        auto agdcon = checkKernelArgument("-lredagdcon");
        auto isCarrizo = (LRed::callback->chipType >= ChipType::Carrizo);

        RouteRequestPlus requests[] = {
            {"__ZN13ATIController20populateDeviceMemoryE13PCI_REG_INDEX", wrapPopulateDeviceMemory,
                orgPopulateDeviceMemory},
            {"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange,
                orgNotifyLinkChange},
            {"__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram},
            {"__ZN24AtiAtomFirmwareInterface16createAtomParserEP18BiosParserServicesPh11DCE_Version",
                wrapCreateAtomBiosParser, orgCreateAtomBiosParser, vbiosdbg},
            {"__ZN25AtiGpioPinLutInterface_V114getGpioPinInfoEjRNS_11GpioPinInfoE", wrapGetGpioPinInfo,
                orgGetGpioPinInfo, gpiodbg},
            {"__ZN13ATIController10doGPUPanicEPKcz", wrapDoGPUPanic},
            {"__ZN14AtiVBiosHelper8getImageEjj", wrapGetImage, orgGetImage},
            {"__ZN30AtiObjectInfoTableInterface_V14initERN21AtiDataTableBaseClass17DataTableInitInfoE",
                wrapObjectInfoTableInit, orgObjectInfoTableInit},
            {"__ZN16AtiDeviceControl5startEP9IOService", wrapADCStart, orgADCStart, agdcon},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "support",
            "Failed to route symbols");

        LookupPatchPlus const patches[] = {
            {&kextRadeonSupport, kAtiDeviceControlGetVendorInfoOriginal, kAtiDeviceControlGetVendorInfoMask,
                kAtiDeviceControlGetVendorInfoPatched, kAtiDeviceControlGetVendorInfoMask,
                arrsize(kAtiDeviceControlGetVendorInfoOriginal), 1, adcpatch},
            {&kextRadeonSupport, kAtiBiosParser1SetDisplayClockOriginal, kAtiBiosParser1SetDisplayClockPatched,
                arrsize(kAtiBiosParser1SetDisplayClockOriginal), 1, isCarrizo},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, address, size), "support",
            "Failed to apply patches: %d", patcher.getError());
        DBGLOG("support", "Applied patches.");

        return true;
    }

    return false;
}

IOReturn Support::wrapPopulateDeviceMemory(void *that, uint32_t reg) {
    DBGLOG("support", "populateDeviceMemory: this = %p reg = 0x%X", that, reg);
    auto ret = FunctionCast(wrapPopulateDeviceMemory, callback->orgPopulateDeviceMemory)(that, reg);
    DBGLOG("support", "populateDeviceMemory returned 0x%X", ret);
    (void)ret;
    return kIOReturnSuccess;
}

bool Support::wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
    uint32_t eventFlags) {
    auto ret = FunctionCast(wrapNotifyLinkChange, callback->orgNotifyLinkChange)(atiDeviceControl, event, eventData,
        eventFlags);

    if (event == kAGDCValidateDetailedTiming) {
        auto cmd = static_cast<AGDCValidateDetailedTiming_t *>(eventData);
        DBGLOG("support", "AGDCValidateDetailedTiming %u -> %d (%u)", cmd->framebufferIndex, ret, cmd->modeStatus);
        if (ret == false || cmd->modeStatus < 1 || cmd->modeStatus > 3) {
            cmd->modeStatus = 2;
            ret = true;
        }
    }

    return ret;
}

bool Support::doNotTestVram([[maybe_unused]] IOService *ctrl, [[maybe_unused]] uint32_t reg,
    [[maybe_unused]] bool retryOnFail) {
    DBGLOG("support", "TestVRAM called! Returning true");
    auto *model = getBranding(LRed::callback->deviceId, LRed::callback->pciRevision);
    // Why do we set it here?
    // Our controller kexts override it, and since TestVRAM is later on after the controllers have started up, it's
    // desirable to do it here rather than later
    if (model) {
        auto len = static_cast<uint32_t>(strlen(model) + 1);
        LRed::callback->iGPU->setProperty("model", const_cast<char *>(model), len);
        LRed::callback->iGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
        LRed::callback->iGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, len - 11);
    }
    return true;
}

IOReturn Support::wrapGetGpioPinInfo(void *that, uint32_t pin, void *pininfo) {
    auto member = getMember<uint32_t>(that, 0x30);
    DBGLOG("support", "getGpioPinInfo: pin %x, member: %x", pin, member);
    (void)member;    // to also get clang-analyze to shut up
    auto ret = FunctionCast(wrapGetGpioPinInfo, callback->orgGetGpioPinInfo)(that, pin, pininfo);
    DBGLOG("support", "getGpioPinInfo: returned %x", ret);
    return ret;
}

void *Support::wrapCreateAtomBiosParser(void *that, void *param1, unsigned char *param2, uint32_t dceVersion) {
    DBGLOG("support", "wrapCreateAtomBiosParser: DCE_Version: %d", dceVersion);
    getMember<uint32_t>(param1, 0x4) = 0xFF;
    auto ret =
        FunctionCast(wrapCreateAtomBiosParser, callback->orgCreateAtomBiosParser)(that, param1, param2, dceVersion);
    return ret;
}

void Support::wrapDoGPUPanic() {
    DBGLOG("support", "doGPUPanic << ()");
    while (true) { IOSleep(3600000); }
}

void *Support::wrapGetImage(void *that, uint32_t offset, uint32_t length) {
    DBGLOG_COND(length == 0x12, "support", "Object Info Table is v1.3");
    if ((length == 0x12 || length == 0x10) && (callback->objectInfoFound == false)) {
        DBGLOG("support", "Current Object Info Table Offset = 0x%x", offset);
        callback->currentObjectInfoOffset = offset;
        callback->objectInfoFound = true;
    }
    DBGLOG("support", "getImage: offset: %x, length %x", offset, length);
    auto ret = FunctionCast(wrapGetImage, callback->orgGetImage)(that, offset, length);
    DBGLOG("support", "getImage: returned %x", ret);
    return ret;
}

bool Support::wrapObjectInfoTableInit(void *that, void *initdata) {
    auto ret = FunctionCast(wrapObjectInfoTableInit, callback->orgObjectInfoTableInit)(that, initdata);
    struct ATOMObjHeader *objHdr = getMember<ATOMObjHeader *>(that, 0x28);    // ?
    DBGLOG("support", "objectInfoTable values: conObjTblOff: %x, encObjTblOff: %x, dispPathTblOff: %x",
        objHdr->connectorObjectTableOffset, objHdr->encoderObjectTableOffset, objHdr->displayPathTableOffset);
    struct ATOMObjTable *conInfoTbl = getMember<ATOMObjTable *>(that, 0x38);
    void *vbiosparser = getMember<void *>(that, 0x10);
    ATOMDispObjPathTable *dispPathTable = static_cast<ATOMDispObjPathTable *>(FunctionCast(wrapGetImage,
        callback->orgGetImage)(vbiosparser, callback->currentObjectInfoOffset + objHdr->displayPathTableOffset, 0xE));
    DBGLOG("support", "dispObjPathTable: numDispPaths = 0x%x, version: 0x%x", dispPathTable->numOfDispPath,
        dispPathTable->version);
    auto n = dispPathTable->numOfDispPath;
    DBGLOG("support", "Fixing VBIOS connectors");
    for (size_t i = 0, j = 0; i < n; i++) {
        // Skip invalid device tags
        if (dispPathTable->dispPath[i].deviceTag) {
            uint8_t conObjType = (conInfoTbl->objects[i].objectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;
            DBGLOG("support", "connectorInfoTable: connector: %x, objects: %x, objectId: %x, objectTypeFromId: %x", i,
                conInfoTbl->numberOfObjects, conInfoTbl->objects[i].objectID,
                (conInfoTbl->objects[i].objectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT);
            if (conObjType != GRAPH_OBJECT_TYPE_CONNECTOR) {
                // Trims out any non-connector objects, proven to work on 2 machines, one with all connectors properly
                // defined, one with 2/3 being valid connectors
                SYSLOG("support",
                    "Connector %x's objectType is not GRAPH_OBJECT_TYPE_CONNECTOR!, detected objectType: %x", i,
                    conObjType);
                conInfoTbl->numberOfObjects--;
                dispPathTable->numOfDispPath--;
            } else {
                conInfoTbl->objects[j++] = conInfoTbl->objects[i];
                dispPathTable->dispPath[j] = dispPathTable->dispPath[i];
            }
        } else {
            dispPathTable->numOfDispPath--;
            conInfoTbl->numberOfObjects--;
        }
    }
    DBGLOG("support", "Results: numOfDispPath: 0x%x, numberOfObjects: 0x%x", dispPathTable->numOfDispPath,
        conInfoTbl->numberOfObjects);
    LRed::callback->iGPU->setProperty("CFG_FB_LIMIT", conInfoTbl->numberOfObjects, sizeof(conInfoTbl->numberOfObjects));
    // WEG sets this property to the number of connectors used in AtiBiosParserX::getConnectorInfo, so we use
    // numberOfObjects instead
    return ret;
}

void *Support::wrapADCStart(void *that, IOService *provider) {
    SYSLOG("support", "Enabling AGDC, be warned that this is untested");
    bool agdcon = true;
    LRed::callback->iGPU->setProperty("CFG_USE_AGDC", agdcon, sizeof(agdcon));
    auto ret = FunctionCast(wrapADCStart, callback->orgADCStart)(that, provider);
    return ret;
}