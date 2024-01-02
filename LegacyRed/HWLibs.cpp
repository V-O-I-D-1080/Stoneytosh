//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "HWLibs.hpp"
#include "LRed.hpp"
#include <Headers/kern_api.hpp>

static const char *pathRadeonX4000HWServices =
    "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/MacOS/AMDRadeonX4000HWServices";
static const char *pathRadeonX4000HWLibs = "/System/Library/Extensions/AMDRadeonX4000HWServices.kext/Contents/PlugIns/"
                                           "AMDRadeonX4000HWLibs.kext/Contents/MacOS/AMDRadeonX4000HWLibs";

static KernelPatcher::KextInfo kextRadeonX4000HWServices {"com.apple.kext.AMDRadeonX4000HWServices",
    &pathRadeonX4000HWServices, 1, {}, {}, KernelPatcher::KextInfo::Unloaded};
static KernelPatcher::KextInfo kextRadeonX4000HWLibs {"com.apple.kext.AMDRadeonX4000HWLibs", &pathRadeonX4000HWLibs, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

HWLibs *HWLibs::callback = nullptr;

void HWLibs::init() {
    callback = this;
    lilu.onKextLoadForce(&kextRadeonX4000HWServices);
    lilu.onKextLoadForce(&kextRadeonX4000HWLibs);
}

bool HWLibs::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
    if (kextRadeonX4000HWServices.loadIndex == index) {
        RouteRequestPlus requests[] = {
            {"__ZN36AMDRadeonX4000_AMDRadeonHWServicesCI16getMatchPropertyEv", forceX4000HWLibs},
            {"__ZN41AMDRadeonX4000_AMDRadeonHWServicesPolaris16getMatchPropertyEv", forceX4000HWLibs},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "HWServices",
            "Failed to route symbols");
        return true;
    } else if (kextRadeonX4000HWLibs.loadIndex == index) {
        LRed::callback->setRMMIOIfNecessary();

        CAILAsicCapsEntry *orgCapsTable = nullptr;
        CAILAsicCapsInitEntry *orgCapsInitTable = nullptr;

        bool gcn3 = LRed::callback->chipType >= ChipType::Carrizo;

        SolveRequestPlus solveRequests[] = {
            {"__ZL20CAIL_ASIC_CAPS_TABLE", orgCapsTable},
            {"_CAILAsicCapsInitTable", orgCapsInitTable},
            {gcn3 ? "__ZN30AtiAppleTongaPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks" :
                    "__ZN31AtiAppleHawaiiPowerTuneServicesC1EP11PP_InstanceP18PowerPlayCallbacks",
                this->orgPowerTuneConstructor},
        };
        PANIC_COND(!SolveRequestPlus::solveAll(patcher, index, solveRequests, address, size), "HWLibs",
            "Failed to resolve symbols");

        RouteRequestPlus requests[] = {
            {"__ZN15AmdCailServicesC2EP11IOPCIDevice", wrapAmdCailServicesConstructor,
                this->orgAmdCailServicesConstructor},
            {"__ZN25AtiApplePowerTuneServices23createPowerTuneServicesEP11PP_InstanceP18PowerPlayCallbacks",
                wrapCreatePowerTuneServices},
            {"_bonaire_perform_srbm_soft_reset", wrapBonairePerformSrbmReset, this->orgBonairePerformSrbmReset},
        };
        PANIC_COND(!RouteRequestPlus::routeAll(patcher, index, requests, address, size), "HWLibs",
            "Failed to route symbols");

        const LookupPatchPlus patches[] = {
            {&kextRadeonX4000HWLibs, AtiPowerPlayServicesCOriginal, AtiPowerPlayServicesCPatched, 1},
        };
        PANIC_COND(!LookupPatchPlus::applyAll(patcher, patches, address, size), "HWLibs", "Failed to apply patches!");

        UInt32 targetDeviceId = LRed::callback->deviceId;
        UInt32 targetFamilyId = LRed::callback->familyId;
        for (auto *tmp = orgCapsInitTable; tmp->deviceId != 0xFFFFFFFF; tmp++) {
            if (tmp->familyId == targetFamilyId && tmp->deviceId == targetDeviceId) {
                PANIC_COND(MachInfo::setKernelWriting(true, KernelPatcher::kernelWriteLock) != KERN_SUCCESS, "HWLibs",
                    "Failed to enable kernel writing");
                orgCapsTable->familyId = LRed::callback->familyId;
                orgCapsTable->deviceId = LRed::callback->deviceId;
                orgCapsTable->revision = LRed::callback->revision;
                orgCapsTable->extRevision = static_cast<UInt32>(LRed::callback->emulatedRevision);
                orgCapsTable->pciRevision = LRed::callback->pciRevision;
                orgCapsTable->caps = tmp->caps;
                *orgCapsInitTable = *tmp;
                MachInfo::setKernelWriting(false, KernelPatcher::kernelWriteLock);
                break;
            }
        }
        PANIC_COND(orgCapsInitTable->deviceId == 0xFFFFFFFF, "HWLibs", "Failed to find init caps table entry");
        DBGLOG("HWLibs", "Applied DDI Caps patches");
        return true;
    }

    return false;
}

const char *HWLibs::forceX4000HWLibs() {
    DBGLOG("HWServices", "Forcing HWServices to load X4000HWLibs");
    //! By default, X4000HWServices on CI loads X4050HWLibs, we override this here because X4050 has no KV logic.
    //! HWServicesFiji loads X4000HWLibs by default, so we don't need it there.
    //! Polaris is an interesting topic because it selects the name by using the Framebuffer name.
    return "Load4000";
}

void HWLibs::wrapAmdCailServicesConstructor(void *that, IOPCIDevice *provider) {
    DBGLOG("HWLibs", "AmdCailServices constructor called!");
    FunctionCast(wrapAmdCailServicesConstructor, callback->orgAmdCailServicesConstructor)(that, provider);
}

void *HWLibs::wrapCreatePowerTuneServices(void *that, void *param2) {
    auto *ret = OSObject::operator new(0x18);
    callback->orgPowerTuneConstructor(ret, that, param2);
    return ret;
}

void HWLibs::wrapBonairePerformSrbmReset(void *param1, UInt32 bit) {
    DBGLOG("HWLibs", "_bonaire_perform_srbm_reset >>");
    bit &= ~SRBM_SOFT_RESET__SOFT_RESET_MC_MASK;
    DBGLOG("HWLibs", "Stripping SRBM_SOFT_RESET__SOFT_RESET_MC_MASK bit");
    FunctionCast(wrapBonairePerformSrbmReset, callback->orgBonairePerformSrbmReset)(param1, bit);
}
