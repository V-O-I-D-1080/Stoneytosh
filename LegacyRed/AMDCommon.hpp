//!  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//!  details.

#ifndef AMDCommon_hpp
#define AMDCommon_hpp
#include <Headers/kern_util.hpp>

using t_GenericConstructor = void (*)(void *that);
using t_sendMsgToSmc = UInt32 (*)(void *smum, UInt32 msgId);
using t_pspLoadExtended = UInt32 (*)(void *, UInt64, UInt64, const void *, size_t);

constexpr UInt32 AMDGPU_FAMILY_CZ = 0x87;
constexpr UInt32 AMDGPU_FAMILY_KV = 0x7D;

constexpr UInt32 PPSMC_MSG_GetSmuVersion = 0x2;
constexpr UInt32 PPSMC_MSG_PowerUpSdma = 0xE;
constexpr UInt32 PPSMC_MSG_PowerGateMmHub = 0x35;
// applies to Carrizo
constexpr UInt16 PPSMC_MSG_UVDPowerOff = 0x7;
constexpr UInt16 PPSMC_MSG_UVDPowerOn = 0x8;
constexpr UInt16 PPSMC_MSG_VCEPowerOff = 0x9;
constexpr UInt16 PPSMC_MSG_VCEPowerOn = 0xA;
constexpr UInt16 PPSMC_MSG_ACPPowerOff = 0xB;
constexpr UInt16 PPSMC_MSG_ACPPowerOn = 0xC;
constexpr UInt16 PPSMC_MSG_SDMAPowerOff = 0xD;
constexpr UInt16 PPSMC_MSG_SDMAPowerOn = 0xE;
constexpr UInt16 PPSMC_MSG_XDMAPowerOff = 0xF;
constexpr UInt16 PPSMC_MSG_XDMAPowerOn = 0x10;

constexpr UInt32 GFX_FW_TYPE_CP_MEC = 5;
constexpr UInt32 GFX_FW_TYPE_CP_MEC_ME2 = 6;

constexpr UInt32 MP_BASE = 0x16000;

constexpr UInt32 AMDGPU_MAX_USEC_TIMEOUT = 100000;

constexpr UInt32 mmMP1_SMN_C2PMSG_90 = 0x29A;
constexpr UInt32 mmMP1_SMN_C2PMSG_82 = 0x292;
constexpr UInt32 mmMP1_SMN_C2PMSG_66 = 0x282;

constexpr UInt32 mmMP0PUB_IND_INDEX = 0x180;
constexpr UInt32 mmMP0PUB_IND_DATA = 0x181;

constexpr UInt32 mmPCIE_INDEX2 = 0xC;
constexpr UInt32 mmPCIE_DATA2 = 0xD;

constexpr UInt32 mmCHUB_CONTROL = 0x619;
constexpr UInt32 bypassVM = (1 << 0);

constexpr UInt32 mmSRBM_STATUS = 0x394;

constexpr UInt32 SRBM_STATUS__MCB_BUSY_MASK = 0x200;
constexpr UInt32 SRBM_STATUS__MCB_NON_DISPLAY_BUSY_MASK = 0x400;
constexpr UInt32 SRBM_STATUS__MCC_BUSY_MASK = 0x800;
constexpr UInt32 SRBM_STATUS__MCD_BUSY_MASK = 0x1000;

struct CAILUcodeInfo {
    UInt32 *rlcUcode;
    UInt32 *sdma0Ucode;
    UInt32 *sdma1Ucode;
    UInt32 *ceUcode;
    UInt32 *pfpUcode;
    UInt32 *meUcode;
    UInt32 *mec1Ucode;
    UInt32 *mec2Ucode;
    UInt32 *rlcVUcode;
    UInt32 *microEngineRegisters;
    UInt32 *rlcRegister;
    UInt32 *sdma0Register;
    UInt32 *sdma1Register;
    UInt32 *ceRegister;
    UInt32 *pfpRegister;
    UInt32 *meRegister;
    UInt32 *mec1Register;
    UInt32 *mec2Register;
    UInt32 *rlcVRegister;
    UInt32 *tmzUcode;
};

struct CAILASICGoldenRegisterSettings {
    UInt32 offset;
    UInt32 mask;
    UInt32 value;
};

struct CAILASICGoldenSettings {
    void *hwConstants;
    CAILASICGoldenRegisterSettings *goldenRegisterSettings;
    CAILUcodeInfo *ucodeInfo;    //! UcodeInfo can be contained in both HWConstants and GoldenSettings.
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

enum AMDReturn : UInt32 {
    kAMDReturnSuccess = 0,
    kAMDReturnInvalidArgument,
    kAMDReturnGeneralFailure,
    kAMDReturnResourcesExhausted,
    kAMDReturnUnsupported,
};

struct CailDeviceTypeEntry {
    UInt32 deviceId;
    UInt32 deviceType;
} PACKED;

constexpr UInt32 ADDR_CHIP_FAMILY_VI = 7;
constexpr UInt32 ADDR_CHIP_FAMILY_CI = 6;

#endif /* AMDCommon.hpp */
