//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#ifndef kern_x4000_hpp
#define kern_x4000_hpp
#include "kern_amd.hpp"
#include "kern_lred.hpp"
#include "kern_patcherplus.hpp"
#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>

enum FwEnum : uint32_t {
    SDMA0 = 1,
    SDMA1,
    PFP,
    CE,
    ME,
    MEC,
    MEC2,
};

class X4000 {
    public:
    static X4000 *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    t_GenericConstructor orgBaffinPM4EngineConstructor {nullptr};
    t_GenericConstructor orgGFX8SDMAEngineConstructor {nullptr};
    t_GenericConstructor orgPolarisVCEEngineConstructor {nullptr};
    t_GenericConstructor orgPolarisUVDEngineConstructor {nullptr};
    t_GenericConstructor orgGFX8SAMUEngineConstructor {nullptr};
    mach_vm_address_t orgAccelStart {0};
    mach_vm_address_t orgGetHWChannel {0};
    mach_vm_address_t orgSetupAndInitializeHWCapabilities {0};
    mach_vm_address_t orgDumpASICHangState {0};
    mach_vm_address_t orgAdjustVRAMAddress {0};
    mach_vm_address_t orgInitializeMicroEngine {0};

    void *callbackAccelerator = nullptr;

    static bool wrapAccelStart(void *that, IOService *provider);
    static void *wrapGetHWChannel(void *that, uint32_t engineType, uint32_t ringId);
    static void wrapInitializeFamilyType(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void wrapDumpASICHangState(bool param1);
    static char *forceX4000HWLibs(void);
    static uint64_t wrapAdjustVRAMAddress(void *that, uint64_t addr);
    static uint64_t wrapInitializeMicroEngine(void *that);
    uint32_t getUcodeAddressOffset(uint32_t fwnum);
    uint32_t getUcodeDataOffset(uint32_t fwnum);
};

#endif /* kern_x4000_hpp */
