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

#include <OmniClient.h>
#include <memory>

struct OmniUsdReadableData
{
    std::string url;
    std::string localFile;
    FILE* file = nullptr;
    OmniClientRequestId clientRequestId = 0;
};

/// \brief A ArAsset implementation that allows reading assets
/// directly from the client-library to Omniverse
///
/// In order to take advantage of memory mapped files and local caching, assets
/// are first written to a local file cache on disk and then memory mapped from that file
class OmniUsdAsset final : public ArAsset
{
public:
    OmniUsdAsset(OmniUsdReadableData&& inputData);
    virtual ~OmniUsdAsset();

    OmniUsdAsset(const OmniUsdAsset&) = delete;
    OmniUsdAsset& operator=(const OmniUsdAsset&) = delete;

    /// Opens the resolved asset for reading. Returns nullptr if the asset can not be read
    static std::shared_ptr<OmniUsdAsset> Open(const ArResolvedPath& resolvedPath);

    /// Returns the total number of bytes for the asset
    virtual size_t GetSize() const override;

    /// Returns the buffer of data for the asset
    virtual std::shared_ptr<const char> GetBuffer() const override;

    /// \brief Reads data from the asset
    /// \param[out] out holds the data that was read from the asset
    /// \param count the number of bytes to read
    /// \param offset the offset for \p out to begin reading the asset to
    /// \return the number of bytes read from the asset
    virtual size_t Read(void* out, size_t count, size_t offset) const override;

    virtual std::pair<FILE*, size_t> GetFileUnsafe() const override;

private:
    OmniUsdReadableData _inputData;
};
