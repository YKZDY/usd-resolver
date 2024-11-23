// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#pragma once

#include "utils/StringUtils.h"

#include <iostream>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace testlog
{
std::vector<std::string> _messages;
std::mutex _mutex;
bool _verbose = false;

void start(std::string const& testName)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_verbose)
    {
        ::printf("Testing %s...\n", testName.c_str());
    }
    else
    {
        ::printf("Testing %s...\n", testName.c_str());
    }
}

void finish(bool success)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (success)
    {
        ::printf("Success\n");
    }
    else
    {
        ::printf("Failed!\n");
        if (!_messages.empty())
        {
            ::printf("----------------\n");
            for (auto& message : _messages)
            {
                ::printf("    %s", message.c_str());
            }
            ::printf("----------------\n");
        }
        CARB_ASSERT(0);
    }
    _messages.clear();
}

void print(const char* msg)
{
    std::unique_lock<std::mutex> lock(_mutex);
    if (_verbose)
    {
        ::printf("    %s", msg);
    }
    else
    {
        _messages.push_back(safeString(msg));
    }
}

void printf(const char* fmt, ...)
{
    std::unique_lock<std::mutex> lock(_mutex);
    va_list args;
    va_list args2;
    va_start(args, fmt);
    va_copy(args2, args);
    char buffer[4000];
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (len < 0)
    {
        if (_verbose)
        {
            ::printf("sprintf(%s) failed!\n", fmt);
        }
        else
        {
            _messages.push_back(concat("sprintf(", fmt, ") failed!\n"));
        }
    }
    else if (size_t(len) < sizeof(buffer))
    {
        if (_verbose)
        {
            ::printf("    %s", buffer);
        }
        else
        {
            _messages.push_back(buffer);
        }
    }
    else
    {
        // static buffer not large enough, I need to dynamically allocate a bigger one
        char* dynbuffer = new char[len + 1];
        vsnprintf(dynbuffer, len + 1, fmt, args2);
        if (_verbose)
        {
            ::printf("    %s", dynbuffer);
        }
        else
        {
            _messages.push_back(dynbuffer);
        }
        delete[] dynbuffer;
    }
    va_end(args2);
    va_end(args);
}

void fail(char const* msg)
{
    print(msg);
    finish(false);
}

} // namespace testlog
