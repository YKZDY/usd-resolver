// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "Notifications.h"

#include <map>
#include <mutex>

namespace
{
std::mutex g_mutex;
std::map<uint32_t, std::pair<void*, OmniUsdResolverEventCallback>> g_callbacks;
uint32_t g_nextHandle = 1;
}; // namespace

OMNIUSDRESOLVER_EXPORT(uint32_t)
omniUsdResolverRegisterEventCallback(void* userData, OmniUsdResolverEventCallback callback) OMNIUSDRESOLVER_NOEXCEPT
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto handle = g_nextHandle++;
    g_callbacks.emplace(handle, std::make_pair(userData, callback));
    return handle;
}

OMNIUSDRESOLVER_EXPORT(void) omniUsdResolverUnregisterCallback(uint32_t handle) OMNIUSDRESOLVER_NOEXCEPT
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_callbacks.erase(handle);
}

OMNIUSDRESOLVER_EXPORT(void)
SendNotification(const char* identifier,
                 OmniUsdResolverEvent eventType,
                 OmniUsdResolverEventState eventState,
                 uint64_t fileSize)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto&& cb : g_callbacks)
    {
        cb.second.second(cb.second.first, identifier, eventType, eventState, fileSize);
    }
}
