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
                {"__ZN30AMDRadeonX4000_AMDFijiHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDVIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
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
    if (LRed::callback->stoney3CU) { cuPerSh = 3; }
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
    /** Redirect SDMA1 engine type to SDMA0 */
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

void X4000::wrapAMDHWRegsWrite(void *that, UInt32 addr, UInt32 val) {
    //! DBGLOG("X4000", "write >> addr: 0x%x, val: 0x%x", addr, val);
    if (addr == mmSRBM_SOFT_RESET) {
        val &= ~SRBM_SOFT_RESET__SOFT_RESET_MC_MASK;
        DBGLOG("X4000", "Stripping SRBM_SOFT_RESET__SOFT_RESET_MC_MASK bit");
    }
    FunctionCast(wrapAMDHWRegsWrite, callback->orgAMDHWRegsWrite)(that, addr, val);
}
