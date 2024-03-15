//! Copyright © 2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#pragma once
#include "AMDCommon.hpp"
#include <Headers/kern_patcher.hpp>

using t_getPropsForUserClient = void (*)(void *fb, OSDictionary *dict);

class Framebuffer {
    public:
    static Framebuffer *callback;
    void init();
    bool processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size);
    IOReturn fbDumpDevProps();

    private:
    mach_vm_address_t orgPopulateDisplayModeInfo {0};
    t_getPropsForUserClient orgGetDevPropsForUC {0};

    static IOReturn wrapPopulateDisplayModeInfo(void *that, void *detailedTiming, void *param2, void *param3,
        void *param4, void *modeInfo);

    UInt32 bitsPerComponent {0};
    void *fbPtr {nullptr};
};
