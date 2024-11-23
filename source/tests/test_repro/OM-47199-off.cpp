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
#include "UsdIncludes.h"

#include <carb/ClientUtils.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/shader.h>

#include <OmniClient.h>
#include <string>

void logCallback(char const* threadName, char const* component, OmniClientLogLevel level, char const* message) noexcept
{
    printf("%c %s %s %s\n", omniClientGetLogLevelChar(level), threadName, component, message);
}

#define TEST_NAME "OM-47199-off"

CARB_GLOBALS(TEST_NAME);

PXR_NAMESPACE_USING_DIRECTIVE;

bool testMdlStage(const std::string& testFile)
{

    ArResolver& resolver = ArGetResolver();
    UsdStageRefPtr stage = UsdStage::Open(testFile);
    if (!stage)
    {
        return false;
    }

    UsdShadeShader pbrShader = UsdShadeShader::Get(stage, SdfPath("/World/Looks/OmniPBR/Shader"));
    if (!pbrShader)
    {
        testlog::printf("Unable to get OmniPBR Shader");
        return false;
    }

    SdfAssetPath pbrAssetPath;
    if (!pbrShader.GetSourceAsset(&pbrAssetPath, TfToken("mdl")))
    {
        testlog::printf("Unable to get source asset for %s", pbrShader.GetPath().GetText());
        return false;
    }

    // Validate that anchoring does not bypass MDL builtin paths

    // In Ar 2.0, we are now responsible for creating the proper identifier for a search path as it
    // is no longer done in Sdf (layerUtils.cpp).
    // This means that when MDL bypass is off we should only anchor the MDL path if it actually
    // lives next to the anchor asset path (e.g look here first strategy). In this case, OmniPBR.mdl does not live
    // next to the anchor asset path so it should be returned as-is (OmniPBR.mdl)
    std::string mdlPath = resolver.CreateIdentifier(pbrAssetPath.GetAssetPath(), ArResolvedPath(testFile));
    if (mdlPath != pbrAssetPath.GetAssetPath())
    {
        testlog::printf("Expected mdl path %s, Actual %s", pbrAssetPath.GetAssetPath().c_str(), mdlPath.c_str());
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

    std::string mdlscenestr = test::randomUrl / "Scene.usda";
    const char* mdlscene = mdlscenestr.c_str();

    omniClientWait(omniClientCopy("TestMdlStage/Scene.usda", mdlscene, {}, {}));

    std::string mdlfilestr = test::randomUrl / "OmniSurface.mdl";
    const char* mdlfile = mdlfilestr.c_str();

    omniClientWait(omniClientCopy("TestMdlStage/OmniSurface.mdl", mdlfile, {}, {}));

    // assumes OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS = 0
    testlog::start(TEST_NAME);
    bool success = testMdlStage(mdlscenestr);
    testlog::finish(success);

    omniClientWait(omniClientDelete(test::randomUrl.c_str(), {}, {}));
    omniClientShutdown();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
