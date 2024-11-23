// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.
#pragma once

#include "OmniUsdResolver.h"

OMNIUSDRESOLVER_EXPORT(void)
SendNotification(const char* identifier,
                 OmniUsdResolverEvent eventType,
                 OmniUsdResolverEventState eventState,
                 uint64_t fileSize = 0);
