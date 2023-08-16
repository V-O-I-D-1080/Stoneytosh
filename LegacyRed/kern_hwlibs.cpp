//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#include "kern_hwlibs.hpp"
#include "kern_lred.hpp"
#include "kern_patches.hpp"
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
        const void *goldenCaps[static_cast<uint32_t>(ChipType::Unknown)] = {nullptr};
        const uint32_t *ddiCaps[static_cast<uint32_t>(ChipType::Unknown)] = {nullptr};

        // The pains of supporting more than two iGPU generations
        switch (LRed::callback->chipType) {
            case ChipType::Spectre: {
                auto needsECaps = (LRed::callback->deviceId >= 0x131B);

                SolveRequestPlus solveRequests[] = {
                    {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<uint32_t>(ChipType::Spectre)], !needsECaps},
                    {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<uint32_t>(ChipType::Spectre)],
                        !needsECaps},
                    {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<uint32_t>(ChipType::Spectre)], needsECaps},
                    {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<uint32_t>(ChipType::Spectre)],
                        needsECaps},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                    "Failed to resolve symbols");
                DBGLOG("hwlibs", "Set ASIC caps to Spectre");
                break;
            }
            case ChipType::Spooky: {
                auto needsECaps = (LRed::callback->deviceId >= 0x131B);

                SolveRequestPlus solveRequests[] = {
                    {"_CAIL_DDI_CAPS_SPECTRE_A0", ddiCaps[static_cast<uint32_t>(ChipType::Spectre)], !needsECaps},
                    {"_SPECTRE_GoldenSettings_A0_8812", goldenCaps[static_cast<uint32_t>(ChipType::Spectre)],
                        !needsECaps},
                    {"_CAIL_DDI_CAPS_SPECTRE_A0_E", ddiCaps[static_cast<uint32_t>(ChipType::Spectre)], needsECaps},
                    {"_SPECTRE_GoldenSettings_A0_8812_E", goldenCaps[static_cast<uint32_t>(ChipType::Spectre)],
                        needsECaps},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                    "Failed to resolve symbols");
                DBGLOG("hwlibs", "Set ASIC caps to Spectre");
                break;
            }
            case ChipType::Kalindi: {
                // Thanks AMD.
                auto needsECaps = ((LRed::callback->deviceId == 0x9831) || (LRed::callback->deviceId == 0x9833) ||
                                   (LRed::callback->deviceId == 0x9835) || (LRed::callback->deviceId == 0x9837) ||
                                   (LRed::callback->deviceId == 0x9839));

                switch (LRed::callback->enumeratedRevision) {
                    case 0x81: {
                        SolveRequestPlus solveRequests[] = {
                            {"_CAIL_DDI_CAPS_KALINDI_A0", ddiCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                !needsECaps},
                            {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                !needsECaps},
                            {"_CAIL_DDI_CAPS_KALINDI_A0_E", ddiCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                needsECaps},
                            {"_KALINDI_GoldenSettings_A0_4882_E", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                needsECaps},
                        };
                        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                            "Failed to resolve symbols");
                        break;
                    }
                    case 0x82: {
                        SolveRequestPlus solveRequests[] = {
                            {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                !needsECaps},
                            {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                !needsECaps},
                            {"_CAIL_DDI_CAPS_KALINDI_A1_E", ddiCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                needsECaps},
                            {"_KALINDI_GoldenSettings_A0_4882_E", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                needsECaps},
                        };
                        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                            "Failed to resolve symbols");
                        break;
                    }
                    case 0x85: {
                        SolveRequestPlus solveRequests[] = {
                            {"_CAIL_DDI_CAPS_KALINDI_A1", ddiCaps[static_cast<uint32_t>(ChipType::Kalindi)]},
                            {"_KALINDI_GoldenSettings_A0_4882", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                !needsECaps},
                            {"_KALINDI_GoldenSettings_A0_4882_E", goldenCaps[static_cast<uint32_t>(ChipType::Kalindi)],
                                needsECaps},
                        };
                        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                            "Failed to resolve symbols");
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
                    {"_CAIL_DDI_CAPS_GODAVARI_A0", ddiCaps[static_cast<uint32_t>(ChipType::Godavari)]},
                    {"_GODAVARI_GoldenSettings_A0_2411", goldenCaps[static_cast<uint32_t>(ChipType::Godavari)]},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                    "Failed to resolve symbols");
                DBGLOG("hwlibs", "Set ASIC Caps to Goadavari");
                break;
            }
            case ChipType::Carrizo: {
                // HWLibs uses A0 on Revision 0 hardware.
                auto isRevZero = ((LRed::callback->revision + LRed::callback->enumeratedRevision) == 1);
                if (!isRevZero) {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A1", ddiCaps[static_cast<uint32_t>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A1", goldenCaps[static_cast<uint32_t>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                        "Failed to resolve symbols");
                    DBGLOG("hwlibs", "Set ASIC Caps to Carrizo, variant A1");
                    break;
                } else {
                    SolveRequestPlus solveRequests[] = {
                        {"_CAIL_DDI_CAPS_CARRIZO_A0", ddiCaps[static_cast<uint32_t>(ChipType::Carrizo)]},
                        {"_CARRIZO_GoldenSettings_A0", goldenCaps[static_cast<uint32_t>(ChipType::Carrizo)]},
                    };
                    PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                        "Failed to resolve symbols");
                    DBGLOG("hwlibs", "Set ASIC Caps to Carrizo, variant A0");
                    break;
                }
            }
            case ChipType::Stoney: {
                auto needs2cuSettings = (LRed::callback->chipVariant == ChipVariant::s2CU);

                SolveRequestPlus solveRequests[] = {
                    {"_STONEY_GoldenSettings_A0_2CU", goldenCaps[static_cast<uint32_t>(ChipType::Stoney)],
                        needs2cuSettings},
                    {"_STONEY_GoldenSettings_A0_3CU", goldenCaps[static_cast<uint32_t>(ChipType::Stoney)],
                        !needs2cuSettings},
                    {"_CAIL_DDI_CAPS_STONEY_A0", ddiCaps[static_cast<uint32_t>(ChipType::Stoney)]},
                };
                PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
                    "Failed to resolve symbols");
                DBGLOG("hwlibs", "Set ASIC Caps to Stoney, needs2cuSettings: %x", needs2cuSettings);
                break;
            }
            default: {
                PANIC("hwlibs", "ChipType not set!");
                break;
            }
        }

        SolveRequestPlus solveRequests[] = {
            {"__ZN31AtiAppleHawaiiPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgHawaiiPowerTuneConstructor, !LRed::callback->isGCN3},
            {"__ZN30AtiAppleTongaPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgTongaPowerTuneConstructor, LRed::callback->isGCN3},
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable},
            {"_CAILAsicCapsInitTable", orgCapsInitTable},
            {"_CIslands_SendMsgToSmc", this->orgCISendMsgToSmc, LRed::callback->chipType < ChipType::Carrizo},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "hwlibs",
            "Failed to resolve symbols");

        KernelPatcher::RouteRequest requests[] = {
            {"__ZN15AmdCailServicesC2EP11IOPCIDevice", wrapAmdCailServicesConstructor, orgAmdCailServicesConstructor},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"__ZN15AmdCailServices23queryEngineRunningStateEP17CailHwEngineQueueP22CailEngineRunningState",
                wrapCAILQueryEngineRunningState, orgCAILQueryEngineRunningState},
            {"_CailMonitorPerformanceCounter", wrapCailMonitorPerformanceCounter, orgCailMonitorPerformanceCounter},
            {"_CailMonitorEngineInternalState", wrapCailMonitorEngineInternalState, orgCailMonitorEngineInternalState},
            {"_MCILDebugPrint", wrapMCILDebugPrint, orgMCILDebugPrint},
            {"_SMUM_Initialize", wrapSMUMInitialize, orgSMUMInitialize},
            //{"_SmuCz_Initialize", wrapSmuCzInitialize, this->orgSmuCzInitialize},
            {"_Cail_Bonaire_LoadUcode", wrapCailBonaireLoadUcode, this->orgCailBonaireLoadUcode},
            {"_bonaire_load_ucode_via_port_register", wrapBonaireLoadUcodeViaPortRegister,
                this->orgBonaireLoadUcodeViaPortRegister},
            {"_bonaire_program_aspm", wrapBonaireProgramAspm, this->orgBonaireProgramAspm},
        };
        PANIC_COND(!patcher.routeMultiple(index, requests, address, size), "hwlibs", "Failed to route symbols");

        PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "hwlibs",
            "Failed to enable kernel writing");

        bool found = false;
        auto targetExtRev = ((LRed::callback->chipType == ChipType::Kalindi)) ?
                                static_cast<uint32_t>(LRed::callback->enumeratedRevision) :
                                static_cast<uint32_t>(LRed::callback->enumeratedRevision) + LRed::callback->revision;
        uint32_t targetDeviceId;
        uint32_t targetFamilyId;
        switch (LRed::callback->chipType) {
            case ChipType::Spectre:
                [[fallthrough]];
            case ChipType::Spooky: {
                targetDeviceId = 0x130A;
                targetFamilyId = AMDGPU_FAMILY_KV;
                SYSLOG("hwlibs", "targetDeviceId == 0x%x, targetFamilyId == %x", targetDeviceId, targetFamilyId);
            }
            case ChipType::Kalindi: {
                targetDeviceId = 0x9830;
                targetFamilyId = AMDGPU_FAMILY_KV;
                SYSLOG("hwlibs", "targetDeviceId == 0x%x, targetFamilyId == %x", targetDeviceId, targetFamilyId);
            }
            case ChipType::Godavari: {
                targetDeviceId = 0x9850;
                targetFamilyId = AMDGPU_FAMILY_KV;
                SYSLOG("hwlibs", "targetDeviceId == 0x%x, targetFamilyId == %x", targetDeviceId, targetFamilyId);
            }
            case ChipType::Carrizo: {
                targetDeviceId = 0x9874;
                targetFamilyId = AMDGPU_FAMILY_CZ;
                SYSLOG("hwlibs", "targetDeviceId == 0x%x, targetFamilyId == %x", targetDeviceId, targetFamilyId);
            }
            case ChipType::Stoney: {
                targetDeviceId = 0x98E4;
                targetFamilyId = AMDGPU_FAMILY_CZ;
                SYSLOG("hwlibs", "targetDeviceId == 0x%x, targetFamilyId == %x", targetDeviceId, targetFamilyId);
            }
            default: {
                if (!LRed::callback->isGCN3) {
                    targetDeviceId = 0x6649;
                    targetFamilyId = 0x78;
                } else {
                    targetDeviceId = 0x67DF;    // Ellesmere device ID
                    targetFamilyId = 0x82;
                }
                SYSLOG("hwlibs", "No chipType found, defaulting to 0x%x and 0x%x", targetDeviceId, targetFamilyId);
            }
        }
        while (orgCapsTable->deviceId != 0xFFFFFFFF) {
            if (orgCapsTable->familyId == targetFamilyId && orgCapsTable->deviceId == targetDeviceId) {
                orgCapsTable->familyId = LRed::callback->currentFamilyId;
                orgCapsTable->deviceId = LRed::callback->deviceId;
                orgCapsTable->revision = LRed::callback->revision;
                orgCapsTable->extRevision = static_cast<uint32_t>(targetExtRev);
                orgCapsTable->pciRevision = LRed::callback->pciRevision;
                orgCapsTable->caps = ddiCaps[static_cast<uint32_t>(LRed::callback->chipType)];
                if (orgCapsInitTable) {
                    *orgCapsInitTable = {
                        .familyId = LRed::callback->currentFamilyId,
                        .deviceId = LRed::callback->deviceId,
                        .revision = LRed::callback->revision,
                        .extRevision = static_cast<uint32_t>(targetExtRev),
                        .pciRevision = LRed::callback->pciRevision,
                        .caps = ddiCaps[static_cast<uint32_t>(LRed::callback->chipType)],
                        .goldenCaps = goldenCaps[static_cast<uint32_t>(LRed::callback->chipType)],
                    };
                }
                found = true;
                break;
            }
            orgCapsTable++;
        }
        PANIC_COND(!found, "hwlibs", "Failed to find caps table entry for target 0x%x", targetDeviceId);
        // we do not have the Device Capability table on X4000
        MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
        SYSLOG("hwlibs", "Managed to find caps entries for target 0x%x", targetDeviceId);
        DBGLOG("hwlibs", "Applied DDI Caps patches");

        return true;
    }

    return false;
}

void HWLibs::wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider) {
    DBGLOG("hwlibs", "AmdCailServices constructor called!");
    FunctionCast(wrapAmdCailServicesConstructor, callback->orgAmdCailServicesConstructor)(that, provider);
}

void *HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = operator new(0x18);
    (LRed::callback->isGCN3 ? callback->orgTongaPowerTuneConstructor : callback->orgHawaiiPowerTuneConstructor)(ret,
        that, param2);
    return ret;
}

uint64_t HWLibs::wrapSMUMInitialize(uint64_t param1, uint32_t *param2, uint64_t param3) {
    DBGLOG("hwlibs", "_SMUM_Initialize: param1 = 0x%llX param2 = %p param3 = 0x%llX", param1, param2, param3);
    auto ret = FunctionCast(wrapSMUMInitialize, callback->orgSMUMInitialize)(param1, param2, param3);
    DBGLOG("hwlibs", "_SMUM_Initialize returned 0x%llX", ret);
    return ret;
}
/** Modifications made, highly unlikely to work, will change later on */
void HWLibs::wrapMCILDebugPrint(uint32_t level_max, char *fmt, uint64_t param3, uint64_t param4, uint64_t param5,
    uint level) {
    printf("_MCILDebugPrint PARAM1 = 0x%X: ", level_max);
    printf(fmt, param3, param4, param5, level);
    FunctionCast(wrapMCILDebugPrint, callback->orgMCILDebugPrint)(level_max, fmt, param3, param4, param5, level);
}

uint64_t HWLibs::wrapCAILQueryEngineRunningState(void *param1, uint32_t *param2, uint64_t param3) {
    DBGLOG("hwlibs", "_CAILQueryEngineRunningState: param1 = %p param2 = %p param3 = %llX", param1, param2, param3);
    DBGLOG("hwlibs", "_CAILQueryEngineRunningState: *param2 = 0x%X", *param2);
    auto ret =
        FunctionCast(wrapCAILQueryEngineRunningState, callback->orgCAILQueryEngineRunningState)(param1, param2, param3);
    DBGLOG("hwlibs", "_CAILQueryEngineRunningState: after *param2 = 0x%X", *param2);
    DBGLOG("hwlibs", "_CAILQueryEngineRunningState returned 0x%llX", ret);
    return ret;
}

uint64_t HWLibs::wrapCailMonitorEngineInternalState(void *that, uint32_t param1, uint32_t *param2) {
    DBGLOG("hwlibs", "_CailMonitorEngineInternalState: this = %p param1 = 0x%X param2 = %p", that, param1, param2);
    DBGLOG("hwlibs", "_CailMonitorEngineInternalState: *param2 = 0x%X", *param2);
    auto ret = FunctionCast(wrapCailMonitorEngineInternalState, callback->orgCailMonitorEngineInternalState)(that,
        param1, param2);
    DBGLOG("hwlibs", "_CailMonitorEngineInternalState: after *param2 = 0x%X", *param2);
    DBGLOG("hwlibs", "_CailMonitorEngineInternalState returned 0x%llX", ret);
    return ret;
}

uint64_t HWLibs::wrapCailMonitorPerformanceCounter(void *that, uint32_t *param1) {
    DBGLOG("hwlibs", "_CailMonitorPerformanceCounter: this = %p param1 = %p", that, param1);
    DBGLOG("hwlibs", "_CailMonitorPerformanceCounter: *param1 = 0x%X", *param1);
    auto ret =
        FunctionCast(wrapCailMonitorPerformanceCounter, callback->orgCailMonitorPerformanceCounter)(that, param1);
    DBGLOG("hwlibs", "_CailMonitorPerformanceCounter: after *param1 = 0x%X", *param1);
    DBGLOG("hwlibs", "_CailMonitorPerformanceCounter returned 0x%llX", ret);
    return ret;
}

/** For future reference */
/*
AMDReturn X5000HWLibs::wrapSmuRavenInitialize(void *smum, uint32_t param2) {
    auto ret = FunctionCast(wrapSmuRavenInitialize, callback->orgSmuRavenInitialize)(smum, param2);
    callback->orgRavenSendMsgToSmc(smum, PPSMC_MSG_PowerUpSdma);
    return ret;
}
*/

void HWLibs::wrapCailBonaireLoadUcode(uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4) {
    DBGLOG("hwlibs", "_Cail_Bonaire_LoadUcode << (param1: 0x%llX param2: 0x%llX param3: 0x%llX param4: 0x%llX)", param1,
        param2, param3, param4);
    FunctionCast(wrapCailBonaireLoadUcode, callback->orgCailBonaireLoadUcode)(param1, param2, param3, param4);
    DBGLOG("hwlibs", "_Cail_Bonaire_LoadUcode >> void");
}

void HWLibs::wrapBonaireLoadUcodeViaPortRegister(uint64_t param1, uint64_t param2, void *param3, uint32_t param4,
    uint32_t param5) {
    DBGLOG("hwlibs",
        "_bonaire_load_ucode_via_port_register << (param1: 0x%llX param2: 0x%llX param3: %p param4: 0x%X param5: 0x%X)",
        param1, param2, param3, param4, param5);
    FunctionCast(wrapBonaireLoadUcodeViaPortRegister, callback->orgBonaireLoadUcodeViaPortRegister)(param1, param2,
        param3, param4, param5);
    DBGLOG("hwlibs", "_bonaire_load_ucode_via_port_register >> void");
}

uint64_t HWLibs::wrapBonaireProgramAspm(uint64_t param1) {
    DBGLOG("hwlibs", "_bonaire_program_aspm << (param1: 0x%llX)", param1);
    return 0;
}