// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <time.h>

inline std::chrono::system_clock::time_point convertFromTimeSinceUnixEpoch(std::chrono::nanoseconds nano)
{
    auto t = std::chrono::system_clock::from_time_t(0) + nano;
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(t);
}

inline std::chrono::nanoseconds convertToTimeSinceUnixEpoch(std::chrono::system_clock::time_point tp)
{
    return tp - std::chrono::system_clock::from_time_t(0);
}
