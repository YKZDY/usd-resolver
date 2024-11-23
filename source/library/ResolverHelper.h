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

#include "Defines.h"

#include "OmniUsdResolverCache.h"

#include <chrono>
#include <string>

/// \brief A utility class that assists with resolver-specific functions
/// shared between Ar 1.0 and Ar 2.0. These functions should be valid to call
/// regardless of which Ar version is being used.
class ResolverHelper
{
public:
    /// \brief Determines if the resolvedPath can be written to.
    /// \param resolvedPath the resolvedPath that will be checked for write access
    /// \param[out] whyNot outputs the reason why the resolvedPath can not be written to
    static bool CanWrite(const std::string& resolvedPath, std::string* whyNot = nullptr);

    /// \brief Resolve the identifier to the final path including any sort of normalization
    /// \param[out] identifier the identifier to resolve
    /// \param[out] url the url that was used to performed for the resolve. This is typically the same as identifier
    /// but in some cases the server my encode additional information, such as converting spaces to %20.
    /// \param[out] version the version that was resolved
    /// \param[out] modifiedTime the time point when the identifier was last modified
    /// \param[out] size the size of the content in bytes
    /// \return the resolved identifier. The result will be empty if the identifier could not be resolved
    static std::string Resolve(const std::string& identifier,
                               std::string& url,
                               std::string& version,
                               std::chrono::system_clock::time_point& modifiedTime,
                               uint64_t& size);
};
