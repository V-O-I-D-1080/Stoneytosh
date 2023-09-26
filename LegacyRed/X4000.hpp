//!  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//!  details.

#ifndef X4000_hpp
#define X4000_hpp
#include "AMDCommon.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>

enum FwEnum : UInt32 {
    SDMA0 = 1,
    SDMA1,
    PFP,
    CE,
    ME,
    MEC,
    MEC2,
    RLC,
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
    static void *wrapGetHWChannel(void *that, UInt32 engineType, UInt32 ringId);
    static void wrapInitializeFamilyType(void *that);
    static void wrapSetupAndInitializeHWCapabilities(void *that);
    static void wrapDumpASICHangState(bool param1);
    static const char *forceX4000HWLibs(void);
    static UInt64 wrapAdjustVRAMAddress(void *that, UInt64 addr);
    static UInt64 wrapInitializeMicroEngine(void *that);
};

// ---- Patches ---- //

//! makes a loop run once in startHWEngines
static const UInt8 kStartHWEnginesOriginal[] = {0x40, 0x83, 0xF0, 0x02};
static const UInt8 kStartHWEnginesMask[] = {0xF0, 0xFF, 0xF0, 0xFF};
static const UInt8 kStartHWEnginesPatched[] = {0x40, 0x83, 0xF0, 0x01};

//! erase the 2nd SDMA engine. this is a viable method for us since we don't have VCN or any other component found in
//! the other kexts
static const UInt8 kAMDEllesmereHWallocHWEnginesOriginal[] = {0xE8, 0xAA, 0xF3, 0xFE, 0xFF, 0x48, 0x89, 0xC3, 0x48,
    0x89, 0xC7, 0xE8, 0xA9, 0xF3, 0xFE, 0xFF, 0x49, 0x89, 0x9E, 0xC0, 0x03, 0x00, 0x00};
static const UInt8 kAMDEllesmereHWallocHWEnginesPatched[] = {0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90,
    0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x66, 0x90, 0x90};

#endif /* X4000_hpp */
