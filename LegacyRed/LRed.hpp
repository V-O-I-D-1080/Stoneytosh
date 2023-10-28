//!  Copyright © 2022-2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5. See LICENSE for
//!  details.

#ifndef kern_lred_hpp
#define kern_lred_hpp
#include "AMDCommon.hpp"
#include "ATOMBIOS.hpp"
#include "Firmware.hpp"
#include <Headers/kern_iokit.hpp>
#include <IOKit/acpi/IOACPIPlatformExpert.h>
#include <IOKit/graphics/IOFramebuffer.h>
#include <IOKit/pci/IOPCIDevice.h>

//! GFX core codenames
enum struct ChipType : UInt32 {
    Spectre = 0,    //! Kaveri
    Spooky,         //! Kaveri 2? downgraded from Spectre core
    Kalindi,        //! Kabini/Bhavani
    Godavari,       //! Mullins
    Carrizo,
    Stoney,
    Unknown,
};

//! Front-end consumer names, includes non-consumer names as-well
enum struct ChipVariant : UInt32 {
    Kaveri = 0,
    Kabini,
    Temash,
    Bhavani,
    Mullins,
    Carrizo,
    Bristol,    //! Bristol is actually just a Carrizo+, hence why it isn't in ChipType
    Stoney,
    Unknown,
};

//! Hack
class AppleACPIPlatformExpert : IOACPIPlatformExpert {
    friend class LRed;
};

// https://elixir.bootlin.com/linux/latest/source/drivers/gpu/drm/amd/amdgpu/amdgpu_bios.c#L49
static bool checkAtomBios(const uint8_t *bios, size_t size) {
    uint16_t tmp, bios_header_start;

    if (size < 0x49) {
        DBGLOG("lred", "VBIOS size is invalid");
        return false;
    }

    if (bios[0] != 0x55 || bios[1] != 0xAA) {
        DBGLOG("lred", "VBIOS signature <%x %x> is invalid", bios[0], bios[1]);
        return false;
    }

    bios_header_start = bios[0x48] | (bios[0x49] << 8);
    if (!bios_header_start) {
        DBGLOG("lred", "Unable to locate VBIOS header");
        return false;
    }

    tmp = bios_header_start + 4;
    if (size < tmp) {
        DBGLOG("lred", "BIOS header is broken");
        return false;
    }

    if (!memcmp(bios + tmp, "ATOM", 4) || !memcmp(bios + tmp, "MOTA", 4)) {
        DBGLOG("lred", "ATOMBIOS detected");
        return true;
    }

    return false;
}

class LRed {
    friend class GFXCon;
    friend class HWLibs;
    friend class X4000;
    friend class Support;

    public:
    static LRed *callback;

    void init();
    void processPatcher(KernelPatcher &patcher);
    void processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
    void setRMMIOIfNecessary();

    private:
    static const char *getPrefix() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "LRed", "Unknown chip type");
        static const char *chipNames[] = {"kaveri_", "kaveri_", "kabini_", "mullins_", "carrizo_", "stoney_"};
        return chipNames[static_cast<int>(callback->chipType)];
    }

    static const char *getVCEPrefix() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "LRed", "Unknown chip type");
        static const char *vcePrefix[] = {"ativce02", "ativce02", "ativce02", "ativce02", "amde31a", "amde34a"};
        return vcePrefix[static_cast<int>(callback->chipType)];
    }

    static const char *getUVDPrefix() {
        PANIC_COND(callback->chipType == ChipType::Unknown, "LRed", "Unknown chip type");
        static const char *uvdPrefix[] = {"ativvaxy_cik", "ativvaxy_cik", "ativvaxy_cik", "ativvaxy_cik", "ativvaxy_cz",
            "ativvaxy_stn"};
        return uvdPrefix[static_cast<int>(callback->chipType)];
    }

    bool getVBIOSFromVFCT(IOPCIDevice *obj) {
        DBGLOG("lred", "Fetching VBIOS from VFCT table");
        auto *expert = reinterpret_cast<AppleACPIPlatformExpert *>(obj->getPlatform());
        PANIC_COND(!expert, "lred", "Failed to get AppleACPIPlatformExpert");

        auto *vfctData = expert->getACPITableData("VFCT", 0);
        if (!vfctData) {
            DBGLOG("lred", "No VFCT from AppleACPIPlatformExpert");
            return false;
        }

        auto *vfct = static_cast<const VFCT *>(vfctData->getBytesNoCopy());
        PANIC_COND(!vfct, "lred", "VFCT OSData::getBytesNoCopy returned null");

        auto offset = vfct->vbiosImageOffset;

        while (offset < vfctData->getLength()) {
            auto *vHdr =
                static_cast<const GOPVideoBIOSHeader *>(vfctData->getBytesNoCopy(offset, sizeof(GOPVideoBIOSHeader)));
            if (!vHdr) {
                DBGLOG("lred", "VFCT header out of bounds");
                return false;
            }

            auto *vContent = static_cast<const uint8_t *>(
                vfctData->getBytesNoCopy(offset + sizeof(GOPVideoBIOSHeader), vHdr->imageLength));
            if (!vContent) {
                DBGLOG("lred", "VFCT VBIOS image out of bounds");
                return false;
            }

            offset += sizeof(GOPVideoBIOSHeader) + vHdr->imageLength;

            if (vHdr->imageLength && vHdr->pciBus == obj->getBusNumber() && vHdr->pciDevice == obj->getDeviceNumber() &&
                vHdr->pciFunction == obj->getFunctionNumber() &&
                vHdr->vendorID == obj->configRead16(kIOPCIConfigVendorID) &&
                vHdr->deviceID == obj->configRead16(kIOPCIConfigDeviceID)) {
                if (!checkAtomBios(vContent, vHdr->imageLength)) {
                    DBGLOG("lred", "VFCT VBIOS is not an ATOMBIOS");
                    return false;
                }
                this->vbiosData = OSData::withBytes(vContent, vHdr->imageLength);
                PANIC_COND(!this->vbiosData, "lred", "VFCT OSData::withBytes failed");
                obj->setProperty("ATY,bin_image", this->vbiosData);
                return true;
            }
        }

        return false;
    }

    bool getVBIOSFromVRAM(IOPCIDevice *provider) {
        auto *bar0 = provider->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
        if (!bar0 || !bar0->getLength()) {
            DBGLOG("lred", "FB BAR not enabled");
            OSSafeReleaseNULL(bar0);
            return false;
        }
        auto *fb = reinterpret_cast<const uint8_t *>(bar0->getVirtualAddress());
        UInt32 size = 256 * 1024;    //! ???
        if (!checkAtomBios(fb, size)) {
            DBGLOG("lred", "VRAM VBIOS is not an ATOMBIOS");
            OSSafeReleaseNULL(bar0);
            return false;
        }
        this->vbiosData = OSData::withBytes(fb, size);
        PANIC_COND(!this->vbiosData, "lred", "VRAM OSData::withBytes failed");
        provider->setProperty("ATY,bin_image", this->vbiosData);
        OSSafeReleaseNULL(bar0);
        return true;
    }

    UInt32 readReg32(UInt32 reg) {
        if (reg * 4 < this->rmmio->getLength()) {
            return this->rmmioPtr[reg];
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            return this->rmmioPtr[mmPCIE_DATA2];
        }
    }

    void writeReg32(UInt32 reg, UInt32 val) {
        if (reg * 4 < this->rmmio->getLength()) {
            this->rmmioPtr[reg] = val;
        } else {
            this->rmmioPtr[mmPCIE_INDEX2] = reg;
            this->rmmioPtr[mmPCIE_DATA2] = val;
        }
    }

    UInt32 smcReadReg32Cz(UInt32 reg) {
        writeReg32(mmMP0PUB_IND_INDEX, reg);
        return readReg32(mmMP0PUB_IND_DATA);
    }

    template<typename T>
    T *getVBIOSDataTable(UInt32 index) {
        auto *vbios = static_cast<const uint8_t *>(this->vbiosData->getBytesNoCopy());
        auto base = *reinterpret_cast<const uint16_t *>(vbios + ATOM_ROM_TABLE_PTR);
        auto dataTable = *reinterpret_cast<const uint16_t *>(vbios + base + ATOM_ROM_DATA_PTR);
        auto *mdt = reinterpret_cast<const uint16_t *>(vbios + dataTable + 4);

        if (mdt[index]) {
            UInt32 offset = index * 2 + 4;
            auto index = *reinterpret_cast<const uint16_t *>(vbios + dataTable + offset);
            return reinterpret_cast<T *>(const_cast<uint8_t *>(vbios) + index);
        }

        return nullptr;
    }

    OSData *vbiosData {nullptr};
    ChipType chipType = ChipType::Unknown;
    ChipVariant chipVariant = ChipVariant::Unknown;
    bool isGCN3 = false;
    bool isStoney3CU = false;
    UInt64 fbOffset {0};
    IOMemoryMap *rmmio {nullptr};
    volatile UInt32 *rmmioPtr {nullptr};
    UInt32 deviceId {0};
    UInt16 enumeratedRevision {0};
    UInt16 revision {0};
    UInt32 pciRevision {0};
    UInt32 currentFamilyId {0};
    UInt32 currentEmulatedRevisionId {0};
    IOPCIDevice *iGPU {nullptr};

    OSMetaClass *metaClassMap[4][2] = {{nullptr}};

    mach_vm_address_t orgSafeMetaCast {0};
    static OSMetaClassBase *wrapSafeMetaCast(const OSMetaClassBase *anObject, const OSMetaClass *toMeta);

    mach_vm_address_t orgApplePanelSetDisplay {0};

    static size_t wrapFunctionReturnZero();
    static bool wrapApplePanelSetDisplay(IOService *that, IODisplay *display);
};

/* ---- Patches ---- */

//! Change frame-buffer count >= 2 check to >= 1.
static const UInt8 kAGDPFBCountCheckOriginal[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x02};
static const UInt8 kAGDPFBCountCheckPatched[] = {0x02, 0x00, 0x00, 0x83, 0xF8, 0x01};

//! Ditto
static const UInt8 kAGDPFBCountCheckVenturaOriginal[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x02};
static const UInt8 kAGDPFBCountCheckVenturaPatched[] = {0x41, 0x83, 0xBE, 0x14, 0x02, 0x00, 0x00, 0x01};

//! Neutralise access to AGDP configuration by board identifier.
static const UInt8 kAGDPBoardIDKeyOriginal[] = "board-id";
static const UInt8 kAGDPBoardIDKeyPatched[] = "applehax";

#endif /* kern_lred_hpp */
