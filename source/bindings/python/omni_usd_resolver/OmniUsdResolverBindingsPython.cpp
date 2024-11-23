// SPDX-FileCopyrightText: Copyright (c) 2022-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "OmniUsdResolver.h"

#include <carb/BindingsPythonUtils.h>

// Include after BindingsPythonUtils.h
#include "utils/PythonUtils.h"

/**
 * Holds subscription for python in RAII way. unregister function is called when destroyed.
 */
class Subscription
{
public:
    explicit Subscription(const std::function<void()>& unregisterFn) : m_unregisterFn(unregisterFn)
    {
    }

    void unregister()
    {
        if (m_unregisterFn)
        {
            // Unregistering almost always requires taking another lock
            // So release the gil to avoid a lock inversion (CC-357)
            PyReleaseGil release;
            m_unregisterFn();
            m_unregisterFn = nullptr;
        }
    }

    ~Subscription()
    {
        unregister();
    }

private:
    std::function<void()> m_unregisterFn;
};

CARB_BINDINGS("omni.usd.resolver.python")

PYBIND11_MODULE(_omni_usd_resolver, m)
{
    static_assert(Count_eOmniUsdResolverEvent == 3, "Missing entries");
    py::enum_<OmniUsdResolverEvent>(m, "Event", R"()")
        .value("RESOLVING", eOmniUsdResolverEvent_Resolving)
        .value("READING", eOmniUsdResolverEvent_Reading)
        .value("WRITING", eOmniUsdResolverEvent_Writing)
        .attr("__module__") = "omni.usd_resolver";

    static_assert(Count_eOmniUsdResolverEventState == 3, "Missing entries");
    py::enum_<OmniUsdResolverEventState>(m, "EventState", R"()")
        .value("STARTED", eOmniUsdResolverEventState_Started)
        .value("SUCCESS", eOmniUsdResolverEventState_Success)
        .value("FAILURE", eOmniUsdResolverEventState_Failure)
        .attr("__module__") = "omni.usd_resolver";

    py::class_<Subscription, std::shared_ptr<Subscription>>(m, "Subscription")
        .def(
            "__enter__", [&](std::shared_ptr<Subscription> sub) { return sub; }, py::call_guard<py::gil_scoped_release>())
        .def("__exit__", [&](std::shared_ptr<Subscription> sub, pybind11::object exc_type, pybind11::object exc_value,
                             pybind11::object traceback) { sub->unregister(); })
        .def("unregister", [&](std::shared_ptr<Subscription> sub) { sub->unregister(); })
        .attr("__module__") = "omni.usd_resolver";

    m.def("set_checkpoint_message", &omniUsdResolverSetCheckpointMessage,
          R"(Set the message to be used for atomic checkpoints created when saving files.
          
          Args:
              message (str): Checkpoint message.)",
          py::arg("message"), py::call_guard<py::gil_scoped_release>());

    using RegisterEventCallbackFn =
        std::function<void(const char*, OmniUsdResolverEvent, OmniUsdResolverEventState, uint64_t)>;
    m.def(
        "register_event_callback",
        [](const RegisterEventCallbackFn& cb)
        {
            auto* callback = new RegisterEventCallbackFn(cb);

            auto id = omniUsdResolverRegisterEventCallback(
                callback,
                [](void* userData, const char* url, OmniUsdResolverEvent eventType,
                   OmniUsdResolverEventState eventState, uint64_t fileSize) noexcept
                {
                    auto callback = (RegisterEventCallbackFn*)userData;
                    carb::callPythonCodeSafe(*callback, url, eventType, eventState, fileSize);
                });
            auto subscription = std::make_shared<Subscription>(
                [=]()
                {
                    omniUsdResolverUnregisterCallback(id);
                    delete callback;
                });

            return subscription;
        },
        R"(
            Register a function that will be called any time something interesting happens.

            Args:
                callback: Callback to be called with the event.

            Returns:
                Subscription Object. Callback will be unregistered once subcription is released.
        )",
        py::arg("callback"), py::call_guard<py::gil_scoped_release>());

    m.def("get_version", &omniUsdResolverGetVersionString, py::call_guard<py::gil_scoped_release>(),
          R"(
            Get the version of USD Resolver being used.

            Returns:
                Returns a human readable version string.
        )");

    m.def(
        "set_mdl_builtins",
        [](std::vector<std::string> const& builtins)
        {
            std::vector<char const*> builtins_cstr;
            builtins_cstr.resize(builtins.size());
            for (size_t i = 0; i < builtins.size(); i++)
            {
                builtins_cstr[i] = builtins[i].c_str();
            }
            omniUsdResolverSetMdlBuiltins(builtins_cstr.data(), builtins_cstr.size());
        },
        py::call_guard<py::gil_scoped_release>(),
        R"(
            Set a list of built-in MDLs.

            Resolving an MDL in this list will return immediately rather than performing a full resolution.
        )");
}
