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

#include "utils/StringUtils.h"

#include <carb/extras/EnvironmentVariable.h>
#include <carb/filesystem/IFileSystem.h>

#include <random>

namespace test
{
static std::string user;
static std::string pass;
static std::string host;

// A random number
static std::string randomNumber;

// The root of the test folder
static std::string testFolder;

// A random folder (testFolder/random)
static std::string randomFolder;

// Url to the random folder
static std::string randomUrl;

inline void generateRandomFolder(std::string n)
{
    randomNumber = std::move(n);
    randomFolder = testFolder / randomNumber / std::string();
    randomUrl = concat("omniverse://", host, randomFolder);
}

inline bool setupEnvironment(char const* testName)
{
    user = "omniverse";
    pass = "omniverse";
    host = "localhost";

    carb::extras::EnvironmentVariable::getValue("OMNI_TEST_USER", user);
    carb::extras::EnvironmentVariable::getValue("OMNI_TEST_PASS", pass);
    carb::extras::EnvironmentVariable::getValue("OMNI_TEST_HOST", host);

    if (user.empty() || pass.empty() || host.empty())
    {
        printf("Must specify OMNI_TEST_USER, OMNI_TEST_PASS, and OMN_TEST_HOST\n");
        return false;
    }

    // All tests assume that the current working directory is ./test-data where
    // files such as box.usda can be found. This was originally handled in run_tests.py
    // but as we moved to repo_test for usd_build_bom integration there was not a way to
    // configure the current working directory
    auto fs = carb::getFramework()->acquireInterface<carb::filesystem::IFileSystem>();
    fs->setCurrentDirectoryPath("test-data");

    testFolder = "/Tests" / test::user / std::string(testName) / std::string();

    generateRandomFolder(std::to_string(std::random_device()()));

    carb::extras::EnvironmentVariable::setValue("OMNI_USER", user.c_str());
    carb::extras::EnvironmentVariable::setValue("OMNI_PASS", pass.c_str());

    return true;
}
} // namespace test
