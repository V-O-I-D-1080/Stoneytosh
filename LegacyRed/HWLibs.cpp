//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "HWLibs.hpp"
#include "LRed.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX4000HWLibs = "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX4000HWLibs.kext/Contents/MacOS/AMDRadeonX4000HWLibs";

static KernelPatcher::KextInfo kextRadeonX4000HWLibs {"com.apple.kext.AMDRadeonX4000HWLibs", &pathRadeonX4000HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

HWLibs *HWLibs::callback = nullptr;

void HWLibs::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX4000HWLibs);
}

bool HWLibs::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX4000HWLibs.loadIndex == index) {
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

                if (!needsECaps) {
                    SolveRequestPlus solveRequests[] = {
                        {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                }
                DBGLOG("HWLibs", "Set ASIC caps to Spectre");
                SolveRequestPlus solveRequest {"_Spectre_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to rseolve Ucode info");
                break;
            }
            case ChipType::Spooky: {
                auto needsECaps = (LRed::callback->deviceId >= 0x131B);

                if (!needsECaps) {
                    SolveRequestPlus solveRequests[] = {
                        {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Spectre)]},
                        {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<UInt32>(ChipType::Spectre)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                }
                DBGLOG("HWLibs", "Set ASIC caps to Spectre");
                SolveRequestPlus solveRequest {"_Spectre_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to rseolve Ucode info");
                break;
            }
            case ChipType::Kalindi: {
                // Thanks AMD.
                auto needsECaps = ((LRed::callback->deviceId == 0x9831) || (LRed::callback->deviceId == 0x9833) ||
                                   (LRed::callback->deviceId == 0x9835) || (LRed::callback->deviceId == 0x9837) ||
                                   (LRed::callback->deviceId == 0x9839));

                switch (LRed::callback->enumeratedRevision) {
                    case 0x81: {
                        if (!needsECaps) {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A0", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        } else {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A0_E", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882_E",
                                    goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        }
                        break;
                    }
                    case 0x82: {
                        if (!needsECaps) {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        } else {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1_E", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882_E",
                                    goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        }
                        break;
                    }
                    case 0x85: {
                        if (!needsECaps) {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        } else {
                            SolveRequestPlus solveRequests[] = {
                                {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                                {"_KALINDI_GoldenSettings_A0_4882_E",
                                    goldenCaps[static_cast<UInt32>(ChipType::Kalindi)]},
                            };
                            PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size),
                                "HWLibs", "Failed to resolve symbols");
                        }
                        break;
                    }
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
                DBGLOG("HWLibs", "Set ASIC Caps to Goadavari");
                SolveRequestPlus solveRequest {"_Godavari_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to rseolve Ucode info");
                break;
            }
            case ChipType::Carrizo: {
                // HWLibs uses A0 on Revision 0 hardware.
                auto isRevZero = ((LRed::callback->revision + LRed::callback->enumeratedRevision) == 1);
                if (!isRevZero) {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A1", ddiCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A1", goldenCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                    DBGLOG("HWLibs", "Set ASIC Caps to Carrizo, variant A1");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A0", ddiCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A0", goldenCaps[static_cast<UInt32>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                    DBGLOG("HWLibs", "Set ASIC Caps to Carrizo, variant A0");
                }
                SolveRequestPlus solveRequest {"_Carrizo_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to rseolve Ucode info");
                break;
            }
            case ChipType::Stoney: {
                if (!LRed::callback->isStoney3CU) {
                    SolveRequestPlus solveRequests[] = {
                        {"_STONEY_GoldenSettings_A0_2CU", goldenCaps[static_cast<UInt32>(ChipType::Stoney)]},
                        {"_CAIL_DDI_CAPS_STONEY_A0", ddiCaps[static_cast<UInt32>(ChipType::Stoney)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_STONEY_GoldenSettings_A0_2CU", goldenCaps[static_cast<UInt32>(ChipType::Stoney)]},
                        {"_CAIL_DDI_CAPS_STONEY_A0", ddiCaps[static_cast<UInt32>(ChipType::Stoney)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
                        "Failed to resolve symbols");
                }
                SolveRequestPlus solveRequest {"_Stoney_UcodeInfo", this->orgCailUcodeInfo};
                PANIC_COND(!solveRequest.solve(patcher, index, address, size), "HWLibs",
                    "Failed to rseolve Ucode info");
                DBGLOG("HWLibs", "Set ASIC Caps to Stoney");
                break;
            }
            default: {
                PANIC("HWLibs", "ChipType not set!");
                break;
            }
        }

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable},
            {"_CAILAsicCapsInitTable", orgCapsInitTable},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
            "Failed to resolve symbols");
        if (gcn3) {
            SolveRequestPlus request {"__ZN30AtiAppleTongaPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgPowerTuneConstructor};
            PANIC_COND(!request.solve(patcher, index, address, size), "HWLibs",
                "Failed to resolve PowerTuneServices symbol");
        } else {
            SolveRequestPlus request {"__ZN31AtiAppleHawaiiPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgPowerTuneConstructor};
            PANIC_COND(!request.solve(patcher, index, address, size), "HWLibs",
                "Failed to resolve PowerTuneServices symbol");
        }

        const char *qERS;

        //! Same signature, different enumeration
        if (catalina) {
            qERS = "__ZN15AmdCailServices23queryEngineRunningStateEP20_AMD_HW_ENGINE_QUEUEP22CailEngineRunningState";
        } else {
            qERS = "__ZN15AmdCailServices23queryEngineRunningStateEP17CailHwEngineQueueP22CailEngineRunningState";
        }

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN15AmdCailServicesC2EP11IOPCIDevice", wrapAmdCailServicesConstructor, orgAmdCailServicesConstructor},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {qERS, wrapCAILQueryEngineRunningState, orgCAILQueryEngineRunningState},
            {"_CailMonitorPerformanceCounter", wrapCailMonitorPerformanceCounter, orgCailMonitorPerformanceCounter},
            {"_CailMonitorEngineInternalState", wrapCailMonitorEngineInternalState, orgCailMonitorEngineInternalState},
            {"_MCILDebugPrint", wrapMCILDebugPrint, orgMCILDebugPrint},
            {"_SMUM_Initialize", wrapSMUMInitialize, orgSMUMInitialize},
            {"_Cail_Bonaire_LoadUcode", wrapCailBonaireLoadUcode, this->orgCailBonaireLoadUcode},
            {"_bonaire_load_ucode_via_port_register", wrapBonaireLoadUcodeViaPortRegister,
                this->orgBonaireLoadUcodeViaPortRegister},
            {"_bonaire_program_aspm", wrapBonaireProgramAspm, this->orgBonaireProgramAspm},
            {"_bonaire_perform_srbm_soft_reset", wrapBonairePerformSrbmReset, this->orgBonairePerformSrbmReset},
            {"_CailCapsEnabled", wrapCailCapsEnabled, this->orgCailCapsEnabled},
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
            {&kextRadeonX4000HWLibs, kBonaireInitRlcOriginal, kBonaireInitRlcPatched, 1},
            //{&kextRadeonX4000HWLibs, kCAILBonaireLoadUcode1Original, kCAILBonaireLoadUcode1Patched, 1},
        };

        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, 1, address, size), "HWLibs",
            "Failed to apply patches!");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
            "Failed to enable kernel writing");

        bool found = false;
        auto targetExtRev = ((LRed::callback->chipType == ChipType::Kalindi)) ?
                                static_cast<UInt32>(LRed::callback->enumeratedRevision) :
                                static_cast<UInt32>(LRed::callback->enumeratedRevision) + LRed::callback->revision;
        UInt32 targetDeviceId;
        UInt32 targetFamilyId;
        if (!LRed::callback->isGCN3) {
            targetDeviceId = 0x6649;
            targetFamilyId = 0x78;
        } else {
            targetDeviceId = 0x67DF;    // Ellesmere device ID
            targetFamilyId = 0x82;
        }
        while (orgCapsTable->deviceId != 0xFFFFFFFF) {
            if (orgCapsTable->familyId == targetFamilyId && orgCapsTable->deviceId == targetDeviceId) {
                orgCapsTable->familyId = LRed::callback->currentFamilyId;
                orgCapsTable->deviceId = LRed::callback->deviceId;
                orgCapsTable->revision = LRed::callback->revision;
                orgCapsTable->extRevision = static_cast<UInt32>(targetExtRev);
                orgCapsTable->pciRevision = LRed::callback->pciRevision;
                orgCapsTable->caps = ddiCaps[static_cast<UInt32>(LRed::callback->chipType)];
                if (orgCapsInitTable) {
                    *orgCapsInitTable = {
                        .familyId = LRed::callback->currentFamilyId,
                        .deviceId = LRed::callback->deviceId,
                        .revision = LRed::callback->revision,
                        .extRevision = static_cast<UInt32>(targetExtRev),
                        .pciRevision = LRed::callback->pciRevision,
                        .caps = ddiCaps[static_cast<UInt32>(LRed::callback->chipType)],
                        .goldenCaps = goldenCaps[static_cast<UInt32>(LRed::callback->chipType)],
                    };
                }
                found = true;
                break;
            }
            orgCapsTable++;
        }
        PANIC_COND(!found, "HWLibs", "Failed to find caps table entry for target 0x%x", targetDeviceId);
        // we do not have the Device Capability table on X4000
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        DBGLOG("HWLibs", "Managed to find caps entries for target 0x%x", targetDeviceId);
        DBGLOG("HWLibs", "Applied DDI Caps patches");
        return true;
    }

    return false;
}

void HWLibs::wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider) {
    DBGLOG("HWLibs", "AmdCailServices constructor called!");
    FunctionCast(wrapAmdCailServicesConstructor, callback->orgAmdCailServicesConstructor)(that, provider);
}

void *HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = operator new(0x18);
    callback->orgPowerTuneConstructor(ret, that, param2);
    return ret;
}

UInt64 HWLibs::wrapSMUMInitialize(UInt64 param1, UInt32 *param2, UInt64 param3) {
    DBGLOG("HWLibs", "_SMUM_Initialize: param1 = 0x%llX param2 = %p param3 = 0x%llX", param1, param2, param3);
    auto ret = FunctionCast(wrapSMUMInitialize, callback->orgSMUMInitialize)(param1, param2, param3);
    DBGLOG("HWLibs", "_SMUM_Initialize returned 0x%llX", ret);
    return ret;
}
/** Modifications made, highly unlikely to work, will change later on */
void HWLibs::wrapMCILDebugPrint(UInt32 level_max, char *fmt, UInt64 param3, UInt64 param4, UInt64 param5, uint level) {
    printf("_MCILDebugPrint PARAM1 = 0x%X: ", level_max);
    printf(fmt, param3, param4, param5, level);
    FunctionCast(wrapMCILDebugPrint, callback->orgMCILDebugPrint)(level_max, fmt, param3, param4, param5, level);
}

UInt64 HWLibs::wrapCAILQueryEngineRunningState(void *param1, UInt32 *param2, UInt64 param3) {
    DBGLOG("HWLibs", "_CAILQueryEngineRunningState: param1 = %p param2 = %p param3 = %llX", param1, param2, param3);
    DBGLOG("HWLibs", "_CAILQueryEngineRunningState: *param2 = 0x%X", *param2);
    auto ret =
        FunctionCast(wrapCAILQueryEngineRunningState, callback->orgCAILQueryEngineRunningState)(param1, param2, param3);
    DBGLOG("HWLibs", "_CAILQueryEngineRunningState: after *param2 = 0x%X", *param2);
    DBGLOG("HWLibs", "_CAILQueryEngineRunningState returned 0x%llX", ret);
    return ret;
}

UInt64 HWLibs::wrapCailMonitorEngineInternalState(void *that, UInt32 param1, UInt32 *param2) {
    DBGLOG("HWLibs", "_CailMonitorEngineInternalState: this = %p param1 = 0x%X param2 = %p", that, param1, param2);
    DBGLOG("HWLibs", "_CailMonitorEngineInternalState: *param2 = 0x%X", *param2);
    auto ret = FunctionCast(wrapCailMonitorEngineInternalState, callback->orgCailMonitorEngineInternalState)(that,
        param1, param2);
    DBGLOG("HWLibs", "_CailMonitorEngineInternalState: after *param2 = 0x%X", *param2);
    DBGLOG("HWLibs", "_CailMonitorEngineInternalState returned 0x%llX", ret);
    return ret;
}

UInt64 HWLibs::wrapCailMonitorPerformanceCounter(void *that, UInt32 *param1) {
    DBGLOG("HWLibs", "_CailMonitorPerformanceCounter: this = %p param1 = %p", that, param1);
    DBGLOG("HWLibs", "_CailMonitorPerformanceCounter: *param1 = 0x%X", *param1);
    auto ret =
        FunctionCast(wrapCailMonitorPerformanceCounter, callback->orgCailMonitorPerformanceCounter)(that, param1);
    DBGLOG("HWLibs", "_CailMonitorPerformanceCounter: after *param1 = 0x%X", *param1);
    DBGLOG("HWLibs", "_CailMonitorPerformanceCounter returned 0x%llX", ret);
    return ret;
}

void HWLibs::wrapCailBonaireLoadUcode(void *param1, UInt64 ucodeId, UInt8 *ucodeData, void *param4, UInt64 param5,
    UInt64 param6) {
    DBGLOG("HWLibs",
        "_Cail_Bonaire_LoadUcode << (param1: 0x%llX id: 0x%llX data: 0x%llX param4: 0x%llX param5: 0x%llx, param6: "
        "0x%llx)",
        param1, ucodeId, ucodeData, param4, param5, param6);
    callback->isRunningLoaderTask = true;
    FunctionCast(wrapCailBonaireLoadUcode, callback->orgCailBonaireLoadUcode)(param1, ucodeId, ucodeData, param4,
        param5, param6);
    callback->isRunningLoaderTask = false;
    DBGLOG("HWLibs", "_Cail_Bonaire_LoadUcode >> void");
}

void HWLibs::wrapBonaireLoadUcodeViaPortRegister(UInt64 param1, UInt64 param2, void *param3, UInt32 param4,
    UInt32 param5) {
    DBGLOG("HWLibs",
        "_bonaire_load_ucode_via_port_register << (param1: 0x%llX param2: 0x%llX param3: %p param4: 0x%X param5: 0x%X)",
        param1, param2, param3, param4, param5);
    FunctionCast(wrapBonaireLoadUcodeViaPortRegister, callback->orgBonaireLoadUcodeViaPortRegister)(param1, param2,
        param3, param4, param5);
    DBGLOG("HWLibs", "_bonaire_load_ucode_via_port_register >> void");
}

UInt64 HWLibs::wrapBonaireProgramAspm(UInt64 param1) {
    DBGLOG("HWLibs", "_bonaire_program_aspm << (param1: 0x%llX)", param1);
    return 0;
}

void HWLibs::wrapBonairePerformSrbmReset(void *param1, UInt32 bit) {
    UInt32 tmp = LRed::callback->readReg32(mmSRBM_STATUS);
    DBGLOG("HWLibs", "_bonaire_perform_srbm_reset >>");
    //! According to AMDGPU's source, it shouldnt be reset if it matches this
    if (tmp & (SRBM_STATUS__MCB_BUSY_MASK | SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK | SRBM_STATUS__MCC_BUSY_MASK |
                  SRBM_STATUS__MCD_BUSY_MASK)) {
        DBGLOG("HWLibs", "Skipping reset.");
        return;
    }
    FunctionCast(wrapBonairePerformSrbmReset, callback->orgBonairePerformSrbmReset)(param1, bit);
}

UInt32 HWLibs::wrapCailCapsEnabled(UInt64 caps, UInt32 cap) {
    auto ret = FunctionCast(wrapCailCapsEnabled, callback->orgCailCapsEnabled)(caps, cap);
    if (callback->isRunningLoaderTask == true && cap == 0x53) { return 0; }
    return ret;
}
