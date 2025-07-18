// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "RegisterPlugin.h"
#include "TestEnvironment.h"

#include <carb/ClientUtils.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>

#include <OmniClient.h>
#include <string>

void logCallback(char const* threadName, char const* component, OmniClientLogLevel level, char const* message) noexcept
{
    printf("%c %s %s %s\n", omniClientGetLogLevelChar(level), threadName, component, message);
}

#define TEST_NAME "OM-27691"

CARB_GLOBALS(TEST_NAME);

int main(int argc, char** argv)
{
    carb::acquireFrameworkAndRegisterBuiltins();

    if (!test::setupEnvironment(TEST_NAME))
    {
        return EXIT_FAILURE;
    }
    if (!test::registerPlugin())
    {
        return EXIT_FAILURE;
    }

    omniClientSetLogCallback(logCallback);
    omniClientSetLogLevel(eOmniClientLogLevel_Warning);

    if (!omniClientInitialize(kOmniClientVersion))
    {
        return EXIT_FAILURE;
    }

    std::string testfilestr = test::randomUrl / "World.usd";
    const char* testfile = testfilestr.c_str();

    omniClientWait(omniClientCopy("World.usda", testfile, {}, {}));

    omniClientDelete(test::randomUrl.c_str(), {}, {});
    omniClientSignOut(test::randomUrl.c_str());

    omniClientShutdown();

    return EXIT_SUCCESS;
}
