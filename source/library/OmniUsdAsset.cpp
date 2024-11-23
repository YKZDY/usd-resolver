// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "OmniUsdAsset.h"

#include "DebugCodes.h"
#include "Notifications.h"
#include "utils/PathUtils.h"
#include "utils/PythonUtils.h"

#include <pxr/base/arch/errno.h>

#include <OmniClient.h>

PXR_NAMESPACE_USING_DIRECTIVE

std::shared_ptr<OmniUsdAsset> OmniUsdAsset::Open(const ArResolvedPath& resolvedPath)
{
    TF_DEBUG(OMNI_USD_RESOLVER_ASSET).Msg("%s: %s\n", TF_FUNC_NAME().c_str(), resolvedPath.GetPathString().c_str());

    SendNotification(
        resolvedPath.GetPathString().c_str(), eOmniUsdResolverEvent_Reading, eOmniUsdResolverEventState_Started);
    auto eventFinished = eOmniUsdResolverEventState_Failure;
    uint64_t fileSize = 0;
    CARB_SCOPE_EXIT
    {
        SendNotification(resolvedPath.GetPathString().c_str(), eOmniUsdResolverEvent_Reading, eventFinished, fileSize);
    };

    OmniUsdReadableData inputData;
    inputData.url = resolvedPath.GetPathString();

    PyReleaseGil g;

    // A local file is being used here for a few reasons:
    // 1. to serve as a caching mechanism so subsequent reads are fast and efficient.
    // 2. reduce traffic and latency with Nucleus
    // 3. allows for memory-mapped files which is usually optimized at the OS level.
    // 4. the internals of Usd Crate files still really want a file to memory-map or read. Reading directly
    //    from an ArAsset requires an environment variable, USDC_USE_ASSET, to be turned on. During testing with
    //    this environment variable I ran into multiple crashes. Since USDC_USE_ASSET is disabled by default
    //    the choice was made to just use omniClientGetLocalFile
    std::string filePath;
    inputData.clientRequestId =
        omniClientGetLocalFile(inputData.url.c_str(), true, &filePath,
                               [](void* userData, OmniClientResult result, char const* localFilePath) noexcept
                               {
                                   if (result == eOmniClientResult_Ok)
                                   {
                                       *static_cast<std::string*>(userData) = localFilePath;
                                   }
                               });
    omniClientWait(inputData.clientRequestId);

    if (filePath.empty())
    {
        TF_DEBUG(OMNI_USD_RESOLVER_ASSET)
            .Msg("%s: unable to open %s. Could not download local file\n", TF_FUNC_NAME().c_str(), inputData.url.c_str());
        return nullptr;
    }

    inputData.localFile = fixLocalPath(filePath);
    inputData.file = ArchOpenFile(inputData.localFile.c_str(), "rb");
    if (inputData.file)
    {
        eventFinished = eOmniUsdResolverEventState_Success;
        auto usdAsset = std::make_shared<OmniUsdAsset>(std::move(inputData));
        fileSize = usdAsset->GetSize();
        return usdAsset;
    }

    return nullptr;
}

OmniUsdAsset::OmniUsdAsset(OmniUsdReadableData&& inputData) : _inputData(std::move(inputData))
{
    TF_DEBUG(OMNI_USD_RESOLVER_ASSET).Msg("%s: %s\n", TF_FUNC_NAME().c_str(), _inputData.url.c_str());

    if (!_inputData.file)
    {
        TF_CODING_ERROR("Invalid handle to local file");
    }
}

OmniUsdAsset::~OmniUsdAsset()
{
    if (_inputData.file)
    {
        fclose(_inputData.file);
        _inputData.file = nullptr;
    }
    if (_inputData.clientRequestId)
    {
        omniClientStop(_inputData.clientRequestId);
        _inputData.clientRequestId = 0;
    }
}

size_t OmniUsdAsset::GetSize() const
{
    return ArchGetFileLength(_inputData.file);
}

std::shared_ptr<const char> OmniUsdAsset::GetBuffer() const
{
    ArchConstFileMapping mapping = ArchMapFileReadOnly(_inputData.file);
    if (!mapping)
    {
        TF_DEBUG(OMNI_USD_RESOLVER_ASSET)
            .Msg("%s: Unable to create memory mapping of %s (%s)\n", TF_FUNC_NAME().c_str(), _inputData.url.c_str(),
                 _inputData.localFile.c_str());
        return nullptr;
    }

    struct _Deleter
    {
        explicit _Deleter(ArchConstFileMapping&& mapping) : _mapping(new ArchConstFileMapping(std::move(mapping)))
        {
        }

        void operator()(const char* b)
        {
            _mapping.reset();
        }

        std::shared_ptr<ArchConstFileMapping> _mapping;
    };

    const char* buffer = mapping.get();
    return { buffer, _Deleter(std::move(mapping)) };
}

size_t OmniUsdAsset::Read(void* out, size_t count, size_t offset) const
{
    int64_t numRead = ArchPRead(_inputData.file, out, count, offset);
    if (numRead == -1)
    {
        TF_RUNTIME_ERROR("Error occurred reading local file for %s: %s", _inputData.url.c_str(), ArchStrerror().c_str());
        return 0;
    }
    return numRead;
}

std::pair<FILE*, size_t> OmniUsdAsset::GetFileUnsafe() const
{
    return std::make_pair(_inputData.file, 0);
}
