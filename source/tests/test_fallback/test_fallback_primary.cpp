// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "TestEnvironment.h"
#include "TestHelpers.h"
#include "TestLog.h"
#include "UsdIncludes.h"

#include <carb/ClientUtils.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/type.h>

#include <OmniClient.h>

CARB_GLOBALS("omni_usd_test_fallback")

namespace test
{
inline bool registerFallbackPlugin()
{
    std::string thisPath;
    if (!PXR_NS::ArchGetAddressInfo(
            reinterpret_cast<void*>(&registerFallbackPlugin), &thisPath, nullptr, nullptr, nullptr))
    {
        return false;
    }

    std::vector<std::string> paths{ PXR_NS::TfGetPathName(thisPath) + "test/fallback/resources/",
                                    PXR_NS::TfGetPathName(thisPath) + "test/redist/resources/" };

    PXR_NS::PlugRegistry::GetInstance().RegisterPlugins(paths);

    // We are intentionally not using the registerPlugin() from RegisterPlugin.h
    // One of the caveats when dealing with "primary" resolvers is that they can't be primary
    // and also define the URI schemes they support. The "preferred" resolver also needs to be
    // a "primary" resolver for it to be correctly set as such. So what this means is that we need a slightly
    // different plugInfo.json for OmniUsdResovler (i.e it declares "uriSchemes" metadata) when it is intended
    // to be used with a different vender's ArResolver implementation. This will allow the other vender to
    // handle their own resoltuion logic but schemes like omniverse://some/path.usd can still be resolved
    // via OmniUsdResolver
    PXR_NS::ArSetPreferredResolver("TestPrimaryResolver");
    return true;
}
} // namespace test

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

TEST(differentPrimaryResolver, "Tests that a different primary resolver can be set")
{
    // make sure that we have both resolver plugins and types loaded
    auto& registry = PlugRegistry::GetInstance();
    if (!registry.GetPluginWithName("TestFallbackResolver"))
    {
        testlog::print("TestFallbackResolver was not loaded by PlugRegistry\n");
        return EXIT_FAILURE;
    }

    if (!TfType::FindByName("TestPrimaryResolver"))
    {
        testlog::print("Unable to find TestPrimaryResolver Type\n");
        return EXIT_FAILURE;
    }

    if (!registry.GetPluginWithName("OmniverseUSDResolver"))
    {
        testlog::print("OmniverseUSDResolver was not loaded by PlugRegistry\n");
        return EXIT_FAILURE;
    }

    if (!TfType::FindByName("OmniUsdResolver"))
    {
        testlog::print("Unable to find OmniUsdResolver Type\n");
        return EXIT_FAILURE;
    }

    // When first calling ArGetResolver() it will look-up all available resolvers (found via plugInfo.json)
    // Any resolvers found that do not specify uriSchemes will be considered a primary resolver. If multiple
    // primary resolvers are found the first one in the sorted list (by TfType::GetTypeName()) will be used.
    // The primary resolver used can be forced by explicitly calling ArSetPreferredResolver() before
    // the first call to ArGetResolver() (typically during application startup).
    // In our case, TestPrimaryResolver is set as the preferred resolver and we want to make sure that we can
    // change to a different primary resolver
    ArResolver& resolver = ArGetResolver();

    std::string rootUrl = "fake://a.host.com/foo/bar.usd";
    std::string identifer = resolver.CreateIdentifier("./baz.usd", ArResolvedPath(rootUrl));
    if (identifer.empty())
    {
        testlog::print("Identifier is empty\n");
        return EXIT_FAILURE;
    }

    std::string expected{ "fake://a.host.com/foo/baz.usd" };
    if (identifer != expected)
    {
        testlog::printf(
            "Invalid identifier for ./baz.usd. Expected %s, actual %s\n", expected.c_str(), identifer.c_str());
        return EXIT_FAILURE;
    }

    auto result = resolver.Resolve(identifer);
    expected = std::string("/test_primrary/foo/baz.usd");
    if (result != expected)
    {
        testlog::printf("Invalid result for %s. Expected %s\n", identifer.c_str(), expected.c_str());
        return EXIT_FAILURE;
    }

    // even with a different primary resolver we should still be able to resolve OmniUsdResolver supported URLs
    std::string worldUrl = test::randomUrl / "World.usd";
    result = resolver.Resolve(worldUrl);
    if (result.empty())
    {
        testlog::printf("%s resolved to an empty result\n", worldUrl.c_str());
        return EXIT_FAILURE;
    }

    // we should also be able to open the omniverse asset
    auto asset = resolver.OpenAsset(result);
    if (!asset)
    {
        testlog::printf("Unable to open asset\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int run_test(testmap_type::value_type& test)
{
    testlog::start(test.first);
    int test_result = test.second.function();
    testlog::finish(test_result == EXIT_SUCCESS);
    return test_result;
}

int main(int argc, char** argv)
{
    omniClientSetLogCallback(
        [](const char* threadName, const char* component, OmniClientLogLevel level, const char* message) noexcept
        { testlog::printf("%c: %s: %s: %s\n", omniClientGetLogLevelChar(level), threadName, component, message); });
    omniClientSetLogLevel(eOmniClientLogLevel_Warning);

    carb::acquireFrameworkAndRegisterBuiltins();

    if (!test::setupEnvironment("fallback"))
    {
        return EXIT_FAILURE;
    }

    if (!test::registerFallbackPlugin())
    {
        return EXIT_FAILURE;
    }

    if (!omniClientInitialize(kOmniClientVersion))
    {
        return EXIT_FAILURE;
    }

    std::string testfilestr = test::randomUrl / "World.usd";
    const char* testfile = testfilestr.c_str();

    omniClientWait(omniClientCopy("World.usda", testfile, {}, {}));

    int test_result = EXIT_SUCCESS;
    for (auto&& it : testmap)
    {
        test_result = run_test(it);
        if (test_result != EXIT_SUCCESS)
        {
            break;
        }
    }

    omniClientDelete(test::randomUrl.c_str(), nullptr, nullptr);

    return test_result;
}
