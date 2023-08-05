//  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//  details.

#ifndef kern_support_hpp
#define kern_support_hpp
#include "kern_amd.hpp"
#include "kern_patcherplus.hpp"
#include "kern_vbios.hpp"
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
    uint32_t horizontalScaledInset;    // pixels
    uint32_t verticalScaledInset;      // lines

    uint32_t scalerFlags;
    uint32_t horizontalScaled;
    uint32_t verticalScaled;

    uint32_t signalConfig;
    uint32_t signalLevels;

    uint64_t pixelClock;    // Hz

    uint64_t minPixelClock;    // Hz - With error what is slowest actual clock
    uint64_t maxPixelClock;    // Hz - With error what is fasted actual clock

    uint32_t horizontalActive;            // pixels
    uint32_t horizontalBlanking;          // pixels
    uint32_t horizontalSyncOffset;        // pixels
    uint32_t horizontalSyncPulseWidth;    // pixels

    uint32_t verticalActive;            // lines
    uint32_t verticalBlanking;          // lines
    uint32_t verticalSyncOffset;        // lines
    uint32_t verticalSyncPulseWidth;    // lines

    uint32_t horizontalBorderLeft;     // pixels
    uint32_t horizontalBorderRight;    // pixels
    uint32_t verticalBorderTop;        // lines
    uint32_t verticalBorderBottom;     // lines

    uint32_t horizontalSyncConfig;
    uint32_t horizontalSyncLevel;    // Future use (init to 0)
    uint32_t verticalSyncConfig;
    uint32_t verticalSyncLevel;    // Future use (init to 0)
    uint32_t numLinks;
    uint32_t verticalBlankingExtension;
    uint16_t pixelEncoding;
    uint16_t bitsPerColorComponent;
    uint16_t colorimetry;
    uint16_t dynamicRange;
    uint16_t dscCompressedBitsPerPixel;
    uint16_t dscSliceHeight;
    uint16_t dscSliceWidth;
};

struct AGDCValidateDetailedTiming_t {
    uint32_t framebufferIndex;    // IOFBDependentIndex
    AGDCDetailedTimingInformation_t timing;
    uint16_t padding1[5];
    void *cfgInfo;    // AppleGraphicsDevicePolicy configuration
    int32_t frequency;
    uint16_t padding2[6];
    uint32_t modeStatus;    // 1 - invalid, 2 - success, 3 - change timing
    uint16_t padding3[2];
};

struct AtomConnectorInfo {
    uint16_t *atomObject;
    uint16_t usConnObjectId;
    uint16_t usGraphicObjIds;
    uint8_t *hpdRecord;
    uint8_t *i2cRecord;
};

using t_getAtomObjectTableForType = void *(*)(void *that, AtomObjectTableType type, uint8_t *sz);
constexpr t_getAtomObjectTableForType orgGetAtomObjectTableForType {nullptr};

enum ConnectorType {
    ConnectorLVDS = 0x2,
    ConnectorDigitalDVI = 0x4,
    ConnectorSVID = 0x8,
    ConnectorVGA = 0x10,
    ConnectorDP = 0x400,
    ConnectorHDMI = 0x800,
    ConnectorAnalogDVI = 0x2000
};

struct LegacyConnector {
    uint32_t type;
    uint32_t flags;
    uint16_t features;
    uint16_t priority;
    uint8_t transmitter;
    uint8_t encoder;
    uint8_t hotplug;
    uint8_t sense;
};

struct ModernConnector {
    uint32_t type;
    uint32_t flags;
    uint16_t features;
    uint16_t priority;
    uint32_t reserved1;
    uint8_t transmitter;
    uint8_t encoder;
    uint8_t hotplug;
    uint8_t sense;
    uint32_t reserved2;
};

union Connector {
    LegacyConnector legacy;
    ModernConnector modern;

    static_assert(sizeof(LegacyConnector) == 16, "LegacyConnector has wrong size");
    static_assert(sizeof(ModernConnector) == 24, "ModernConnector has wrong size");

    template<typename T, typename Y>
    static inline void assign(T &out, const Y &in) {
        memset(&out, 0, sizeof(T));
        out.type = in.type;
        out.flags = in.flags;
        out.features = in.features;
        out.priority = in.priority;
        out.transmitter = in.transmitter;
        out.encoder = in.encoder;
        out.hotplug = in.hotplug;
        out.sense = in.sense;
    }
};

inline const char *printType(uint32_t type) {
    switch (type) {
        case ConnectorLVDS:
            return "LVDS";
        case ConnectorDigitalDVI:
            return "DVI ";
        case ConnectorSVID:
            return "SVID";
        case ConnectorVGA:
            return "VGA ";
        case ConnectorDP:
            return "DP  ";
        case ConnectorHDMI:
            return "HDMI";
        case ConnectorAnalogDVI:
            return "ADVI";
        default:
            return "UNKN";
    }
}

template<typename T, size_t N>
inline char *printConnector(char (&out)[N], T &connector) {
    snprintf(out, N, "type %08X (%s) flags %08X feat %04X pri %04X txmit %02X enc %02X hotplug %02X sense %02X",
        connector.type, printType(connector.type), connector.flags, connector.features, connector.priority,
        connector.transmitter, connector.encoder, connector.hotplug, connector.sense);
    return out;
}

inline void print(Connector *con, uint8_t num) {
#ifdef DEBUG
    for (uint8_t i = 0; con && i < num; i++) {
        char tmp[192];
        DBGLOG("support", "%u is %s", i,
            (getKernelVersion() >= KernelVersion::HighSierra) ? printConnector(tmp, (&con->modern)[i]) :
                                                                printConnector(tmp, (&con->legacy)[i]));
    }
#endif
}

inline bool valid(uint32_t size, uint8_t num) {
    return (size % sizeof(ModernConnector) == 0 && size / sizeof(ModernConnector) == num) ||
           (size % sizeof(LegacyConnector) == 0 && size / sizeof(LegacyConnector) == num);
}

inline void copy(Connector *out, uint8_t num, const Connector *in, uint32_t size) {
    bool outModern = (getKernelVersion() >= KernelVersion::HighSierra);
    bool inModern = size % sizeof(ModernConnector) == 0 && size / sizeof(ModernConnector) == num;

    for (uint8_t i = 0; i < num; i++) {
        if (outModern) {
            if (inModern) Connector::assign((&out->modern)[i], (&in->modern)[i]);
            else
                Connector::assign((&out->modern)[i], (&in->legacy)[i]);
        } else {
            if (inModern) Connector::assign((&out->legacy)[i], (&in->modern)[i]);
            else
                Connector::assign((&out->legacy)[i], (&in->legacy)[i]);
        }
    }
}

class Support {
    public:
    static Support *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);

    private:
    mach_vm_address_t orgPopulateDeviceMemory {0};
    mach_vm_address_t orgNotifyLinkChange {0};
    mach_vm_address_t orgGetConnectorsInfo {0};
    mach_vm_address_t orgGetAtomConnectorInfo {0};
    mach_vm_address_t orgTranslateAtomConnectorInfo {0};
    mach_vm_address_t orgGetGpioPinInfo {0};
    mach_vm_address_t orgGetNumberOfConnectors {0};
    mach_vm_address_t orgCreateAtomBiosParser {0};
    mach_vm_address_t orgATIControllerStart {0};
    mach_vm_address_t orgAtiGpuWranglerStart {0};

    static bool wrapNotifyLinkChange(void *atiDeviceControl, kAGDCRegisterLinkControlEvent_t event, void *eventData,
        uint32_t eventFlags);
    static bool doNotTestVram(IOService *ctrl, uint32_t reg, bool retryOnFail);
    static IOReturn wrapPopulateDeviceMemory(void *that, uint32_t reg);
    static IOReturn wrapGetAtomConnectorInfo(void *that, uint32_t connector, AtomConnectorInfo *coninfo);
    static IOReturn wrapGetGpioPinInfo(void *that, uint32_t pin, void *pininfo);
    static uint32_t wrapGetNumberOfConnectors(void *that);
    static void *wrapCreateAtomBiosParser(void *that, void *param1, unsigned char *param2, uint32_t dceVersion);
    void applyPropertyFixes(IOService *service, uint32_t connectorNum = 0);
    void updateConnectorsInfo(void *atomutils, t_getAtomObjectTableForType gettable, IOService *ctrl,
        Connector *connectors, uint8_t *sz);
    void autocorrectConnectors(uint8_t *baseAddr, AtomDisplayObjectPath *displayPaths, uint8_t displayPathNum,
        AtomConnectorObject *connectorObjects, uint8_t connectorObjectNum, Connector *connectors, uint8_t sz);
    void autocorrectConnector(uint8_t connector, uint8_t sense, uint8_t txmit, uint8_t enc, Connector *connectors,
        uint8_t sz);
    void processConnectorOverrides(KernelPatcher &patcher, mach_vm_address_t address, size_t size, bool modern);
    static IOReturn wrapGetConnectorsInfo(void *that, Connector *connectors, uint8_t *sz);
    static IOReturn wrapTranslateAtomConnectorInfo(void *that, AtomConnectorInfo *info, Connector *connector);
    static bool wrapATIControllerStart(IOService *ctrl, IOService *provider);
    static bool wrapAtiGpuWranglerStart(IOService *ctrl, IOService *provider);

    ThreadLocal<IOService *, 8> currentPropProvider;

    bool dviSingleLink {false};
    int count {0};
};

#endif /* kern_support_hpp */
