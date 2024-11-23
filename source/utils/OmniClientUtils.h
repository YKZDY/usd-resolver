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

#include <OmniClient.h>

namespace std
{
template <>
struct default_delete<OmniClientUrl>
{
    default_delete() = default;
    template <class U>
    constexpr default_delete(default_delete<U>) noexcept
    {
    }
    void operator()(OmniClientUrl* p) const noexcept
    {
        omniClientFreeUrl(p);
    }
};
}; // namespace std

template <typename Callable, typename... Args>
std::string makeString(Callable function, Args... args)
{
    size_t size = 100;
    std::string str;
    str.resize(size);
    char* ptr = nullptr;
    while (ptr == nullptr)
    {
        ptr = function(args..., &str[0], &size);
        str.resize(size - 1); // -1 for the terminating null
    };
    return str;
}

inline std::string urlToString(OmniClientUrl const& url)
{
    return makeString(omniClientMakeUrl, &url);
}

inline std::string normalizeUrl(std::string const& url)
{
    return makeString(omniClientNormalizeUrl, url.c_str());
}

inline std::string resolveUrlComposed(std::string const& url)
{
    return makeString(omniClientCombineWithBaseUrl, url.c_str());
}

inline std::unique_ptr<OmniClientUrl> resolveUrl(std::string const& url)
{
    return std::unique_ptr<OmniClientUrl>(omniClientCombineWithBaseUrl2(url.c_str()));
}

inline std::unique_ptr<OmniClientUrl> parseUrl(std::string const& url)
{
    return std::unique_ptr<OmniClientUrl>(omniClientBreakUrl(url.c_str()));
}

inline bool isLocal(std::unique_ptr<OmniClientUrl> const& url)
{
    if (url->isRaw)
    {
        return true;
    }
    // scheme==nullptr means it's either a relative reference or a local file
    // this "isLocal" function assumes that you always pass an absolute URL
    if (url->scheme == nullptr || strcmp(url->scheme, "file") == 0)
    {
        return true;
    }
    return false;
}

inline bool isAnonymous(std::unique_ptr<OmniClientUrl> const& url)
{
    if (url->scheme && strcmp(url->scheme, "anon") == 0)
    {
        return true;
    }
    return false;
}

inline bool isOmniverse(std::unique_ptr<OmniClientUrl> const& url)
{
    return url->scheme && (strncmp(url->scheme, "omni", 4) == 0);
}
