// SPDX-FileCopyrightText: Copyright (c) 2021-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.
#include <Defines.h>

// clang-format off
#include "OmniUsdResolver.h"
#include "OmniUsdWrapperData.h"
#include "OmniUsdWrapperFileFormat.h"

#include "Checkpoint.h"
#include "Notifications.h"
#include "utils/OmniClientUtils.h"
#include "utils/PathUtils.h"
#include "utils/PythonUtils.h"
#include "utils/StringUtils.h"
// clang-format on

#include <pxr/base/arch/library.h>

#include <mutex>

#pragma warning(push)
#pragma warning(disable : 4003) // not enough actual parameters for macro

TF_DEFINE_PUBLIC_TOKENS(OmniUsdWrapperFileFormatTokens, OMNI_USD_FILE_FORMAT_TOKENS);

#pragma warning(pop)

namespace
{
static const std::string kArgExtension{ "_wrapper_extension" };
static const std::string kArgRealPath{ "_wrapper_realpath" };
static const std::string kArgAssetInfoVersion{ "_wrapper_assetinfo_version" };
static const std::string kArgAssetInfoAssetName{ "_wrapper_assetinfo_assetname" };
static const std::string kArgAssetInfoRepoPath{ "_wrapper_assetinfo_repopath" };
} // namespace

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(OmniUsdWrapperFileFormat, SdfFileFormat);
}

OmniUsdWrapperFileFormat::OmniUsdWrapperFileFormat()
    : SdfFileFormat(OmniUsdWrapperFileFormatTokens->Id,
                    OmniUsdWrapperFileFormatTokens->Version,
                    OmniUsdWrapperFileFormatTokens->Target,
                    OmniUsdWrapperFileFormatTokens->Id)
{
    _dummyFileFormat = TfCreateRefPtr(new DummyFileFormat);
}

OmniUsdWrapperFileFormat::~OmniUsdWrapperFileFormat()
{
}

static std::string _GetRealFormatExt(std::string const& realPath)
{
    auto dot1It = std::find(realPath.crbegin(), realPath.crend(), '.');
    if (dot1It == realPath.crend())
    {
        return std::string();
    }

    const auto& wrapperExt = OmniUsdWrapperFileFormatTokens->Id.GetString();
    if (realPath.compare(std::distance(realPath.cbegin(), dot1It.base()), wrapperExt.length(), wrapperExt) != 0)
    {
        return std::string(dot1It.base(), realPath.cend());
    }

    auto dot2It = std::find(std::next(dot1It), realPath.crend(), '.');
    if (dot2It == realPath.crend())
    {
        return std::string();
    }
    return std::string(dot2It.base(), std::prev(dot1It.base()));
}

SdfLayer* OmniUsdWrapperFileFormat::_InstantiateNewLayer(const SdfFileFormatConstPtr& fileFormat,
                                                         const std::string& identifier,
                                                         const std::string& realPath,
                                                         const ArAssetInfo& assetInfo,
                                                         const FileFormatArguments& args) const
{
    if (realPath.empty())
    {
        // This happens with "OpenAsAnonymous"
        // We won't know the real path until "Read"
        return SdfFileFormat::_InstantiateNewLayer(fileFormat, identifier, realPath, assetInfo, args);
    }
    else
    {
        // We need to know the extension inside InitData so we can forward to the correct format
        auto parsedUrl = parseUrl(realPath);
        FileFormatArguments args2 = args;
        args2.emplace(kArgExtension, _GetRealFormatExt(parsedUrl->path));

        // Some FileFormat plugins may need the original data passed in from SdfLayer
        // Forward this data along via FileFormatArguments which will be passed into InitData
        args2.emplace(kArgRealPath, realPath);

        // ArAssetInfo might also be needed in the FileFormat plugin so send that along to
        // NOTE: resolverInfo could also be helpful here but requires more effort since it is a VtValue
        // Skipping for now until we have a good use case
        args2.emplace(kArgAssetInfoVersion, assetInfo.version);
        args2.emplace(kArgAssetInfoAssetName, assetInfo.assetName);
        args2.emplace(kArgAssetInfoRepoPath, assetInfo.repoPath);

        return SdfFileFormat::_InstantiateNewLayer(fileFormat, identifier, realPath, assetInfo, args2);
    }
}

SdfAbstractDataRefPtr OmniUsdWrapperFileFormat::InitData(const FileFormatArguments& args) const
{
    SdfFileFormatConstPtr realFormat;

    auto extensionIt = args.find(kArgExtension);
    if (extensionIt != args.end())
    {
        realFormat = SdfFileFormat::FindByExtension(extensionIt->second, args);
    }
    if (!realFormat || realFormat.PointsTo(*this))
    {
        realFormat = _dummyFileFormat;
    }

    // Grab all the data from args that was stashed away in _InstantiateNewLayer.
    // For any args not found the default value will be used, the only exception being
    // identifier.
    auto realPathIter = args.find(kArgRealPath);
    const std::string realPath = realPathIter != args.end() ? realPathIter->second : std::string();

    ArAssetInfo assetInfo{};
    auto versionIter = args.find(kArgAssetInfoVersion);
    assetInfo.version = versionIter != args.end() ? versionIter->second : std::string();

    auto assetNameIter = args.find(kArgAssetInfoAssetName);
    assetInfo.assetName = assetNameIter != args.end() ? assetNameIter->second : std::string();

    // We need to stash the original identifier inside of ArAssetInfo to not cause conflicts
    // with the SdfLayerRegistry since this layer is already using that identifier.
    // However, relative paths should still work since Ar 1.0 will use layer->GetRepositoryPath()
    // when calling ArGetResolver().AnchorRelativePath() if it's not empty
    // For Ar 2.0, repoPath is deprecated but the changes there *should* provide better support for URLs
    // and we can hopefully remove this
    auto repoPathIter = args.find(kArgAssetInfoRepoPath);
    assetInfo.repoPath = repoPathIter != args.end() ? repoPathIter->second : std::string();

    auto realLayer = _InstantiateNewLayer(realFormat, std::string("anon:%p"), realPath, assetInfo, args);
    auto wrappedLayer = TfCreateRefPtr(realLayer);
    auto wrappedData = TfConst_cast<SdfAbstractDataRefPtr>(_GetLayerData(*realLayer));
    return TfCreateRefPtr(new OmniUsdWrapperData(wrappedLayer, wrappedData));
}

bool OmniUsdWrapperFileFormat::CanRead(std::string const& file) const
{
    // Fixme?
    return true;
}

SdfFileFormatConstPtr OmniUsdWrapperFileFormat::GetFileFormat(SdfLayer const* wrappedLayer, std::string const& path) const
{
    auto wrappedFileFormat = wrappedLayer->GetFileFormat();
    if (wrappedFileFormat == _dummyFileFormat)
    {
        auto extension = _GetRealFormatExt(path);
        if (extension.empty())
        {
            return {};
        }
        wrappedFileFormat = SdfFileFormat::FindByExtension(extension, wrappedLayer->GetFileFormatArguments());
    }
    return wrappedFileFormat;
}

bool OmniUsdWrapperFileFormat::Read(SdfLayer* layer, std::string const& resolvedPath, bool metadataOnly) const
{
    SendNotification(layer->GetIdentifier().c_str(), eOmniUsdResolverEvent_Reading, eOmniUsdResolverEventState_Started);
    auto eventFinished = eOmniUsdResolverEventState_Failure;
    uint64_t fileSize = 0;
    CARB_SCOPE_EXIT
    {
        SendNotification(layer->GetIdentifier().c_str(), eOmniUsdResolverEvent_Reading, eventFinished, fileSize);
    };

    if (resolvedPath.empty())
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::Read: Failed to resolve path");
        return false;
    }
    auto layerData = TfConst_cast<SdfAbstractDataRefPtr>(_GetLayerData(*layer));
    auto wrapperData = TfDynamic_cast<OmniUsdWrapperDataRefPtr>(layerData);
    if (!wrapperData)
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::Read: Failed to get layer wrapper data");
        return false;
    }

    std::string filePath;

    PyReleaseGil g;
    omniClientWait(omniClientStat(resolvedPath.c_str(), &fileSize,
                                  [](void* userData, OmniClientResult result, OmniClientListEntry const* entry) noexcept
                                  {
                                      if (result == eOmniClientResult_Ok && entry)
                                      {
                                          *(uint64_t*)userData = entry->size;
                                      }
                                  }));
    auto requestId = omniClientGetLocalFile(resolvedPath.c_str(), true, &filePath,
                                            [](void* userData, OmniClientResult result, char const* localFilePath) noexcept
                                            {
                                                if (result == eOmniClientResult_Ok)
                                                {
                                                    *(std::string*)userData = localFilePath;
                                                }
                                            });
    omniClientWait(requestId);

    // Don't stop. This will prevent Hub from garbage collecting while this application is running.
    // This is fixed in Ar2, but the Ar1 API is not flexible enough to support this.
    // omniClientStop(requestId);

    if (filePath.empty())
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::Read: Failed to fetch file");
        return false;
    }

    filePath = fixLocalPath(filePath);

    auto wrappedLayer = wrapperData->GetWrappedLayer();
    if (!wrappedLayer)
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::Read: Failed to get wrapped layer");
        return false;
    }

    auto wrappedLayerPtr = get_pointer(wrappedLayer);

    auto wrappedFileFormat = GetFileFormat(wrappedLayerPtr, filePath);
    if (!wrappedFileFormat)
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::Read: Failed to get file format for %s", filePath.c_str());
        return false;
    }

    bool retVal = wrappedFileFormat->Read(wrappedLayerPtr, filePath, metadataOnly);

    // When reading into a layer, reset wrapperData->_wrappedData because the layer may have read into a different
    // underlying data object. This also triggers layer change notifications during reloads (OM-45532).
    auto wrappedData = TfConst_cast<SdfAbstractDataRefPtr>(_GetLayerData(*wrappedLayer));

    SdfAbstractDataRefPtr newWrpperData = TfCreateRefPtr(new OmniUsdWrapperData(wrappedLayer, wrappedData));
    _SetLayerData(layer, newWrpperData);

    eventFinished = eOmniUsdResolverEventState_Success;

    return retVal;
}

bool OmniUsdWrapperFileFormat::WriteToFile(const SdfLayer& layer,
                                           std::string const& realPath,
                                           std::string const& comment,
                                           const FileFormatArguments& args) const
{
    SendNotification(layer.GetIdentifier().c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Started);
    auto eventFinished = eOmniUsdResolverEventState_Failure;
    CARB_SCOPE_EXIT
    {
        SendNotification(layer.GetIdentifier().c_str(), eOmniUsdResolverEvent_Writing, eventFinished);
    };

    PyReleaseGil g;

    const SdfLayer* wrappedLayer = &layer;

    auto wrapperData =
        TfDynamic_cast<OmniUsdWrapperDataRefPtr>(TfConst_cast<SdfAbstractDataRefPtr>(_GetLayerData(layer)));
    // Note: wrapperData is NULL when calling SdfLayer("box.usda")->Export("omniverse://etc...")
    // In this case, we just set the wrappedLayer to the input layer
    if (wrapperData)
    {
        wrappedLayer = get_pointer(wrapperData->GetWrappedLayer());
        if (!wrappedLayer)
        {
            OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::WriteToFile: Failed to get wrapped layer");
            return false;
        }
    }

    auto parsedUrl = parseUrl(realPath);

    auto wrappedFileFormat = GetFileFormat(wrappedLayer, parsedUrl->path);
    if (!wrappedFileFormat)
    {
        OMNI_LOG_ERROR("OmniUsdWrapperFileFormat::WriteToFile: Failed to get file format for %s", parsedUrl->path);
        return false;
    }

    std::string remoteUri;
    bool saving;

    if (layer.GetRealPath() == realPath)
    {
        // Case 1: Calling Create or Save
        remoteUri = layer.GetRepositoryPath();
        saving = true;

        auto remoteParsed = parseUrl(remoteUri);

        if (isLocal(remoteParsed))
        {
            // Exporting to a local location
            if (wrappedFileFormat->WriteToFile(*wrappedLayer, remoteParsed->path, comment, args))
            {
                eventFinished = eOmniUsdResolverEventState_Success;
                return true;
            }
            return false;
        }
    }
    else
    {
        // Case 2: Exporting
        OMNI_LOG_INFO("Exporting %s to %s", layer.GetIdentifier().c_str(), realPath.c_str());

        // When exporting, use the file format corresponding to the destination format
        wrappedFileFormat = SdfFileFormat::FindByExtension(_GetRealFormatExt(parsedUrl->path), args);

        if (isLocal(parsedUrl))
        {
            // Exporting to a local location
            if (wrappedFileFormat->WriteToFile(*wrappedLayer, parsedUrl->path, comment, args))
            {
                eventFinished = eOmniUsdResolverEventState_Success;
                return true;
            }
            return false;
        }

        // Exporting to non-local path (omniverse, test, etc.)
        remoteUri = realPath;
        saving = false;
    }

    auto resolvedUri = resolveUrl(remoteUri);
    auto extension = TfGetExtension(resolvedUri->path);
    if (!extension.empty())
    {
        // TfGetExtension unfortunately doesn't include the extension
        // and ArchMakeTmpFileName doesn't either
        extension = '.' + extension;
    }

    // Write from layer to a local temp file
    std::string localTempPath = ArchMakeTmpFileName("omni-usd-resolver", extension);

    if (!wrappedFileFormat->WriteToFile(*wrappedLayer, localTempPath, comment, args))
    {
        return false;
    }

    // FIXME: This should use omniClientMakeFileUrl
    auto localTempUrl = concat("file:", localTempPath);

    struct Context
    {
        bool copied = false;
        bool deleted = false;
    } context;

    std::string checkpointMessage = GetCheckpointMessage();

    // move from localTempPath to remoteUri
    omniClientWait(omniClientMove(
        localTempUrl.c_str(), remoteUri.c_str(), &context,
        [](void* userData, OmniClientResult result, bool copied) noexcept
        {
            auto& context = *(Context*)userData;
            context.deleted = (result == eOmniClientResult_Ok);
            if (copied)
            {
                context.copied = true;
            }
            else
            {
                context.copied = context.deleted;
            }
        },
        eOmniClientCopy_Overwrite, checkpointMessage.c_str()));

    if (!context.deleted)
    {
        // delete the temp file even if the copy failed
        omniClientWait(omniClientDelete(localTempUrl.c_str(), nullptr, nullptr));
    }

    if (context.copied)
    {
        eventFinished = eOmniUsdResolverEventState_Success;
        return true;
    }

    return false;
}

bool OmniUsdWrapperFileFormat::ReadFromString(SdfLayer* layer, std::string const& str) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->ReadFromString(layer, str);
}

bool OmniUsdWrapperFileFormat::WriteToString(const SdfLayer& layer, std::string* str, std::string const& comment) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->WriteToString(layer, str, comment);
}

bool OmniUsdWrapperFileFormat::WriteToStream(const SdfSpecHandle& spec, std::ostream& out, size_t indent) const
{
    return SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id)->WriteToStream(spec, out, indent);
}
