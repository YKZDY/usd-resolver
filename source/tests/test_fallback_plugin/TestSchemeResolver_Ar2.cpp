// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "TestSchemeResolver_Ar2.h"

#include "UsdIncludes.h"
#include "utils/OmniClientUtils.h"

#include <OmniClient.h>

std::string TestSchemeResolver::_CreateIdentifier(const std::string& assetPath, const ArResolvedPath& anchorAssetPath) const
{
    omniClientPushBaseUrl(anchorAssetPath.GetPathString().c_str());
    std::string result = resolveUrlComposed(assetPath);
    omniClientPopBaseUrl(anchorAssetPath.GetPathString().c_str());

    return result;
}
std::string TestSchemeResolver::_CreateIdentifierForNewAsset(const std::string& assetPath,
                                                             const ArResolvedPath& anchorAssetPath) const
{
    return _CreateIdentifier(assetPath, anchorAssetPath);
}
ArResolvedPath TestSchemeResolver::_Resolve(const std::string& assetPath) const
{
    if (assetPath.empty())
    {
        return {};
    }

    // Nothing fancy is happening here. We take a URL like test://b.host.com/path/to/my/file.usd
    // and return a path like /test_scheme/path/to/my/file.usd
    static const std::string kRootPath{ "/test_scheme" };
    auto parsedUrl = parseUrl(assetPath);
    if (strcmp(parsedUrl->scheme, "test") != 0)
    {
        return {};
    }

    return ArResolvedPath(TfStringCatPaths(kRootPath, parsedUrl->path));
}
ArResolvedPath TestSchemeResolver::_ResolveForNewAsset(const std::string& assetPath) const
{
    return _Resolve(assetPath);
}
std::shared_ptr<ArAsset> TestSchemeResolver::_OpenAsset(const ArResolvedPath& resolvedPath) const
{
    return {};
}
std::shared_ptr<ArWritableAsset> TestSchemeResolver::_OpenAssetForWrite(const ArResolvedPath& resolvedPath,
                                                                        ArResolver::WriteMode writeMode) const
{
    return {};
}

AR_DEFINE_RESOLVER(TestSchemeResolver, ArResolver);
