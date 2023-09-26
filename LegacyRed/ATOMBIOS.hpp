//!  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//!  details.

#ifndef ATOMBIOS_hpp
#define ATOMBIOS_hpp
#include <Headers/kern_util.hpp>

struct VFCT {
    char signature[4];
    UInt32 length;
    UInt8 revision, checksum;
    char oemId[6];
    char oemTableId[8];
    UInt32 oemRevision;
    char creatorId[4];
    UInt32 creatorRevision;
    char tableUUID[16];
    UInt32 vbiosImageOffset, lib1ImageOffset;
    UInt32 reserved[4];
} PACKED;

struct GOPVideoBIOSHeader {
    UInt32 pciBus, pciDevice, pciFunction;
    UInt16 vendorID, deviceID;
    UInt16 ssvId, ssId;
    UInt32 revision, imageLength;
} PACKED;

struct ATOMCommonTableHeader {
    UInt16 structureSize;
    UInt8 formatRev;
    UInt8 contentRev;
} PACKED;

constexpr UInt32 ATOM_ROM_TABLE_PTR = 0x48;
constexpr UInt32 ATOM_ROM_DATA_PTR = 0x20;

struct IGPSystemInfoV11 : public ATOMCommonTableHeader {
    UInt32 vbiosMisc;
    UInt32 gpuCapInfo;
    UInt32 systemConfig;
    UInt32 cpuCapInfo;
    UInt16 gpuclkSsPercentage;
    UInt16 gpuclkSsType;
    UInt16 lvdsSsPercentage;
    UInt16 lvdsSsRate10hz;
    UInt16 hdmiSsPercentage;
    UInt16 hdmiSsRate10hz;
    UInt16 dviSsPercentage;
    UInt16 dviSsRate10hz;
    UInt16 dpPhyOverride;
    UInt16 lvdsMisc;
    UInt16 backlightPwmHz;
    UInt8 memoryType;
    UInt8 umaChannelCount;
} PACKED;

enum DMIT17MemType : UInt8 {
    kDDR2MemType = 0x13,
    kDDR2FBDIMMMemType,
    kDDR3MemType = 0x18,
    kDDR4MemType = 0x1A,
    kLPDDR2MemType = 0x1C,
    kLPDDR3MemType,
    kLPDDR4MemType,
    kDDR5MemType = 0x22,
    kLPDDR5MemType,
};

struct IGPSystemInfoV2 : public ATOMCommonTableHeader {
    UInt32 vbiosMisc;
    UInt32 gpuCapInfo;
    UInt32 systemConfig;
    UInt32 cpuCapInfo;
    UInt16 gpuclkSsPercentage;
    UInt16 gpuclkSsType;
    UInt16 dpPhyOverride;
    UInt8 memoryType;
    UInt8 umaChannelCount;
} PACKED;

union IGPSystemInfo {
    ATOMCommonTableHeader header;
    IGPSystemInfoV11 infoV11;
    IGPSystemInfoV2 infoV2;
};

struct ATOMDispObjPathV2 {
    UInt16 dispObjId;
    UInt16 dispRecordOff;
    UInt16 encoderObjId;
    UInt16 extEncoderObjId;
    UInt16 encoderRecordOff;
    UInt16 extEncoderRecordOff;
    UInt16 devTag;
    UInt8 priorityId;
    UInt8 _reserved;
} PACKED;

struct DispObjInfoTableV1_4 : public ATOMCommonTableHeader {
    UInt16 supportedDevices;
    UInt8 pathCount;
    UInt8 _reserved;
    ATOMDispObjPathV2 paths[];
} PACKED;

struct ATOMConnectorDeviceTag {
    UInt32 ulACPIDeviceEnum;    //! Reserved for now
    UInt16 usDeviceID;          //! This Id is same as "ATOM_DEVICE_XXX_SUPPORT"
    UInt16 usPadding;
} PACKED;

struct ATOMCommonRecordHeader {
    UInt8 recordType;
    UInt8 recordSize;
} PACKED;

struct ATOMConnectorDeviceTagRecord : public ATOMCommonRecordHeader {
    UInt8 numberOfDevice;
    UInt8 reserved;
    ATOMConnectorDeviceTag deviceTag[];
} PACKED;

struct ATOMSrcDstTable {
    UInt8 ucNumberOfSrc;
    UInt16 usSrcObjectID[1];
    UInt8 ucNumberOfDst;
    UInt16 usDstObjectID[1];
} PACKED;

struct ATOMObjHeader : public ATOMCommonTableHeader {
    UInt16 deviceSupport;
    UInt16 connectorObjectTableOffset;
    UInt16 routerObjectTableOffset;
    UInt16 encoderObjectTableOffset;
    UInt16 protectionObjectTableOffset;
    UInt16 displayPathTableOffset;
} PACKED;

struct ATOMDispObjPath {
    UInt16 deviceTag;
    UInt16 size;
    UInt16 connObjectId;
    UInt16 GPUObjectId;
    UInt16 graphicObjIds[];
} PACKED;

struct ATOMObjHeader_V3 : public ATOMObjHeader {
    UInt16 miscObjectTableOffset;
} PACKED;

struct ATOMDispObjPath_V2 {
    UInt16 displayObjId;
    UInt16 dispRecordOffset;
    UInt16 encoderObjId;
    UInt16 extEncoderObjId;
    UInt16 encoderRecordOffset;
    UInt16 extEncoderRecordOffset;
    UInt16 deviceTag;
    UInt8 priorityId;
    UInt8 reserved;
};

struct ATOMDispObjPathTable {
    UInt8 numOfDispPath;
    UInt8 version;
    UInt8 padding[2];
    ATOMDispObjPath dispPath[];
} PACKED;

struct ATOMObj {
    UInt16 objectID;
    UInt16 srcDstTableOffset;
    UInt16 recordOffset;
    UInt16 reserved;
} PACKED;

struct ATOMObjTable {
    UInt8 numberOfObjects;
    UInt8 padding[3];
    ATOMObj objects[];
} PACKED;

//! Definitions taken from asic_reg/ObjectID.h

enum {
    GRAPH_OBJECT_TYPE_NONE = 0x0,
    GRAPH_OBJECT_TYPE_GPU = 0x1,
    GRAPH_OBJECT_TYPE_ENCODER = 0x2,
    GRAPH_OBJECT_TYPE_CONNECTOR = 0x3,
    GRAPH_OBJECT_TYPE_ROUTER = 0x4
};

static constexpr UInt16 OBJECT_TYPE_MASK = 0x7000;
static constexpr UInt16 OBJECT_TYPE_SHIFT = 0x0C;

#endif /* ATOMBIOS_hpp */
