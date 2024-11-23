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

#include <pxr/base/tf/safeOutputFile.h>
#include <pxr/usd/ar/writableAsset.h>

struct OmniUsdWritableData
{
    std::string url;
    std::string file;
    TfSafeOutputFile safeFile;
};

/// \brief A ArWritableAsset implementation that allows writing assets
/// directly through the client-library to Omniverse
///
/// This writes to a temporary file on disk then moves that content to Omniverse at close
class OmniUsdWritableAsset final : public ArWritableAsset
{
public:
    OmniUsdWritableAsset(OmniUsdWritableData&& outputData);
    virtual ~OmniUsdWritableAsset() = default;

    /// Opens the resolved asset for writing. Returns nullptr if the asset can not be written to
    static std::shared_ptr<OmniUsdWritableAsset> Open(const ArResolvedPath& resolvedPath, ArResolver::WriteMode mode);

    /// \brief Closes the asset for writing and moves the written content to Nucleus
    /// \return true if the asset was successfully written. Otherwise, false
    virtual bool Close() override;
    /// \brief Writes the data to the asset
    /// \param buffer holds the data that will be written to the asset
    /// \param count the number of bytes to write to the asset
    /// \param offset the offset for \p buffer to begin writing to the asset from
    /// \return the number of bytes written to the asset.
    virtual size_t Write(const void* buffer, size_t count, size_t offset) override;

private:
    OmniUsdWritableData _outputData;
};
