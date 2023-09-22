//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

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

struct CommonFirmwareHeader {
    UInt32 size;
    UInt32 headerSize;
    UInt16 headerMajor;
    UInt16 headerMinor;
    UInt16 ipMajor;
    UInt16 ipMinor;
    UInt32 ucodeVer;
    UInt32 ucodeSize;
    UInt32 ucodeOff;
    UInt32 crc32;
} PACKED;

struct GfxFwHeaderV1 : public CommonFirmwareHeader {
    UInt32 ucodeFeatureVer;
    UInt32 jtOff;
    UInt32 jtSize;
} PACKED;

struct SdmaFwHeaderV1 : public CommonFirmwareHeader {
    UInt32 ucodeFeatureVer;
    UInt32 ucodeChangeVer;
    UInt32 jtOff;
    UInt32 jtSize;
} PACKED;

struct RlcFwHeaderV2_1 : public CommonFirmwareHeader {
    UInt32 ucodeFeatureVer;
    UInt32 jtOff;
    UInt32 jtSize;
    UInt32 saveAndRestoreOff;
    UInt32 clearStateDescOff;
    UInt32 availScratchRamLocations;
    UInt32 regRestoreListSize;
    UInt32 regListFmtStart;
    UInt32 regListFmtSeparateStart;
    UInt32 startingOffsetsStart;
    UInt32 regListFmtSize;
    UInt32 regListFmtArrayOff;
    UInt32 regListSize;
    UInt32 regListArrayOff;
    UInt32 regListFmtSeparateSize;
    UInt32 regListFmtSeparateArrayOff;
    UInt32 regListSeparateSize;
    UInt32 regListSeparateArrayOff;
    UInt32 regListFmtDirectRegListLen;
    UInt32 saveRestoreListCntlUcodeVer;
    UInt32 saveRestoreListCntlFeatureVer;
    UInt32 saveRestoreListCntlSize;
    UInt32 saveRestoreListCntlOff;
    UInt32 saveRestoreListGpmUcodeVer;
    UInt32 saveRestoreListGpmFeatureVer;
    UInt32 saveRestoreListGpmSize;
    UInt32 saveRestoreListGpmOff;
    UInt32 saveRestoreListSrmUcodeVer;
    UInt32 saveRestoreListSrmFeatureVer;
    UInt32 saveRestoreListSrmSize;
    UInt32 saveRestoreListSrmOff;
} PACKED;

struct RlcFwHeaderV1_0 : public CommonFirmwareHeader {
    UInt32 ucodeFeatureVer;
    UInt32 saveRestoreOff;
    UInt32 clearStateDescOff;
    UInt32 availScratchRamLocations;
    UInt32 masterPktDescOff;
};

struct GcFwConstant {
    const char *firmwareVer;
    UInt32 featureVer, size;
    UInt32 addr, unknown4;
    UInt32 unknown5, unknown6;
    const UInt8 *data;
} PACKED;

struct SdmaFwConstant {
    const char *unknown1;
    UInt32 size, unknown2;
    const UInt8 *data;
} PACKED;

struct GPUInfoFirmware {
    UInt32 gcNumSe;
    UInt32 gcNumCuPerSh;
    UInt32 gcNumShPerSe;
    UInt32 gcNumRbPerSe;
    UInt32 gcNumTccs;
    UInt32 gcNumGprs;
    UInt32 gcNumMaxGsThds;
    UInt32 gcGsTableDepth;
    UInt32 gcGsPrimBuffDepth;
    UInt32 gcParameterCacheDepth;
    UInt32 gcDoubleOffchipLdsBuffer;
    UInt32 gcWaveSize;
    UInt32 gcMaxWavesPerSimd;
    UInt32 gcMaxScratchSlotsPerCu;
    UInt32 gcLdsSize;
} PACKED;

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
    const void *goldenCaps;
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

constexpr UInt32 ADDR_CHIP_FAMILY_VI = 7;
constexpr UInt32 ADDR_CHIP_FAMILY_CI = 6;

#endif /* AMDCommon.hpp */
