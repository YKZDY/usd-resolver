// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#pragma once

#include "UsdIncludes.h"

/// \brief The Omniverse Usd ResolverContext. A very bare-bones implementation
/// which just stores the assetPath
class OmniUsdResolverContext
{
public:
    OmniUsdResolverContext() = default;
    explicit OmniUsdResolverContext(std::string assetPath);

    /// Returns the assetPath that the context is currently bound to
    const std::string& GetAssetPath() const;

    bool operator<(const OmniUsdResolverContext& rhs) const;
    bool operator==(const OmniUsdResolverContext& rhs) const;

    friend size_t hash_value(const OmniUsdResolverContext& ctx);

private:
    std::string _assetPath;
};

PXR_NAMESPACE_OPEN_SCOPE
AR_DECLARE_RESOLVER_CONTEXT(OmniUsdResolverContext);
PXR_NAMESPACE_CLOSE_SCOPE
