// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "utils/Trace.h"

#include <carb/CarbWindows.h>
#include <carb/ClientUtils.h>

CARB_GLOBALS("omni_usd_resolver")
CARB_TRACE_GLOBAL()

struct ScopedFramework
{
    ScopedFramework()
    {
        g_carbFramework = carb::acquireFramework(g_carbClientName);
        carb::logging::registerLoggingForClient();
        carb::assert::registerAssertForClient();
    }
    ~ScopedFramework()
    {
        if (carb::isFrameworkValid())
        {
            carb::assert::deregisterAssertForClient();
            carb::logging::deregisterLoggingForClient();
        }
    }
} scopedFramework;
