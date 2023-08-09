//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.0. See LICENSE for
//  details.

#ifndef kern_vbios_hpp
#define kern_vbios_hpp
#include <Headers/kern_util.hpp>

struct VFCT {
    char signature[4];
    uint32_t length;
    uint8_t revision, checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    char creatorId[4];
    uint32_t creatorRevision;
    char tableUUID[16];
    uint32_t vbiosImageOffset, lib1ImageOffset;
    uint32_t reserved[4];
} PACKED;

struct GOPVideoBIOSHeader {
    uint32_t pciBus, pciDevice, pciFunction;
    uint16_t vendorID, deviceID;
    uint16_t ssvId, ssId;
    uint32_t revision, imageLength;
} PACKED;

struct ATOMCommonTableHeader {
    uint16_t structureSize;
    uint8_t formatRev;
    uint8_t contentRev;
} PACKED;

constexpr uint32_t ATOM_ROM_TABLE_PTR = 0x48;
constexpr uint32_t ATOM_ROM_DATA_PTR = 0x20;

struct IGPSystemInfoV11 : public ATOMCommonTableHeader {
    uint32_t vbiosMisc;
    uint32_t gpuCapInfo;
    uint32_t systemConfig;
    uint32_t cpuCapInfo;
    uint16_t gpuclkSsPercentage;
    uint16_t gpuclkSsType;
    uint16_t lvdsSsPercentage;
    uint16_t lvdsSsRate10hz;
    uint16_t hdmiSsPercentage;
    uint16_t hdmiSsRate10hz;
    uint16_t dviSsPercentage;
    uint16_t dviSsRate10hz;
    uint16_t dpPhyOverride;
    uint16_t lvdsMisc;
    uint16_t backlightPwmHz;
    uint8_t memoryType;
    uint8_t umaChannelCount;
} PACKED;

enum DMIT17MemType : uint8_t {
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
    uint32_t vbiosMisc;
    uint32_t gpuCapInfo;
    uint32_t systemConfig;
    uint32_t cpuCapInfo;
    uint16_t gpuclkSsPercentage;
    uint16_t gpuclkSsType;
    uint16_t dpPhyOverride;
    uint8_t memoryType;
    uint8_t umaChannelCount;
} PACKED;

union IGPSystemInfo {
    ATOMCommonTableHeader header;
    IGPSystemInfoV11 infoV11;
    IGPSystemInfoV2 infoV2;
};

struct ATOMDispObjPathV2 {
    uint16_t dispObjId;
    uint16_t dispRecordOff;
    uint16_t encoderObjId;
    uint16_t extEncoderObjId;
    uint16_t encoderRecordOff;
    uint16_t extEncoderRecordOff;
    uint16_t devTag;
    uint8_t priorityId;
    uint8_t _reserved;
} PACKED;

struct DispObjInfoTableV1_4 : public ATOMCommonTableHeader {
    uint16_t supportedDevices;
    uint8_t pathCount;
    uint8_t _reserved;
    ATOMDispObjPathV2 paths[];
} PACKED;

struct ATOMConnectorDeviceTag {
    uint32_t ulACPIDeviceEnum;    // Reserved for now
    uint16_t usDeviceID;          // This Id is same as "ATOM_DEVICE_XXX_SUPPORT"
    uint16_t usPadding;
} PACKED;

struct ATOMCommonRecordHeader {
    uint8_t recordType;
    uint8_t recordSize;
} PACKED;

struct ATOMConnectorDeviceTagRecord : public ATOMCommonRecordHeader {
    uint8_t numberOfDevice;
    uint8_t reserved;
    ATOMConnectorDeviceTag deviceTag[];
} PACKED;

struct ATOMSrcDstTable {
    uint8_t ucNumberOfSrc;
    uint16_t usSrcObjectID[1];
    uint8_t ucNumberOfDst;
    uint16_t usDstObjectID[1];
} PACKED;

struct ATOMObjHeader : public ATOMCommonTableHeader {
    uint16_t deviceSupport;
    uint16_t connectorObjectTableOffset;
    uint16_t routerObjectTableOffset;
    uint16_t encoderObjectTableOffset;
    uint16_t protectionObjectTableOffset;
    uint16_t displayPathTableOffset;
} PACKED;

struct ATOMDispObjPath {
    uint16_t deviceTag;
    uint16_t size;
    uint16_t connObjectId;
    uint16_t GPUObjectId;
    uint16_t graphicObjIds[];
} PACKED;

struct ATOMObjHeader_V3 : public ATOMObjHeader {
    uint16_t miscObjectTableOffset;
} PACKED;

struct ATOMDispObjPath_V2 {
    uint16_t displayObjId;
    uint16_t dispRecordOffset;
    uint16_t encoderObjId;
    uint16_t extEncoderObjId;
    uint16_t encoderRecordOffset;
    uint16_t extEncoderRecordOffset;
    uint16_t deviceTag;
    uint8_t priorityId;
    uint8_t reserved;
};

struct ATOMDispObjPathTable {
    uint8_t numOfDispPath;
    uint8_t version;
    uint8_t padding[2];
    ATOMDispObjPath dispPath[];
} PACKED;

struct ATOMObj {
    uint16_t objectID;
    uint16_t srcDstTableOffset;
    uint16_t recordOffset;
    uint16_t reserved;
} PACKED;

struct ATOMObjTable {
    uint8_t numberOfObjects;
    uint8_t padding[3];
    ATOMObj objects[];
} PACKED;

// this is NOT the same as AtomConnectorInfo seen in AtiObjectInfoTableInterface_V1
// as to how AtiObjectInfoTableInterface_V2's AtomConnectorInfo differs? I don't know exactly.
struct AtomConnectorInfo_V2 {
    uint16_t *atomObject;
    uint16_t usConnObjectId;
    uint16_t usGraphicObjIds;
    uint8_t *hpdRecord;
    uint8_t *i2cRecord;
};

enum class AtomObjectTableType : uint8_t {
    DisplayPath,
    // This appears to be a mistake left in getAtomObjectTableForType
    // Instead of adding DeviceSupport table offset placed at 0
    // Apple adds just 0, resulting in AtomObjectTable base address + 4 bytes.
    // As a result we could save some time by solving one less pointer.
    Common,
    EncoderObject,
    ConnectorObject,
    RouterObject,
    ProtectionObject
};

struct AtomDisplayObjectPath {
    uint16_t usDeviceTag;     /* supported device  */
    uint16_t usSize;          /* the size of ATOM_DISPLAY_OBJECT_PATH */
    uint16_t usConnObjectId;  /* Connector Object ID  */
    uint16_t usGPUObjectId;   /* GPU ID  */
    uint16_t usGraphicObjIds; /* 1st Encoder Obj source from GPU to last Graphic Obj destinate to connector. */
};

struct AtomConnectorObject {
    uint16_t usObjectID;
    uint16_t usSrcDstTableOffset;
    uint16_t usRecordOffset;
    uint16_t usReserved;
};

enum class AtomRecordType : uint8_t { Unknown = 0, I2C = 1, Max = 0xFF };

struct AtomCommonRecordHeader {
    AtomRecordType ucRecordType;
    uint8_t ucRecordSize;
};

// Definitions taken from asic_reg/ObjectID.h
enum {
    /* External Third Party Encoders */
    ENCODER_OBJECT_ID_SI170B = 0x08,
    ENCODER_OBJECT_ID_CH7303 = 0x09,
    ENCODER_OBJECT_ID_CH7301 = 0x0A,
    ENCODER_OBJECT_ID_INTERNAL_DVO1 = 0x0B, /* This belongs to Radeon Class Display Hardware */
    ENCODER_OBJECT_ID_EXTERNAL_SDVOA = 0x0C,
    ENCODER_OBJECT_ID_EXTERNAL_SDVOB = 0x0D,
    ENCODER_OBJECT_ID_TITFP513 = 0x0E,
    ENCODER_OBJECT_ID_INTERNAL_LVTM1 = 0x0F, /* not used for Radeon */
    ENCODER_OBJECT_ID_VT1623 = 0x10,
    ENCODER_OBJECT_ID_HDMI_SI1930 = 0x11,
    ENCODER_OBJECT_ID_HDMI_INTERNAL = 0x12,
    /* Kaleidoscope (KLDSCP) Class Display Hardware (internal) */
    ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1 = 0x13,
    ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1 = 0x14,
    ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1 = 0x15,
    ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2 = 0x16, /* Shared with CV/TV and CRT */
    ENCODER_OBJECT_ID_SI178 = 0X17,                /* External TMDS (dual link, no HDCP.) */
    ENCODER_OBJECT_ID_MVPU_FPGA = 0x18,            /* MVPU FPGA chip */
    ENCODER_OBJECT_ID_INTERNAL_DDI = 0x19,
    ENCODER_OBJECT_ID_VT1625 = 0x1A,
    ENCODER_OBJECT_ID_HDMI_SI1932 = 0x1B,
    ENCODER_OBJECT_ID_DP_AN9801 = 0x1C,
    ENCODER_OBJECT_ID_DP_DP501 = 0x1D,
    ENCODER_OBJECT_ID_INTERNAL_UNIPHY = 0x1E,
    ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA = 0x1F,
    ENCODER_OBJECT_ID_INTERNAL_UNIPHY1 = 0x20,
    ENCODER_OBJECT_ID_INTERNAL_UNIPHY2 = 0x21,
    ENCODER_OBJECT_ID_ALMOND = 0x22,
    ENCODER_OBJECT_ID_NUTMEG = 0x22,
    ENCODER_OBJECT_ID_TRAVIS = 0x23,
    ENCODER_OBJECT_ID_INTERNAL_VCE = 0x24,
    ENCODER_OBJECT_ID_INTERNAL_UNIPHY3 = 0x25,
    ENCODER_OBJECT_ID_INTERNAL_AMCLK = 0x27
};

enum {
    CONNECTOR_OBJECT_ID_NONE = 0x00,
    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_I = 0x01,
    CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_I = 0x02,
    CONNECTOR_OBJECT_ID_SINGLE_LINK_DVI_D = 0x03,
    CONNECTOR_OBJECT_ID_DUAL_LINK_DVI_D = 0x04,
    CONNECTOR_OBJECT_ID_VGA = 0x05,
    CONNECTOR_OBJECT_ID_COMPOSITE = 0x06,
    CONNECTOR_OBJECT_ID_SVIDEO = 0x07,
    CONNECTOR_OBJECT_ID_YPbPr = 0x08,
    CONNECTOR_OBJECT_ID_D_CONNECTOR = 0x09,
    CONNECTOR_OBJECT_ID_9PIN_DIN = 0x0A, /* Supports both CV & TV */
    CONNECTOR_OBJECT_ID_SCART = 0x0B,
    CONNECTOR_OBJECT_ID_HDMI_TYPE_A = 0x0C,
    CONNECTOR_OBJECT_ID_HDMI_TYPE_B = 0x0D,
    CONNECTOR_OBJECT_ID_LVDS = 0x0E,
    CONNECTOR_OBJECT_ID_7PIN_DIN = 0x0F,
    CONNECTOR_OBJECT_ID_PCIE_CONNECTOR = 0x10,
    CONNECTOR_OBJECT_ID_CROSSFIRE = 0x11,
    CONNECTOR_OBJECT_ID_HARDCODE_DVI = 0x12,
    CONNECTOR_OBJECT_ID_DISPLAYPORT = 0x13,
    CONNECTOR_OBJECT_ID_eDP = 0x14,
    CONNECTOR_OBJECT_ID_MXM = 0x15,
    CONNECTOR_OBJECT_ID_LVDS_eDP = 0x16
};

enum {
    GRAPH_OBJECT_TYPE_NONE = 0x0,
    GRAPH_OBJECT_TYPE_GPU = 0x1,
    GRAPH_OBJECT_TYPE_ENCODER = 0x2,
    GRAPH_OBJECT_TYPE_CONNECTOR = 0x3,
    GRAPH_OBJECT_TYPE_ROUTER = 0x4
};

static constexpr uint16_t OBJECT_ID_MASK = 0x00FF;
static constexpr uint16_t ENUM_ID_MASK = 0x0700;
static constexpr uint16_t RESERVED1_ID_MASK = 0x0800;
static constexpr uint16_t OBJECT_TYPE_MASK = 0x7000;
static constexpr uint16_t RESERVED2_ID_MASK = 0x8000;
static constexpr uint16_t OBJECT_ID_SHIFT = 0x00;
static constexpr uint16_t ENUM_ID_SHIFT = 0x08;
static constexpr uint16_t OBJECT_TYPE_SHIFT = 0x0C;

/**
 *  Checks if object is encoder
 *
 *  @param objid  object id
 *
 *  @return true one success
 */
inline bool isEncoder(uint16_t objid) {
    return ((objid & OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT) == GRAPH_OBJECT_TYPE_ENCODER;
}

/**
 *  Retrieve connector
 *
 *  @param objid  object id
 *
 *  @return connector id
 */
inline uint8_t getConnectorID(uint16_t objid) { return static_cast<uint8_t>(objid); }
/**
 *  Retrieve sense ID
 *
 *  @param record  pointer to atom records
 *
 *  @return sense id or 0
 */
inline uint8_t getSenseID(uint8_t *record) {
    // Partially reversed from AtiAtomBiosDceInterface::parseSenseId
    if (record) {
        while (true) {
            auto h = reinterpret_cast<AtomCommonRecordHeader *>(record);
            if (h->ucRecordType == AtomRecordType::I2C) {
                if (record[2] > 0) return (record[2] & 0xF) + 1;
                return 0;
            } else if (h->ucRecordType == AtomRecordType::Max) {
                return 0;
            }
            record += h->ucRecordSize;
        }
    }

    return 0;
}

/**
 *  Retrieve transmitter and encoder
 *
 *  @param usGraphicObjIds  encoder objcect id
 *  @param txmit            reference to transmitter var
 *  @param enc              reference to encoder var
 *
 *  @return true on success
 */
inline bool getTxEnc(uint16_t usGraphicObjIds, uint8_t &txmit, uint8_t &enc) {
    bool extra = ((usGraphicObjIds & ENUM_ID_MASK) >> ENUM_ID_SHIFT) > 1;
    uint8_t encoder = static_cast<uint8_t>(usGraphicObjIds);
    // This is partially taken from AtiAtomBiosDce60::getPropertiesForConnectorObject and similar functions.
    if (encoder == ENCODER_OBJECT_ID_INTERNAL_UNIPHY) {
        txmit = extra ? 0x20 : 0x10;
        enc = extra;
    } else if (encoder == ENCODER_OBJECT_ID_INTERNAL_UNIPHY1 || encoder == ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1) {
        txmit = extra ? 0x21 : 0x11;
        enc = 2 + extra;
        // Apple also has ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA here
    } else if (encoder == ENCODER_OBJECT_ID_INTERNAL_UNIPHY2) {
        txmit = extra ? 0x22 : 0x12;
        enc = 4 + extra;
    } else {
        DBGLOG("vbios", "getTxEnc found unsupported encoder %02X objid %04X", encoder, usGraphicObjIds);
        return false;
    }

    return true;
}

// Above new ATOM structs are for later.

#endif /* kern_vbios_hpp */
