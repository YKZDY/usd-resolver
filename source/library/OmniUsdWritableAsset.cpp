// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "OmniUsdWritableAsset.h"

#include "Defines.h"

#include "Checkpoint.h"
#include "DebugCodes.h"
#include "Notifications.h"
#include "OmniUsdResolver.h"
#include "UsdIncludes.h"
#include "utils/OmniClientUtils.h"
#include "utils/PythonUtils.h"
#include "utils/StringUtils.h"

#include <pxr/base/arch/errno.h>

#include <OmniClient.h>

PXR_NAMESPACE_USING_DIRECTIVE

std::shared_ptr<OmniUsdWritableAsset> OmniUsdWritableAsset::Open(const ArResolvedPath& resolvedPath,
                                                                 ArResolver::WriteMode writeMode)
{
    if (resolvedPath.empty())
    {
        return {};
    }

    SendNotification(
        resolvedPath.GetPathString().c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Started);

    OmniUsdWritableData outputData;
    outputData.url = resolvedPath.GetPathString();

    auto parsedUrl = parseUrl(resolvedPath.GetPathString());
    const std::string ext = TfGetExtension(parsedUrl->path);

    // XXX: Should ArchMakeTmpFile be used over ArchMakeTmpFileName for safety?
    static const std::string kPrefix{ "omni-usd-resolver" };
    outputData.file = ArchMakeTmpFileName(kPrefix, TfStringPrintf(".%s", ext.c_str()));

    TfErrorMark m;
    if (writeMode == ArResolver::WriteMode::Update)
    {
        // There is not an easy way around this. We can not append to a file in Nucleus
        // so we have to copy the file to a temporary location and then write to it
        // In most cases the file will already be local and not require a download
        struct Context
        {
            bool copied;
            std::string error;
        };
        Context context{ false, std::string() };
        omniClientWait(omniClientCopyFile(outputData.url.c_str(), outputData.file.c_str(), &context,
                                          [](void* userData, OmniClientResult result) noexcept
                                          {
                                              auto& context = *(Context*)userData;
                                              if (result == eOmniClientResult_Ok)
                                              {
                                                  context.copied = true;
                                              }
                                              else
                                              {
                                                  context.error = safeString(omniClientGetResultString(result));
                                              }
                                          }));
        if (context.copied)
        {
            outputData.safeFile = TfSafeOutputFile::Update(outputData.file);
        }
        else
        {
            SendNotification(outputData.url.c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Failure);
            TF_RUNTIME_ERROR("Unable to update %s at %s: %s", outputData.url.c_str(), outputData.file.c_str(),
                             context.error.c_str());
            return {};
        }
    }
    else if (writeMode == ArResolver::WriteMode::Replace)
    {
        // Not much to do if we are just replacing the file
        outputData.safeFile = TfSafeOutputFile::Replace(outputData.file);
    }

    if (!m.IsClean())
    {
        return {};
    }

    return std::make_shared<OmniUsdWritableAsset>(std::move(outputData));
}

OmniUsdWritableAsset::OmniUsdWritableAsset(OmniUsdWritableData&& outputData) : _outputData(std::move(outputData))
{
    if (!_outputData.safeFile.Get())
    {
        TF_CODING_ERROR("Invalid asset file to write to for '%s'", _outputData.url.c_str());
    }
}

bool OmniUsdWritableAsset::Close()
{
    // make a valid file url for the asset file that was written to
    char urlBuffer[ARCH_PATH_MAX];
    size_t urlBufferSize = sizeof(urlBuffer);
    auto fileUrl = omniClientMakeFileUrl(_outputData.file.c_str(), urlBuffer, &urlBufferSize);

    // close the temporary file that we were writing to
    TfErrorMark m;
    _outputData.safeFile.Close();
    if (!m.IsClean())
    {
        SendNotification(_outputData.url.c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Failure);
        TF_DEBUG(OMNI_USD_RESOLVER_ASSET).Msg("%s: Unable to close %s\n", TF_FUNC_NAME().c_str(), _outputData.file.c_str());
        return false;
    }

    struct Context
    {
        bool copied = false;
        bool deleted = false;
    } context;

    std::string checkpointMessage = GetCheckpointMessage();

    PyReleaseGil g;

    // XXX: When moving the content from the temporary file to a Nucleus URL
    // the modification time is determined on the Nucleus server. At the moment,
    // Nucleus only provides precision down to the nearest second. See comments in
    // OmniUsdResolver::_GetModificationTimestamp on how this impacts things such as SdfLayer::Reload

    // move all the content from the temporary file to output file URL
    omniClientWait(omniClientMove(
        fileUrl, _outputData.url.c_str(), &context,
        [](void* userData, OmniClientResult result, bool copied) noexcept
        {
            auto& context = *static_cast<Context*>(userData);
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
        TF_DEBUG(OMNI_USD_RESOLVER_ASSET)
            .Msg("%s: copy of '%s' failed for '%s'\n", TF_FUNC_NAME().c_str(), fileUrl, _outputData.url.c_str());

        // delete the temporary file even if the copy failed
        omniClientWait(omniClientDelete(fileUrl, nullptr, nullptr));
    }

    TF_DEBUG(OMNI_USD_RESOLVER_ASSET)
        .Msg("%s: %s -> %s\n", TF_FUNC_NAME().c_str(), _outputData.url.c_str(), TfStringify(context.copied).c_str());

    if (context.copied)
    {
        SendNotification(_outputData.url.c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Success);
        return true;
    }
    else
    {
        SendNotification(_outputData.url.c_str(), eOmniUsdResolverEvent_Writing, eOmniUsdResolverEventState_Failure);
        return false;
    }
}
size_t OmniUsdWritableAsset::Write(const void* buffer, size_t count, size_t offset)
{
    int64_t bytesWritten = ArchPWrite(_outputData.safeFile.Get(), buffer, count, offset);
    if (bytesWritten == -1)
    {
        TF_RUNTIME_ERROR("Error writing temporary file for '%s': %s", _outputData.url.c_str(), ArchStrerror().c_str());

        return 0;
    }

    return bytesWritten;
}
