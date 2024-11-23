// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "TestPrimaryResolver_Ar2.h"

#include "UsdIncludes.h"
#include "utils/OmniClientUtils.h"

#include <OmniClient.h>
#include <iostream>

std::string TestPrimaryResolver::_CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const
{
    omniClientPushBaseUrl(anchorAssetPath.GetPathString().c_str());
    std::string result = resolveUrlComposed(assetPath);
    omniClientPopBaseUrl(anchorAssetPath.GetPathString().c_str());

    return result;
}
std::string TestPrimaryResolver::_CreateIdentifierForNewAsset(const std::string& assetPath,
                                                              const ArResolvedPath& anchorAssetPath) const
{
    return _CreateIdentifier(assetPath, anchorAssetPath);
}
ArResolvedPath TestPrimaryResolver::_Resolve(const std::string& assetPath) const
{
    if (assetPath.empty())
    {
        return {};
    }

    // Nothing fancy is happening here. We take a URL like fake://a.host.com/path/to/my/file.usd
    // and return a path like /test_primrary/path/to/my/file.usd
    static const std::string kRootPath{ "/test_primrary" };
    auto parsedUrl = parseUrl(assetPath);
    return ArResolvedPath(TfStringCatPaths(kRootPath, parsedUrl->path));
}
ArResolvedPath TestPrimaryResolver::_ResolveForNewAsset(const std::string& assetPath) const
{
    return _Resolve(assetPath);
}
std::shared_ptr<ArAsset> TestPrimaryResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    return {};
}
std::shared_ptr<ArWritableAsset> TestPrimaryResolver::_OpenAssetForWrite(const ArResolvedPath& resolvedPath,
                                                                         WriteMode writeMode) const
{
    return {};
}

AR_DEFINE_RESOLVER(TestPrimaryResolver, ArResolver);
