//! Copyright © 2023 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "LRed.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_version.hpp>
#include <Headers/plugin_start.hpp>
#include <IOKit/IOCatalogue.h>

static LRed lred;

static const char *bootargDebug = "-LRedDebug";

static const char *bootargBeta = "-LRedBeta";

PluginConfiguration ADDPR(config) {
    xStringify(PRODUCT_NAME),
    parseModuleVersion(xStringify(MODULE_VERSION)),
    LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
    nullptr,
    0,
    &bootargDebug,
    1,
    &bootargBeta,
    1,
    KernelVersion::HighSierra,    //! we cannot go older, AMD patches only support HS+
    KernelVersion::Sonoma,        //! we don't actually support Sonoma, here for debugging/testing
    []() { lred.init(); },
};
