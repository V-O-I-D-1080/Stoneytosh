# LegacyRed ![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/NootInc/LegacyRed/main.yml?branch=master&logo=github&style=for-the-badge)

> [!IMPORTANT]
> It is highly likely that you need this [fork of Lilu](https://github.com/Zormeister/Lilu) because of a flaw in the detection of AMD iGPUs; most pre-Vega ones don’t get detected.

> [!NOTE]
> GCN 2-based and Carrizo iGPUs are only supported on macOS 12 and older due to the code removed since 13 along with MacPro6,1
>
> AMD’s Vega and newer dGPUs do not conflict with this iGPU kext

Thanks to [Acidanthera](https://github.com/acidanthera) for the [WhateverGreen](https://github.com/acidanthera/WhateverGreen) kern_agdc and kern_rad code used in Support.cpp
