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

#include <cstddef>
#include <ctype.h>
#include <functional>
#include <utility>

namespace details
{
template <typename CharType, bool IgnoreCase>
struct Sanitize;
template <typename CharType>
struct Sanitize<CharType, true>
{
    static constexpr CharType sanitize(CharType a) noexcept
    {
        return (a == '\\') ? '/' : ((a >= 'A' && a <= 'Z') ? a - 'A' + 'a' : a);
    }
};
template <typename CharType>
struct Sanitize<CharType, false>
{
    static constexpr CharType sanitize(CharType a) noexcept
    {
        return (a == '\\') ? '/' : a;
    }
};
} // namespace details

inline char sanitizePathChar(char c, bool ignoreCase = false)
{
    return ignoreCase ? details::Sanitize<char, true>::sanitize(c) : details::Sanitize<char, false>::sanitize(c);
}

template <typename CharType, bool IgnoreCase>
struct PathTraits : public std::char_traits<CharType>
{
    static constexpr CharType sanitize(CharType a) noexcept
    {
        return details::Sanitize<CharType, IgnoreCase>::sanitize(a);
    }

    static constexpr bool eq(CharType a, CharType b) noexcept
    {
        return sanitize(a) == sanitize(b);
    }

    static constexpr bool lt(CharType a, CharType b) noexcept
    {
        return sanitize(a) < sanitize(b);
    }

    static int compare(const CharType* s1, const CharType* s2, std::size_t count)
    {
        for (size_t i = 0; i < count; i++)
        {
            auto a = sanitize(s1[i]);
            auto b = sanitize(s2[i]);
            if (a != b)
            {
                return a < b ? -1 : 1;
            }
        }
        return 0;
    }

    const CharType* find(const CharType* p, std::size_t count, const CharType& ch)
    {
        auto a = sanitize(ch);
        for (size_t i = 0; i < count; i++)
        {
            if (sanitize(p[i]) == ch)
            {
                return p + i;
            }
        }
        return nullptr;
    }
};

using WindowsPathString = std::basic_string<char, PathTraits<char, true>>;

namespace std
{

template <>
struct hash<WindowsPathString>
{
    size_t operator()(WindowsPathString const& str) const noexcept
    {
        size_t hash = 0;
        for (char c : str)
        {
            hash = hash * 37 + WindowsPathString::traits_type::sanitize(c);
        }
        return hash;
    }
};

} // namespace std

#ifdef _WIN32
inline std::pair<std::string, std::string> splitWindowsPath(std::string suffix)
{
    // See 2.2.57 of MS-DTYP for more information.
    // https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/62e862f4-2a51-452e-8eeb-dc4ff5ee33cc
    //
    // Split the path into a "prefix" part and a "suffix" part.
    // In each of the following examples, the "prefix" comes before "path"
    // C:\path
    // C:
    // \\?\C:\path
    // \\?\C:
    // \\server\share\path
    // \\server\share
    // \\?\UNC\server\share\path

    std::string prefix;
    replaceAll(suffix, '\\', '/');

    if (suffix.length() >= 2 && isalpha(suffix[0]) && suffix[1] == ':')
    {
        prefix = suffix.substr(0, 2);
        suffix = suffix.substr(2);
    }
    else if (suffix.length() >= 2 && suffix[0] == '/' && suffix[1] == '/')
    {
        size_t numSlashes;
        // In all cases the prefix is the first 4/6 slashes and the suffix is the rest.
        if (suffix.compare(0, 8, "//?/UNC/") == 0)
        {
            // Expect server/share/ so find the 2nd slash after the UNC prefix
            numSlashes = 4;
        }
        else
        {
            // Expect //?/C:/ or expect //server/share/ so find the 2nd slash after the "//" prefix
            numSlashes = 2;
        }
        size_t offset = 2;
        for (size_t i = 0; i < numSlashes; i++)
        {
            auto slash = suffix.find('/', offset);
            if (slash == std::string::npos)
            {
                // This can happen in a valid case where there is no trailing path. E.g.
                // '//?/C:', '//server/name', etc. So we assume this is what is going on
                // in this case.
                offset = suffix.length();
                break;
            }
            offset = slash + 1;
        }
        prefix = suffix.substr(0, offset);
        suffix = suffix.substr(offset);
    }
    else
    {
        // If the base path doesn't start with C: or \\ then it's not an absolute path
    }
    return { std::move(prefix), std::move(suffix) };
}
#endif
