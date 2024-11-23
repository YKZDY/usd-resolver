// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "OmniUsdResolverCache.h"

#include <tbb/concurrent_hash_map.h>

bool OmniUsdResolverCache::Get(const std::string& key, Entry& entry) const
{
    Cache::const_accessor accessor;
    bool result = _cache.find(accessor, key);
    if (result)
    {
        entry.identifier = accessor->second.identifier;
        entry.url = accessor->second.url;
        entry.resolvedPath = accessor->second.resolvedPath;
        entry.version = accessor->second.version;
        entry.modifiedTime = accessor->second.modifiedTime;
        entry.size = accessor->second.size;
    }

    return result;
}

void OmniUsdResolverCache::Add(const std::string& key, const Entry& entry)
{
    Cache::accessor accessor;
    if (_cache.insert(accessor, key))
    {
        accessor->second = entry;
    }
}

bool OmniUsdResolverCache::Remove(const std::string& key)
{
    return _cache.erase(key);
}
