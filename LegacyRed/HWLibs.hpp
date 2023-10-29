//!  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//!  details.

#ifndef HWLibs_hpp
#define HWLibs_hpp
#include "AMDCommon.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_util.hpp>

using t_XPowerTuneConstructor = void (*)(void *that, void *ppInstance, void *ppCallbacks);
using t_sendMsgToSmc = UInt32 (*)(void *smum, UInt32 msgId);

class HWLibs {
    public:
    static HWLibs *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_XPowerTuneConstructor orgPowerTuneConstructor {nullptr};
    t_sendMsgToSmc orgCISendMsgToSmc {nullptr};
    t_sendMsgToSmc orgCzSendMsgToSmc {nullptr};
    mach_vm_address_t orgAmdCailServicesConstructor {0};
    mach_vm_address_t orgCAILQueryEngineRunningState {0};
    mach_vm_address_t orgCailMonitorEngineInternalState {0};
    mach_vm_address_t orgCailMonitorPerformanceCounter {0};
    mach_vm_address_t orgSMUMInitialize {0};
    mach_vm_address_t orgSmuCzInitialize {0};
    mach_vm_address_t orgMCILDebugPrint {0};
    mach_vm_address_t orgCailBonaireLoadUcode {0};
    mach_vm_address_t orgBonaireLoadUcodeViaPortRegister {0};
    mach_vm_address_t orgBonaireProgramAspm {0};
    mach_vm_address_t orgVWriteMmRegisterUlong {0};
    mach_vm_address_t orgGetGpuHwConstants {0};
    mach_vm_address_t orgBonairePerformSrbmReset {0};
    mach_vm_address_t orgBonaireInitRlc {0};
    CAILUcodeInfo *orgCailUcodeInfo {0};

    static void wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider);
    static UInt64 wrapCAILQueryEngineRunningState(void *param1, UInt32 *param2, UInt64 param3);
    static UInt64 wrapCailMonitorEngineInternalState(void *that, UInt32 param1, UInt32 *param2);
    static UInt64 wrapCailMonitorPerformanceCounter(void *that, UInt32 *param1);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static CAILResult wrapSmuCzInitialize(void *smum, UInt32 param2);
    static UInt64 wrapSMUMInitialize(UInt64 param1, UInt32 *param2, UInt64 param3);
    static void wrapMCILDebugPrint(UInt32 level_max, char *fmt, UInt64 param3, UInt64 param4, UInt64 param5,
        uint level);
    static void wrapCailBonaireLoadUcode(void *param1, UInt64 ucodeId, UInt8 *ucodeData, void *param4, UInt64 param5,
        UInt64 param6);
    static void wrapBonaireLoadUcodeViaPortRegister(UInt64 param1, UInt64 param2, void *param3, UInt32 param4,
        UInt32 param5);
    static UInt64 wrapBonaireProgramAspm(UInt64 param1);
    static void wrapVWriteMmRegisterUlong(void *param1, UInt64 addr, UInt64 val);
    static void *wrapGetGpuHwConstants(void *param1);
    static void wrapBonairePerformSrbmReset(void *param1, UInt32 bit);
    static UInt64 wrapBonaireInitRlc(void *data);
};

/* ---- Patterns ---- */

static const UInt8 kCailAsicCapsTableHWLibsPattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

static const UInt8 kCailBonaireLoadUcodeMontereyPattern[] = {0x55, 0x48, 0x89, 0xE5, 0x41, 0x57, 0x41, 0x56, 0x41, 0x55,
    0x41, 0x54, 0x53, 0x48, 0x83, 0xEC, 0x28, 0x31, 0xC0};

// ----- Patch ----- //

//! Force allow all log levels.
static const UInt8 AtiPowerPlayServicesCOriginal[] = {0x8B, 0x40, 0x60, 0x48, 0x8D};
static const UInt8 AtiPowerPlayServicesCPatched[] = {0x6A, 0xFF, 0x58, 0x48, 0x8D};

static const UInt8 kCAILBonaireLoadUcode1Original[] = {0x85, 0xC0, 0x74, 0x00};
static const UInt8 kCAILBonaireLoadUcode1Mask[] = {0xFF, 0xFF, 0xFF, 0x00};
static const UInt8 kCAILBonaireLoadUcode1Patched[] = {0x85, 0xC0, 0x75, 0x00};

//! It gets _bonaire_init_rlc writing the RLC version, HWLibs thinks that the old ASICs load the firmware by itself?
static const UInt8 kBonaireInitRlcOriginal[] = {0xBE, 0x53, 0x00, 0x00, 0x00, 0xE8, 0x91, 0x62, 0xFD, 0xFF, 0x4C, 0x89,
    0xE7, 0x85, 0xC0, 0x74, 0x09};
static const UInt8 kBonaireInitRlcPatched[] = {0xBE, 0x53, 0x00, 0x00, 0x00, 0xE8, 0x91, 0x62, 0xFD, 0xFF, 0x4C, 0x89,
    0xE7, 0x85, 0xC0, 0xEB, 0x09};

#endif /* HWLibs_hpp */
