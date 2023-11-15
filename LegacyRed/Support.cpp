//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "Support.hpp"
#include "ATOMBIOS.hpp"
#include "LRed.hpp"
#include "Model.hpp"
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

        const bool agdcon = checkKernelArgument("-lredagdcon");
        if (agdcon) {
            RouteRequestPlus request {"__ZN16AtiDeviceControl5startEP9IOService", wrapADCStart, this->orgADCStart};
            PANIC_COND(!request.route(patcher, index, address, size), "Support",
                "Failed to route AtiDeviceControl::start");
        }

        if (checkKernelArgument("-lredvbiosdbg")) {
            RouteRequestPlus request {
                "__ZN24AtiAtomFirmwareInterface16createAtomParserEP18BiosParserServicesPh11DCE_Version",
                wrapCreateAtomBiosParser, this->orgCreateAtomBiosParser};
            PANIC_COND(!request.route(patcher, index, address, size), "Support", "Failed to route createAtomParser");
        }

        RouteRequestPlus requests[] = {
            {"__ZN13ATIController20populateDeviceMemoryE13PCI_REG_INDEX", wrapPopulateDeviceMemory,
                this->orgPopulateDeviceMemory},
            {"__ZN16AtiDeviceControl16notifyLinkChangeE31kAGDCRegisterLinkControlEvent_tmj", wrapNotifyLinkChange,
                this->orgNotifyLinkChange},
            {"__ZN13ATIController8TestVRAME13PCI_REG_INDEXb", doNotTestVram},
            {"__ZN13ATIController10doGPUPanicEPKcz", wrapDoGPUPanic},
            {"__ZN14AtiVBiosHelper8getImageEjj", wrapGetImage, this->orgGetImage},
            {"__ZN30AtiObjectInfoTableInterface_V14initERN21AtiDataTableBaseClass17DataTableInitInfoE",
                wrapObjectInfoTableInit, this->orgObjectInfoTableInit},
            {"__ZN30AtiObjectInfoTableInterface_V121createObjectInfoTableEP14AtiVBiosHelperj",
                wrapCreateObjectInfoTable, this->orgCreateObjectInfoTable},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "Support",
            "Failed to route symbols");

        if (checkKernelArgument("-lredadcpatch")) {
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
    auto ret = FunctionCast(wrapNotifyLinkChange, callback->orgNotifyLinkChange)(atiDeviceControl, event, eventData,
        eventFlags);

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
    DBGLOG("Support", "TestVRAM called! Returning true");
    auto *model = getBranding(LRed::callback->deviceId, LRed::callback->pciRevision);
    //! Why do we set it here?
    //! Our controller kexts override it, and since TestVRAM is later on after the controllers have started up, it's
    //! desirable to do it here rather than later
    if (model) {
        auto len = static_cast<UInt32>(strlen(model) + 1);
        LRed::callback->iGPU->setProperty("model", const_cast<char *>(model), len);
        LRed::callback->iGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
        LRed::callback->iGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, len - 11);
    }
    return true;
}

IOReturn Support::wrapGetGpioPinInfo(void *that, UInt32 pin, void *pininfo) {
    auto member = getMember<UInt32>(that, 0x30);
    DBGLOG("Support", "getGpioPinInfo: pin %x, member: %x", pin, member);
    (void)member;    // to also get clang-analyze to shut up
    auto ret = FunctionCast(wrapGetGpioPinInfo, callback->orgGetGpioPinInfo)(that, pin, pininfo);
    DBGLOG("Support", "getGpioPinInfo: returned %x", ret);
    return ret;
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

void *Support::wrapGetImage(void *that, UInt32 offset, UInt32 length) {
    DBGLOG("Support", "getImage: offset: %x, length %x", offset, length);
    auto ret = FunctionCast(wrapGetImage, callback->orgGetImage)(that, offset, length);
    DBGLOG("Support", "getImage: returned %llX", ret);
    return ret;
}

void *Support::wrapCreateObjectInfoTable(void *helper, UInt32 offset) {
    DBGLOG("Support", "wrapCreateObjectInfoTable: Object Info Table Offset: 0x%x", offset);
    callback->currentObjectInfoOffset = offset;
    auto ret = FunctionCast(wrapCreateObjectInfoTable, callback->orgCreateObjectInfoTable)(helper, offset);
    return ret;
}

bool Support::wrapObjectInfoTableInit(void *that, void *initdata) {
    auto ret = FunctionCast(wrapObjectInfoTableInit, callback->orgObjectInfoTableInit)(that, initdata);
    struct ATOMObjHeader *objHdr = getMember<ATOMObjHeader *>(that, 0x28);    // ?
    DBGLOG("Support", "objectInfoTable values: conObjTblOff: %x, encObjTblOff: %x, dispPathTblOff: %x",
        objHdr->connectorObjectTableOffset, objHdr->encoderObjectTableOffset, objHdr->displayPathTableOffset);
    struct ATOMObjTable *conInfoTbl = getMember<ATOMObjTable *>(that, 0x38);
    void *vbiosparser = getMember<void *>(that, 0x10);
    ATOMDispObjPathTable *dispPathTable = static_cast<ATOMDispObjPathTable *>(FunctionCast(wrapGetImage,
        callback->orgGetImage)(vbiosparser, callback->currentObjectInfoOffset + objHdr->displayPathTableOffset, 0xE));
    DBGLOG("Support", "dispObjPathTable: numDispPaths = 0x%x, version: 0x%x", dispPathTable->numOfDispPath,
        dispPathTable->version);
    auto n = dispPathTable->numOfDispPath;
    DBGLOG("Support", "Fixing VBIOS connectors");
    for (size_t i = 0, j = 0; i < n; i++) {
        //! Skip invalid device tags
        if (dispPathTable->dispPath[i].deviceTag) {
            UInt8 conObjType = (conInfoTbl->objects[i].objectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;
            DBGLOG("Support", "connectorInfoTable: connector: %zx, objects: %x, objectId: %x, objectTypeFromId: %x", i,
                conInfoTbl->numberOfObjects, conInfoTbl->objects[i].objectID,
                (conInfoTbl->objects[i].objectID & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT);
            if (conObjType != GRAPH_OBJECT_TYPE_CONNECTOR) {
                //! Trims out any non-connector objects, proven to work on 2 machines, one with all connectors properly
                //! defined, one with 2/3 being valid connectors
                SYSLOG("Support",
                    "Connector %zx's objectType is not GRAPH_OBJECT_TYPE_CONNECTOR!, detected objectType: %x, if this "
                    "is a mistake please file a bug report!",
                    i, conObjType);
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
    DBGLOG("Support", "Results: numOfDispPath: 0x%x, numberOfObjects: 0x%x", dispPathTable->numOfDispPath,
        conInfoTbl->numberOfObjects);
    LRed::callback->iGPU->setProperty("CFG_FB_LIMIT", conInfoTbl->numberOfObjects, sizeof(conInfoTbl->numberOfObjects));
    // WEG sets this property to the number of connectors used in AtiBiosParserX::getConnectorInfo, so we use
    // numberOfObjects instead
    return ret;
}

void *Support::wrapADCStart(void *that, IOService *provider) {
    SYSLOG("Support", "Enabling AGDC, be warned that this is untested");
    auto ret = FunctionCast(wrapADCStart, callback->orgADCStart)(that, provider);
    getMember<bool>(that, 0x22A) = true;
    return ret;
}
