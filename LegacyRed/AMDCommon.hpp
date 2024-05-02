//! Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

using t_GenericConstructor = void (*)(void *that);
using t_sendMsgToSmc = UInt32 (*)(void *smum, UInt32 msgId);

constexpr UInt32 AMDGPU_FAMILY_CZ = 0x87;
constexpr UInt32 AMDGPU_FAMILY_KV = 0x7D;

constexpr UInt64 CIK_DEFAULT_GART_SIZE =
    1024 << 20;    //! Used on all APU ASICs supported here, look in gfx_v7_0.c and gfx_v8_0.c for details

//! seems to be a universal constant for iGPU ASICs
//! don't know the origin, however, this is observed on even Vega iGPUs
//! also found on our supported ASICs via research through AMDGPU
//! bottom paddr: 0xF400000000 (512M)
//! top paddr:    0xF41FFFFFFF (512M)
constexpr UInt64 USUAL_VRAM_PADDR = 0xF4 << 24;

//! seems to be another universal constant for iGPU ASICs
//! don't know the origin, however, this is observed on even Vega iGPUs
//! also found on our supported ASICs via research through AMDGPU
//! bottom paddr: 0xFF00000000 (1024M)
//! top paddr:    0xFF3FFFFFFF (1024M)
constexpr UInt64 USUAL_GART_PADDR = 0xFF << 24;

constexpr UInt32 AMDGPU_MAX_USEC_TIMEOUT = 100000;

//-------- SMU 8 Registers --------//

constexpr UInt32 mmMP1_SMN_C2PMSG_90 = 0x29A;
constexpr UInt32 mmMP1_SMN_C2PMSG_82 = 0x292;
constexpr UInt32 mmMP1_SMN_C2PMSG_66 = 0x282;

constexpr UInt32 mmMP0PUB_IND_INDEX = 0x180;
constexpr UInt32 mmMP0PUB_IND_DATA = 0x181;

//-------- BIF 4.1/BIF 5 Registers --------//

constexpr UInt32 mmPCIE_INDEX_2 = 0xC;
constexpr UInt32 mmPCIE_DATA_2 = 0xD;

constexpr UInt32 mmCONFIG_MEMSIZE = 0x150A;
constexpr UInt32 mmCONFIG_APER_SIZE = 0x150C;    //! Why does AMDGPU not use this?

constexpr UInt32 mmINTERRUPT_CNTL = 0x151A;
constexpr UInt32 mmINTERRUPT_CNTL2 = 0x151B;

//-------- GFX 7/GFX 8 Registers --------//

constexpr UInt32 mmCHUB_CONTROL = 0x619;
constexpr UInt32 bypassVM = (1 << 0);

constexpr UInt32 mmSRBM_STATUS = 0x394;

constexpr UInt32 SRBM_STATUS__MCB_BUSY_MASK = 0x200;
constexpr UInt32 SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK = 0x400;
constexpr UInt32 SRBM_STATUS__MCC_BUSY_MASK = 0x800;
constexpr UInt32 SRBM_STATUS__MCD_BUSY_MASK = 0x1000;

constexpr UInt32 mmSRBM_SOFT_RESET = 0x398;

constexpr UInt32 SRBM_SOFT_RESET__SOFT_RESET_MC_MASK = 0x800;

//-------- GMC Registers --------//

constexpr UInt32 mmMC_VM_FB_LOCATION = 0x809;
constexpr UInt32 mmMC_VM_AGP_TOP = 0x80A;
constexpr UInt32 mmMC_VM_AGP_BOT = 0x80B;
constexpr UInt32 mmMC_VM_AGP_BASE = 0x80C;
constexpr UInt32 mmMC_VM_SYSTEM_APERTURE_LOW_ADDR = 0x80D;
constexpr UInt32 mmMC_VM_SYSTEM_APERTURE_HIGH_ADDR = 0x80E;
constexpr UInt32 mmMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR = 0x80F;
constexpr UInt32 mmMC_VM_FB_OFFSET = 0x81A;

constexpr UInt32 mmVM_CONTEXT0_PAGE_TABLE_START_ADDR = 0x557;
constexpr UInt32 mmVM_CONTEXT1_PAGE_TABLE_START_ADDR = 0x558;
constexpr UInt32 mmVM_CONTEXT0_PAGE_TABLE_END_ADDR = 0x55F;
constexpr UInt32 mmVM_CONTEXT1_PAGE_TABLE_END_ADDR = 0x560;

//-------- OSS 3.0.1/IH Registers --------//

constexpr UInt32 mmIH_RB_CNTL = 0xE30;
constexpr UInt32 IH_RB_CNTL__RB_ENABLE = 0x00000001;
constexpr UInt32 mmIH_RB_BASE = 0xE31;
constexpr UInt32 mmIH_RB_RPTR = 0xE32;
constexpr UInt32 mmIH_RB_WPTR = 0xE33;
constexpr UInt32 mmIH_RB_WPTR_ADDR_HI = 0xE34;
constexpr UInt32 mmIH_RB_WPTR_ADDR_LO = 0xE35;
constexpr UInt32 mmIH_CNTL = 0xE36;
constexpr UInt32 IH_CNTL__ENABLE_INTR = 0x00000001;
constexpr UInt32 mmIH_LEVEL_STATUS = 0xE37;
constexpr UInt32 mmIH_STATUS = 0xE38;

//-------- AMD Catalyst Data Types --------//

struct CAILASICGoldenRegisterSettings {
    UInt32 offset;
    UInt32 mask;
    UInt32 value;
};

struct CAILASICGoldenSettings {
    void *hwConstants;
    CAILASICGoldenRegisterSettings *goldenRegisterSettings;
    void *ucodeInfo;    //! UcodeInfo can be contained in both HWConstants and GoldenSettings.
};

struct CAILAsicCapsEntry {
    UInt32 familyId, deviceId;
    UInt32 revision, extRevision;
    UInt32 pciRevision;
    UInt32 _reserved;
    const UInt32 *caps;
    const UInt32 *skeleton;
} PACKED;

struct CAILAsicCapsInitEntry {
    UInt64 familyId, deviceId;
    UInt64 revision, extRevision;
    UInt64 pciRevision;
    const UInt32 *caps;
    const CAILASICGoldenSettings *goldenCaps;
} PACKED;

enum CAILResult : UInt32 {
    kCAILResultSuccess = 0,
    kCAILResultInvalidArgument,
    kCAILResultGeneralFailure,
    kCAILResultResourcesExhausted,
    kCAILResultUnsupported,
};

struct CailDeviceTypeEntry {
    UInt32 deviceId;
    UInt32 deviceType;
} PACKED;

constexpr UInt32 ADDR_CHIP_FAMILY_VI = 7;
constexpr UInt32 ADDR_CHIP_FAMILY_CI = 6;
