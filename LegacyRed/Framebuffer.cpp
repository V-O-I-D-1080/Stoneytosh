//! Copyright © 2024 ChefKiss Inc. Licensed under the Thou Shalt Not Profit License version 1.5.
//! See LICENSE for details.

#include "Framebuffer.hpp"
#include "LRed.hpp"
#include "PatcherPlus.hpp"
#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

static const char *pathAMDFramebuffer = "/System/Library/Extensions/AMDFramebuffer.kext/Contents/MacOS/AMDFramebuffer";

static KernelPatcher::KextInfo kextAMDFramebuffer = {"com.apple.kext.AMDFramebuffer", &pathAMDFramebuffer, 1,
    {}, {}, KernelPatcher::KextInfo::Unloaded};

Framebuffer *Framebuffer::callback = nullptr;

void Framebuffer::init() {
    callback = this;
    lilu.onKextLoadForce(&kextAMDFramebuffer);
}

bool Framebuffer::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	if (kextAMDFramebuffer.loadIndex == index) {
		//if (PE_parse_boot_argn("AMDBitsPerComponent", &this->bitsPerComponent, sizeof(this->bitsPerComponent))) {
			//DBGLOG("Framebuffer", "Bits Per Component: %d", this->bitsPerComponent);
		//}
		SolveRequestPlus solveRequest {"__ZN14AMDFramebuffer32getDevicePropertiesForUserClientEP12OSDictionary", this->orgGetDevPropsForUC};
		PANIC_COND(!solveRequest.solve(patcher, index, address, size), "Framebuffer", "Failed to solve symbol");
		
		
		RouteRequestPlus request {"__ZN14AMDFramebuffer30populateDisplayModeInformationEP28AtiDetailedTimingInformationP6WindowS3_S3_P24IODisplayModeInformation", wrapPopulateDisplayModeInfo, this->orgPopulateDisplayModeInfo};
		PANIC_COND(!request.route(patcher, index, address, size), "Framebuffer", "Failed to route populateDisplayModeInformation!");
		return true;
	}
	return false;
}

IOReturn Framebuffer::fbDumpDevProps() {
	if (this->fbPtr == nullptr) {
		DBGLOG("Framebuffer", "Cannot dump at this time, FB pointer is null.");
		return kIOReturnNoDevice;
	}
	
	OSDictionary *dict = OSDictionary::withCapacity(1);
	if (dict == nullptr) {
		DBGLOG("Framebuffer", "Failed to create dictionary");
		return kIOReturnNoMemory;
	}
	
	//! funnily enough theres one where we can dump the FB itself
	callback->orgGetDevPropsForUC(callback->fbPtr, dict); //! attrocity #1
	
	LRed::callback->iGPU->setProperty("iGPU Device Config", dict);
	OSSafeReleaseNULL(dict);
	
	return kIOReturnSuccess;
}

IOReturn Framebuffer::wrapPopulateDisplayModeInfo(void *that, void *detailedTiming, void *param2, void *param3, void *param4, void *modeInfo) {
	DBGLOG("Framebuffer", "populateDisplayModeInfo: that %p, detailedTming %p, param2 %p, param3 %p, param5 %p, modeInfo %p", that, detailedTiming, param2, param3, param4, modeInfo);
	IOReturn ret = FunctionCast(wrapPopulateDisplayModeInfo, callback->orgPopulateDisplayModeInfo)(that, detailedTiming, param2, param3, param4, modeInfo);
	//! BROKEN
	/*
	switch (callback->bitsPerComponent) {
		case 0:
			break;
		case 5:
			getMember<UInt32>(modeInfo, 0xC) = 0;
			break;
		case 8:
			getMember<UInt32>(modeInfo, 0xC) = 1;
			break;
		case 10:
			getMember<UInt32>(modeInfo, 0xC) = 2;
			break;
		default:
			PANIC("Framebuffer", "Unsupported bits per component value!");
	}
	*/
	DBGLOG("Framebuffer", "populateDisplayModeInfo: ret: 0x%x, maxDepthIndex: 0x%x (%d)", ret, getMember<UInt32>(modeInfo, 0xC), getMember<UInt32>(modeInfo, 0xC));
	if (callback->fbPtr == nullptr) {
		callback->fbPtr = that;
	}
	return ret;
}
