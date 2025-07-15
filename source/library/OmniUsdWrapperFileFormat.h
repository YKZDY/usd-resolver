// SPDX-FileCopyrightText: Copyright (c) 2021-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#define OMNI_USD_FILE_FORMAT_TOKENS ((Id, "omnicache"))((Version, "1.0"))((Target, "usd"))

#pragma warning(push)
#pragma warning(disable : 4003) // not enough actual parameters for macro
TF_DECLARE_PUBLIC_TOKENS(OmniUsdWrapperFileFormatTokens, OMNI_USD_FILE_FORMAT_TOKENS);
#pragma warning(pop)

TF_DECLARE_WEAK_AND_REF_PTRS(OmniUsdWrapperFileFormat);

class OmniUsdWrapperData;

class OmniUsdWrapperFileFormat : public SdfFileFormat
{
public:
    // SdfFileFormat overrides.
    using SdfFileFormat::FileFormatArguments;

    virtual SdfAbstractDataRefPtr InitData(const FileFormatArguments& args) const override;

    virtual bool CanRead(std::string const& file) const override;

    virtual bool Read(SdfLayer* layer, std::string const& resolvedPath, bool metadataOnly) const override;

    virtual bool WriteToFile(const SdfLayer& layer,
                             std::string const& filePath,
                             std::string const& comment,
                             const FileFormatArguments& args) const override;

    virtual bool ReadFromString(SdfLayer* layer, std::string const& str) const override;

    virtual bool WriteToString(const SdfLayer& layer, std::string* str, std::string const& comment) const override;

    virtual bool WriteToStream(const SdfSpecHandle& spec, std::ostream& out, size_t indent) const override;

protected:
    template <typename T>
    friend class PXR_INTERNAL_NS::Sdf_FileFormatFactory;

private:
    OmniUsdWrapperFileFormat();
    virtual ~OmniUsdWrapperFileFormat();

    class DummyFileFormat : public SdfFileFormat
    {
    public:
        virtual bool CanRead(std::string const& file) const override
        {
            return false;
        }
        virtual bool Read(SdfLayer* layer, std::string const& resolvedPath, bool metadataOnly) const override
        {
            return false;
        }
        DummyFileFormat() : SdfFileFormat(TfToken(), TfToken(), TfToken(), std::string())
        {
        }
    };
    SdfFileFormatRefPtr _dummyFileFormat;
    SdfFileFormatConstPtr GetFileFormat(SdfLayer const* wrappedLayer, std::string const& path) const;

protected:
    virtual SdfLayer* _InstantiateNewLayer(const SdfFileFormatConstPtr& fileFormat,
                                           const std::string& identifier,
                                           const std::string& realPath,
                                           const ArAssetInfo& assetInfo,
                                           const FileFormatArguments& args) const override;
};
