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
#include "TestHelpers.h"
#include "TestLog.h"
#include "UsdIncludes.h"
#include "utils/OmniClientUtils.h"
#include "utils/StringUtils.h"
#include "utils/Trace.h"

#include <carb/ClientUtils.h>
#include <carb/extras/MemoryUsage.h>
#include <pxr/base/tf/diagnosticMgr.h>
#include <pxr/usd/ar/packageUtils.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/shader.h>

#include <OmniClient.h>
#include <OmniUsdResolver.h>
#include <chrono>
#include <cstdlib>
#include <map>
#include <random>
#include <thread>

/*
 * This contains a number of unit tests operating on NON live layers
 */

CARB_GLOBALS("omni_usd_test_resolver")
CARB_TRACE_GLOBAL()

using test_function = std::function<int()>;
struct test_info
{
    test_function function;
    std::string description;
};
using testmap_type = std::map<std::string, test_info>;
testmap_type testmap;

struct test_init
{
    test_init(const char* name, test_function function, const char* description)
    {
        testmap.emplace(name, test_info{ function, std::string(description) });
    }
};

#define TEST(name, description)                                                                                        \
    int test_##name();                                                                                                 \
    test_init test_##name##_init(#name, test_##name, description);                                                     \
    int test_##name()

///////////////////////////////////////////////////////////////////////////////////////////
// Some simple utilities

std::string GenerateTestUrl()
{
    std::string testFile = test::randomUrl;
    testFile.append(std::to_string(rand()));
    testFile.append(".usd");
    return testFile;
}

SdfLayerRefPtr CreateTestLayer()
{
    auto testFile = GenerateTestUrl();

    auto testLayer = SdfLayer::CreateNew(testFile);
    if (!testLayer)
    {
        testlog::printf("Failed to create %s\n", testFile.c_str());
    }
    return testLayer;
}

SdfAttributeSpecHandle CreateSphere(SdfLayerRefPtr& testLayer)
{
    // Create a sphere
    auto sphere = SdfPrimSpec::New(testLayer->GetPseudoRoot(), "sphere", SdfSpecifierDef, "Sphere");
    TF_AXIOM(sphere);
    auto radius = SdfAttributeSpec::New(sphere, "radius", SdfValueTypeNames->Double);
    TF_AXIOM(radius);
    testLayer->SetField(radius->GetPath(), SdfFieldKeys->Default, 1.0);
    TF_AXIOM(testLayer->GetField(radius->GetPath(), SdfFieldKeys->Default) == VtValue(1.0));
    testLayer->Save();
    return radius;
}

// Verify that the sphere prim in the given layer matches the
// given radius.
static bool VerifyRadius(SdfLayerRefPtr layer, const double trueRadius)
{
    double radius = layer->GetFieldAs<double>(PXR_NS::SdfPath("/sphere.radius"), SdfFieldKeys->Default, 0.0);

    if (radius != trueRadius)
    {
        testlog::printf("Wrong radius: expecting %f got %f\n", trueRadius, radius);
        return false;
    }

    return true;
}

// Verify that the sphere prim in the given file matches the
// given radius.
static bool VerifyRadius(std::string const& testFile, const double trueRadius)
{
    SdfLayerRefPtr layer = SdfLayer::FindOrOpen(testFile);

    if (!layer)
    {
        testlog::printf("Failed to load %s layer\n", testFile.c_str());
        return false;
    }

    return VerifyRadius(layer, trueRadius);
}

///////////////////////////////////////////////////////////////////////////////////////////

TEST(createLayer, "Simple test that just creates a layer")
{
    auto testFile = GenerateTestUrl();

    auto testLayer = SdfLayer::CreateNew(testFile);
    if (!testLayer)
    {
        testlog::printf("Failed to create %s\n", testFile.c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(exportLayer, "Simple test that exports a local file to omniverse")
{
    auto box = SdfLayer::FindOrOpen("box.usda");
    if (!box)
    {
        testlog::printf("Failed to load box.usda\n");
        return EXIT_FAILURE;
    }

    if (dynamic_cast<ArDefaultResolver*>(&(ArGetResolver())))
    {
        testlog::printf("OmniUsdRsolver was not installed\n");
        return EXIT_FAILURE;
    }

    auto timestamp = ArGetResolver().GetModificationTimestamp("box.usda", {});
    if (!timestamp.IsValid())
    {
        testlog::printf("Failed to get modification timestemp for box.usda\n");
        return EXIT_FAILURE;
    }

    timestamp = ArGetResolver().GetModificationTimestamp("box.usda:SDF_FORMAT_ARGS:target=usd", {});
    if (!timestamp.IsValid())
    {
        testlog::printf("Failed to get modification timestemp for box.usda:SDF_FORMAT_ARGS:target=usd\n");
        return EXIT_FAILURE;
    }

    auto testFile = GenerateTestUrl();

    if (!box->Export(testFile))
    {
        testlog::printf("Failed to export box.usda to omniverse\n");
        return EXIT_FAILURE;
    }

    auto box2 = SdfLayer::FindOrOpen(testFile);
    if (!box2)
    {
        testlog::printf("Failed to load %s after export\n", testFile.c_str());
        return EXIT_FAILURE;
    }
    std::set<SdfPath> boxPaths;
    box->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath& path) { boxPaths.insert(path); });
    std::set<SdfPath> box2Paths;
    box2->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath& path) { box2Paths.insert(path); });
    if (boxPaths != box2Paths)
    {
        testlog::printf("Layers not the same after export\n");
        testlog::printf("  Source contains the following specs which are not in Dest:\n");
        for (auto&& boxIt : boxPaths)
        {
            if (box2Paths.find(boxIt) == box2Paths.end())
            {
                testlog::printf("    %s\n", boxIt.GetText());
            }
        }
        testlog::printf("  Dest contains the following specs which are not in Source:\n");
        for (auto&& box2It : box2Paths)
        {
            if (boxPaths.find(box2It) == boxPaths.end())
            {
                testlog::printf("    %s\n", box2It.GetText());
            }
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// This is how Kit implements "Save As"
TEST(saveLocal, "Simple test that opens a local file and saves it locally")
{
    std::string layerIdentifier = "box.usda";
    std::string newLayerPath = "_temp/box.usd";

    auto layer = PXR_NS::SdfLayer::FindOrOpen(layerIdentifier);
    if (!layer)
    {
        testlog::printf("Could not open %s\n", layerIdentifier.c_str());
        return EXIT_FAILURE;
    }
    {
        auto newLayer = PXR_NS::SdfLayer::CreateNew(newLayerPath);
        if (!newLayer)
        {
            testlog::printf("Could not create new layer %s\n", newLayerPath.c_str());
            return EXIT_FAILURE;
        }
        newLayer->SetPermissionToEdit(true);
        newLayer->SetPermissionToSave(true);
        newLayer->TransferContent(layer);
        if (!newLayer->Save())
        {
            testlog::printf("Failed to save new layer %s\n", newLayerPath.c_str());
            return EXIT_FAILURE;
        }
    }
    {
        auto newLayer = PXR_NS::SdfLayer::FindOrOpen(newLayerPath);
        if (!newLayer)
        {
            testlog::printf("Could not open new layer %s\n", newLayerPath.c_str());
            return EXIT_FAILURE;
        }

        bool fail = false;
        layer->Traverse(PXR_NS::SdfPath::AbsoluteRootPath(),
                        [&](const SdfPath& path)
                        {
                            auto newObj = newLayer->GetObjectAtPath(path);
                            if (!newObj)
                            {
                                testlog::printf("Could not find object %s in new layer\n", path.GetText());
                                fail = true;
                                return;
                            }
                            auto oldObj = layer->GetObjectAtPath(path);
                            for (auto field : oldObj->ListFields())
                            {
                                auto oldValue = oldObj->GetField(field);
                                auto newValue = newObj->GetField(field);
                                if (oldValue != newValue)
                                {
                                    testlog::printf("Field %s on prim %s in new layer does not match old layer\n",
                                                    field.GetText(), path.GetText());
                                    fail = true;
                                    return;
                                }
                            }
                        });
        if (fail)
        {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

TEST(createIdentifier, "Simple test for creating identifiers from anchoring paths")
{
    PXR_NS::ArResolver& resolver = PXR_NS::ArGetResolver();

    // Create the ArDefaultResolver to compare the results when dealing with normal file paths
    const PXR_NS::TfType defaultResolverType = TfType::Find<ArDefaultResolver>();
    auto defaultResolverPtr = PXR_NS::ArCreateResolver(defaultResolverType);
    if (!defaultResolverPtr)
    {
        testlog::print("Failed to create ArDefaultResolver\n");
        return EXIT_FAILURE;
    }

    PXR_NS::ArResolver& defaultResolver = *defaultResolverPtr;

    std::string assetPath = "./box.usda";
    std::string anchor;
    std::string expected = "box.usda";

    const std::string cwd = makeString(omniClientCombineWithBaseUrl, ".");

    auto assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    // an empty anchor for a relative asset path should result in a normalized asset path
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create asset identifier for %s with empty anchor. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // verify that the default resolver creates the asset identifier without an anchor the same way
    expected = defaultResolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (assetIdentifier != expected)
    {
        testlog::printf(
            "Failed to verify asset identifier for %s with empty anchor matched ArDefaultResolver. Expected: %s, Actual: %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // search path without configured paths to search
    anchor = {};
    assetPath = "box.usda";
    expected = "box.usda";

    assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    // an empty anchor for a search path should return the asset path as-is
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create asset identifier for %s with empty anchor. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // verify that the default resolver creates the asset identifier without an anchor the same way
    expected = defaultResolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (assetIdentifier != expected)
    {
        testlog::printf(
            "Failed to verify asset identifier for %s with empty anchor matched ArDefaultResolver. Expected: %s, Actual: %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // check that a relative anchor asset path is treated the same as an relative anchor asset path
    anchor = "relative/path.usda";
    assetPath = "./box.usda";
    expected = "box.usda";

    assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    // an empty anchor for a search path should return the asset path as-is
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create asset identifier for %s with relative anchor. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    expected = defaultResolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (assetIdentifier != expected)
    {
        testlog::printf(
            "Failed to verify asset identifier for %s with relative anchor matched ArDefaultResolver. Expected: %s, Actual: %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // check that absolute URLs with an empty anchor asset path results in a normalized URL
    anchor = {};
    assetPath = "omniverse://sandbox.ov.nvidia.com/./path/test.usda";
    expected = "omniverse://sandbox.ov.nvidia.com/path/test.usda";

    assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (!PXR_NS::TfStringStartsWith(assetIdentifier, anchor))
    {
        expected = PXR_NS::TfStringCatPaths(anchor, assetPath);
        testlog::printf("Failed to create anchored asset identifier from %s. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(createIdentifierSearchPaths, "Simple test for creating identifiers for search-path like assets")
{
    PXR_NS::ArResolver& resolver = PXR_NS::ArGetResolver();

    // Create the ArDefaultResolver to compare the results when dealing with normal file paths
    const PXR_NS::TfType defaultResolverType = TfType::Find<ArDefaultResolver>();
    auto defaultResolverPtr = PXR_NS::ArCreateResolver(defaultResolverType);
    if (!defaultResolverPtr)
    {
        testlog::print("Failed to create ArDefaultResolver\n");
        return EXIT_FAILURE;
    }

    PXR_NS::ArResolver& defaultResolver = *defaultResolverPtr;

    const std::string cwd = makeString(omniClientCombineWithBaseUrl, ".");

    // search path with configured paths to search
    std::string assetPath = "Root.usda";
    std::string searchPath = makeString(omniClientCombineUrls, cwd.c_str(), "TestStage/");

    // check that absolute URLs with an empty anchor asset path results in a normalized URL
    std::string anchor = {};
    std::string expected = assetPath;

    // Search Paths are handled differently in Ar 1.0 and not required
    // for Ar 2.0 implementations. Since our Ar 2.0 implementation of ArResolver
    // supports search paths we need to make sure that CreateIdentifier
    // properly handles them
    omniClientAddDefaultSearchPath(searchPath.c_str());

    // also check that a non-empty anchor asset path will result in the search path being returned as-is
    std::string assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create asset identifier from search path %s. Expected %s, Actual %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    omniClientRemoveDefaultSearchPath(searchPath.c_str());

    {
        // verify similar behavior with the default resolver
        PXR_NS::ArDefaultResolverContext context(std::vector<std::string>{ searchPath });
        PXR_NS::ArResolverContextBinder contextBinder(defaultResolverPtr.get(), context);

        expected = defaultResolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));
        if (assetIdentifier != normalizeUrl(expected))
        {
            testlog::printf(
                "Failed to verify asset identifier from search path %s matched ArDefaultResolver. Expected: %s, Actual %s\n",
                assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
            return EXIT_FAILURE;
        }
    }

    // check that a search path that lives next-to / under the anchor results in an anchored asset path
    anchor = searchPath;
    assetIdentifier = resolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (!PXR_NS::TfStringStartsWith(assetIdentifier, anchor))
    {
        testlog::printf("Failed to create anchored asset identifier from %s. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), PXR_NS::TfStringCatPaths(anchor, assetPath).c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // verify similar behavior with the default resolver
    expected = defaultResolver.CreateIdentifier(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != normalizeUrl(expected))
    {
        testlog::printf(
            "Failed to verify asset identifier from search path %s matched ArDefaultResolver. Expected: %s, Actual %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(createIdentifierForNewAsset, "Simple test for creating identifiers for new assets")
{
    PXR_NS::ArResolver& resolver = PXR_NS::ArGetResolver();

    // Create the ArDefaultResolver to compare the results when dealing with normal file paths
    const PXR_NS::TfType defaultResolverType = TfType::Find<ArDefaultResolver>();
    auto defaultResolverPtr = PXR_NS::ArCreateResolver(defaultResolverType);
    if (!defaultResolverPtr)
    {
        testlog::print("Failed to create ArDefaultResolver\n");
        return EXIT_FAILURE;
    }

    PXR_NS::ArResolver& defaultResolver = *defaultResolverPtr;

    std::string anchor;

    const std::string cwd = makeString(omniClientCombineWithBaseUrl, ".");

    std::string expected = makeString(omniClientCombineUrls, cwd.c_str(), "test.usda");
    std::string assetPath = "./test.usda";
    std::string assetIdentifier = resolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));

    // check that a relative path with an empty anchor is based off the current working directory
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create a new asset identifier for %s. Expected: %s, Actual: %s\n", assetPath.c_str(),
                        expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // verify that the ArDefaultResolver creates the same identifier
    expected = defaultResolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (assetIdentifier != normalizeUrl(expected))
    {
        testlog::printf(
            "Failed to verify a new asset identifier for %s matched ArDefaultResolver. Expected: %s, Actual: %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    std::string base = makeString(omniClientCombineUrls, cwd.c_str(), "TestTmp/");
    expected = makeString(omniClientCombineUrls, base.c_str(), "test.usda");

    // set up a different base URL to use when an empty anchor asset path is provided
    omniClientPushBaseUrl(base.c_str());

    // check that explicitly set base URLs are used for an anchor when an empty anchor asset path is provided
    assetPath = "./test.usda";
    assetIdentifier = resolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create a new asset identifier for %s with empty anchor. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // check that explicity set base URLs are used for an anchor when a relative anchor asset path is provided
    assetPath = "./test.usda";
    anchor = "relative/anchor/";
    assetIdentifier = resolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create a new asset identifier for %s with relative anchor. Expected: %s, Actual: %s\n",
                        assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    omniClientPopBaseUrl(base.c_str());

    // check that specifying an explicit anchor asset path that the new asset identifier is properly anchored
    assetPath = "./test.usda";
    anchor = base;
    expected = makeString(omniClientCombineUrls, base.c_str(), "test.usda");

    assetIdentifier = resolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create a new asset identifier for %s. Expected: %s, Actual: %s\n", assetPath.c_str(),
                        expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // verify that the ArDefaultResolver creates the same identifier
    expected = defaultResolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));

    if (assetIdentifier != normalizeUrl(expected))
    {
        testlog::printf(
            "Failed to verify a new asset identifier for %s matched ArDefaultResolver. Expected: %s, Actual: %s\n",
            assetPath.c_str(), expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    // check that specifying an absolute asset path without an anchor results in a normalized URL
    anchor = {};
    assetPath = "omniverse://sandbox.ov.nvidia.com/./path/test.usda";
    expected = "omniverse://sandbox.ov.nvidia.com/path/test.usda";

    assetIdentifier = resolver.CreateIdentifierForNewAsset(assetPath, PXR_NS::ArResolvedPath(anchor));
    if (assetIdentifier != expected)
    {
        testlog::printf("Failed to create a new asset identifier for %s. Expected: %s, Actual: %s\n", assetPath.c_str(),
                        expected.c_str(), assetIdentifier.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(createPrim, "Simple test that just creates a stage and defines a prim")
{
    auto testFile = GenerateTestUrl();

    auto testStage = PXR_NS::UsdStage::CreateNew(testFile);
    if (!testStage)
    {
        testlog::printf("Failed to create %s\n", testFile.c_str());
        return EXIT_FAILURE;
    }

    auto testCube = testStage->DefinePrim(PXR_NS::SdfPath("/Cube"), PXR_NS::TfToken("Cube"));
    if (!testCube)
    {
        testlog::printf("Failed to create cube\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(comments, "Test saving and restoring comments")
{
    static const char* kComment = "This is pCube1!";
    auto testUrl = GenerateTestUrl();
    {
        auto layer = SdfLayer::FindOrOpen("box.usda");
        if (!layer)
        {
            return EXIT_FAILURE;
        }
        auto pCube1 = layer->GetPrimAtPath(SdfPath("pCube1"));
        if (!pCube1)
        {
            return EXIT_FAILURE;
        }
        pCube1->SetComment(kComment);
        layer->Export(testUrl);
    }
    {
        auto layer2 = SdfLayer::FindOrOpen(testUrl);
        if (!layer2)
        {
            return EXIT_FAILURE;
        }
        auto pCube1 = layer2->GetPrimAtPath(SdfPath("pCube1"));
        if (!pCube1)
        {
            return EXIT_FAILURE;
        }
        if (pCube1->GetComment() != kComment)
        {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

/*
Test inheriting server:port

Create a scene "baselayer.usd" which references "omni:/<randomFolder>/sublayer.usd"
Then try to load that scene
*/
TEST(inheritHost, "Test inheriting host:port from a parent layer")
{
    auto baseUrl = GenerateTestUrl();
    auto subLayerName = std::to_string(rand()) + ".usd";
    auto subLayerPath = "omni:" + test::randomFolder + subLayerName;
    auto subLayerUrl = test::randomUrl + subLayerName;

    {
        auto baseLayer = SdfLayer::CreateNew(baseUrl);
        if (!baseLayer)
        {
            testlog::printf("Failed to create base layer.\n");
            return EXIT_FAILURE;
        }

        {
            auto subLayer = SdfLayer::CreateNew(subLayerUrl);
            if (!subLayer)
            {
                testlog::printf("Failed to create sub layer.\n");
                return EXIT_FAILURE;
            }
        }

        std::vector<std::string> newPaths;
        newPaths.push_back(subLayerPath);
        baseLayer->SetSubLayerPaths(newPaths);
        baseLayer->Save();
    }
    UsdStageRefPtr stage = UsdStage::Open(baseUrl);
    if (!stage)
    {
        testlog::printf("Failed to open stage.\n");
        return EXIT_FAILURE;
    }
    auto usedLayers = stage->GetUsedLayers();
    if (usedLayers.size() != 3)
    {
        // There should be the session layer, the base layer, and the sub layer
        testlog::printf("Stage did not contain 3 layers.\n");
        for (auto&& layer : usedLayers)
        {
            testlog::printf("    %s\n", layer->GetIdentifier().c_str());
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(defaultSearchPath, "Test loading from a default search path")
{
    OMNI_TRACE_SCOPE(__FUNCTION__);

    std::string testFile = std::to_string(rand()) + ".usd";
    SdfLayer::CreateNew(test::randomUrl / testFile);

    omniClientAddDefaultSearchPath(test::randomUrl.c_str());
    auto layer = SdfLayer::FindOrOpen(testFile);
    omniClientRemoveDefaultSearchPath(test::randomUrl.c_str());
    if (!layer)
    {
        testlog::printf("Unable to FindOrOpen %s\n", testFile.c_str());
        return EXIT_FAILURE;
    }
    auto layerIdentifier = layer->GetRepositoryPath();
    if (layerIdentifier.compare(0, test::randomUrl.size(), test::randomUrl) != 0)
    {
        testlog::printf("Found incorrect identifier %s. Expected %s\n", layerIdentifier.c_str(), test::randomUrl.c_str());
        return EXIT_FAILURE;
    }

    // the following tests are added only for Ar 2.0 since search path logic needs
    // to be handled by the resolver implementation. This means that we need to verify
    // a search path will be anchored if it lives next to the anchoring asset path.
    // If it does not live next to the anchoring asset path, the asset path must be returned as-is
    // so it can later be resolved using the configured search paths.
    //
    // Ar 1.0 on the other hand is always expected to anchor the asset path (AnchorRelativePath)
    // and Sdf would handle the search path logic
    std::string boxAssetPath = "box.usda";
    std::string boxUrl = test::randomUrl / boxAssetPath;
    omniClientWait(omniClientCopy(boxAssetPath.c_str(), boxUrl.c_str(), nullptr, nullptr));

    std::string anchorUrl = test::randomUrl / testFile;

    omniClientAddDefaultSearchPath(test::randomUrl.c_str());
    auto identifier = ArGetResolver().CreateIdentifier(boxAssetPath, ArResolvedPath(anchorUrl));
    omniClientRemoveDefaultSearchPath(test::randomUrl.c_str());

    // verify "look here first"
    if (identifier != boxUrl)
    {
        testlog::printf("Failed to create identifier for %s. Expected %s, Actual %s", boxAssetPath.c_str(),
                        boxUrl.c_str(), identifier.c_str());
        return EXIT_FAILURE;
    }

    std::string missingAssetPath = "missing.usda";

    omniClientAddDefaultSearchPath(test::randomUrl.c_str());
    identifier = ArGetResolver().CreateIdentifier(missingAssetPath, ArResolvedPath(anchorUrl));
    omniClientRemoveDefaultSearchPath(test::randomUrl.c_str());

    // verify a search path is returned as-is so it can be resolved later
    if (identifier != missingAssetPath)
    {
        testlog::printf("Failed to create identifier for %s. Expected %s, Actual %s", missingAssetPath.c_str(),
                        missingAssetPath.c_str(), identifier.c_str());
        return EXIT_FAILURE;
    }

    std::string missingUrl = test::randomUrl / missingAssetPath;
    omniClientWait(omniClientCopy(boxAssetPath.c_str(), missingUrl.c_str(), nullptr, nullptr));

    omniClientAddDefaultSearchPath(test::randomUrl.c_str());
    auto resolvedPath = ArGetResolver().Resolve(missingAssetPath);
    omniClientRemoveDefaultSearchPath(test::randomUrl.c_str());

    // verify that when a search path is resolved it properly uses the configured search paths
    if (resolvedPath != missingUrl)
    {
        testlog::printf("Failed to resolve %s. Expected %s, Actual %s", missingAssetPath.c_str(), missingUrl.c_str(),
                        resolvedPath.GetPathString().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(openAsAnonymous, "Test OpenAsAnonymous")
{
    auto testUrl = GenerateTestUrl();
    omniClientWait(omniClientCopy("box.usda", testUrl.c_str(), nullptr, nullptr));

    auto layer = PXR_NS::SdfLayer::OpenAsAnonymous(testUrl);
    if (!layer)
    {
        testlog::print("Failed to open layer as anonymous\n");
        return EXIT_FAILURE;
    }
    if (layer->Save(true))
    {
        testlog::print("Should not be able to save an anonymous layer\n");
        return EXIT_FAILURE;
    }
    layer->SetIdentifier(GenerateTestUrl());
    if (!layer->Save(true))
    {
        testlog::print("Failed to save layer after setting it's identifier\n");
        return EXIT_FAILURE;
    }
    // Now try with a bad file extension (should fail, but not crash)
    testUrl += "-bad";
    layer = PXR_NS::SdfLayer::OpenAsAnonymous(testUrl);
    if (layer)
    {
        testlog::print("Opened layer with a bad file extension!\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(loadFromMount, "Test loading from a mount")
{
    std::string testUrl = concat("omniverse://", test::host, "/NVIDIA/Samples/OldAttic/Props/ball.usd");

    auto layer = SdfLayer::FindOrOpen(testUrl);
    if (!layer)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(loadFromHttp, "Test loading from a http")
{
    std::string testUrl = "http://dcb18d6mfegct.cloudfront.net/Samples/Marbles/assets/standalone/A_marble/A_marble.usd";

    auto layer = SdfLayer::FindOrOpen(testUrl);
    if (!layer)
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(saveToMountFail, "Test saving to a mount should fail")
{
    std::string testUrl = concat("omniverse://", test::host, "/NVIDIA/Samples/OldAttic/Props/ball.usd");

    auto layer = SdfLayer::FindOrOpen(testUrl);
    if (!layer)
    {
        return EXIT_FAILURE;
    }

    SdfPrimSpecHandle prim = SdfPrimSpec::New(layer->GetPseudoRoot(), "_test_prim_", SdfSpecifierDef);
    if (!prim)
    {
        testlog::printf("Failed to create primitive.\n");
        return EXIT_FAILURE;
    }

    if (layer->Save())
    {
        testlog::printf("Save to read-only path returned true.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(saveToHttp, "Test saving to a http should fail")
{
    std::string testUrl = "http://dcb18d6mfegct.cloudfront.net/Samples/Marbles/assets/standalone/A_marble/A_marble.usd";

    auto layer = SdfLayer::FindOrOpen(testUrl);
    if (!layer)
    {
        return EXIT_FAILURE;
    }

    SdfPrimSpecHandle prim = SdfPrimSpec::New(layer->GetPseudoRoot(), "_test_prim_", SdfSpecifierDef);
    if (!prim)
    {
        testlog::printf("Failed to create primitive.\n");
        return EXIT_FAILURE;
    }

    if (layer->Save())
    {
        testlog::printf("Save to read-only path returned true.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


/*
Test reading updates to an exported anonymous layer:
1. Create anonymous layer containing a sphere prim with radius 1.4.
2. Export the layer
3. Open this path in a new layer, and verify that the radius is 1.4.
4. Update the sphere radius in the original layer to .65
5. Export the original layer to the same path.
6. Open this path in a new layer, and verify that the radius is .65.
*/
TEST(anonLayerUpdate, "Test reading an updates to an exported anonymous layer")
{
    auto testFile = GenerateTestUrl();
    testlog::printf("Testing anon for %s\n", testFile.c_str());

    // Create anonymous layer
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    if (!layer)
    {
        testlog::printf("Failed to create anonymous layer.\n");
        return EXIT_FAILURE;
    }

    // Create sphere
    SdfPrimSpecHandle sphere = SdfPrimSpec::New(layer->GetPseudoRoot(), "sphere", SdfSpecifierDef, "Sphere");

    if (!sphere)
    {
        testlog::printf("Failed to create sphere.\n");
        return EXIT_FAILURE;
    }

    // Create radius attribute
    SdfAttributeSpecHandle radiusAttr = SdfAttributeSpec::New(sphere, "radius", SdfValueTypeNames->Double);

    if (!radiusAttr)
    {
        testlog::printf("Failed to create radius attribute.\n");
        return EXIT_FAILURE;
    }

    // Set the radius
    double radius = 1.4;
    layer->SetField(radiusAttr->GetPath(), SdfFieldKeys->Default, radius);

    // Export the anonymous layer
    if (!layer->Export(testFile))
    {
        testlog::printf("Failed to export sphere layer %s to omniverse\n", testFile.c_str());
        return EXIT_FAILURE;
    }

    if (!VerifyRadius(testFile, radius))
    {
        testlog::printf("Failed to verify radius after first export\n");
        return EXIT_FAILURE;
    }

    // Update the sphere radius re-export.
    radius = .65;
    layer->SetField(radiusAttr->GetPath(), SdfFieldKeys->Default, radius);

    // Sanity check: the original layer should have the updated radius.
    if (!VerifyRadius(layer, radius))
    {
        testlog::printf("Failed to verify updated radius\n");
        return EXIT_FAILURE;
    }

    if (!layer->Export(testFile))
    {
        testlog::printf("Failed to export updated sphere layer %s to omniverse\n", testFile.c_str());
        return EXIT_FAILURE;
    }

    if (!VerifyRadius(testFile, radius))
    {
        testlog::printf("Failed to verify radius after second export\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(omniReferenceLocal, "Reference a local file from an omniverse file")
{
    OMNI_TRACE_SCOPE(__FUNCTION__);

    auto testFile = GenerateTestUrl();

    auto testStage = PXR_NS::UsdStage::CreateNew(testFile);
    if (!testStage)
    {
        testlog::printf("Failed to create %s\n", testFile.c_str());
        return EXIT_FAILURE;
    }

    auto box = testStage->DefinePrim(PXR_NS::SdfPath("/Box"));
    box.GetReferences().AddReference("file:box.usda");

    if (box.GetTypeName() != "Mesh")
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(verifyUpdateAssetInfoRealPath, "Verify UpdateAssetInfo doesn't change RealPath")
{
    auto layer =
        PXR_NS::SdfLayer::FindOrOpen(concat("omniverse://", test::host, "/NVIDIA/Samples/OldAttic/Attic_NVIDIA.usd"));
    auto oldRealPath = layer->GetRealPath();
    layer->UpdateAssetInfo();
    auto realPath = layer->GetRealPath();
    if (realPath != oldRealPath)
    {
        testlog::printf("UpdateAssetInfo changed RealPath: %s != %s\n", realPath.c_str(), oldRealPath.c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

// 1. Export a layer to a given omniverse: path
// 2. Open the same omniverse: path.
// 3. Open the same path but with the omni: prefix.
//
// If URLs aren't normalized by the OmniUsdResolver, step 3 will generate a duplicate
// layer registry entry error.
TEST(openLayerAlias, "Export and open layers using equivalent URLs")
{
    TfErrorMark m;

    // Create anonymous layer
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    if (!layer)
    {
        testlog::printf("Failed to create anonymous layer.\n");
        return EXIT_FAILURE;
    }

    // Create dummy prim.
    SdfPrimSpecHandle sphere = SdfPrimSpec::New(layer->GetPseudoRoot(), "sphere", SdfSpecifierDef, "Sphere");

    if (!sphere)
    {
        testlog::printf("Failed to create sphere.\n");
        return EXIT_FAILURE;
    }

    // Create a url with an omniverse: prefix
    std::string url1 = GenerateTestUrl();

    // Export the layer.
    if (!layer->Export(url1))
    {
        testlog::printf("Failed to export %s\n", url1.c_str());
        return EXIT_FAILURE;
    }

    // Open the same url
    auto layer1 = SdfLayer::FindOrOpen(url1);
    if (!layer1)
    {
        testlog::printf("Failed to open %s\n", url1.c_str());
        return EXIT_FAILURE;
    }

    // Create an alias for the same path with an omni: prefix.
    std::string url2 = url1;
    url2.replace(0, strlen("omniverse:"), "omni:");

    // Open the omniverse: path.
    auto layer2 = SdfLayer::FindOrOpen(url2);
    if (!layer2)
    {
        testlog::printf("Failed to open %s\n", url2.c_str());
        return EXIT_FAILURE;
    }

    if (!m.IsClean())
    {
        testlog::print("Error mark dirty\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

namespace test_memleak
{
// Create a simple box in USD with normals and UV information
float h = 50.0;
int gBoxVertexIndices[] = { 0,  1,  2,  1,  3,  2,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
                            12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23 };
double gBoxNormals[][3] = { { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, -1 }, { 0, 0, 1 },  { 0, 0, 1 },
                            { 0, 0, 1 },  { 0, 0, 1 },  { 0, -1, 0 }, { 0, -1, 0 }, { 0, -1, 0 }, { 0, -1, 0 },
                            { 1, 0, 0 },  { 1, 0, 0 },  { 1, 0, 0 },  { 1, 0, 0 },  { 0, 1, 0 },  { 0, 1, 0 },
                            { 0, 1, 0 },  { 0, 1, 0 },  { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 } };
float gBoxPoints[][3] = { { h, -h, -h }, { -h, -h, -h }, { h, h, -h },  { -h, h, -h }, { h, h, h },    { -h, h, h },
                          { -h, -h, h }, { h, -h, h },   { h, -h, h },  { -h, -h, h }, { -h, -h, -h }, { h, -h, -h },
                          { h, h, h },   { h, -h, h },   { h, -h, -h }, { h, h, -h },  { -h, h, h },   { h, h, h },
                          { h, h, -h },  { -h, h, -h },  { -h, -h, h }, { -h, h, h },  { -h, h, -h },  { -h, -h, -h } };
float gBoxUV[][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 },
                      { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 },
                      { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

#define HW_ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

static UsdGeomMesh createBox(UsdStageRefPtr stage, int boxNumber = 0)
{
    // Keep the model contained inside of "Root", only need to do this once per model
    SdfPath rootPrimPath = SdfPath::AbsoluteRootPath().AppendChild(TfToken("Root"));
    UsdGeomXform::Define(stage, rootPrimPath);

    // Create the geometry inside of "Root"
    std::string boxName("box_");
    boxName.append(std::to_string(boxNumber));
    SdfPath boxPrimPath = rootPrimPath.AppendChild(TfToken(boxName)); //_tokens->box);
    UsdGeomMesh mesh = UsdGeomMesh::Define(stage, boxPrimPath);

    if (!mesh)
    {
        return mesh;
    }

    // Set orientation
    mesh.CreateOrientationAttr(VtValue(UsdGeomTokens->rightHanded));

    const int vertMultiplier = 512;

    // Add all of the vertices
    int num_vertices = HW_ARRAY_COUNT(gBoxPoints) * vertMultiplier;
    VtArray<GfVec3f> points;
    points.resize(num_vertices);
    for (int i = 0; i < num_vertices; i++)
    {
        points[i] = GfVec3f(
            gBoxPoints[i / vertMultiplier][0], gBoxPoints[i / vertMultiplier][1], gBoxPoints[i / vertMultiplier][2]);
    }
    mesh.CreatePointsAttr(VtValue(points));

    // Calculate indices for each triangle
    int num_indices = HW_ARRAY_COUNT(gBoxVertexIndices) * vertMultiplier; // 2 Triangles per face * 3 Vertices per
                                                                          // Triangle * 6 Faces
    VtArray<int> vecIndices;
    vecIndices.resize(num_indices);
    for (int i = 0; i < num_indices; i++)
    {
        vecIndices[i] = gBoxVertexIndices[i / vertMultiplier];
    }
    mesh.CreateFaceVertexIndicesAttr(VtValue(vecIndices));

    // Add vertex normals
    int num_normals = HW_ARRAY_COUNT(gBoxNormals) * vertMultiplier;
    VtArray<GfVec3f> meshNormals;
    meshNormals.resize(num_vertices);
    for (int i = 0; i < num_vertices; i++)
    {
        meshNormals[i] = GfVec3f((float)gBoxNormals[i / vertMultiplier][0], (float)gBoxNormals[i / vertMultiplier][1],
                                 (float)gBoxNormals[i / vertMultiplier][2]);
    }
    mesh.CreateNormalsAttr(VtValue(meshNormals));

    // Add face vertex count
    VtArray<int> faceVertexCounts;
    faceVertexCounts.resize(12); // 2 Triangles per face * 6 faces
    std::fill(faceVertexCounts.begin(), faceVertexCounts.end(), 3);
    mesh.CreateFaceVertexCountsAttr(VtValue(faceVertexCounts));

    // Set the color on the mesh
    UsdPrim meshPrim = mesh.GetPrim();
    UsdAttribute displayColorAttr = mesh.CreateDisplayColorAttr();
    {
        VtVec3fArray valueArray;
        GfVec3f rgbFace(0.463f, 0.725f, 0.0f);
        valueArray.push_back(rgbFace);
        displayColorAttr.Set(valueArray);
    }

    // Set the UV (st) values for this mesh
    auto primvarsApi = UsdGeomPrimvarsAPI(mesh);
    UsdGeomPrimvar attr2 = primvarsApi.CreatePrimvar(TfToken("st"), SdfValueTypeNames->TexCoord2fArray);
    {
        int uv_count = HW_ARRAY_COUNT(gBoxUV);
        VtVec2fArray valueArray;
        valueArray.resize(uv_count);
        for (int i = 0; i < uv_count; ++i)
        {
            valueArray[i].Set(gBoxUV[i]);
        }

        bool status = attr2.Set(valueArray);
    }
    attr2.SetInterpolation(UsdGeomTokens->vertex);

    return mesh;
}
} // namespace test_memleak

TEST(memoryLeak, "Make sure there's not a memory leak when we free a layer")
{
    size_t initialBytes = 0;

    for (int i = 0; i < 10; i++)
    {
        // Ignore the first 2 runs because it creates a lot of growable arrays
        if (i == 2)
        {
            initialBytes = carb::extras::getCurrentProcessMemoryUsage();
        }

        const std::string stageUrl = GenerateTestUrl();

        auto stage = UsdStage::CreateNew(stageUrl);
        if (!stage)
        {
            testlog::printf("Failed to create stage %s\n", stageUrl.c_str());
            return EXIT_FAILURE;
        }

        for (int bn = 0; bn < 100; bn++)
        {
            test_memleak::createBox(stage, bn);
        }

        stage->Save();
    }

    auto finalBytes = carb::extras::getCurrentProcessMemoryUsage();

    // There will be some growth from testlog storing the messages (among other things)
    // If there's a layer leak it'll be MUCH larger than this

    // FIXME: This is currently being disabled with the transition to Gitlab CI
    // We've hit a lot of what seem to be false-positives with this check and it's hard to know
    // if we are actually leaking memory. If this test fails, a retry will usually correct the problem
    // so it doesn't seem to be an actual memory leak. If possible we should use LSAN, or ASAN with detect_leaks=1
    // auto acceptableGrowth = 30 * 1024 * 1024;
    // if (finalBytes > initialBytes + acceptableGrowth)
    // {
    //     testlog::printf("Memory leak! %zu MB\n", (finalBytes - initialBytes) / (1024 * 1024));
    //     return EXIT_FAILURE;
    // }
    return EXIT_SUCCESS;
}

TEST(checkpointMessage, "Test setting a default message for atomic checkpoints")
{
    OMNI_TRACE_SCOPE(__FUNCTION__);

    bool checkpointsEnabled = false;
    omniClientWait(omniClientGetServerInfo(
        test::randomUrl.c_str(), &checkpointsEnabled,
        [](void* userData, OmniClientResult result, struct OmniClientServerInfo const* info) noexcept
        {
            if (result == eOmniClientResult_Ok)
            {
                *(bool*)userData = info->checkpointsEnabled;
            }
        }));
    if (!checkpointsEnabled)
    {
        return EXIT_SUCCESS;
    }

    std::string checkpointMessage1 = "test checkpoint message 1";
    omniUsdResolverSetCheckpointMessage(checkpointMessage1.c_str());

    const std::string stageUrl = GenerateTestUrl();
    auto stage = PXR_NS::SdfLayer::CreateNew(stageUrl);
    if (!stage)
    {
        testlog::printf("Failed to create stage %s\n", stageUrl.c_str());
        return EXIT_FAILURE;
    }

    std::string checkpointMessage2 = "test checkpoint message 2";
    omniUsdResolverSetCheckpointMessage(checkpointMessage2.c_str());
    stage->Save();

    struct Context
    {
        OmniClientResult result;
        std::vector<std::string> checkpointMessages;
    } context;

    omniClientWait(omniClientListCheckpoints(
        stageUrl.c_str(), &context,
        [](void* userData, OmniClientResult result, uint32_t numEntries, const OmniClientListEntry* entries) noexcept
        {
            auto context = static_cast<Context*>(userData);
            context->result = result;
            for (size_t i = 0; i < numEntries; ++i)
            {
                context->checkpointMessages.push_back(std::string(entries[i].comment));
            }
        }));

    if (context.result != eOmniClientResult_Ok)
    {
        testlog::printf(
            "Error listing checkpoints of %s: %s\n", stageUrl.c_str(), omniClientGetResultString(context.result));
        return EXIT_FAILURE;
    }

    if (context.checkpointMessages.size() != 2)
    {
        testlog::printf("Unexpected number of checkpoints for %s: Expected 2, got %u\n", stageUrl.c_str(),
                        context.checkpointMessages.size());
        return EXIT_FAILURE;
    }

    if (context.checkpointMessages[0].find(checkpointMessage1) == std::string::npos)
    {
        testlog::printf("Unexpected checkpoint message for %s: Expected %s, got %s\n", stageUrl.c_str(),
                        checkpointMessage1.c_str(), context.checkpointMessages[0].c_str());
        return EXIT_FAILURE;
    }

    if (context.checkpointMessages[1].find(checkpointMessage2) == std::string::npos)
    {
        testlog::printf("Unexpected checkpoint message for %s: Expected %s, got %s\n", stageUrl.c_str(),
                        checkpointMessage2.c_str(), context.checkpointMessages[1].c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// This verifies OM-35050
TEST(reloadAfterRestore, "Test reloading a layer after restoring a checkpoint")
{
    auto testUrl = GenerateTestUrl();
    bool checkpointsEnabled = false;
    omniClientWait(omniClientGetServerInfo(
        test::randomUrl.c_str(), &checkpointsEnabled,
        [](void* userData, OmniClientResult result, struct OmniClientServerInfo const* info) noexcept
        {
            if (result == eOmniClientResult_Ok)
            {
                *(bool*)userData = info->checkpointsEnabled;
            }
        }));
    if (!checkpointsEnabled)
    {
        return EXIT_SUCCESS;
    }
    auto testLayer = PXR_NS::SdfLayer::CreateNew(testUrl);
    testLayer->SetCustomLayerData({ { "empty", VtValue("") } });
    testLayer->Save();

    testLayer->SetCustomLayerData({ { "test", VtValue("test") } });
    testLayer->Save();

    omniClientWait(omniClientCopy((testUrl + "?&1").c_str(), testUrl.c_str(), nullptr, nullptr));

    testLayer->Reload();

    auto layerData = testLayer->GetCustomLayerData();
    if (layerData.find("test") != layerData.end())
    {
        testlog::print("Custom layer data still exists after restoring the original checkpoint\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(getExtension, "Test ArGetResolver().GetExtension()")
{
    auto& resolver = PXR_NS::ArGetResolver();
    auto ret = EXIT_SUCCESS;
    std::string result;
#define VERIFY_EXTENSION(path, extension)                                                                              \
    if ((result = resolver.GetExtension(path)) != extension)                                                           \
    {                                                                                                                  \
        testlog::printf("GetExtension(%s): %s != %s\n", path, result.c_str(), extension);                              \
        ret = EXIT_FAILURE;                                                                                            \
    }

    VERIFY_EXTENSION("", "");
    VERIFY_EXTENSION("something", "");

    VERIFY_EXTENSION("file:something.live", "live");
    VERIFY_EXTENSION("file:something.usd", "usd");
    VERIFY_EXTENSION("file:something.usda", "usda");
    VERIFY_EXTENSION("file:something.usdc", "usdc");
    VERIFY_EXTENSION("file:something.mdl", "mdl");
    VERIFY_EXTENSION("file:something.png", "png");

    VERIFY_EXTENSION("omni:something.live", "live");
    VERIFY_EXTENSION("omni:something.usd", "usd");
    VERIFY_EXTENSION("omni:something.usda", "usda");
    VERIFY_EXTENSION("omni:something.usdc", "usdc");
    VERIFY_EXTENSION("omni:something.mdl", "mdl");
    VERIFY_EXTENSION("omni:something.png", "png");
    VERIFY_EXTENSION("anon:0x12345678910:something.foo", "foo");

#undef VERIFY_EXTENSION

    return ret;
}

TEST(skipAnonymous, "Test that anonymous layer identifiers are not changed")
{
    // This test is to make sure that we do not wrap an anonymous layer
    // with the OmniUsdWrapperFileFormat. If a layer is anonymous we do not need
    // to do any reading or writing to Nucleus. OpenAsAnonymous will still function as normal
    // since it will read the content from the original URL
    char const* testUrl = "anon.usda";
    auto testLayer = SdfLayer::FindOrOpen(testUrl);
    if (!testLayer)
    {
        testlog::printf("Failed to open %s\n", testUrl);
        return EXIT_FAILURE;
    }

    auto stage = UsdStage::Open(testLayer);
    if (!stage)
    {
        testlog::printf("Unable to compose %s\n", testUrl);
        return EXIT_FAILURE;
    }

    // the layer stack should contain the root layer (anon.usda) and the sub-layer anon:0x12345678910:test.testff
    auto layers = stage->GetLayerStack(false);
    if (layers.size() != 2)
    {
        testlog::printf("Expected %s to have 2 layers\n", testUrl);
        return EXIT_FAILURE;
    }

    auto subLayer = layers.back();
    if (!subLayer->IsAnonymous())
    {
        testlog::print("Expected an anonymous subLayer\n");
        return EXIT_FAILURE;
    }

    auto fileFormat = subLayer->GetFileFormat();
    if (!fileFormat)
    {
        testlog::printf("Expected %s to have an associated file format\n", subLayer->GetRepositoryPath().c_str());
        return EXIT_FAILURE;
    }

    if (fileFormat->GetPrimaryFileExtension() != "testff")
    {
        testlog::printf("Expected %s to use the testff file format not %s\n", subLayer->GetIdentifier().c_str(),
                        fileFormat->GetPrimaryFileExtension().c_str());
        return EXIT_FAILURE;
    }

#if PXR_VERSION <= 2008
    // SdfLayer creates an anonymous identifier by calling Sdf_ComputeAnonLayerIdentifier
    // which would ultimately call TfStringPrintf()
    // But due to issues with URL-encoded values such as %20 Sdf_ComputeAnonLayerIdentifier / TfStringPrintf would
    // incorrectly try to format those values. nv-usd 20.08 introduces a fix for the URL-encoded
    // values but causes an issue where an anonymous identifier such as anon:0x12345678910:test.testff
    // would insert another pointer address after anon:
    // creating an identifier such as anon:0x01987654321:0x12345678910:test.testff
    if (!TfStringEndsWith(subLayer->GetIdentifier(), "0x12345678910:test.testff"))
#else
    if (subLayer->GetIdentifier() != "anon:0x12345678910:test.testff")
#endif
    {
        testlog::printf(
            "Expected sub-layer path to be anon:0x12345678910:test.testff: %s\n", subLayer->GetIdentifier().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(createNewFile, "Make sure CreateNew with a file: URL works")
{
    char const* testUrl = "file:_temp/test.usd";
    auto testLayer = SdfLayer::CreateNew(testUrl);
    if (!testLayer)
    {
        testlog::printf("Failed to create %s\n", testUrl);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(createWithPercent, "Make sure we can create files with percent signs in them")
{
    // Note this is intentionally a raw file name
    // This verifies a bug fix where some places were percent-decoding and others weren't
    // So the create would suceed but then the "GetModifiedTime" would fail, resulting in
    // the layer->Save (inside CreateNew) failing.
    char const* testUrl = "_temp/test%20test.usd";
    auto testLayer = SdfLayer::CreateNew(testUrl);
    if (!testLayer)
    {
        testlog::printf("Failed to create %s\n", testUrl);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(loadRawWithPercent, "Make sure we can load files with percent signs in them")
{
    // Note this is intentionally a raw file name
    // This verifies a bug fix where loading a local file with a percent sign in it
    // should _not_ percent-decode the filename. The test file here is actually named
    // with a raw "%20" in it.
    char const* testUrl = "test%20test.usd";
    auto testLayer = SdfLayer::FindOrOpen(testUrl);
    if (!testLayer)
    {
        testlog::printf("Failed to open %s\n", testUrl);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

TEST(reloadNotifications, "Make sure layer->Reload triggers notifications")
{
    std::string testUrl = GenerateTestUrl();
    std::string otherUrl = TfStringReplace(testUrl, test::host, "connection2@" + test::host);

    auto testLayer = SdfLayer::CreateNew(testUrl);
    if (!testLayer)
    {
        testlog::printf("Failed to create %s\n", testUrl.c_str());
        return EXIT_FAILURE;
    }

    SdfPrimSpecHandle sphere = SdfPrimSpec::New(testLayer->GetPseudoRoot(), "sphere", SdfSpecifierDef, "Sphere");

    if (!sphere)
    {
        testlog::printf("Failed to create sphere.\n");
        return EXIT_FAILURE;
    }

    // Create radius attribute
    SdfAttributeSpecHandle radiusAttr = SdfAttributeSpec::New(sphere, "radius", SdfValueTypeNames->Double);

    if (!radiusAttr)
    {
        testlog::printf("Failed to create radius attribute.\n");
        return EXIT_FAILURE;
    }

    // Set the radius
    testLayer->SetField(radiusAttr->GetPath(), SdfFieldKeys->Default, 1.4);

    testLayer->Save();

    // Open the layer using the "connection2" Url
    auto otherLayer = SdfLayer::FindOrOpen(otherUrl);
    if (!otherLayer)
    {
        testlog::printf("Failed to open %s\n", otherUrl.c_str());
        return EXIT_FAILURE;
    }

    // Make sure the radius is set correctly
    if (!VerifyRadius(otherLayer, 1.4))
    {
        return EXIT_FAILURE;
    }

    // Change the radius
    testLayer->SetField(radiusAttr->GetPath(), SdfFieldKeys->Default, 0.7);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    testLayer->Save();

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Make sure otherLayer gets a notification about the value changing after reload
    struct UsdLayerNoticeListener : public UsdNoticeListener<PXR_NS::SdfNotice::LayersDidChange>
    {
        UsdLayerNoticeListener()
        {
        }

        void handleNotice(const PXR_NS::SdfNotice::LayersDidChange& layersDidChange) override
        {
            for (auto& change : layersDidChange.GetChangeListVec())
            {
                for (const auto& entry : change.second.GetEntryList())
                {
                    printf("%s\n", entry.first.GetText());
                }
            }
            receivedNotice = true;
        }

        bool receivedNotice = false;
    };

    {
        UsdLayerNoticeListener listener;
        listener.registerListener();

        otherLayer->Reload();

        if (!listener.receivedNotice)
        {
            testlog::printf("Did not receive a layer change notice\n");
            return EXIT_FAILURE;
        }
    }

    if (!VerifyRadius(otherLayer, 0.7))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(packageFileFormats, "Test that USDZ archives and their interal textures are loaded correctly.")
{
    char const* filename = "Skull_downloadable.usdz";

    // Verify we can open it locally first
    std::string localResolved;
    omniClientWait(omniClientResolve(
        filename, nullptr, 0, &localResolved,
        [](void* userData, OmniClientResult result, struct OmniClientListEntry const* entry, char const* url) noexcept
        {
            if (result == eOmniClientResult_Ok)
            {
                *(std::string*)userData = safeString(url);
            }
        }));
    if (localResolved.empty())
    {
        testlog::printf("Could not resolve %s\n", filename);
        return EXIT_FAILURE;
    }
    {
        UsdStageRefPtr stage = UsdStage::Open(localResolved);
        if (!stage)
        {
            testlog::printf("Unable to open stage %s\n", localResolved.c_str());
            return EXIT_FAILURE;
        }
    }

    // This will test OM-53998
    std::string sceneStr = test::randomUrl / filename;
    const char* scene = sceneStr.c_str();

    omniClientWait(omniClientCopy(localResolved.c_str(), scene, {}, {}));

    auto& resolver = ArGetResolver();

    // Also test OM-63507 and OM-67700
    std::string sdfFormatArgs = ":SDF_FORMAT_ARGS:test=test";
    auto sceneWithArgs = sceneStr + sdfFormatArgs;

    auto sceneIdentifier =
        resolver.CreateIdentifier("./Skull_downloadable.usdz" + sdfFormatArgs, ArResolvedPath(test::randomUrl));
    if (!TfStringEndsWith(sceneIdentifier, sdfFormatArgs))
    {
        testlog::printf("CreateIdentifier(%s, ./Skull_downloadable.usdz%s) failed\n", test::randomUrl.c_str(),
                        sdfFormatArgs.c_str());
        return EXIT_FAILURE;
    }

    auto resolvedFile = resolver.Resolve(sceneWithArgs);

    if (resolvedFile.empty())
    {
        testlog::printf("Resolve(%s) failed\n", sceneWithArgs.c_str());
        return EXIT_FAILURE;
    }

    auto asset = resolver.OpenAsset(resolvedFile);
    if (!asset)
    {
        testlog::printf("Unable to open asset %s\n", resolver.Resolve(sceneStr).GetPathString().c_str());
        return EXIT_FAILURE;
    }

    auto buffer = asset->GetBuffer();
    if (!buffer)
    {
        testlog::printf("Unable to get buffer for asset %s\n", resolver.Resolve(sceneStr).GetPathString().c_str());
        return EXIT_FAILURE;
    }

    if (asset->GetSize() == 0)
    {
        testlog::printf("Invalid size for asset %s\n", resolver.Resolve(sceneStr).GetPathString().c_str());
        return EXIT_FAILURE;
    }

    UsdStageRefPtr stage = UsdStage::Open(sceneStr);
    if (!stage)
    {
        testlog::printf("Unable to open stage %s\n", sceneStr.c_str());
        return EXIT_FAILURE;
    }

    UsdShadeShader texBase = UsdShadeShader::Get(stage, SdfPath("/scene/Materials/defaultMat/tex_base"));
    if (!texBase)
    {
        testlog::printf("Unable to get tex_base Shader\n");
        return EXIT_FAILURE;
    }

    auto inputs = texBase.GetInputs();
    auto inputsFile = texBase.GetInput(TfToken("file"));
    auto textureAttr = inputsFile.GetAttr();
    SdfAssetPath assetPath;
    textureAttr.Get(&assetPath);
    auto resolvedPath = assetPath.GetResolvedPath();
    if (resolvedPath.empty())
    {
        testlog::printf("Unable to resolve texture path\n");
        return EXIT_FAILURE;
    }

    if (!ArIsPackageRelativePath(resolvedPath))
    {
        testlog::printf("Path is not a package relative path\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(differentSchemeResolver, "Test that a different registered URI scheme resolver works with OmniUsdResolver")
{
    // We want to make sure that we can still fallback to other ArResolver implementations that support specific
    // URI schemes
    ArResolver& resolver = ArGetResolver();

    std::string rootUrl = "test://b.host.com/foo/bar.usd";
    std::string identifer = resolver.CreateIdentifier("./baz.usd", ArResolvedPath(rootUrl));
    if (identifer.empty())
    {
        testlog::print("Identifier is empty\n");
        return EXIT_FAILURE;
    }

    std::string expected{ "test://b.host.com/foo/baz.usd" };
    if (identifer != expected)
    {
        testlog::printf(
            "Invalid identifier for ./baz.usd. Expected %s, actual %s\n", expected.c_str(), identifer.c_str());
        return EXIT_FAILURE;
    }

    auto result = resolver.Resolve(identifer);
    expected = std::string("/test_scheme/foo/baz.usd");
    if (result != expected)
    {
        testlog::printf("Invalid result for %s. Expected %s\n", identifer.c_str(), expected.c_str());
        return EXIT_FAILURE;
    }

    // More tests against OmniUsdResolver are omitted here. All the other tests are testing that behavior
    // (since test::registerPlugin() is registering all the resolvers)

    return EXIT_SUCCESS;
}

TEST(missingURLs, "Test URLs that can not be found return properly")
{
    // verifies OM-60387 that missing URLs do not hang and return an empty string
    ArResolver& resolver = ArGetResolver();

    // verify missing Nucleus URLs
    std::string nucleusURLStr = test::randomUrl / "box_missing.usda";
    auto resolvedPath = resolver.Resolve(nucleusURLStr);
    if (!resolvedPath.empty())
    {
        testlog::printf("Invalid result for %s. Expected empty string\n", nucleusURLStr.c_str());
        return EXIT_FAILURE;
    }

    // verify that opening empty resolved paths do not hang and return null assets
    auto asset = resolver.OpenAsset(resolvedPath);
    if (asset != nullptr)
    {
        testlog::print("Invalid result for empty string. Expected null ArAsset\n");
        return EXIT_FAILURE;
    }

    asset = resolver.OpenAsset(ArResolvedPath(nucleusURLStr));

    // verify that opening missing URLs does not hang and return null assets
    if (asset != nullptr)
    {
        testlog::printf("Invalid result %s. Expected null ArAsset", nucleusURLStr.c_str());
        return EXIT_FAILURE;
    }

    // verify missing file URLs
    std::string fileURLStr = "file:/tmp/box_missing.usda";
    resolvedPath = resolver.Resolve(fileURLStr);
    if (!resolvedPath.empty())
    {
        testlog::printf("Invalid result for %s. Expected empty string\n", fileURLStr.c_str());
        return EXIT_FAILURE;
    }

    asset = resolver.OpenAsset(ArResolvedPath(fileURLStr));

    // verify that opening missing URLs does not hang and return null assets
    if (asset != nullptr)
    {
        testlog::printf("Invalid result %s. Expected null ArAsset", fileURLStr.c_str());
        return EXIT_FAILURE;
    }

    // verify normal file paths
    std::string filePathStr = "/var/tmp/box_missing.usda";
    resolvedPath = resolver.Resolve(filePathStr);
    if (!resolvedPath.empty())
    {
        testlog::printf("Invalid result for %s. Expected empty string\n", filePathStr.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(overwriteUrls, "Test that URLs are properly overwritten")
{
    std::string testUrl = GenerateTestUrl();
    auto stage = PXR_NS::UsdStage::CreateNew(testUrl);

    // Create an almost empty stage that only contains the headers
    stage->Save();

    ArResolver& resolver = ArGetResolver();
    auto firstTimestamp = resolver.GetModificationTimestamp(testUrl, resolver.Resolve(testUrl));

    if (!firstTimestamp.IsValid())
    {
        testlog::printf("Failed to get first modification timestamp for %s\n", testUrl.c_str());
        return EXIT_FAILURE;
    }

    std::string localAssetPath = "test%20test.usd";
    auto exportStage = PXR_NS::UsdStage::Open(localAssetPath);
    if (!exportStage)
    {
        testlog::print("Unable to open stage 'test test.usd'\n");
        return EXIT_FAILURE;
    }

    // export the stage to the existing URL we created to overwrite the almost empty stage
    bool exportResult = exportStage->Export(testUrl, false);
    if (!exportResult)
    {
        testlog::printf("Unable to export %s to %s\n", localAssetPath.c_str(), testUrl.c_str());
        return EXIT_FAILURE;
    }

    auto nextTimestamp = resolver.GetModificationTimestamp(testUrl, resolver.Resolve(testUrl));

    if (!nextTimestamp.IsValid())
    {
        testlog::printf("Failed to get next modification timestamp for %s\n", testUrl.c_str());
        return EXIT_FAILURE;
    }

    if (firstTimestamp == nextTimestamp)
    {
        testlog::printf("%s was not modified after export, timestamps are equal\n", testUrl.c_str());
        return EXIT_FAILURE;
    }

    // We must reload the stage which should see that the modification time has changed
    // for the layer and should be updated
    stage->Reload();

    auto urlLayer = stage->GetRootLayer();
    if (!urlLayer)
    {
        testlog::printf("Unable to open root layer from %s\n", testUrl.c_str());
        return EXIT_FAILURE;
    }

    auto localLayer = exportStage->GetRootLayer();
    if (!localLayer)
    {
        testlog::printf("Unable to open root layer from %s\n", localAssetPath.c_str());
        return EXIT_FAILURE;
    }

    std::set<SdfPath> boxPaths;
    urlLayer->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath& path) { boxPaths.insert(path); });
    std::set<SdfPath> box2Paths;
    localLayer->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath& path) { box2Paths.insert(path); });

    if (boxPaths != box2Paths)
    {
        testlog::printf("Layers not the same after export\n");
        testlog::printf("  Source contains the following specs which are not in Dest:\n");
        for (auto&& boxIt : boxPaths)
        {
            if (box2Paths.find(boxIt) == box2Paths.end())
            {
                testlog::printf("    %s\n", boxIt.GetText());
            }
        }
        testlog::printf("  Dest contains the following specs which are not in Source:\n");
        for (auto&& box2It : box2Paths)
        {
            if (boxPaths.find(box2It) == boxPaths.end())
            {
                testlog::printf("    %s\n", box2It.GetText());
            }
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

bool verifyCanWrite(std::string const& testUrl)
{
    ArResolver& resolver = ArGetResolver();
    std::string reason;
    bool canWrite = resolver.CanWriteAssetToPath(ArResolvedPath(testUrl), &reason);
    if (!canWrite)
    {
        testlog::printf("Invalid result. Expected to be able to write, but got: %s\n", reason.c_str());
        return false;
    }
    if (!reason.empty())
    {
        testlog::printf("Invalid reason. Expected reason to be empty, but got: %\n", reason.c_str());
        return false;
    }
    return true;
}

bool verifyCanNotWrite(std::string const& testUrl, char const* expectedReason, char const* expectedUrl = nullptr)
{
    if (expectedUrl == nullptr)
    {
        expectedUrl = testUrl.c_str();
    }

    ArResolver& resolver = ArGetResolver();
    std::string reason;
    bool canWrite = resolver.CanWriteAssetToPath(ArResolvedPath(testUrl), &reason);
    if (canWrite)
    {
        testlog::printf("Invalid result. Expected to not be able to write.\n");
        return false;
    }

    std::string expected = TfStringPrintf(expectedReason, expectedUrl);
    if (reason != expected)
    {
        testlog::printf("Invalid reason: %s, expected: %s\n", reason.c_str(), expected.c_str());
        return false;
    }
    return true;
}

TEST(checkWriteAccess, "Test that write permissions are checked before writing assets")
{
    const auto testUrlAdmin = GenerateTestUrl();

    // make sure that we can write to a file that does not exist
    if (!verifyCanWrite(testUrlAdmin))
    {
        return EXIT_FAILURE;
    }

    omniClientWait(omniClientCopy("box.usda", testUrlAdmin.c_str(), nullptr, nullptr));

    // make sure we can write to a file that does exist (assuming we have permissions)
    if (!verifyCanWrite(testUrlAdmin))
    {
        return EXIT_FAILURE;
    }

    // make sure we can't write to a folder
    if (!verifyCanNotWrite(test::randomUrl, "%s is a folder"))
    {
        return EXIT_FAILURE;
    }

    // Normally tests run as the admin account, but this test requires a second user which is not an admin.
    // Only run if there are OMNI_TEST_USER2 / OMNI_TEST_PASS2 environment variables defined, which are assumed
    // to be for a non-admin user.
    struct Credentials
    {
        std::string user;
        std::string pass;
    } tc2;
    if (!carb::extras::EnvironmentVariable::getValue("OMNI_TEST_USER2", tc2.user))
    {
        testlog::print("Test skipped because OMNI_TEST_USER2 is not defined\n");
        return EXIT_SUCCESS;
    }
    if (!carb::extras::EnvironmentVariable::getValue("OMNI_TEST_PASS2", tc2.pass))
    {
        testlog::print("Test skipped because OMNI_TEST_PASS2 is not defined\n");
        return EXIT_SUCCESS;
    }

    const auto schemePrefix = "omniverse://";
    const auto schemePrefixLength = strlen(schemePrefix);
    if (testUrlAdmin.compare(0, schemePrefixLength, schemePrefix) != 0)
    {
        testlog::printf("%s does not start with %s\n", testUrlAdmin.c_str(), schemePrefix);
        return EXIT_FAILURE;
    }

    // testUrl2 is "omniverse://test@host/...
    // It is used to force a second login as the test user who is not an admin
    std::string testUrl2 = testUrlAdmin;
    testUrl2.insert(schemePrefixLength, "test@");

    auto authCbHandle = omniClientRegisterAuthCallback(
        &tc2,
        [](void* userData, char const* prefix, struct OmniClientCredentials* credentials) noexcept -> bool
        {
            const auto schemeAndUserPrefix = "omniverse://test@";
            const auto schemeAndUserPrefixLength = strlen(schemeAndUserPrefix);
            if (strncmp(prefix, schemeAndUserPrefix, schemeAndUserPrefixLength) == 0)
            {
                Credentials& tc2 = *(Credentials*)userData;
                credentials->username = omniClientReferenceContent((void*)tc2.user.c_str(), tc2.user.size() + 1);
                credentials->password = omniClientReferenceContent((void*)tc2.pass.c_str(), tc2.pass.size() + 1);
                return true;
            }
            return false;
        });
    CARB_SCOPE_EXIT
    {
        omniClientUnregisterCallback(authCbHandle);
    };

    OmniClientAclEntry entries[] = {
        { test::user.c_str(), fOmniClientAccess_Read },
        { "users", fOmniClientAccess_Read },
    };
    uint32_t numEntries = 2;

    struct Context
    {
        bool success;
    };
    Context context = { false };
    omniClientWait(omniClientSetAcls(testUrlAdmin.c_str(), numEntries, entries, &context,
                                     [](void* userData, OmniClientResult result) noexcept
                                     {
                                         auto context = (Context*)userData;
                                         context->success = result == eOmniClientResult_Ok;
                                     }));

    if (!context.success)
    {
        testlog::print("Unable to change ACLs\n");
        return EXIT_FAILURE;
    }

    // make sure that we can not write after setting the permissions to read-only
    if (!verifyCanNotWrite(testUrl2, "You do not have permission to write to %s"))
    {
        return EXIT_FAILURE;
    }

    entries[0].access = fOmniClientAccess_Full;
    entries[1].access = fOmniClientAccess_Full;

    omniClientWait(omniClientSetAcls(testUrlAdmin.c_str(), numEntries, entries, &context,
                                     [](void* userData, OmniClientResult result) noexcept
                                     {
                                         auto context = (Context*)userData;
                                         context->success = result == eOmniClientResult_Ok;
                                     }));

    if (!context.success)
    {
        testlog::print("Unable to change ACLs\n");
        return EXIT_FAILURE;
    }

    // make sure that we can write after changing the permissions back to read-write
    if (!verifyCanWrite(testUrl2))
    {
        return EXIT_FAILURE;
    }

    test::generateRandomFolder(std::to_string(std::random_device()()));
    std::string testUrlAdminFolder = test::randomUrl;

    // make 2 sibling URLs underneath a common parent
    // one as an admin and the other as a read-only user
    std::string testUrlAdmin2 = test::randomUrl;
    testUrlAdmin2.append(std::to_string(rand()));
    testUrlAdmin2.append("/box.usd");

    std::string testUrl3Folder = test::randomUrl;
    testUrl3Folder.insert(schemePrefixLength, "test@");

    std::string testUrl3 = test::randomUrl;
    testUrl3.insert(schemePrefixLength, "test@");
    testUrl3.append(std::to_string(rand()));
    testUrl3.append("/box.usd");

    // create the parent folder
    omniClientWait(omniClientCopy("box.usda", testUrlAdmin2.c_str(), nullptr, nullptr));

    entries[0].access = fOmniClientAccess_Read;
    entries[1].access = fOmniClientAccess_Read;

    omniClientWait(omniClientSetAcls(testUrlAdminFolder.c_str(), numEntries, entries, &context,
                                     [](void* userData, OmniClientResult result) noexcept
                                     {
                                         auto context = (Context*)userData;
                                         context->success = result == eOmniClientResult_Ok;
                                     }));

    if (!context.success)
    {
        testlog::print("Unable to change ACLs\n");
        return EXIT_FAILURE;
    }

    // make sure that we can not write to a file underneath a folder that we do not have permission to
    if (!verifyCanNotWrite(testUrl3, "You do not have permission to write to folder %s", testUrl3Folder.c_str()))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(layerSave, "Test saving layers in USDC")
{
    std::string testUrl = GenerateTestUrl();
    auto stage = PXR_NS::UsdStage::CreateNew(testUrl);

    auto cube = PXR_NS::UsdGeomCube::Define(stage, PXR_NS::SdfPath("/World/Cube"));
    cube.CreateSizeAttr().Set(100.0);

    PXR_NS::VtArray<PXR_NS::GfVec3f> extents;
    extents.push_back(PXR_NS::GfVec3f(-50.0, -50.0, -50.0));
    extents.push_back(PXR_NS::GfVec3f(50.0, 50.0, 50.0));
    cube.CreateExtentAttr().Set(extents);

    auto cubePrim = cube.GetPrim();

    PXR_NS::VtArray<int> expected{ 1, 2, 3, 4 };
    auto arrayAttr = cubePrim.CreateAttribute(PXR_NS::TfToken("customArray"), PXR_NS::SdfValueTypeNames->IntArray, true);
    arrayAttr.Set(expected);

    static const PXR_NS::GfVec3d kIdentityTranslate(0.0, 0.0, 0.0);
    static const PXR_NS::GfVec3f kIdentityRotate(0.0, 0.0, 0.0);
    static const PXR_NS::GfVec3f kIdentityScale(1.0, 1.0, 1.0);

    auto xform = PXR_NS::UsdGeomXformable(cubePrim);
    auto translateOp = xform.AddTranslateOp();
    translateOp.Set(kIdentityTranslate);

    auto rotateOp = xform.AddRotateXYZOp();
    rotateOp.Set(kIdentityRotate);

    auto scaleOp = xform.AddScaleOp();
    scaleOp.Set(kIdentityScale);

    // Save out the initial state of the stage
    stage->Save();

    // Verify everything saved out correctly
    arrayAttr = cubePrim.GetAttribute(PXR_NS::TfToken("customArray"));
    if (!arrayAttr)
    {
        testlog::printf("Failed to get customArray attribute\n");
        return EXIT_FAILURE;
    }

    PXR_NS::VtValue arrayValue;
    arrayAttr.Get(&arrayValue);

    if (!arrayValue.IsHolding<PXR_NS::VtArray<int>>())
    {
        testlog::printf("customArray attribute is not holding a VtArray<int>\n");
        return EXIT_FAILURE;
    }

    auto actual = arrayValue.UncheckedGet<PXR_NS::VtArray<int>>();
    if (actual != expected)
    {
        testlog::printf("customArray attribute does not match expected\n");
        return EXIT_FAILURE;
    }

    // Now change some other values on the stage before saving
    static const PXR_NS::GfVec3d kNonIdentityTranslate(10.0, 20.0, 30.0);
    static const PXR_NS::GfVec3f kNonIdentityRotate(45.0, 0.0, 0.0);
    static const PXR_NS::GfVec3f kNonIdentityScale(2.0, 2.0, 2.0);

    translateOp.Set(kNonIdentityTranslate);
    rotateOp.Set(kNonIdentityRotate);
    scaleOp.Set(kNonIdentityScale);

    // Save out the modified state of the stage
    stage->Save();

    // Verify everything saved out correctly
    arrayAttr = cubePrim.GetAttribute(PXR_NS::TfToken("customArray"));
    if (!arrayAttr)
    {
        testlog::printf("Failed to get customArray attribute after save\n");
        return EXIT_FAILURE;
    }

    arrayAttr.Get(&arrayValue);

    if (!arrayValue.IsHolding<PXR_NS::VtArray<int>>())
    {
        testlog::printf("customArray attribute is not holding a VtArray<int> after save\n");
        return EXIT_FAILURE;
    }

    actual = arrayValue.UncheckedGet<PXR_NS::VtArray<int>>();
    if (actual != expected)
    {
        testlog::printf("customArray attribute does not match expected after save. Expected size: %zu, actual size: %zu\n",
                        expected.size(), actual.size());
        return EXIT_FAILURE;
    }

    PXR_NS::VtValue xformOpOrderValue;
    auto xformOpOrderAttr = xform.GetXformOpOrderAttr().Get(&xformOpOrderValue);

    if (!xformOpOrderValue.IsHolding<PXR_NS::VtTokenArray>())
    {
        testlog::printf("xformOpOrder attribute is not holding a VtTokenArray\n");
        return EXIT_FAILURE;
    }

    PXR_NS::VtTokenArray xformOpOrder = xformOpOrderValue.UncheckedGet<PXR_NS::VtTokenArray>();
    if (xformOpOrder.size() != 3)
    {
        testlog::printf("xformOpOrder attribute does not have 3 elements\n");
        return EXIT_FAILURE;
    }

    if (xformOpOrder[0] != PXR_NS::TfToken("xformOp:translate"))
    {
        testlog::printf("xformOpOrder attribute does not have translate as the first element\n");
        return EXIT_FAILURE;
    }

    if (xformOpOrder[1] != PXR_NS::TfToken("xformOp:rotateXYZ"))
    {
        testlog::printf("xformOpOrder attribute does not have rotateXYZ as the second element\n");
        return EXIT_FAILURE;
    }

    if (xformOpOrder[2] != PXR_NS::TfToken("xformOp:scale"))
    {
        testlog::printf("xformOpOrder attribute does not have scale as the third element\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

TEST(alembicUrls, "Test that alembic URLs will be associated with the Wrapper file format")
{
    std::string testUrl = test::randomUrl;
    testUrl.append(std::to_string(rand()));
    testUrl.append(".abc");

    ArResolver& resolver = ArGetResolver();
    auto extension = resolver.GetExtension(testUrl);
    if (extension != "omnicache")
    {
        testlog::printf("Invalid extension for %s. Expected omnicache, got %s\n", testUrl.c_str(), extension.c_str());
        return EXIT_FAILURE;
    }

    auto fileFormat = SdfFileFormat::FindByExtension(extension);
    if (!fileFormat)
    {
        testlog::printf("Failed to find file format for %s\n", extension.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////

void PrintTestList()
{
    testlog::printf("Available tests are: \n");
    for (auto&& t : testmap)
    {
        testlog::printf("   %s : %s\n", t.first.c_str(), t.second.description.c_str());
    }
    testlog::printf("Specify 'all' to run all of them\n");
}

int run_test(testmap_type::value_type& test)
{
    testlog::start(test.first);
    int test_result = test.second.function();
    testlog::finish(test_result == EXIT_SUCCESS);
    return test_result;
}

class TestDelegate : public PXR_NS::TfDiagnosticMgr::Delegate
{
public:
    TestDelegate()
    {
        PXR_NS::TfDiagnosticMgr::GetInstance().AddDelegate(this);
    }

    virtual ~TestDelegate()
    {
        PXR_NS::TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
    }

    virtual void IssueError(TfError const& err)
    {
        testlog::printf("USD Error! %s\n", err.GetCommentary().c_str());
    }

    virtual void IssueFatalError(TfCallContext const& context, std::string const& msg)
    {
        testlog::printf("USD Fatal Error! %s\n", msg.c_str());
    }

    virtual void IssueStatus(TfStatus const& status)
    {
        testlog::printf("USD: %s\n", status.GetCommentary().c_str());
    }

    virtual void IssueWarning(TfWarning const& warning)
    {
        testlog::printf("USD Warning! %s\n", warning.GetCommentary().c_str());
    }
};

int main(int argc, char** argv)
{
    TestDelegate td;

    omniClientSetLogCallback(
        [](const char* threadName, const char* component, OmniClientLogLevel level, const char* message) noexcept
        { testlog::printf("%c: %s: %s: %s\n", omniClientGetLogLevelChar(level), threadName, component, message); });
#ifdef _NDEBUG
    omniClientSetLogLevel(eOmniClientLogLevel_Verbose);
#else
    omniClientSetLogLevel(eOmniClientLogLevel_Debug);
#endif

    // We intentionally don't call omniClientInitialize here to verify this doesn't crash

    carb::acquireFrameworkAndRegisterBuiltins();

    if (!test::setupEnvironment("resolver"))
    {
        return EXIT_FAILURE;
    }

    if (!test::registerPlugin())
    {
        return EXIT_FAILURE;
    }

    int test_result = EXIT_SUCCESS;

    const char* testName = argc > 1 ? argv[1] : "all";
    if (strcmp(testName, "all") == 0)
    {
        for (auto&& it : testmap)
        {
            test_result = run_test(it);
            if (test_result != EXIT_SUCCESS)
            {
                break;
            }
        }
    }
    else
    {
        auto it = testmap.find(testName);
        if (it == testmap.end())
        {
            testlog::printf("Test '%s' not found\n", testName);
            PrintTestList();
            test_result = EXIT_FAILURE;
        }
        else
        {
            test_result = run_test(*it);
        }
    }

    omniClientDelete(test::randomUrl.c_str(), nullptr, nullptr);
    omniClientShutdown();

    return test_result;
}
