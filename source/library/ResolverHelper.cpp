// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "ResolverHelper.h"

#include "DebugCodes.h"
#include "MdlHelper.h"
#include "Notifications.h"
#include "utils/OmniClientUtils.h"
#include "utils/PathUtils.h"
#include "utils/PythonUtils.h"
#include "utils/StringUtils.h"
#include "utils/Time.h"

#include <carb/profiler/Profile.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>

#include <OmniClient.h>

bool ResolverHelper::CanWrite(const std::string& resolvedPath, std::string* whyNot)
{
    if (resolvedPath.empty())
    {
        return false;
    }

    struct StatContext
    {
        StatContext() : exists(false), url({}), reason(nullptr)
        {
        }
        StatContext(std::string url_) : exists(false), url(std::move(url_)), reason(nullptr)
        {
        }

        bool exists;
        std::string url;
        char const* reason;
    };

    auto fileCallback = [](void* userData, OmniClientResult result, OmniClientListEntry const* entry) noexcept
    {
        auto statContext = static_cast<StatContext*>(userData);
        if (result == eOmniClientResult_Ok)
        {
            if ((entry->flags & fOmniClientItem_CanHaveChildren) != 0)
            {
                statContext->exists = true;
                statContext->reason = "%s is a folder";
            }
            else if ((entry->flags & fOmniClientItem_IsChannel) != 0)
            {
                statContext->exists = true;
                statContext->reason = "%s is a channel";
            }
            else if ((entry->flags & (fOmniClientItem_WriteableFile | fOmniClientItem_IsOmniObject)) == 0)
            {
                statContext->exists = true;
                statContext->reason = "%s is not writeable";
            }
            else if ((entry->access & fOmniClientAccess_Write) == 0)
            {
                statContext->exists = true;
                statContext->reason = "You do not have permission to write to %s";
            }
            else
            {
                // This is fine, we have a file that exists that we can write to
                statContext->exists = true;
                statContext->reason = nullptr;
            }
        }
        else if (result == eOmniClientResult_ErrorNotFound)
        {
            // This is fine, we can write to a file that's not found
            statContext->exists = false;
            statContext->reason = nullptr;
        }
        else
        {
            statContext->exists = false;
            statContext->reason = omniClientGetResultString(result);
        }
    };

    auto folderCallback = [](void* userData, OmniClientResult result, OmniClientListEntry const* entry) noexcept
    {
        auto statContext = static_cast<StatContext*>(userData);
        if (result == eOmniClientResult_Ok)
        {
            if ((entry->flags & fOmniClientItem_CanHaveChildren) != 0)
            {
                if ((entry->access & fOmniClientAccess_Write) == 0)
                {
                    // the folder does exist but we can not write to it
                    statContext->exists = true;
                    statContext->reason = "You do not have permission to write to folder %s";
                }
                else
                {
                    // The folder does exist and we can write underneath it
                    statContext->exists = true;
                    statContext->reason = nullptr;
                }
            }
            else
            {
                statContext->exists = true;
                statContext->reason = "%s can not have children written underneath it";
            }
        }
        else if (result == eOmniClientResult_ErrorNotFound)
        {
            // This is fine, we can write to a folder that does not exist
            statContext->exists = false;
            statContext->reason = nullptr;
        }
        else
        {
            // Even in the case of a folder not existing we still need to continue checking
            // parent directories
            statContext->exists = false;
            statContext->reason = omniClientGetResultString(result);
        }
    };

    PyReleaseGil g;

    // since we will be calling omniClientStop on calls to omniClientStat
    // we can't rely on the callback to clean up the allocated memory
    std::vector<std::unique_ptr<StatContext>> statContexts;
    statContexts.push_back(std::make_unique<StatContext>(resolvedPath));

    // the first part of the process is to check if the fully resolved path can be written to
    std::vector<OmniClientRequestId> stats;
    stats.push_back(omniClientStat(resolvedPath.c_str(), statContexts.back().get(), fileCallback));

    // In the event that the fully resolved path does not exist, we need to check parent folders
    // to see if they have any permissions preventing writes. We do this by "walking up" the path section of the URL
    auto parsedUrl = parseUrl(resolvedPath);
    std::string path = parsedUrl->path;

    std::string::size_type end = path.rfind('/');
    while (end != std::string::npos && end > 0)
    {
        // build the parent folder URL making sure to include the '/' at the end
        std::string parentPath = path.substr(0, end + 1);
        parsedUrl->path = parentPath.c_str();
        std::string url = makeString(omniClientMakeUrl, parsedUrl.get());

        statContexts.push_back(std::make_unique<StatContext>(url));
        stats.push_back(omniClientStat(url.c_str(), statContexts.back().get(), folderCallback));

        // move on to the next parent folder
        end = path.rfind('/', end - 1);
    }

    // we assume that files can be written to if the path does not exist
    bool canWrite = true;
    for (size_t i = 0; i < stats.size(); ++i)
    {
        omniClientWait(stats[i]);
        if (statContexts[i]->exists || statContexts[i]->reason != nullptr)
        {
            // stop any additional requests from running since we have either:
            // 1. found the file or any parent folders that grant write access
            // 2. have a reason not to write the file
            for (size_t j = i + 1; j < stats.size(); ++j)
            {
                omniClientStop(stats[j]);
            }

            // without a reason it means that we are able to write to the fully resolved path
            // or it's parent folder
            canWrite = (statContexts[i]->reason == nullptr);
            if (!canWrite && whyNot)
            {
                *whyNot = TfStringPrintf(statContexts[i]->reason, statContexts[i]->url.c_str());
            }

            break;
        }
    }

    return canWrite;
}

std::string ResolverHelper::Resolve(const std::string& identifierStripped,
                                    std::string& url,
                                    std::string& version,
                                    std::chrono::system_clock::time_point& modifiedTime,
                                    uint64_t& size)
{
    CARB_PROFILE_ZONE(1, "ResolverHelper::Resolve %s", identifierStripped.c_str());

    SendNotification(identifierStripped.c_str(), eOmniUsdResolverEvent_Resolving, eOmniUsdResolverEventState_Started);
    auto eventFinished = eOmniUsdResolverEventState_Failure;
    CARB_SCOPE_EXIT
    {
        SendNotification(identifierStripped.c_str(), eOmniUsdResolverEvent_Resolving, eventFinished, size);
    };

    struct Context
    {
        bool found;
        std::string& url;
        std::string& version;
        std::chrono::system_clock::time_point& modifiedTime;
        uint64_t& size;
    };
    Context context = { false, url, version, modifiedTime, size };
    auto callback =
        [](void* userData, OmniClientResult result, struct OmniClientListEntry const* entry, char const* url) noexcept
    {
        auto& context = *(Context*)userData;
        if (result == eOmniClientResult_Ok)
        {
            context.found = true;
            context.url = safeString(url);
            context.version = safeString(entry->version);
            context.modifiedTime = convertFromTimeSinceUnixEpoch(std::chrono::nanoseconds(entry->modifiedTimeNs));
            context.size = entry->size;
        }
    };

    const bool isMdlIdentifier = mdl_helper::IsMdlIdentifier(identifierStripped);
    if (isMdlIdentifier)
    {
        TF_DEBUG(OMNI_USD_RESOLVER_MDL)
            .Msg("%s: Disabling base URL to resolve %s\n", TF_FUNC_NAME().c_str(), identifierStripped.c_str());

        // OMPE-16448: An MDL identifier (e.g nvidia/core_definitions.mdl) should not try to resolve against
        // the current base URL. It should only be resolved against the configured search paths.
        // Note that OmniUsdResolver::_BindContext will push the current layer's URL to the base URL stack
        // which will be used as part of client-libraries resolve process
        omniClientPushBaseUrl("");
    }


    // searchPaths are intentionally left empty here. Should the need arise to support searchPaths
    // the OmniUsdResolverContext would be the ideal place to store them and use GetCurrentContext()
    PyReleaseGil g;
    omniClientWait(omniClientResolve(identifierStripped.c_str(), {}, 0, &context, callback));

    if (isMdlIdentifier)
    {
        omniClientPopBaseUrl("");
    }

    if (!context.found)
    {
        return {};
    }

    auto parsedUrl = parseUrl(context.url);
    if (isLocal(parsedUrl))
    {
        // Local files can be accessed directly
        return fixLocalPath(safeString(parsedUrl->path));
    }

    eventFinished = eOmniUsdResolverEventState_Success;

    // context.url is bound by reference to the passed in arg url
    // returning url or context.url is one of the same
    return url;
}
