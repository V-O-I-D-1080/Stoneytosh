//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include "ATOMBIOS.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_util.hpp>
#include <IOKit/IOService.h>

// Taken from WhateverGreen's kern_agdc.h, used for a wrap in kern_support.cpp
enum kAGDCRegisterLinkControlEvent_t {
    kAGDCRegisterLinkInsert = 0,
    kAGDCRegisterLinkRemove = 1,
    kAGDCRegisterLinkChange = 2,
    kAGDCRegisterLinkChangeMST = 3,
    kAGDCRegisterLinkFramebuffer = 4,
    kAGDCValidateDetailedTiming = 10,
    kAGDCRegisterLinkChangeWakeProbe = 0x80,
};

struct AGDCDetailedTimingInformation_t {
    UInt32 horizontalScaledInset;    // pixels
    UInt32 verticalScaledInset;      // lines

    UInt32 scalerFlags;
    UInt32 horizontalScaled;
    UInt32 verticalScaled;

    UInt32 signalConfig;
    UInt32 signalLevels;

    UInt64 pixelClock;    // Hz

    UInt64 minPixelClock;    // Hz - With error what is slowest actual clock
    UInt64 maxPixelClock;    // Hz - With error what is fasted actual clock

    UInt32 horizontalActive;            // pixels
    UInt32 horizontalBlanking;          // pixels
    UInt32 horizontalSyncOffset;        // pixels
    UInt32 horizontalSyncPulseWidth;    // pixels

    UInt32 verticalActive;            // lines
    UInt32 verticalBlanking;          // lines
    UInt32 verticalSyncOffset;        // lines
    UInt32 verticalSyncPulseWidth;    // lines

    UInt32 horizontalBorderLeft;     // pixels
    UInt32 horizontalBorderRight;    // pixels
    UInt32 verticalBorderTop;        // lines
    UInt32 verticalBorderBottom;     // lines

    UInt32 horizontalSyncConfig;
    UInt32 horizontalSyncLevel;    // Future use (init to 0)
    UInt32 verticalSyncConfig;
    UInt32 verticalSyncLevel;    // Future use (init to 0)
    UInt32 numLinks;
    UInt32 verticalBlankingExtension;
    UInt16 pixelEncoding;
    UInt16 bitsPerColorComponent;
    UInt16 colorimetry;
    UInt16 dynamicRange;
    UInt16 dscCompressedBitsPerPixel;
    UInt16 dscSliceHeight;
    UInt16 dscSliceWidth;
};

struct AGDCValidateDetailedTiming_t {
    UInt32 framebufferIndex;    // IOFBDependentIndex
    AGDCDetailedTimingInformation_t timing;
    UInt16 padding1[5];
    void *cfgInfo;    // AppleGraphicsDevicePolicy configuration
    SInt32 frequency;
    UInt16 padding2[6];
    UInt32 modeStatus;    // 1 - invalid, 2 - success, 3 - change timing
    UInt16 padding3[2];
};

using t_AcknowledgeAllOutStandingInterrupts = void (*)(void *that);
using t_InitPulseBasedInterrupts = void (*)(void *that, bool enabled);

class Support {
    friend class GFXCon;

    public:
    static Support *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPopulateDeviceMemory {0};
    mach_vm_address_t orgNotifyLinkChange {0};
    mach_vm_address_t orgCreateAtomBiosParser {0};
    mach_vm_address_t orgObjectInfoTableInit {0};
    mach_vm_address_t orgADCStart {0};
    t_AcknowledgeAllOutStandingInterrupts IHAcknowledgeAllOutStandingInterrupts {nullptr};
    t_InitPulseBasedInterrupts IHInitPulseBasedInterrupts {nullptr};

    static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
        UInt32 eventFlags);
    static bool doNotTestVram(IOService *ctrl, UInt32 reg, bool retryOnFail);
    static IOReturn wrapPopulateDeviceMemory(void *that, UInt32 reg);
    static void *wrapCreateAtomBiosParser(void *that, void *param1, unsigned char *param2, UInt32 dceVersion);
    static void wrapDoGPUPanic();
    static bool wrapObjectInfoTableInit(void *that, void *initdata);
    static void *wrapADCStart(void *that, IOService *provider);
};

/* ---- Patches ---- */

// tells AGDC that we're an iGPU
static const UInt8 kAtiDeviceControlGetVendorInfoOriginal[] = {0xC7, 0x00, 0x24, 0x02, 0x10, 0x00, 0x00, 0x48, 0x00,
    0x4D, 0xE8, 0xC7, 0x00, 0x28, 0x02, 0x00, 0x00, 0x00};
static const UInt8 kAtiDeviceControlGetVendorInfoMask[] = {0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF,
    0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UInt8 kAtiDeviceControlGetVendorInfoPatched[] = {0xC7, 0x00, 0x24, 0x02, 0x10, 0x00, 0x00, 0x48, 0x00,
    0x4D, 0xE8, 0xC7, 0x00, 0x28, 0x01, 0x00, 0x00, 0x00};

// forces AtiDeviceControl to be created
static const UInt8 kATIControllerStartAGDCCheckOriginal[] = {0x41, 0x00, 0x00, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kATIControllerStartAGDCCheckMask[] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};
static const UInt8 kATIControllerStartAGDCCheckPatched[] = {0x41, 0x00, 0x00, 0x0F, 0x84, 0x00, 0x00, 0x00, 0x00};
