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
#include "TestLog.h"

#include <carb/ClientUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/cube.h>

#include <OmniClient.h>
#include <iostream>
#include <string>

void logCallback(char const* threadName, char const* component, OmniClientLogLevel level, char const* message) noexcept
{
    printf("%c %s %s %s\n", omniClientGetLogLevelChar(level), threadName, component, message);
}

#define TEST_NAME "OM-49309"

CARB_GLOBALS(TEST_NAME);

PXR_NAMESPACE_USING_DIRECTIVE;

bool testFileFormat(const std::string& testFile)
{
    UsdStageRefPtr stage = UsdStage::Open(testFile);
    if (!stage)
    {
        testlog::printf("failed to open stage %s\n", testFile.c_str());
        return false;
    }

    SdfLayerRefPtr layer = stage->GetRootLayer();
    if (!layer)
    {
        testlog::print("failed to get root layer\n");
        return false;
    }

    // Make sure we can still get the original URL
    const std::string& repoPath = layer->GetRepositoryPath();
    if (repoPath != testFile)
    {
        testlog::printf("%s does not match expected %s\n", repoPath.c_str(), testFile.c_str());
        return false;
    }

    // Verify that we were able to load relative layers for the SdfFileFormat plugins
    UsdPrim boxPrim = stage->GetPrimAtPath(SdfPath("/pCube1"));
    if (!boxPrim)
    {
        testlog::print("could not get boxPrim /pCube1\n");
        return false;
    }

    return true;
}

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

    std::string testfilestr = test::randomUrl / "test.testff";
    const char* testfile = testfilestr.c_str();

    omniClientWait(omniClientCopy("test.testff", testfile, {}, {}));

    std::string testboxstr = test::randomUrl / "box.usda";
    const char* testbox = testboxstr.c_str();

    omniClientWait(omniClientCopy("box.usda", testbox, {}, {}));

    testlog::start(TEST_NAME);
    bool success = testFileFormat(testfile);
    testlog::finish(success);

    omniClientWait(omniClientDelete(test::randomUrl.c_str(), {}, {}));
    omniClientShutdown();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
