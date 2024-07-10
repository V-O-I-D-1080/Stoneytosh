//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "X4000.hpp"
#include "LRed.hpp"
#include "Model.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX4000 = "/System/Library/Extensions/AMDRadeonX4000.kext/Contents/MacOS/AMDRadeonX4000";
static KernelPatcher::KextInfo kextRadeonX4000 {"com.apple.kext.AMDRadeonX4000", &pathRadeonX4000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

X4000 *X4000::callback = nullptr;

void X4000::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX4000);
}

bool X4000::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX4000.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();
        //! To keep things brief:
        //! Carizzo uses UVD 6.0 and VCE 3.1, Stoney uses UVD 6.2 and VCE 3.4.
        //! Both can only encode H264, but can decode HEVC.
        const bool stoney = LRed::callback->chipType == ChipType::Stoney;
        const bool carrizo = LRed::callback->chipType == ChipType::Carrizo;

        this->dumpIBs = checkKernelArgument("-X4KDumpAllIBs");

        UInt32 *orgChannelTypes = nullptr;
        mach_vm_address_t startHWEngines = 0;

        const bool sml = checkKernelArgument("-CKSMLFirmwareInjection");

        if (stoney) {
            SolveRequestPlus solveRequests[] = {
                {"__ZN28AMDRadeonX4000_AMDVIHardware32setupAndInitializeHWCapabilitiesEv",
                    this->orgSetupAndInitializeHWCapabilities},
                {"__ZZN37AMDRadeonX4000_AMDGraphicsAccelerator19createAccelChannelsEbE12channelTypes", orgChannelTypes},
                {"__ZN26AMDRadeonX4000_AMDHardware14startHWEnginesEv", startHWEngines},
            };
            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "X4000",
                "Failed to resolve symbols for Stoney");
        } else if (carrizo) {
            SolveRequestPlus request {"__ZN28AMDRadeonX4000_AMDVIHardware32setupAndInitializeHWCapabilitiesEv",
                this->orgSetupAndInitializeHWCapabilities};
            PANIC_COND(!request.solve(patcher, index, address, size), "X4000", "Failed to solve HWCapabilities");
        } else {
            SolveRequestPlus request {"__ZN28AMDRadeonX4000_AMDCIHardware32setupAndInitializeHWCapabilitiesEv",
                this->orgSetupAndInitializeHWCapabilities};
            PANIC_COND(!request.solve(patcher, index, address, size), "X4000", "Failed to solve HWCapabilities");
        }

        if (stoney) {
            RouteRequestPlus requests[] = {
                {"__ZN35AMDRadeonX4000_AMDEllesmereHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDVIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
                {"__ZN26AMDRadeonX4000_AMDHardware12getHWChannelE20_eAMD_HW_ENGINE_TYPE18_eAMD_HW_RING_TYPE",
                    wrapGetHWChannel, this->orgGetHWChannel},
                {"__ZN38AMDRadeonX4000_AMDVIPM4CommandsUtility26buildIndirectBufferCommandEPjyj26_eAMD_INDIRECT_BUFFER_"
                 "TYPEjbj",
                    wrapBuildIBCommand, this->orgBuildIBCommand},
                {"__ZN28AMDRadeonX4000_AMDVIHardware28initializeSystemApertureRegsEv", initializeSystemApertureRegs},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                "Failed to route symbols");
            if (sml) {
                RouteRequestPlus requests[] = {
                    {nullptr, wrapAMDSMLUVDInit, this->orgAMDSMLUVDInit, kAMDUVD6v3InitBigSur},
                    {nullptr, wrapAMDSMLVCEInit, this->orgAMDSMLVCEInit, kAMDVCE3v4InitBigSur},
                };
                PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                    "Failed to route symbols");
            }
        } else if (carrizo) {
            RouteRequestPlus requests[] = {
                // replace with Tonga PM4 instead? or did that HW cap in Tonga specifiy something?
                {"__ZN31AMDRadeonX4000_AMDTongaHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDVIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
                {"__ZN38AMDRadeonX4000_AMDVIPM4CommandsUtility26buildIndirectBufferCommandEPjyj26_eAMD_INDIRECT_BUFFER_"
                 "TYPEjbj",
                    wrapBuildIBCommand, this->orgBuildIBCommand},
                {"__ZN28AMDRadeonX4000_AMDVIHardware28initializeSystemApertureRegsEv", initializeSystemApertureRegs},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                "Failed to route symbols");
            if (sml) {
                RouteRequestPlus requests[] = {
                    {nullptr, wrapAMDSMLUVDInit, this->orgAMDSMLUVDInit, kAMDUVD6v3InitBigSur},
                    {nullptr, wrapAMDSMLVCEInit, this->orgAMDSMLVCEInit, kAMDVCE3v4InitBigSur},
                };
                PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                    "Failed to route symbols");
            }
        } else {
            RouteRequestPlus requests[] = {
                {"__ZN33AMDRadeonX4000_AMDBonaireHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDCIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
                {"__ZN29AMDRadeonX4000_AMDCIPM4Engine21initializeMicroEngineEv", wrapInitializeMicroEngine,
                    this->orgInitializeMicroEngine},
                {"__ZN28AMDRadeonX4000_AMDCIHardware16initializeVMRegsEv", wrapInitializeVMRegs,
                    this->orgInitializeVMRegs},
                {"__ZN38AMDRadeonX4000_AMDCIPM4CommandsUtility26buildIndirectBufferCommandEPjyj26_eAMD_INDIRECT_BUFFER_"
                 "TYPEjbj",
                    wrapBuildIBCommand, this->orgBuildIBCommand},
                {"__ZN28AMDRadeonX4000_AMDCIHardware28initializeSystemApertureRegsEv", initializeSystemApertureRegs},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                "Failed to route symbols");
        }

        RouteRequestPlus requests[] = {
            {"__ZN37AMDRadeonX4000_AMDGraphicsAccelerator5startEP9IOService", wrapAccelStart, orgAccelStart},
            {"__ZN26AMDRadeonX4000_AMDHardware17dumpASICHangStateEb.cold.1", wrapDumpASICHangState},
            {"__ZN26AMDRadeonX4000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress,
                this->orgAdjustVRAMAddress},
            {"__ZN4Addr2V15CiLib19HwlInitGlobalParamsEPK18_ADDR_CREATE_INPUT", wrapHwlInitGlobalParams,
                orgHwlInitGlobalParams},
            {"__ZN35AMDRadeonX4000_AMDAccelVideoContext9getHWInfoEP13sHardwareInfo", wrapGetHWInfo, this->orgGetHWInfo},
            {"__ZN29AMDRadeonX4000_AMDHWRegisters5writeEjj", wrapAMDHWRegsWrite, this->orgAMDHWRegsWrite},
            {"__ZN29AMDRadeonX4000_AMDCommandRing9writeDataEPKjj", wrapWriteData, this->orgWriteData},
            {"__ZN25AMDRadeonX4000_IAMDHWRing5writeEj", wrapHWRingWrite, this->orgHWRingWrite},
            {"__ZN27AMDRadeonX4000_AMDHWChannel19submitCommandBufferEP30AMD_SUBMIT_COMMAND_BUFFER_INFO",
                wrapSubmitCommandBufferInfo, this->orgSubmitCommandBufferInfo},
            {"__ZN30AMDRadeonX4000_AMDPM4HWChannel17performClearStateEv", performClearState,
                this->orgPerformClearState},
            {"__ZN26AMDRadeonX4000_AMDHWMemory12getRangeInfoE22eAMD_MEMORY_RANGE_TYPEP21AMD_MEMORY_RANGE_INFO",
                wrapGetRangeInfo, this->orgGetRangeInfo},
            {"__ZN26AMDRadeonX4000_AMDHWMemory4initEP30AMDRadeonX4000_IAMDHWInterface", wrapHWMemoryInit, this->orgHWMemoryInit},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
            "Failed to route symbols");

        if (stoney) {
            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X4000",
                "Failed to enable kernel writing");
            orgChannelTypes[5] = 1;     //! Fix createAccelChannels so that it only starts SDMA0
            orgChannelTypes[11] = 0;    //! Fix getPagingChannel so that it gets SDMA0
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

            const LookupPatchPlus allocHWEnginesPatch {&kextRadeonX4000, kAMDEllesmereHWallocHWEnginesOriginal,
                kAMDEllesmereHWallocHWEnginesPatched, 1};
            PANIC_COND(!allocHWEnginesPatch.apply(patcher, address, size), "X4000",
                "Failed to apply AllocateHWEngines patch: %d", patcher.getError());

            const LookupPatchPlus patch {&kextRadeonX4000, kStartHWEnginesOriginal, kStartHWEnginesMask,
                kStartHWEnginesPatched, kStartHWEnginesMask, 1};
            PANIC_COND(!patch.apply(patcher, startHWEngines, PAGE_SIZE), "X4000", "Failed to patch startHWEngines");
            DBGLOG("X4000", "Applied Singular SDMA lookup patch");
        }

        this->mcLocation = ((LRed::callback->readReg32(mmMC_VM_FB_LOCATION) & 0xFFFF) << 24);
        DBGLOG("X4000", "mcLocation: 0x%llx", this->mcLocation);

        return true;
    }

    return false;
}

bool X4000::wrapAccelStart(void *that, IOService *provider) {
    DBGLOG("X4000", "accelStart << (this: %p provider: %p)", that, provider);
    callback->callbackAccelerator = that;
    auto ret = FunctionCast(wrapAccelStart, callback->orgAccelStart)(that, provider);
    DBGLOG("X4000", "accelStart >> %d", ret);
    return ret;
}

enum HWCapability : UInt64 {
    DisplayPipeCount = 0x04,    //! UInt32, unsure
    SECount = 0x34,             //! UInt32
    SHPerSE = 0x3C,             //! UInt32
    CUPerSH = 0x70,             //! UInt32
    Unknown0 = 0x8F,            //! bool
};

template<typename T>
static inline void setHWCapability(void *that, HWCapability capability, T value) {
    getMember<T>(that, (getKernelVersion() >= KernelVersion::Ventura ? 0x30 : 0x28) + capability) = value;
}

void X4000::wrapSetupAndInitializeHWCapabilities(void *that) {
    DBGLOG("X4000", "setupAndInitializeHWCapabilities: this = %p", that);
    UInt32 cuPerSh = 2;
    if (LRed::callback->chipType <= ChipType::Spooky || LRed::callback->chipType == ChipType::Carrizo) { cuPerSh = 8; }
    setHWCapability<UInt32>(that, HWCapability::SECount, 1);
    setHWCapability<UInt32>(that, HWCapability::SHPerSE, 1);
    setHWCapability<UInt32>(that, HWCapability::CUPerSH, cuPerSh);
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callback->orgSetupAndInitializeHWCapabilities)(that);

    if (LRed::callback->chipType == ChipType::Stoney) { setHWCapability<bool>(that, HWCapability::Unknown0, true); }
}

void X4000::wrapInitializeFamilyType(void *that) {
    DBGLOG("X4000", "initializeFamilyType << %x", LRed::callback->familyId);
    getMember<UInt32>(that, 0x308) = LRed::callback->familyId;
    auto *model = getBranding(LRed::callback->deviceId, LRed::callback->pciRevision);
    //! Why do we set it here?
    //! Our controller kexts override it if done @ processPatcher
    if (model) {
        auto len = static_cast<UInt32>(strlen(model) + 1);
        LRed::callback->iGPU->setProperty("model", const_cast<char *>(model), len);
        LRed::callback->iGPU->setProperty("ATY,FamilyName", const_cast<char *>("Radeon"), 7);
        LRed::callback->iGPU->setProperty("ATY,DeviceName", const_cast<char *>(model) + 11, len - 11);
    }
}

void *X4000::wrapGetHWChannel(void *that, UInt32 engineType, UInt32 ringId) {
    //! Redirect SDMA1 engine type to SDMA0
    return FunctionCast(wrapGetHWChannel, callback->orgGetHWChannel)(that, (engineType == 2) ? 1 : engineType, ringId);
}

void X4000::wrapDumpASICHangState() {
    DBGLOG("X4000", "dumpASICHangState <<");
    while (true) { IOSleep(36000000); }
}

UInt64 X4000::wrapAdjustVRAMAddress(void *that, UInt64 addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, callback->orgAdjustVRAMAddress)(that, addr);
    SYSLOG("X4000", "AdjustVRAMAddress: returned: 0x%llx, our value: 0x%llx", ret,
        ret != addr ? (ret + LRed::callback->fbOffset) : ret);
    return ret != addr ? (ret + LRed::callback->fbOffset) : ret;
}

bool X4000::wrapInitializeMicroEngine(void *that) {
    DBGLOG("X4000", "initializeMicroEngine << (that: %p)", that);
    LRed::callback->signalFBDumpDeviceInfo();
    auto ret = FunctionCast(wrapInitializeMicroEngine, callback->orgInitializeMicroEngine)(that);
    DBGLOG("X4000", "initializeMicroEngine >> %d", ret);
    return ret;
}

void X4000::wrapInitializeVMRegs(void *that) {
    if (LRed::callback->chipType <= ChipType::Spooky) {
        UInt32 tmp = LRed::callback->readReg32(mmCHUB_CONTROL);
        tmp &= ~bypassVM;
        LRed::callback->writeReg32(mmCHUB_CONTROL, tmp);
    }
    FunctionCast(wrapInitializeVMRegs, callback->orgInitializeVMRegs)(that);
}

int X4000::wrapHwlInitGlobalParams(void *that, const void *creationInfo) {
    auto ret = FunctionCast(wrapHwlInitGlobalParams, callback->orgHwlInitGlobalParams)(that, creationInfo);
    getMember<UInt32>(that, 0x38) = (LRed::callback->chipType == ChipType::Spectre) ? 4 : 2;
    return ret;
}

IOReturn X4000::wrapGetHWInfo(void *ctx, void *hwInfo) {
    auto ret = FunctionCast(wrapGetHWInfo, callback->orgGetHWInfo)(ctx, hwInfo);
    //! 0x67DF - Ellesmere, 0x7300 - Fiji, 0x6640 - Bonaire.
    getMember<UInt32>(hwInfo, 0x4) = LRed::callback->gcn3 ? (LRed::callback->stoney ? 0x67DF : 0x7300) : 0x6640;
    return ret;
}

bool X4000::wrapAMDSMLUVDInit(void *that) {
    auto ret = FunctionCast(wrapAMDSMLUVDInit, callback->orgAMDSMLUVDInit)(that);
    DBGLOG("X4000", "SML UVD: init >>");
    char filename[128];
    const char *prefix = LRed::getUVDPrefix();
    snprintf(filename, 128, "%s_nd.dat", prefix);
    auto &fwDesc = getFWDescByName(filename);
    getMember<UInt32>(that, 0x2C) = fwDesc.size;
    getMember<const UInt8 *>(that, 0x30) = fwDesc.data;
    return ret;
}

bool X4000::wrapAMDSMLVCEInit(void *that) {
    auto ret = FunctionCast(wrapAMDSMLVCEInit, callback->orgAMDSMLVCEInit)(that);
    DBGLOG("X4000", "SML VCE: init >>");
    char filename[128];
    const char *prefix = LRed::getVCEPrefix();
    snprintf(filename, 128, "%s.dat", prefix);
    auto &fwDesc = getFWDescByName(filename);
    getMember<UInt32>(that, 0x14) = fwDesc.size;
    getMember<const UInt8 *>(that, 0x18) = fwDesc.data;
    return ret;
}

//! free dmesg spam for 150$!!!!!!
void X4000::wrapAMDHWRegsWrite(void *that, UInt32 addr, UInt32 val) {
    DBGLOG("X4000", "ACCEL REG WRITE >> addr: 0x%x, val: 0x%x", addr, val);
    if (addr == mmSRBM_SOFT_RESET) {
        val &= ~SRBM_SOFT_RESET__SOFT_RESET_MC_MASK;
        DBGLOG("X4000", "Stripping SRBM_SOFT_RESET__SOFT_RESET_MC_MASK bit");
    }
    FunctionCast(wrapAMDHWRegsWrite, callback->orgAMDHWRegsWrite)(that, addr, val);
}

uint64_t X4000::wrapWriteData(void *that, const UInt32 *data, UInt32 size) {
    SYSLOG("X4000", "COMMAND RING WRITE --- DATA: 0x%x --- SIZE: 0x%x", *data, size);
    return FunctionCast(wrapWriteData, callback->orgWriteData)(that, data, size);
}

bool X4000::wrapHWRingWrite(void *that, UInt32 data) {
    SYSLOG("X4000", "IAMDHWRing WRITE --- DATA: 0x%x", data);
    return FunctionCast(wrapHWRingWrite, callback->orgHWRingWrite)(that, data);
}

bool isInPerformClearState = false;

enum HWMemoryFields {
    VRAMMCBaseAddress = 0x50,
    VRAMPhysicalOffset = 0x58,
    SharedApertureBaseAddr = 0x60,
};

bool X4000::performClearState(void *that) {
    isInPerformClearState = true;
    DBGLOG("X4000", "mmVM_CONTEXT0_PAGE_TABLE_START_ADDR = 0x%x",
        LRed::callback->readReg32(mmVM_CONTEXT0_PAGE_TABLE_START_ADDR));
    DBGLOG("X4000", "mmVM_CONTEXT1_PAGE_TABLE_START_ADDR = 0x%x",
        LRed::callback->readReg32(mmVM_CONTEXT1_PAGE_TABLE_START_ADDR));
    DBGLOG("X4000", "mmVM_CONTEXT0_PAGE_TABLE_END_ADDR = 0x%x",
        LRed::callback->readReg32(mmVM_CONTEXT0_PAGE_TABLE_END_ADDR));
    DBGLOG("X4000", "mmVM_CONTEXT1_PAGE_TABLE_END_ADDR = 0x%x",
        LRed::callback->readReg32(mmVM_CONTEXT1_PAGE_TABLE_END_ADDR));
    DBGLOG("X4000", "mmMC_VM_AGP_TOP = 0x%x", LRed::callback->readReg32(mmMC_VM_AGP_TOP));
    DBGLOG("X4000", "mmMC_VM_AGP_BOT = 0x%x", LRed::callback->readReg32(mmMC_VM_AGP_BOT));
    DBGLOG("X4000", "mmMC_VM_AGP_BASE = 0x%x", LRed::callback->readReg32(mmMC_VM_AGP_BASE));
    DBGLOG("X4000", "mmMC_VM_SYSTEM_APERTURE_LOW_ADDR = 0x%x",
        LRed::callback->readReg32(mmMC_VM_SYSTEM_APERTURE_LOW_ADDR));
    DBGLOG("X4000", "mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR = 0x%x",
        LRed::callback->readReg32(mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR));
    DBGLOG("X4000", "mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR = 0x%x",
        LRed::callback->readReg32(mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR));
    if (callback->hwMemPtr != nullptr) { //! juuuust in case.
        SYSLOG("X4000", "HWMem: base: 0x%llx", getMember<UInt64>(callback->hwMemPtr, HWMemoryFields::VRAMMCBaseAddress));
        SYSLOG("X4000", "HWMem: offset: 0x%llx", getMember<UInt64>(callback->hwMemPtr, HWMemoryFields::VRAMPhysicalOffset));
        //! TODO: find out if we need to inject sharedaper
        SYSLOG("X4000", "HWMem: sharedaper: 0x%llx", getMember<UInt64>(callback->hwMemPtr, HWMemoryFields::SharedApertureBaseAddr));
    }
    auto ret = FunctionCast(performClearState, callback->orgPerformClearState)(that);
    isInPerformClearState = false;
    return ret;
}

UInt32 X4000::wrapSubmitCommandBufferInfo(void *that, UInt8 *data) {
    if (isInPerformClearState || callback->dumpIBs) {
        const size_t size = 2048;
        char *output = new char[size];
        bzero(output, size);
        for (size_t i = 0; i < 0x60; i++) {
            if ((i + 1) % 0x10) {
                sprintf(output, "%s %02X", output, data[i]);
            } else {
                sprintf(output, "%s %02X\n", output, data[i]);
            }
        }
        SYSLOG("X4000", "COMMAND_BUFFER_INFO:\n%s", output);
        delete[] output;
    }
    auto ret = FunctionCast(wrapSubmitCommandBufferInfo, callback->orgSubmitCommandBufferInfo)(that, data);
    return ret;
}

UInt64 X4000::wrapBuildIBCommand(void *that, UInt32 *rawPkt, UInt64 param2, UInt32 param3, UInt64 ibType, UInt32 param5,
    bool param6, UInt32 param7) {
    DBGLOG("X4000", "buildIBCommand >> 0x%llx, 0x%x, 0x%llx, 0x%x, 0x%x, 0x%x", param2, param3, ibType, param5, param6,
        param7);
    auto ret = FunctionCast(wrapBuildIBCommand, callback->orgBuildIBCommand)(that, rawPkt, param2, param3, ibType,
        param5, param6, param7);
    DBGLOG("X4000", "IB: 0x%x, 0x%x, 0x%x, 0x%x", *rawPkt, rawPkt[1], rawPkt[2], rawPkt[3]);
    return ret;
}

enum AMDMemoryRangeType {
    kAMDMemoryRangeTypeGART = 1,
};

bool X4000::wrapGetRangeInfo(void *that, int memType, void *outData) {
    auto ret = FunctionCast(wrapGetRangeInfo, callback->orgGetRangeInfo)(that, memType, outData);
    DBGLOG("X4000", "getRangeInfo - off 0x0: 0x%llx - off 0x8: 0x%llx - off 0x10: 0x%llx",
        getMember<UInt64>(outData, 0x0), getMember<UInt64>(outData, 0x8), getMember<UInt64>(outData, 0x10));
    if (memType == kAMDMemoryRangeTypeGART) {
        //! So... this stopped the page faulting. But the PM4 remains hung. What am I missing?
        getMember<UInt64>(outData, 0x0) = APU_COMMON_GART_PADDR; //! same as before
        getMember<UInt64>(outData, 0x8) = CIK_DEFAULT_GART_SIZE;
    }
    return ret;
}

void X4000::initializeSystemApertureRegs(void *) {
    //! 0xFF00000000
    //!    0xFEFFFFF

    //! do these in X4K order
    //! KIQ times out now, but that's something.
    LRed::callback->writeReg32(mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR, (LRed::callback->vramEnd >> 12)); //! VRAM START
    LRed::callback->writeReg32(mmMC_VM_SYSTEM_APERTURE_LOW_ADDR, (LRed::callback->vramStart >> 12));
    if (checkKernelArgument("-X4KProgramAperDefault")) { //! tmp 4 if the 0 write has the PM4 still borked
        LRed::callback->writeReg32(mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR, (LRed::callback->vramStart >> 12));
    } else {
        //! `mem_scratch.gpu_addr` tf is that?
        LRed::callback->writeReg32(mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR, 0x0); //! does the PM4 need this to be non-zero here?
    }

    LRed::callback->writeReg32(mmVM_CONTEXT0_PROTECTION_FAULT_DEFAULT_ADDR, 0x0); //! does this ever get reprogrammed?

    LRed::callback->writeReg32(mmMC_VM_AGP_BASE, 0x0);
    LRed::callback->writeReg32(mmMC_VM_AGP_TOP, 0x0);
    LRed::callback->writeReg32(mmMC_VM_AGP_BOT, AGP_DISABLE_ADDR);
}

bool X4000::wrapHWMemoryInit(void * that, void * hwIf) {
    SYSLOG("X4000", "HWMemory::init(%p)", hwIf);
    //! patch sharedaper?
    getMember<UInt64>(that, HWMemoryFields::VRAMMCBaseAddress) = LRed::callback->vramStart;
    getMember<UInt64>(that, HWMemoryFields::VRAMPhysicalOffset) = LRed::callback->fbOffset;
    auto ret = FunctionCast(wrapHWMemoryInit, callback->orgHWMemoryInit)(that, hwIf);
    if (ret) {
        callback->hwMemPtr = that;
    }
    return ret;
}