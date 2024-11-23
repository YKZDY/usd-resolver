// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "OmniUsdResolverContext_Ar2.h"

OmniUsdResolverContext::OmniUsdResolverContext(std::string assetPath) : _assetPath(std::move(assetPath))
{
}

const std::string& OmniUsdResolverContext::GetAssetPath() const
{
    return _assetPath;
}

bool OmniUsdResolverContext::operator<(const OmniUsdResolverContext& rhs) const
{
    return _assetPath < rhs._assetPath;
}

bool OmniUsdResolverContext::operator==(const OmniUsdResolverContext& rhs) const
{
    return _assetPath == rhs._assetPath;
}

size_t hash_value(const OmniUsdResolverContext& ctx)
{
    return TfHash()(ctx._assetPath);
}
