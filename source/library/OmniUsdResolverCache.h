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

#include <chrono>
#include <memory>

/// \brief A simple thread-safe cache used by the Omniverse Usd Resolver
class OmniUsdResolverCache
{
public:
    /// \brief structure containing the data that will be stored in the cache
    struct Entry
    {
        std::string identifier;
        std::string url;
        std::string resolvedPath;
        std::string version;
        std::chrono::system_clock::time_point modifiedTime;
        uint64_t size;
    };

    OmniUsdResolverCache() = default;

    /// \brief Finds the entry in the cache located at \p key
    /// \param key the key to the entry in the cache
    /// \param[out] entry the data entry stored in the cache
    /// \returns true if the entry was found. Otherwise, false
    bool Get(const std::string& key, Entry& entry) const;

    /// \brief Adds an entry to the cache located at \p key if not present
    /// \param key the key to the entry that is being cached
    /// \param[out] entry the data entry that will be added to the cache
    void Add(const std::string& key, const Entry& entry);
    /// \brief Removes the entry in the cache located at \p key
    /// \param key the key to the entry that will be removed from the cache
    /// \returns true if the entry was removed. Otherwise, false
    bool Remove(const std::string& key);

private:
    // XXX: If we want to get rid of the direct tbb dependency we could put this
    // behind an Impl
    using Cache = tbb::concurrent_hash_map<std::string, OmniUsdResolverCache::Entry>;
    Cache _cache;
};

typedef PXR_NS::ArThreadLocalScopedCache<OmniUsdResolverCache> OmniUsdResolverScopedCache;
typedef OmniUsdResolverScopedCache::CachePtr OmniUsdResolverScopedCachePtr;
