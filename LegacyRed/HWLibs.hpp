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

    static void wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider);
    static UInt64 wrapCAILQueryEngineRunningState(void *param1, UInt32 *param2, UInt64 param3);
    static UInt64 wrapCailMonitorEngineInternalState(void *that, UInt32 param1, UInt32 *param2);
    static UInt64 wrapCailMonitorPerformanceCounter(void *that, UInt32 *param1);
    static void *wrapCreatePowerTuneServices(void *that, void *param2);
    static AMDReturn wrapSmuCzInitialize(void *smum, UInt32 param2);
    static UInt64 wrapSMUMInitialize(UInt64 param1, UInt32 *param2, UInt64 param3);
    static void wrapMCILDebugPrint(UInt32 level_max, char *fmt, UInt64 param3, UInt64 param4, UInt64 param5,
        uint level);
    static void wrapCailBonaireLoadUcode(void *param1, UInt64 ucodeId, void *param3, void *param4);
    static void wrapBonaireLoadUcodeViaPortRegister(UInt64 param1, UInt64 param2, void *param3, UInt32 param4,
        UInt32 param5);
    static UInt64 wrapBonaireProgramAspm(UInt64 param1);
    static void wrapVWriteMmRegisterUlong(void *param1, UInt64 addr, UInt64 val);
};

/* ---- Pattern ---- */

static const UInt8 kCailAsicCapsTableHWLibsPattern[] = {0x6E, 0x00, 0x00, 0x00, 0x98, 0x67, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

#endif /* HWLibs_hpp */
