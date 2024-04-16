//! Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include <Headers/kern_util.hpp>

using t_GenericConstructor = void (*)(void *that);
using t_sendMsgToSmc = UInt32 (*)(void *smum, UInt32 msgId);

constexpr UInt32 AMDGPU_FAMILY_CZ = 0x87;
constexpr UInt32 AMDGPU_FAMILY_KV = 0x7D;

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
constexpr UInt32 mmCONFIG_APER_SIZE = 0x150C;

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
constexpr UInt32 mmMC_VM_FB_OFFSET = 0x81A;

//-------- OSS 3.0.1/IH Registers --------//

constexpr UInt32 mmIH_RB_CNTL = 0xE30;
constexpr UInt32 mmIH_RB_BASE = 0xE31;
constexpr UInt32 mmIH_RB_RPTR = 0xE32;
constexpr UInt32 mmIH_RB_WPTR = 0xE33;
constexpr UInt32 mmIH_RB_WPTR_ADDR_HI = 0xE34;
constexpr UInt32 mmIH_RB_WPTR_ADDR_LO = 0xE35;
constexpr UInt32 mmIH_CNTL = 0xE36;
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
