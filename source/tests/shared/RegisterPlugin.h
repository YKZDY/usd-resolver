// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.
#pragma once

#include <pxr/base/arch/symbols.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/resolver.h>

namespace test
{

inline bool registerPluginPaths(const std::vector<std::string>& paths)
{
    std::string thisPath;
    if (!PXR_NS::ArchGetAddressInfo(reinterpret_cast<void*>(&registerPluginPaths), &thisPath, nullptr, nullptr, nullptr))
    {
        return false;
    }

    for (const auto& path : paths)
    {
        PXR_NS::PlugRegistry::GetInstance().RegisterPlugins(PXR_NS::TfGetPathName(thisPath) + path);
    }

    return true;
}

inline bool registerPlugin()
{
    std::vector<std::string> paths{ "usd/omniverse/resources/", "test/fileformat/resources/", "test/fallback/resources/" };
    if (!registerPluginPaths(paths))
    {
        return false;
    }

    // We always want to make sure that the OmniUsdResolver is the resolver used
    // This needs to be called before the first call to ArGetResolver() so we do it here immediately after
    // registering the path to OmniUsdResolver
    PXR_NS::ArSetPreferredResolver("OmniUsdResolver");
    return true;
}

} // namespace test
