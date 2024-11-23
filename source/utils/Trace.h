// SPDX-FileCopyrightText: Copyright (c) 2018-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#pragma once

#include <carb/StartupUtils.h>
#include <carb/extras/EnvironmentVariable.h>
#include <carb/tracer/Tracer.h>
#include <carb/tracer/TracerCarbLoading.h>
#include <carb/tracer/TracerUtil.h>

#ifdef ENABLE_USD_TRACE
#    include <pxr/base/trace/trace.h>
#endif

struct TracerInitializer
{
    ~TracerInitializer()
    {
        shutdown();
    }
    bool initialize(const char* processName = g_carbClientName)
    {
        // Already loaded, putting two TracerInitializers on the stack will break shutdown order
        CARB_ASSERT(!carb::tracer::g_carbTracer);

        carb::tracer::g_carbTracer = carb::tracer::acquireTracerInterface();

        carb::tracer::TracerSettings tracerSettings;
        tracerSettings.process_name = processName;
        tracerSettings.autoflush = 4096;

        return carb::tracer::startup(tracerSettings) == carb::tracer::ErrorType::eOk;
    }
    void shutdown()
    {
        if (carb::tracer::g_carbTracer)
        {
            carb::tracer::shutdown();
            carb::tracer::releaseTracerInterface(carb::tracer::g_carbTracer);
            carb::tracer::g_carbTracer = nullptr;
        }
    }
};

#define CARB_TRACE_GLOBAL()                                                                                            \
    namespace carb                                                                                                     \
    {                                                                                                                  \
    namespace tracer                                                                                                   \
    {                                                                                                                  \
    Tracer* g_carbTracer;                                                                                              \
    }                                                                                                                  \
    }

#ifdef ENABLE_USD_TRACE
#    define OMNI_TRACE_SCOPE(name)                                                                                     \
        TRACE_SCOPE(name)                                                                                              \
        carb::tracer::SpanScope _span(                                                                                 \
            carb::tracer::kCaptureMaskDetailFull, g_carbClientName, __FILE__, __LINE__, {}, name);
#    define OMNI_TRACE_SCOPE_VERBOSE(name)                                                                             \
        TRACE_SCOPE(name)                                                                                              \
        carb::tracer::SpanScope _span(                                                                                 \
            carb::tracer::kCaptureMaskDetailVerbose, g_carbClientName, __FILE__, __LINE__, {}, name);
#else
#    define OMNI_TRACE_SCOPE(name)                                                                                     \
        carb::tracer::SpanScope _span(                                                                                 \
            carb::tracer::kCaptureMaskDetailFull, g_carbClientName, __FILE__, __LINE__, {}, name);
#    define OMNI_TRACE_SCOPE_VERBOSE(name)                                                                             \
        carb::tracer::SpanScope _span(                                                                                 \
            carb::tracer::kCaptureMaskDetailVerbose, g_carbClientName, __FILE__, __LINE__, {}, name);
#    define OMNI_TRACE_SCOPE_MASKED(name, mask)                                                                        \
        carb::tracer::SpanScope _span(mask, g_carbClientName, __FILE__, __LINE__, {}, name);
#endif
