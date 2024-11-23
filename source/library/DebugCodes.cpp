// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "DebugCodes.h"

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    // Resolver Debug Codes
    TF_DEBUG_ENVIRONMENT_SYMBOL(OMNI_USD_RESOLVER, "OmniUsdResolver general resolve information");
    TF_DEBUG_ENVIRONMENT_SYMBOL(OMNI_USD_RESOLVER_MDL, "OmniUsdResolver MDL specific resolve information");
    TF_DEBUG_ENVIRONMENT_SYMBOL(OMNI_USD_RESOLVER_CONTEXT, "OmniUsdResolver Context information");
    TF_DEBUG_ENVIRONMENT_SYMBOL(OMNI_USD_RESOLVER_ASSET, "OmniUsdResolver asset read / write information");
}

PXR_NAMESPACE_CLOSE_SCOPE
