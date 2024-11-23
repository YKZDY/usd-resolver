// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <fstream>
#include <iostream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

#define TEST_FILE_FORMAT_TOKENS ((Extension, "testff"))((Id, "testff"))((Version, "1.0"))((Target, "usd"))

#pragma warning(push)
#pragma warning(disable : 4003) // not enough actual parameters for macro
TF_DECLARE_PUBLIC_TOKENS(TestFileFormatTokens, TEST_FILE_FORMAT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(TestFileFormatTokens, TEST_FILE_FORMAT_TOKENS);
#pragma warning(pop)

TF_DECLARE_WEAK_AND_REF_PTRS(TestFileFormat);

class TestFileFormat : public SdfFileFormat
{
public:
    SDF_FILE_FORMAT_FACTORY_ACCESS;
    TestFileFormat()
        : SdfFileFormat(TestFileFormatTokens->Id,
                        TestFileFormatTokens->Version,
                        TestFileFormatTokens->Target,
                        TestFileFormatTokens->Extension)
    {
    }
    virtual ~TestFileFormat() = default;

    bool CanRead(const std::string& resolvedPath) const override
    {
        return true;
    }

    void _Read(const SdfLayerHandle& srcLayer, const SdfLayerHandle& destLayer, const std::string& resolvedPath) const
    {
        if (!srcLayer)
        {
            return;
        }

        if (!destLayer)
        {
            return;
        }

        const std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(ArResolvedPath(resolvedPath));
        if (asset)
        {
            // The TestFileFormat expects very small files with a few relative paths so we should
            // be able to read everything into the buffer. For production-worthy SdfFileFormat plugins
            // we would probably need to do buffered reads
            std::shared_ptr<const char> buffer = asset->GetBuffer();
            size_t bufferSize = asset->GetSize();
            const std::string content(buffer.get(), bufferSize);

            const std::vector<std::string> relativePaths = TfStringTokenize(content, "\r\n");
            for (const std::string& relativePath : relativePaths)
            {
                SdfLayerRefPtr sublayer = SdfLayer::FindOrOpenRelativeToLayer(srcLayer, relativePath);
                if (sublayer)
                {
                    destLayer->TransferContent(sublayer);
                }
            }
        }
    }

    bool Read(SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const override
    {
        if (!layer)
        {
            return false;
        }

        const FileFormatArguments& args = layer->GetFileFormatArguments();
        SdfAbstractDataConstPtr data = _GetLayerData(*layer);

        SdfLayerHandle layerHandle = SdfCreateHandle(layer);

        // Create a root prim where we will store some of the attributes
        // that should be provided from OmniUsdWrapperFileFormat
        TfToken rootToken("TestRoot");
        SdfPath rootPath = SdfPath::AbsoluteRootPath().AppendChild(rootToken);
        SdfPrimSpecHandle rootSpec =
            SdfPrimSpec::New(layerHandle->GetPseudoRoot(), rootToken.GetText(), SdfSpecifier::SdfSpecifierDef, "Scope");
        if (!rootSpec)
        {
            return false;
        }

        layerHandle->SetDefaultPrim(rootToken);

        // From here where going to use the Usd API to author some UsdAttributes
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->GetRootLayer()->TransferContent(layerHandle);

        UsdPrim rootPrim = stage->GetDefaultPrim();
        UsdAttribute repoPathAttr = rootPrim.CreateAttribute(TfToken("RepositoryPath"), SdfValueTypeNames->String);
        if (!repoPathAttr)
        {
            return false;
        }
        repoPathAttr.Set(layerHandle->GetRepositoryPath());

        UsdAttribute realPathAttr = rootPrim.CreateAttribute(TfToken("RealPath"), SdfValueTypeNames->String);
        if (!realPathAttr)
        {
            return false;
        }
        realPathAttr.Set(layerHandle->GetRealPath());

        UsdAttribute resolvedPathAttr = rootPrim.CreateAttribute(TfToken("ResolvedPath"), SdfValueTypeNames->String);
        if (!resolvedPathAttr)
        {
            return false;
        }
        resolvedPathAttr.Set(resolvedPath);

        // Read the contents of the file to find relative paths
        _Read(layerHandle, stage->GetRootLayer(), resolvedPath);

        // Transfer content back to the original layer
        layerHandle->TransferContent(stage->GetRootLayer());

        // Make the layer read-only
        layerHandle->SetPermissionToSave(false);
        layerHandle->SetPermissionToEdit(false);
        return true;
    }

protected:
#if PXR_VERSION >= 2108
    bool _ShouldReadAnonymousLayers() const override
    {
        return true;
    }
#endif
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(TestFileFormat, SdfFileFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE
