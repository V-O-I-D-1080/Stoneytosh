//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "HWLibs.hpp"
#include "LRed.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX4000HWServices =
    "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/MacOS/AMDRadeonX4000HWServices";
static const char *pathRadeonX4000HWLibs = "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX4000HWLibs.kext/Contents/MacOS/AMDRadeonX4000HWLibs";

static KernelPatcher::KextInfo kextRadeonX4000HWServices {"com.apple.kext.AMDRadeonX4000HWServices",
    &pathRadeonX4000HWServices, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextRadeonX4000HWLibs {"com.apple.kext.AMDRadeonX4000HWLibs", &pathRadeonX4000HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

HWLibs *HWLibs::callback = nullptr;

void HWLibs::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX4000HWServices);
    lilu.onKextLoadForce(&kextRadeonX4000HWLibs);
}

bool HWLibs::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX4000HWServices.loadIndex == index) {
        RouteRequestPlus requests[] = {
            {"__ZN36AMDRadeonX4000_AMDRadeonHWServicesCI16getMatchPropertyEv", forceX4000HWLibs},
            {"__ZN41AMDRadeonX4000_AMDRadeonHWServicesPolaris16getMatchPropertyEv", forceX4000HWLibs},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "HWServices",
            "Failed to route symbols");
        return true;
    } else if (kextRadeonX4000HWLibs.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;
        CAILASICGoldenSettings *goldenCaps[static_cast<UInt32>(ChipType::Unknown)] = {nullptr};
        const UInt32 *ddiCaps[static_cast<UInt32>(ChipType::Unknown)] = {nullptr};

        bool gcn3 = (LRed::callback->chipType >= ChipType::Carrizo);
        bool catalina = (getKernelVersion() == KernelVersion::Catalina);

        // The pains of supporting more than two iGPU generations
        switch (LRed::callback->chipType) {
            case ChipType::Spectre: {
                auto needsECaps = (LRed::callback->deviceId >= 0x131B);

                if (needsECaps) {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                }
                DBGLOG("HWLibs", "Set ASIC caps to Spectre");
                SolveRequestPlus solveRequest {"_Spectre_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to resolve Ucode info");
                break;
            }
            case ChipType::Spooky: {
                bool needsECaps = (LRed::callback->deviceId >= 0x131B);

                if (needsECaps) {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                }
                DBGLOG("HWLibs", "Set ASIC caps to Spectre");
                SolveRequestPlus solveRequest {"_Spectre_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to resolve Ucode info");
                break;
            }
            case ChipType::Kalindi: {
                //! Thanks AMD.
                bool needsECaps = (LRed::callback->deviceId == 0x9831) || (LRed::callback->deviceId == 0x9833) ||
                                  (LRed::callback->deviceId == 0x9835) || (LRed::callback->deviceId == 0x9837) ||
                                  (LRed::callback->deviceId == 0x9839);

                switch (LRed::callback->enumeratedRevision) {
                    case 0x81: {
                        if (needsECaps) {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882_E",
                                    goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        } else {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A0", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        }
                        break;
                    }
                    case 0x82:
                        [[fallthrough]];
                    case 0x85: {
                        if (needsECaps) {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1_E", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A1_4882_E",
                                    goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        } else {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A1_4882", goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        }
                        break;
                    }
                    default:
                        PANIC("LRed", "Unknown Kalindi revision");
                }
                break;
            }
            case ChipType::Godavari: {
                // auto needsECaps = (LRed::callback->deviceId == 0x9851 );
                // auto needsNoEyefinity = {LRed::callback->deviceId == 0x9855 && LRed::callback->pciRevision != 1}
                // some of these need extra work, like grabbing pciRevision.

                SolveRequestPlus solveRequests[] = {
                    {"_CAIL_DDI_CAPS_GODAVARI_A0", ddiCaps[static_cast<UInt32>(ChipType::Godavari)]},
                    {"_GODAVARI_GoldenSettings_A0_2411", goldenCaps[static_cast<UInt32>(ChipType::Godavari)]},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                    "Failed to resolve symbols");
                DBGLOG("HWLibs", "Set ASIC Caps to Godavari");
                SolveRequestPlus solveRequest {"_Godavari_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to resolve Ucode info");
                break;
            }
            case ChipType::Carrizo: {
                // HWLibs uses A0 on Revision 0 hardware.
                if (LRed::callback->revision == 0) {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A0", ddiCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A0", goldenCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                    DBGLOG("HWLibs", "Set ASIC Caps to Carrizo, variant A0");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A1", ddiCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A1", goldenCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                    DBGLOG("HWLibs", "Set ASIC Caps to Carrizo, variant A1");
                }
                SolveRequestPlus solveRequest {"_Carrizo_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to resolve Ucode info");
                break;
            }
            case ChipType::Stoney: {
                SolveRequestPlus solveRequests[] = {
                    {LRed::callback->isStoney3CU ? "_STONEY_GoldenSettings_A0_3CU" : "_STONEY_GoldenSettings_A0_2CU",
                        goldenCaps[static_cast<UInt32>(ChipType::Stoney)]},
                    {"_CAIL_DDI_CAPS_STONEY_A0", ddiCaps[static_cast<UInt32>(ChipType::Stoney)]},
                    {"_Stoney_UcodeInfo", this->orgCailUcodeInfo},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                    "Failed to resolve symbols");
                DBGLOG("HWLibs", "Set ASIC Caps to Stoney");
                break;
            }
            default:
                PANIC("HWLibs", "ChipType not set!");
        }

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable},
            {"_CAILAsicCapsInitTable", orgCapsInitTable},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
            "Failed to resolve symbols");

        SolveRequestPlus request {gcn3 ? "__ZN30AtiAppleTongaPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks" :
                                         "__ZN31AtiAppleHawaiiPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
            this->orgPowerTuneConstructor};
        PANIC_COND(!request.solve(patcher, index, address, size), "HWLibs",
            "Failed to resolve PowerTuneServices symbol");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN15AmdCailServicesC2EP11IOPCIDevice", wrapAmdCailServicesConstructor,
                this->orgAmdCailServicesConstructor},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_bonaire_perform_srbm_soft_reset", wrapBonairePerformSrbmReset, this->orgBonairePerformSrbmReset},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "HWLibs", "Failed to route symbols");

        if (checkKernelArgument("-CKDumpGoldenRegisters")) {
            CAILASICGoldenRegisterSettings *settings =
                goldenCaps[static_cast<UInt32>(LRed::callback->chipType)]->goldenRegisterSettings;
            size_t num = 0;
            while (settings->offset != 0xFFFFFFFF) {
                DBGLOG("HWLibs", "Reg No. %zu, OFF: 0x%x, MASK: 0x%x VAL: 0x%x", num, settings->offset, settings->mask,
                    settings->value);
                num++;
                settings++;
            };
        }

        LookupPatchPlus const patches[] = {
            {&kextRadeonX4000HWLibs, AtiPowerPlayServicesCOriginal, AtiPowerPlayServicesCPatched, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, address, size), "HWLibs", "Failed to apply patches!");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
            "Failed to enable kernel writing");

        auto targetExtRev = (LRed::callback->chipType == ChipType::Kalindi) ?
                                static_cast<UInt32>(LRed::callback->enumeratedRevision) :
                                static_cast<UInt32>(LRed::callback->enumeratedRevision) + LRed::callback->revision;
        UInt32 targetDeviceId;
        UInt32 targetFamilyId;
        if (LRed::callback->isGCN3) {
            targetDeviceId = 0x67DF;    // Ellesmere device ID
            targetFamilyId = 0x82;
        } else {
            targetDeviceId = 0x6649;
            targetFamilyId = 0x78;
        }
        for (; orgCapsTable->deviceId != 0xFFFFFFFF; orgCapsTable++) {
            if (orgCapsTable->familyId == targetFamilyId && orgCapsTable->deviceId == targetDeviceId) {
                orgCapsTable->familyId = LRed::callback->chipFamilyId;
                orgCapsTable->deviceId = LRed::callback->deviceId;
                orgCapsTable->revision = LRed::callback->revision;
                orgCapsTable->extRevision = static_cast<UInt32>(targetExtRev);
                orgCapsTable->pciRevision = LRed::callback->pciRevision;
                orgCapsTable->caps = ddiCaps[static_cast<UInt32>(LRed::callback->chipType)];
                if (orgCapsInitTable) {
                    *orgCapsInitTable = {
                        .familyId = LRed::callback->chipFamilyId,
                        .deviceId = LRed::callback->deviceId,
                        .revision = LRed::callback->revision,
                        .extRevision = static_cast<UInt32>(targetExtRev),
                        .pciRevision = LRed::callback->pciRevision,
                        .caps = ddiCaps[static_cast<UInt32>(LRed::callback->chipType)],
                        .goldenCaps = goldenCaps[static_cast<UInt32>(LRed::callback->chipType)],
                    };
                }
                break;
            }
        }
        PANIC_COND(orgCapsTable->deviceId == 0xFFFFFFFF, "HWLibs", "Failed to find caps table entry for target 0x%x",
            targetDeviceId);
        // we do not have the Device Capability table on X4000
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Managed to find caps entries for target 0x%x", targetDeviceId);
        DBGLOG("HWLibs", "Applied DDI Caps patches");
        return true;
    }

    return false;
}

const char *HWLibs::forceX4000HWLibs() {
    DBGLOG("HWServices", "Forcing HWServices to load X4000HWLibs");
    //! By default, X4000HWServices on CI loads X4050HWLibs, we override this here because X4050 has no KV logic.
    //! HWServicesFiji loads X4000HWLibs by default, so we don't need it there.
    //! Polaris is an interesting topic because it selects the name by using the Framebuffer name.
    return "Load4000";
}

void HWLibs::wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider) {
    DBGLOG("HWLibs", "AmdCailServices constructor called!");
    FunctionCast(wrapAmdCailServicesConstructor, callback->orgAmdCailServicesConstructor)(that, provider);
}

void *HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = OSObject::operator new(0x18);
    callback->orgPowerTuneConstructor(ret, that, param2);
    return ret;
}

void HWLibs::wrapBonairePerformSrbmReset(void *param1, UInt32 bit) {
    UInt32 tmp = LRed::callback->readReg32(mmSRBM_STATUS);
    DBGLOG("HWLibs", "_bonaire_perform_srbm_reset >>");
    //! According to AMDGPU's source, it shouldnt be reset if it matches this
    if (tmp & (SRBM_STATUS__MCB_BUSY_MASK | SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK | SRBM_STATUS__MCC_BUSY_MASK |
                  SRBM_STATUS__MCD_BUSY_MASK)) {
        DBGLOG("HWLibs", "Skipping reset");
        return;
    }
    FunctionCast(wrapBonairePerformSrbmReset, callback->orgBonairePerformSrbmReset)(param1, bit);
}
