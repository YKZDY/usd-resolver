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
#include "utils/OmniClientUtils.h"
#include "utils/StringUtils.h"

#include <carb/ClientUtils.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layerUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/shader.h>

#include <OmniClient.h>
#include <OmniUsdResolver.h>
#include <random>
#include <string>


void logCallback(char const* threadName, char const* component, OmniClientLogLevel level, char const* message) noexcept
{
    printf("%c %s %s %s\n", omniClientGetLogLevelChar(level), threadName, component, message);
}

#define TEST_NAME "OM-47199"

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
        testlog::printf("Unable to get OmniPBR Shader\n");
        return false;
    }

    UsdShadeShader surfaceShader = UsdShadeShader::Get(stage, SdfPath("/World/Looks/OmniSurface/Shader"));
    if (!surfaceShader)
    {
        testlog::printf("Unable to get OmniSurface Shader\n");
        return false;
    }

    UsdShadeShader auxShader = UsdShadeShader::Get(stage, SdfPath("/World/Looks/OmniSurface/Shader_01"));
    if (!auxShader)
    {
        testlog::printf("Unable to get OmniSurface Shader_01\n");
        return false;
    }

    UsdShadeShader surfaceShader2 = UsdShadeShader::Get(stage, SdfPath("/World/Looks/OmniSurface/Shader_02"));
    if (!surfaceShader2)
    {
        testlog::printf("Unable to get Surface Shader\n");
        return false;
    }

    SdfAssetPath pbrAssetPath;
    if (!pbrShader.GetSourceAsset(&pbrAssetPath, TfToken("mdl")))
    {
        testlog::printf("Unable to get source asset for %s\n", pbrShader.GetPath().GetText());
        return false;
    }

    if (pbrAssetPath.GetAssetPath() != "OmniPBR.mdl")
    {
        testlog::printf("Invalid Source Asset Path %s for %s\n", pbrAssetPath.GetAssetPath().c_str(),
                        pbrShader.GetPath().GetText());
        return false;
    }

    if (!pbrAssetPath.GetResolvedPath().empty())
    {
        testlog::printf("Invalid Source Asset Resolved Path for %s\n", pbrShader.GetPath().GetText());
        return false;
    }

    // Validate that the Resolver is bypassing MDL builtin paths correctly
    std::string anchoredMdlPath = resolver.CreateIdentifier(pbrAssetPath.GetAssetPath(), ArResolvedPath(testFile));
    if (anchoredMdlPath != pbrAssetPath.GetAssetPath())
    {
        testlog::printf(
            "Expected anchored path %s, Actual %s\n", pbrAssetPath.GetAssetPath().c_str(), anchoredMdlPath.c_str());
        return false;
    }

    SdfAssetPath surfaceAssetPath;
    if (!surfaceShader.GetSourceAsset(&surfaceAssetPath, TfToken("mdl")))
    {
        testlog::printf("Unable to get source asset for %s\n", surfaceShader.GetPath().GetText());
        return false;
    }

    if (surfaceAssetPath.GetAssetPath() != "OmniSurface.mdl")
    {
        testlog::printf("Invalid Source Asset Path %s for %s\n", surfaceAssetPath.GetAssetPath().c_str(),
                        surfaceShader.GetPath().GetText());
        return false;
    }

    // MDL builtin paths that live next to the Layer they're referenced from should be resolved to an empty path
    if (!surfaceAssetPath.GetResolvedPath().empty())
    {
        testlog::printf("Invalid Source Asset Resolved Path for %s\n", surfaceShader.GetPath().GetText());
        return false;
    }

    const std::string anchoredSurfacePath =
        resolver.CreateIdentifier(surfaceAssetPath.GetAssetPath(), ArResolvedPath(testFile));
    if (anchoredSurfacePath != surfaceAssetPath.GetAssetPath())
    {
        testlog::printf("Expected Anchored Path %s, Actual %s\n", surfaceAssetPath.GetAssetPath().c_str(),
                        anchoredSurfacePath.c_str());
        return false;
    }

    SdfAssetPath auxAssetPath;
    if (!auxShader.GetSourceAsset(&auxAssetPath, TfToken("mdl")))
    {
        testlog::printf("Unable to get source asset for %s\n", auxShader.GetPath().GetText());
        return false;
    }

    if (auxAssetPath.GetAssetPath() != "nvidia/aux_definitions.mdl")
    {
        testlog::printf("Invalid Source Asset Path %s for %s\n", auxAssetPath.GetAssetPath().c_str(),
                        auxShader.GetPath().GetText());
        return false;
    }

    // Any MDL builtin paths that do not live next to the layer they're referenced from should be resolved to an empty
    // path
    // XXX: If the mdl compiler was loaded this should be the file path that the compiler loads
    if (!auxAssetPath.GetResolvedPath().empty())
    {
        testlog::printf("Invalid Source Asset Resolved Path for %s\n", auxShader.GetPath().GetText());
        return false;
    }

    // With MDL builtin paths SdfComputeAssetPathRelativeToLayer should still return the MDL builtin path
    // and not an anchored path. Even MDL builtin paths that do live next to a referencing USD file should
    // still return the builtin path
    const std::string computedPbrAssetPath =
        SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), pbrAssetPath.GetAssetPath());

    if (computedPbrAssetPath != pbrAssetPath.GetAssetPath())
    {
        testlog::printf("Invalid SdfComputeAssetPathRelativeToLayer %s\n", computedPbrAssetPath.c_str());
        return false;
    }

    const std::string computedSurfaceAssetPath =
        SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), surfaceAssetPath.GetAssetPath());

    if (computedSurfaceAssetPath != surfaceAssetPath.GetAssetPath())
    {
        testlog::printf("Invalid SdfComputeAssetPathRelativeToLayer %s\n", computedSurfaceAssetPath.c_str());
        return false;
    }

    const std::string computedAuxAssetPath =
        SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), auxAssetPath.GetAssetPath());

    if (computedAuxAssetPath != auxAssetPath.GetAssetPath())
    {
        testlog::printf("Invalid SdfComputeAssetPathRelativeToLayer %s\n", computedAuxAssetPath.c_str());
        return false;
    }

    SdfAssetPath surface2AssetPath;
    if (!surfaceShader2.GetSourceAsset(&surface2AssetPath, TfToken("mdl")))
    {
        testlog::printf("Unable to get source asset for %s\n", surfaceShader2.GetPath().GetText());
        return false;
    }

    // With MDL paths that are not prefixed with ./ or ../ but are not builtin paths should still be treated
    // as relative / search paths and resolve to an actual path
    if (surface2AssetPath.GetAssetPath() != "Surface.mdl")
    {
        testlog::printf("Invalid Source Asset Path %s for %s\n", surface2AssetPath.GetAssetPath().c_str(),
                        surfaceShader2.GetPath().GetText());
        return false;
    }

    if (surface2AssetPath.GetResolvedPath().empty())
    {
        testlog::printf("Invalid Source Asset Resolved Path for %s\n", surfaceShader2.GetPath().GetText());
        return false;
    }

    return true;
}

bool testMdlStageLocal(const std::string& testFile)
{
    ArResolver& resolver = ArGetResolver();
    UsdStageRefPtr stage = UsdStage::Open(testFile);
    if (!stage)
    {
        testlog::printf("Unable to open %s\n", testFile.c_str());
        return false;
    }

    // MDL builtin paths that exist locally next to a Usd file should not be anchored
    std::string computedSurfacePath = SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), "OmniSurface.mdl");
    if (computedSurfacePath != "OmniSurface.mdl")
    {
        testlog::printf("Expected MDL path not to be anchored, actual %s\n", computedSurfacePath.c_str());
        return false;
    }

    // The MDL builtin path should resolve to an empty path
    std::string resolvedSurfacePath = resolver.Resolve(computedSurfacePath);
    if (!resolvedSurfacePath.empty())
    {
        testlog::printf("Expected resolved path for %s to be empty, actual %s\n", computedSurfacePath.c_str(),
                        resolvedSurfacePath.c_str());
        return false;
    }

    // MDL paths that are explicitly relative should also still anchor / resolve correctly
    computedSurfacePath = SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), "./OmniSurface.mdl");
    if (computedSurfacePath == "OmniSurface.mdl" || computedSurfacePath == "./OmniSurface.mdl")
    {
        testlog::printf("Expected a path anchored to %s, actual %s\n", stage->GetRootLayer()->GetIdentifier().c_str(),
                        computedSurfacePath.c_str());
        return false;
    }

    resolvedSurfacePath = resolver.Resolve(computedSurfacePath);
    if (!TfPathExists(resolvedSurfacePath))
    {
        testlog::printf("Expected %s to exist\n", resolvedSurfacePath.c_str());
        return false;
    }

    // file-relative MDL builtin path that do not exist locally should be bypassed
    std::string computedPbrPath = SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), "OmniPBR.mdl");
    if (computedPbrPath != "OmniPBR.mdl")
    {
        testlog::printf("Expected OmniPBR.mdl, acutal %s\n", computedPbrPath.c_str());
        return false;
    }

    std::string resolvedPbrPath = resolver.Resolve(computedPbrPath);
    if (!resolvedPbrPath.empty())
    {
        testlog::printf("Expected OmniPBR.mdl to resolve to an empty path, actual %s\n", resolvedPbrPath.c_str());
        return false;
    }

    // MDL paths that are not builtins but exist locally next to a Usd file should be anchored
    std::string computedSurface2Path = SdfComputeAssetPathRelativeToLayer(stage->GetRootLayer(), "Surface.mdl");
    if (computedSurface2Path == "Surface.mdl")
    {
        testlog::printf("Expected MDL path to be anchored, actual %s\n", computedSurface2Path.c_str());
        return false;
    }

    // The MDL path should resolve to a path
    std::string resolvedSurface2Path = resolver.Resolve(computedSurface2Path);
    if (resolvedSurface2Path.empty())
    {
        testlog::printf("Expected resolved path for %s to not be empty, actual %s\n", computedSurface2Path.c_str(),
                        resolvedSurface2Path.c_str());
        return false;
    }

    if (!TfPathExists(resolvedSurface2Path))
    {
        testlog::printf("Expected %s to exist\n", resolvedSurface2Path.c_str());
        return false;
    }

    return true;
}

bool testMdlSearchPaths(const std::string& testFile, const std::string& searchPath)
{
    // Verify that without context binding and no search paths the MDL path is not resolved
    ArResolver& resolver = ArGetResolver();
    ArResolvedPath resolvedPath = resolver.Resolve("OmniSurface.mdl");
    if (!resolvedPath.empty())
    {
        testlog::printf("Expected OmniSurface.mdl to not resolve, actual %s\n", resolvedPath.GetPathString().c_str());
        return false;
    }

    resolvedPath = resolver.Resolve("OmniPBR.mdl");
    if (!resolvedPath.empty())
    {
        testlog::printf("Expected OmniPBR.mdl to not resolve, actual %s\n", resolvedPath.GetPathString().c_str());
        return false;
    }

    // Verify that when a context is bound from a stage a MDL builtin will not resolve local to that stage
    // make sure that we do have a MDL next to the stage matching the MDL builtin
    std::string anchoredUrl = makeString(omniClientCombineUrls, testFile.c_str(), "OmniSurface.mdl");
    resolvedPath = resolver.Resolve(anchoredUrl.c_str());
    if (resolvedPath.empty())
    {
        testlog::printf("Expected %s to resolve\n", anchoredUrl.c_str());
        return false;
    }

    // make sure the MDL builtin does not resolve even when it exists relative to the stage
    ArResolverContext context = ArGetResolver().CreateDefaultContextForAsset(testFile);
    ArResolverContextBinder binder(context);

    resolvedPath = resolver.Resolve("OmniSurface.mdl");
    if (!resolvedPath.empty())
    {
        testlog::printf("Expected OmniSurface.mdl to not resolve, actual %s\n", resolvedPath.GetPathString().c_str());
        return false;
    }

    // make sure other paths (non MDL builtin) resolve relative to the stage
    resolvedPath = resolver.Resolve("Surface.mdl");
    if (resolvedPath.empty())
    {
        testlog::printf("Expected Surface.mdl to resolve\n");
        return false;
    }

    std::string expectedPath = makeString(omniClientCombineUrls, testFile.c_str(), "Surface.mdl");
    if (resolvedPath.GetPathString() != expectedPath)
    {
        testlog::printf("Expected Surface.mdl to resolve to %s, actual %s\n", expectedPath.c_str(),
                        resolvedPath.GetPathString().c_str());
        return false;
    }

    // configure the MDL search path so we can resolve the MDL builtin paths
    omniClientAddDefaultSearchPath(searchPath.c_str());

    resolvedPath = resolver.Resolve("OmniSurface.mdl");
    if (resolvedPath.empty())
    {
        testlog::printf("Expected OmniSurface.mdl to resolve\n");
        return false;
    }

    expectedPath = makeString(omniClientCombineUrls, searchPath.c_str(), "OmniSurface.mdl");
    if (resolvedPath.GetPathString() != expectedPath)
    {
        testlog::printf("Expected OmniSurface.mdl to resolve to %s, actual %s\n", expectedPath.c_str(),
                        resolvedPath.GetPathString().c_str());
        return false;
    }

    omniClientRemoveDefaultSearchPath(searchPath.c_str());

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

    // Force the env var to be set so this test can be run directly
    carb::extras::EnvironmentVariable::setValue("OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS", "1");

    omniClientSetLogCallback(logCallback);
    omniClientSetLogLevel(eOmniClientLogLevel_Warning);

    if (!omniClientInitialize(kOmniClientVersion))
    {
        return EXIT_FAILURE;
    }

    std::string mdlscenestr = test::randomUrl / "Scene.usda";
    const char* mdlscene = mdlscenestr.c_str();

    omniClientWait(omniClientCopy("TestMdlStage/Scene.usda", mdlscene, {}, {}));

    std::string surfacemdlstr = test::randomUrl / "OmniSurface.mdl";
    const char* surfacemdl = surfacemdlstr.c_str();

    omniClientWait(omniClientCopy("TestMdlStage/OmniSurface.mdl", surfacemdl, {}, {}));

    std::string surface2mdlstr = test::randomUrl / "Surface.mdl";
    const char* surface2mdl = surface2mdlstr.c_str();

    omniClientWait(omniClientCopy("TestMdlStage/Surface.mdl", surface2mdl, {}, {}));

    // assumes OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS = 1
    char const* builtins[] = { "OmniSurface.mdl", "OmniPBR.mdl", "nvidia/aux_definitions.mdl" };
    omniUsdResolverSetMdlBuiltins(builtins, CARB_COUNTOF(builtins));

    // Generate a random directory to mock a MDL builtin search path
    std::string mdlsearchpath =
        test::randomUrl / std::to_string(rand()) / std::string("mdl") / std::string("core") / std::string();
    omniClientWait(omniClientCopy("mdl/core", mdlsearchpath.c_str(), {}, {}));

    testlog::start(TEST_NAME);
    bool success = testMdlStage(mdlscenestr);

    success &= testMdlStageLocal("TestMdlStage/Scene.usda");

    success &= testMdlSearchPaths(mdlscenestr, mdlsearchpath);
    testlog::finish(success);

    omniClientWait(omniClientDelete(test::randomUrl.c_str(), {}, {}));
    omniClientShutdown();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
