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

#include <carb/Defines.h>

#include <carb/extras/Path.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

inline std::string safeString(char const* s)
{
    return s ? std::string(s) : std::string();
}

inline void replaceAll(std::string& str, char find, char replace)
{
    std::replace(str.begin(), str.end(), find, replace);
}

inline void replaceAll(std::wstring& str, wchar_t find, wchar_t replace)
{
    std::replace(str.begin(), str.end(), find, replace);
}

inline std::string operator/(std::string const& a, std::string const& b)
{
#if CARB_PLATFORM_WINDOWS
    bool aHasSlash = !a.empty() && (a.back() == '/' || a.back() == '\\');
    bool bHasSlash = !b.empty() && (b.front() == '/' || b.front() == '\\');
#else
    bool aHasSlash = !a.empty() && a.back() == '/';
    bool bHasSlash = !b.empty() && b.front() == '/';
#endif
    std::string result;

    result.reserve(a.length() + b.length() + 1);
    result.append(a, 0, a.size() - (aHasSlash ? 1 : 0));
    // On Windows a & b may disagree in form, so we just assume /.
    result += '/';
    result.append(b, (size_t)(bHasSlash ? 1 : 0), b.size() - (size_t)(bHasSlash ? 1 : 0));
    return result;
}

// These extra operator/ are needed to resolve ambiguity since "char const*" can be implicitly
// converted to both std::string and carb::extras::Path
inline std::string operator/(char const* a, std::string const& b)
{
    return std::string(a) / b;
}

inline std::string operator/(std::string const& a, char const* b)
{
    return a / std::string(b);
}

inline carb::extras::Path operator/(carb::extras::Path const& a, char const* b)
{
    return a / carb::extras::Path(b);
}

inline carb::extras::Path operator/(carb::extras::Path const& a, std::string const& b)
{
    return a / carb::extras::Path(b);
}

inline std::string concat(std::ostringstream& ss)
{
    return ss.str();
}

template <class Type1, class... Types>
std::string concat(std::ostringstream& ss, Type1 arg1, Types... args)
{
    ss << arg1;
    return concat(ss, args...);
}

template <class... Types>
std::string concat(Types... args)
{
    std::ostringstream ss;
    return concat(ss, args...);
}


inline std::wstring safeString(wchar_t const* s)
{
    return s ? std::wstring(s) : std::wstring();
}

inline std::wstring concat(std::wostringstream& ss)
{
    return ss.str();
}

template <class Type1, class... Types>
std::wstring concat(std::wostringstream& ss, Type1 arg1, Types... args)
{
    ss << arg1;
    return concat(ss, args...);
}

template <class... Types>
std::wstring concat(std::wstring const& ws, Types... args)
{
    std::wostringstream ss;
    return concat(ss, ws, args...);
}

inline std::string ltrim(std::string const& str, char ch)
{
    auto pos = str.find_first_not_of(ch);
    return pos == std::string::npos ? "" : str.substr(pos);
}

inline std::string ltrim(std::string const& str, const char* sep)
{
    auto pos = str.find_first_not_of(sep);
    return pos == std::string::npos ? "" : str.substr(pos);
}

inline std::string rtrim(std::string const& str, char ch)
{
    auto pos = str.find_last_not_of(ch);
    return (pos == std::string::npos) ? "" : str.substr(0, pos + 1);
}

inline std::string rtrim(std::string const& str, const char* sep)
{
    auto pos = str.find_last_not_of(sep);
    return (pos == std::string::npos) ? "" : str.substr(0, pos + 1);
}

inline std::string trim(std::string const& str, char ch)
{
    return rtrim(ltrim(str, ch), ch);
}

inline std::string trim(std::string const& str, const char* sep)
{
    return rtrim(ltrim(str, sep), sep);
}

inline void str_tolower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](char c) { return (char)tolower(c); });
}

inline std::vector<std::string> split(std::string const& str, std::string const& delimiter)
{
    std::vector<std::string> res;
    size_t start = 0;
    size_t end = 0;

    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        res.push_back(str.substr(start, end - start));
        start = end + delimiter.size();
    }

    res.push_back(str.substr(start));

    return res;
}

inline std::pair<std::string, std::string> splitPath(std::string const& path)
{
    std::string parentPath(path);
    std::string relativePath;

    if (parentPath.length() > 1 && parentPath.back() == '/')
    {
        // So "/foo" and "/foo/" are treated the same
        parentPath.pop_back();
    }

    auto lastSlash = parentPath.rfind('/');
    if (lastSlash == std::string::npos)
    {
        relativePath = std::move(parentPath);
        parentPath.clear();
    }
    else
    {
        relativePath = parentPath.substr(lastSlash + 1);
        parentPath.resize(lastSlash + 1);
    }

    return std::make_pair(std::move(parentPath), std::move(relativePath));
}

inline std::string ensureSlash(std::string const& path)
{
    std::string pathWithSlash = path;
    if (pathWithSlash.back() != '/')
    {
        pathWithSlash.push_back('/');
    }
    return pathWithSlash;
}

// Convert 0-9a-f into 0-16
// Returns -1 if it's an invalid character
inline int hexDecode(char h)
{
    if (h >= '0' && h <= '9')
    {
        return h - '0';
    }
    if (h >= 'a' && h <= 'f')
    {
        return 10 + h - 'a';
    }
    if (h >= 'A' && h <= 'F')
    {
        return 10 + h - 'A';
    }
    return -1;
}

// Convert 0-16 into 0-9A-F
inline char hexEncode(char i)
{
    if (i < 10)
    {
        return '0' + i;
    }
    else
    {
        return 'A' + (i - 10);
    }
}

inline char* returnCopy(std::string const& str, char* buffer, size_t* bufferSize)
{
    size_t strSize = str.size() + 1;
    if (*bufferSize < strSize)
    {
        *bufferSize = strSize;
        return nullptr;
    }
    else
    {
        *bufferSize = strSize;
        memcpy(buffer, str.c_str(), str.size());
        buffer[str.size()] = 0;
        return buffer;
    }
}
