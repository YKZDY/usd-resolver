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

#include "StringUtils.h"

#include <string>

inline bool isRelativePath(std::string const& path)
{
    // Absolute paths either have a colon before a slash (indicating urls or drive letters in windows)
    // or they start with a '/' (indicating absolute paths in linux or UNC prefix on windows
    // Relative paths are !Absolute paths
    if (path.empty())
    {
        return false;
    }
    if (path[0] == '/')
    {
        return false;
    }
    if (path.find(':') < path.find('/'))
    {
        return false;
    }
    return true;
}

inline bool isFileRelative(std::string const& path)
{
    return path.compare(0, 2, "./") == 0 || path.compare(0, 3, "../") == 0 || path.compare(0, 2, ".\\") == 0 ||
           path.compare(0, 3, "..\\") == 0;
}

inline bool isNormalizedPath(std::string const& path)
{
    const size_t pathLen = path.length();
    if (pathLen == 1 && path[0] == '.')
    {
        return true;
    }

    for (size_t i = 0; i < pathLen; ++i)
    {
        const char c = path[i];
        if (c == '\\' || (c == '.' && (i + 1 == pathLen || path[i + 1] == '/')))
        {
            return false;
        }
    }
    return true;
}

inline std::string fixLocalPath(std::string path)
{
#if CARB_PLATFORM_WINDOWS
    if (path.size() > 2 && path[0] == '/' && path[2] == ':')
    {
        // This is a path like "/C:/"
        // This happens with a URL like "file:/C:/something"
        // Strip the leading slash and convert slashes to backslashes
        // FIXME: BrianH rethink how we really want to do this
        path.erase(0, 1);
    }
    replaceAll(path, '/', '\\');
#endif
    return path;
}
