//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "X4000.hpp"
#include "LRed.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX4000 = "/System/Library/Extensions/AMDRadeonX4000.kext/Contents/MacOS/AMDRadeonX4000";

static const char *pathRadeonX4000HWServices =
    "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/MacOS/AMDRadeonX4000HWServices";

static KernelPatcher::KextInfo kextRadeonX4000 {"com.apple.kext.AMDRadeonX4000", &pathRadeonX4000, 1, {}, {},
    KernelPatcher::KextInfo::Unloaded};

static KernelPatcher::KextInfo kextRadeonX4000HWServices {"com.apple.kext.AMDRadeonX4000HWServices",
    &pathRadeonX4000HWServices, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};

X4000 *X4000::callback = nullptr;

void X4000::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX4000);
    lilu.onKextLoadForce(&kextRadeonX4000HWServices);
}

bool X4000::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX4000HWServices.loadIndex == index) {
        RouteRequestPlus requests[] = {
            {"__ZN36AMDRadeonX4000_AMDRadeonHWServicesCI16getMatchPropertyEv", forceX4000HWLibs},
            {"__ZN41AMDRadeonX4000_AMDRadeonHWServicesPolaris16getMatchPropertyEv", forceX4000HWLibs},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "HWServices",
            "Failed to route symbols");
    } else if (kextRadeonX4000.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();
        /** this is already really complicated, so to keep things breif:
         *  Carizzo uses UVD 6.0 and VCE 3.1, Stoney uses UVD 6.2 and VCE 3.4, both can only encode to H264 for whatever
         * reason, but Stoney can decode HEVC and so can Carrizo
         */
        auto stoney = (LRed::callback->chipType == ChipType::Stoney);
        auto carrizo = (LRed::callback->chipType == ChipType::Carrizo);

        UInt32 *orgChannelTypes = nullptr;
        mach_vm_address_t startHWEngines = 0;

        if (stoney) {
            SolveRequestPlus solveRequests[] = {
                {"__ZN31AMDRadeonX4000_AMDBaffinPM4EngineC1Ev", this->orgBaffinPM4EngineConstructor},
                {"__ZN30AMDRadeonX4000_AMDVIsDMAEngineC1Ev", this->orgGFX8SDMAEngineConstructor},
                {"__ZN32AMDRadeonX4000_AMDUVD6v3HWEngineC1Ev", this->orgPolarisUVDEngineConstructor},
                {"__ZN30AMDRadeonX4000_AMDVISAMUEngineC1Ev", this->orgGFX8SAMUEngineConstructor},
                {"__ZN32AMDRadeonX4000_AMDVCE3v4HWEngineC1Ev", this->orgPolarisVCEEngineConstructor},
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
        } else if (carrizo) {
            RouteRequestPlus requests[] = {
                {"__ZN30AMDRadeonX4000_AMDFijiHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDVIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                "Failed to route symbols");
        } else {
            RouteRequestPlus requests[] = {
                {"__ZN33AMDRadeonX4000_AMDBonaireHardware32setupAndInitializeHWCapabilitiesEv",
                    wrapSetupAndInitializeHWCapabilities},
                {"__ZN28AMDRadeonX4000_AMDCIHardware20initializeFamilyTypeEv", wrapInitializeFamilyType},
                {"__ZN29AMDRadeonX4000_AMDCIPM4Engine21initializeMicroEngineEv", wrapInitializeMicroEngine,
                    this->orgInitializeMicroEngine},
            };
            PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
                "Failed to route symbols");
        }

        RouteRequestPlus requests[] = {
            {"__ZN37AMDRadeonX4000_AMDGraphicsAccelerator5startEP9IOService", wrapAccelStart, orgAccelStart},
            {"__ZN26AMDRadeonX4000_AMDHardware17dumpASICHangStateEb.cold.1", wrapDumpASICHangState,
                this->orgDumpASICHangState},
            {"__ZN26AMDRadeonX4000_AMDHWMemory17adjustVRAMAddressEy", wrapAdjustVRAMAddress,
                this->orgAdjustVRAMAddress},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "X4000",
            "Failed to route symbols");

        if (stoney) {
            PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "X4000",
                "Failed to enable kernel writing");
            /** TODO: Test this */
            orgChannelTypes[5] = 1;     // Fix createAccelChannels so that it only starts SDMA0
            orgChannelTypes[11] = 0;    // Fix getPagingChannel so that it gets SDMA0
            MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);

            LookupPatchPlus const allocHWEnginesPatch {&kextRadeonX4000, kAMDEllesmereHWallocHWEnginesOriginal,
                kAMDEllesmereHWallocHWEnginesPatched, 1};
            PANIC_COND(!allocHWEnginesPatch.apply(patcher, address, size), "X4000",
                "Failed to apply AllocateHWEngines patch: %d", patcher.getError());

            LookupPatchPlus const patch {&kextRadeonX4000, kStartHWEnginesOriginal, kStartHWEnginesMask,
                kStartHWEnginesPatched, kStartHWEnginesMask, 1};
            PANIC_COND(!patch.apply(patcher, startHWEngines, PAGE_SIZE), "X4000", "Failed to patch startHWEngines");
            DBGLOG("X4000", "Applied Singular SDMA lookup patch");
        }
        return true;
    }

    return false;
}

bool X4000::wrapAccelStart(void *that, IOService *provider) {
    DBGLOG("X4000", "accelStart: this = %p provider = %p", that, provider);
    callback->callbackAccelerator = that;
    auto ret = FunctionCast(wrapAccelStart, callback->orgAccelStart)(that, provider);
    DBGLOG("X4000", "accelStart returned %d", ret);
    return ret;
}

enum HWCapability : UInt64 {
    DisplayPipeCount = 0x04,    // UInt32 // unsure
    SECount = 0x34,             // UInt32
    SHPerSE = 0x3C,             // UInt32
    CUPerSH = 0x70,             // UInt32
};

template<typename T>
static inline void setHWCapability(void *that, HWCapability capability, T value) {
    getMember<T>(that, (getKernelVersion() >= KernelVersion::Ventura ? 0x30 : 0x28) + capability) = value;
}

void X4000::wrapSetupAndInitializeHWCapabilities(void *that) {
    DBGLOG("X4000", "setupAndInitializeHWCapabilities: this = %p", that);
    switch (LRed::callback->chipType) {
        case ChipType::Spectre:
            [[fallthrough]];
        case ChipType::Spooky: {
            setHWCapability<UInt32>(that, HWCapability::SECount, 1);
            setHWCapability<UInt32>(that, HWCapability::SHPerSE, 1);
            switch (LRed::callback->deviceId) {
                case 0x1304:
                    [[fallthrough]];
                case 0x1305:
                    [[fallthrough]];
                case 0x130C:
                    [[fallthrough]];
                case 0x130F:
                    [[fallthrough]];
                case 0x1310:
                    [[fallthrough]];
                case 0x1311:
                    [[fallthrough]];
                case 0x131C:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 8);
                    break;
                case 0x1309:
                    [[fallthrough]];
                case 0x130A:
                    [[fallthrough]];
                case 0x130D:
                    [[fallthrough]];
                case 0x1313:
                    [[fallthrough]];
                case 0x131D:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 6);
                    break;
                case 0x1306:
                    [[fallthrough]];
                case 0x1307:
                    [[fallthrough]];
                case 0x130B:
                    [[fallthrough]];
                case 0x130E:
                    [[fallthrough]];
                case 0x1315:
                    [[fallthrough]];
                case 0x131B:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 4);
                    break;
                default:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 3);
                    break;
            }
            break;
        }
        case ChipType::Kalindi:
            [[fallthrough]];
        case ChipType::Godavari: {
            setHWCapability<UInt32>(that, HWCapability::SECount, 1);
            setHWCapability<UInt32>(that, HWCapability::SHPerSE, 1);
            setHWCapability<UInt32>(that, HWCapability::CUPerSH, 2);
            break;
        }
        case ChipType::Carrizo: {
            setHWCapability<UInt32>(that, HWCapability::SECount, 1);
            setHWCapability<UInt32>(that, HWCapability::SHPerSE, 1);
            switch (LRed::callback->revision) {
                case 0xC4:
                    [[fallthrough]];
                case 0x84:
                    [[fallthrough]];
                case 0xC8:
                    [[fallthrough]];
                case 0xCC:
                    [[fallthrough]];
                case 0xE1:
                    [[fallthrough]];
                case 0xE3:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 8);
                    break;
                case 0xC5:
                    [[fallthrough]];
                case 0x81:
                    [[fallthrough]];
                case 0x85:
                    [[fallthrough]];
                case 0xC9:
                    [[fallthrough]];
                case 0xCD:
                    [[fallthrough]];
                case 0xE2:
                    [[fallthrough]];
                case 0xE4:
                    [[fallthrough]];
                case 0xC6:
                    [[fallthrough]];
                case 0xCA:
                    [[fallthrough]];
                case 0xCE:
                    [[fallthrough]];
                case 0x88:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 6);
                    break;
                default:
                    setHWCapability<UInt32>(that, HWCapability::CUPerSH, 4);
                    break;
            }
            break;
        }
        case ChipType::Stoney: {
            setHWCapability<UInt32>(that, HWCapability::SECount, 1);
            setHWCapability<UInt32>(that, HWCapability::SHPerSE, 1);
            if (!LRed::callback->isStoney3CU) {
                setHWCapability<UInt32>(that, HWCapability::CUPerSH, 3);
            } else {
                setHWCapability<UInt32>(that, HWCapability::CUPerSH, 3);
            }
            break;
        }
        default: {
            PANIC("X4000", "Unknown ChipType!");
            break;
        }
    }
    FunctionCast(wrapSetupAndInitializeHWCapabilities, callback->orgSetupAndInitializeHWCapabilities)(that);
}

void X4000::wrapInitializeFamilyType(void *that) {
    DBGLOG("X4000", "initializeFamilyType << %x", LRed::callback->currentFamilyId);
    getMember<UInt32>(that, 0x308) = LRed::callback->currentFamilyId;
}

void *X4000::wrapGetHWChannel(void *that, UInt32 engineType, UInt32 ringId) {
    /** Redirect SDMA1 engine type to SDMA0 */
    return FunctionCast(wrapGetHWChannel, callback->orgGetHWChannel)(that, (engineType == 2) ? 1 : engineType, ringId);
}

char *X4000::forceX4000HWLibs() {
    DBGLOG("HWServices", "Forcing HWServices to load X4000HWLibs");
    // By default, X4000HWServices on CI loads X4050HWLibs, we override this here because X4050 has no KV logic.
    // HWServicesFiji loads X4000HWLibs by default, so we don't need it there.
    // Polaris is an interesting topic because it selects the name by using the Framebuffer name.
    return "Load4000";
}

void X4000::wrapDumpASICHangState(bool param1) {
    DBGLOG("X4000", "dumpASICHangState << (param1: %d)", param1);
    IOSleep(36000000);
}

UInt64 X4000::wrapAdjustVRAMAddress(void *that, UInt64 addr) {
    auto ret = FunctionCast(wrapAdjustVRAMAddress, callback->orgAdjustVRAMAddress)(that, addr);
    SYSLOG("X4000", "AdjustVRAMAddress: returned: 0x%x, our value: 0x%x", ret,
        ret != addr ? (ret + LRed::callback->fbOffset) : ret);
    return ret != addr ? (ret + LRed::callback->fbOffset) : ret;
}

UInt32 X4000::getUcodeAddressOffset(UInt32 fwnum) {
    switch (fwnum) {
        case FwEnum::SDMA0: {
            return 0x3400;
        }
        case FwEnum::SDMA1: {
            return 0x3600;
        }
        case FwEnum::PFP: {
            return (LRed::callback->isGCN3 ? 0xF814 : 0x3054);
        }
        case FwEnum::CE: {
            return (LRed::callback->isGCN3 ? 0xF818 : 0x305A);
        }
        case FwEnum::ME: {
            return (LRed::callback->isGCN3 ? 0xF816 : 0x3057);
        }
        case FwEnum::MEC: {
            return (LRed::callback->isGCN3 ? 0xF81A : 0x305C);
        }
        case FwEnum::MEC2: {
            return (LRed::callback->isGCN3 ? 0xF81C : 0x305E);
        }
        case FwEnum::RLC: {
            return (LRed::callback->isGCN3 ? 0xF83C : 0x30E2);
        }
    }
    return 0x0;
}

UInt32 X4000::getUcodeDataOffset(UInt32 fwnum) {
    switch (fwnum) {
        case FwEnum::SDMA0: {    // SDMA offsets are universal between OSS 2.0.0 and 0SS 3.0.0
            return 0x3401;
        }
        case FwEnum::SDMA1: {
            return 0x3601;
        }
        case FwEnum::PFP: {
            return (LRed::callback->isGCN3 ? 0xF815 : 0x3055);
        }
        case FwEnum::CE: {
            return (LRed::callback->isGCN3 ? 0xF819 : 0x305B);
        }
        case FwEnum::ME: {
            return (LRed::callback->isGCN3 ? 0xF817 : 0x3058);
        }
        case FwEnum::MEC: {
            return (LRed::callback->isGCN3 ? 0xF81B : 0x305D);
        }
        case FwEnum::MEC2: {
            return (LRed::callback->isGCN3 ? 0xF81D : 0x305F);
        }
        case FwEnum::RLC: {
            return (LRed::callback->isGCN3 ? 0xF83D : 0x30E3);
        }
    }
    return 0x0;
}

UInt32 X4000::loadCpFirmware() {
    LRed::callback->writeReg32(0x21B6, ((0x1000000 | 0X4000000) | 0x1000000));
    IODelay(50);
    char filename[64] = {0};
    UInt32 ucodeAddrOff = {0};
    UInt32 ucodeDataOff = {0};
    snprintf(filename, 64, "%s_pfp.bin", LRed::callback->getChipName());
    auto &pfpFwDesc = getFWDescByName(filename);
    auto *pfpFwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(pfpFwDesc.data);
    size_t pfpFwSize = (pfpFwHeader->ucodeSize / 4);
    auto pfpFwData = (pfpFwDesc.data + pfpFwHeader->ucodeOff);
    ucodeAddrOff = callback->getUcodeAddressOffset(FwEnum::PFP);
    ucodeDataOff = callback->getUcodeDataOffset(FwEnum::PFP);
    LRed::callback->writeReg32(ucodeAddrOff, 0);
    for (size_t i = 0; i < pfpFwSize; i++) { LRed::callback->writeReg32(ucodeDataOff, *pfpFwData++); }
    LRed::callback->writeReg32(ucodeAddrOff, pfpFwHeader->ucodeVer);
    DBGLOG("X4000", "PFP FW: version: %x, feature version %x", pfpFwHeader->ucodeVer, pfpFwHeader->ucodeFeatureVer);

    snprintf(filename, 64, "%s_ce.bin", LRed::callback->getChipName());
    auto &ceFwDesc = getFWDescByName(filename);
    auto *ceFwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(ceFwDesc.data);
    size_t ceFwSize = (ceFwHeader->ucodeSize / 4);
    auto ceFwData = (ceFwDesc.data + ceFwHeader->ucodeOff);
    ucodeAddrOff = callback->getUcodeAddressOffset(FwEnum::CE);
    ucodeDataOff = callback->getUcodeDataOffset(FwEnum::CE);
    LRed::callback->writeReg32(ucodeAddrOff, 0);
    for (size_t i = 0; i < ceFwSize; i++) { LRed::callback->writeReg32(ucodeDataOff, *ceFwData++); }
    LRed::callback->writeReg32(ucodeAddrOff, ceFwHeader->ucodeVer);
    DBGLOG("X4000", "CE FW: version: %x, feature version %x", ceFwHeader->ucodeVer, ceFwHeader->ucodeFeatureVer);

    snprintf(filename, 64, "%s_me.bin", LRed::callback->getChipName());
    auto &meFwDesc = getFWDescByName(filename);
    auto *meFwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(meFwDesc.data);
    size_t meFwSize = (meFwHeader->ucodeSize / 4);
    auto meFwData = (meFwDesc.data + meFwHeader->ucodeOff);
    ucodeAddrOff = callback->getUcodeAddressOffset(FwEnum::ME);
    ucodeDataOff = callback->getUcodeDataOffset(FwEnum::ME);
    LRed::callback->writeReg32(ucodeAddrOff, 0);
    for (size_t i = 0; i < meFwSize; i++) { LRed::callback->writeReg32(ucodeDataOff, *meFwData++); }
    LRed::callback->writeReg32(ucodeAddrOff, meFwHeader->ucodeVer);
    DBGLOG("X4000", "ME FW: version: %x, feature version %x", meFwHeader->ucodeVer, meFwHeader->ucodeFeatureVer);

    LRed::callback->writeReg32(0x21B6, 0);
    IODelay(50);
    DBGLOG("X4000", "Loaded PFP, CE and ME firmware.");
    LRed::callback->writeReg32(0x208D, (0X40000000 | 0x10000000));
    IODelay(50);
    snprintf(filename, 64, "%s_mec.bin", LRed::callback->getChipName());
    auto &mecFwDesc = getFWDescByName(filename);
    auto *mecFwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(mecFwDesc.data);
    size_t mecFwSize = (mecFwHeader->ucodeSize / 4);
    auto mecFwData = (mecFwDesc.data + mecFwHeader->ucodeOff);
    ucodeAddrOff = callback->getUcodeAddressOffset(FwEnum::MEC);
    ucodeDataOff = callback->getUcodeDataOffset(FwEnum::MEC);
    LRed::callback->writeReg32(ucodeAddrOff, 0);
    for (size_t i = 0; i < mecFwSize; i++) { LRed::callback->writeReg32(ucodeDataOff, *mecFwData++); }
    LRed::callback->writeReg32(ucodeAddrOff, mecFwHeader->ucodeVer);
    DBGLOG("X4000", "MEC FW: version: %x, feature version %x", mecFwHeader->ucodeVer, mecFwHeader->ucodeFeatureVer);

    if (LRed::callback->chipType <= ChipType::Spooky || LRed::callback->chipType == ChipType::Carrizo) {
        snprintf(filename, 64, "%s_mec2.bin", LRed::callback->getChipName());
        ucodeAddrOff = callback->getUcodeAddressOffset(FwEnum::MEC2);
        ucodeDataOff = callback->getUcodeDataOffset(FwEnum::MEC2);
        auto &mec2FwDesc = getFWDescByName(filename);
        auto *mec2FwHeader = reinterpret_cast<const GfxFwHeaderV1 *>(mec2FwDesc.data);
        size_t mec2FwSize = (mec2FwHeader->ucodeSize / 4);
        auto mec2FwData = (mec2FwDesc.data + mec2FwHeader->ucodeOff);
        LRed::callback->writeReg32(ucodeAddrOff, 0);
        for (size_t i = 0; i < mec2FwSize; i++) { LRed::callback->writeReg32(ucodeDataOff, *mec2FwData++); }
        LRed::callback->writeReg32(ucodeAddrOff, mec2FwHeader->ucodeVer);
        DBGLOG("X4000", "MEC2 FW: version: %x, feature version %x", mec2FwHeader->ucodeVer,
            mec2FwHeader->ucodeFeatureVer);
    }
    LRed::callback->writeReg32(0x208D, 0);
    IODelay(50);
    return 0;
}

UInt32 X4000::loadRlcFirmware() {
    LRed::callback->writeReg32(0x30C0, 0);
    char filename[64] = {0};
    snprintf(filename, 64, "%s_rlc.bin", LRed::callback->getChipName());
    auto &rlcFwDesc = getFWDescByName(filename);
    auto *rlcFwHeader = reinterpret_cast<const RlcFwHeaderV1_0 *>(rlcFwDesc.data);
    LRed::callback->writeReg32(LRed::callback->isGCN3 ? 0xEC03 : 0x30C3, 0);
    LRed::callback->writeReg32(LRed::callback->isGCN3 ? 0xEC27 : 0x30E7, 0);
    size_t rlcFwSize = (rlcFwHeader->ucodeVer);
    auto rlcFwData = (rlcFwDesc.data + rlcFwHeader->ucodeOff);
    LRed::callback->writeReg32(callback->getUcodeAddressOffset(FwEnum::RLC), 0);
    for (size_t i = 0; i < rlcFwSize; i++) {
        LRed::callback->writeReg32(callback->getUcodeDataOffset(FwEnum::RLC), *rlcFwData++);
    }
    LRed::callback->writeReg32(callback->getUcodeAddressOffset(FwEnum::RLC), rlcFwHeader->ucodeVer);
    LRed::callback->writeReg32(0x30C0, 0x1);
    IODelay(50);
    return 0;
}
// CNTL registers are the same accross GFX7 & GFX8
UInt64 X4000::wrapInitializeMicroEngine(void *that) {
    DBGLOG("X4000", "initializeMicroEngine << (that: %p)", that);
    UInt32 cpcntlr0 = LRed::callback->readReg32(0x306A);
    cpcntlr0 &= ~(0x80000 | 0x100000);
    LRed::callback->writeReg32(0x306A, cpcntlr0);
    DBGLOG_COND(!callback->loadCpFirmware(), "X4000", "Loaded CP fw");
    DBGLOG_COND(!callback->loadRlcFirmware(), "X4000", "Loaded RLC fw");
    cpcntlr0 = LRed::callback->readReg32(0x306A);
    cpcntlr0 |= (0x80000 | 0x100000);
    LRed::callback->writeReg32(0x306A, cpcntlr0);
    IODelay(50);
    auto ret = FunctionCast(wrapInitializeMicroEngine, callback->orgInitializeMicroEngine)(that);
    DBGLOG("X4000", "initializeMicroEngine >> 0x%llX", ret);
    return ret;
}
