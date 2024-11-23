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

#include <carb/Defines.h>

#if OMNIUSDRESOLVER_PYVER == 0

struct PyReleaseGil
{
    PyReleaseGil()
    {
    }
    ~PyReleaseGil()
    {
    }

    CARB_PREVENT_COPY_AND_MOVE(PyReleaseGil);
};

#else

#    include <pystate.h>

/*
There are 3 cases to handle:
1. We are not in a python thread.
    PyGILState_GetThisThreadState returns null and we do nothing
2. We are in a python thread, and the gil is released.
    PyGILState_Check returns false and we do nothing
3. We are in a python thread, and the gil is aquired.
    PyEval_SaveThread releases the gil
*/
struct PyReleaseGil
{
    PyThreadState* tstate{};

    PyReleaseGil()
    {
        if (PyGILState_GetThisThreadState() && PyGILState_Check())
        {
            tstate = PyEval_SaveThread();
        }
    }
    ~PyReleaseGil()
    {
        if (tstate)
        {
            PyEval_RestoreThread(tstate);
        }
    }

    CARB_PREVENT_COPY_AND_MOVE(PyReleaseGil);
};

#endif
