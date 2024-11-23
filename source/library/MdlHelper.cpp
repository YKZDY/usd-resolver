// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include "MdlHelper.h"

#include "DebugCodes.h"
#include "OmniUsdResolver.h"
#include "utils/StringUtils.h"

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/stringUtils.h>

#include <set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE
TF_DEFINE_ENV_SETTING(OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS,
                      false,
                      "Enables the OmniUsdResolver to bypass MDL builtins for anchoring or search paths");

/*
The following list of default MDL paths was manually entered from examining the content of the downloaded package of
omni_core_materials.

omni.usd.config sets this env var with the fullt list that was discovered through walking the search paths defined
in the renderer config. This is a temporary solution for AR 1.
*/
TF_DEFINE_ENV_SETTING(OMNI_USD_RESOLVER_MDL_BUILTIN_PATHS,
                      "AperturePBR.mdl,"
                      "AperturePBR_Opacity.mdl,"
                      "AperturePBR_ThinOpaque.mdl,"
                      "AperturePBR_ThinTranslucent.mdl,"
                      "AperturePBR_Translucent.mdl,"
                      "OmniGlass.mdl,"
                      "OmniGlass_Opacity.mdl,"
                      "OmniHair.mdl,"
                      "OmniHairPresets.mdl,"
                      "OmniPBR.mdl,"
                      "OmniPBR_ClearCoat.mdl,"
                      "OmniPBR_ClearCoat_Opacity.mdl,"
                      "OmniPBR_Opacity.mdl,"
                      "OmniSurface.mdl,"
                      "OmniSurfaceBlend.mdl,"
                      "OmniSurfaceLite.mdl,"
                      "OmniSurfacePresets.mdl,"
                      "OmniUe4Base.mdl,"
                      "OmniUe4Function.mdl,"
                      "OmniUe4FunctionExtension17.mdl,"
                      "OmniUe4Subsurface.mdl,"
                      "OmniUe4Translucent.mdl,"
                      "adobe/anisotropy.mdl,"
                      "adobe/annotations.mdl,"
                      "adobe/convert.mdl,"
                      "adobe/materials.mdl,"
                      "adobe/mtl.mdl,"
                      "adobe/util.mdl,"
                      "adobe/volume.mdl,"
                      // See if all the MDL paths under alg need to be included
                      "gltf/pbr.mdl,"
                      "materialx/cm.mdl,"
                      "materialx/core.mdl,"
                      "materialx/hsv.mdl,"
                      "materialx/noise.mdl,"
                      "materialx/pbrlib.mdl,"
                      "materialx/sampling.mdl,"
                      "materialx/stdlib.mdl,"
                      "materialx/swizzle.mdl,"
                      "nvidia/aux_definitions.mdl,"
                      "nvidia/core_definitions.mdl,"
                      "nvidia/support_definitions.mdl,"
                      "OmniSurface/OmniHairBase.mdl,"
                      "OmniSurface/OmniImage.mdl,"
                      "OmniSurface/OmniShared.mdl,"
                      "OmniSurface/OmniSurfaceBase.mdl,"
                      "OmniSurface/OmniSurfaceBlendBase.mdl,"
                      "OmniSurface/OmniSurfaceLiteBase.mdl,"
                      "OmniVolumeDensity.mdl,"
                      "OmniVolumeNoise.mdl,"
                      "DebugWhiteEmissive.mdl,"
                      "DebugWhite.mdl,"
                      "Default.mdl,"
                      "MdlStates.mdl,"
                      "UsdPreviewSurface.mdl,"
                      "architectural.mdl,"
                      "environment.mdl,"
                      "omni_light.mdl,"
                      "ad_3dsmax_maps.mdl,"
                      "ad_3dsmax_materials.mdl,"
                      "vray_maps.mdl,"
                      "vray_materials.mdl",
                      "Comma-separated list for determining MDL builtin materials");
PXR_NAMESPACE_CLOSE_SCOPE
PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
static std::set<std::string> g_builtins;

struct PopulateBuiltinsFromEnv
{
    PopulateBuiltinsFromEnv()
    {
        const std::vector<std::string> paths = TfStringSplit(TfGetEnvSetting(OMNI_USD_RESOLVER_MDL_BUILTIN_PATHS), ",");
        g_builtins.insert(paths.begin(), paths.end());
    };
};
static PopulateBuiltinsFromEnv g_populateBuiltinsFromEnv;
}; // namespace

OMNIUSDRESOLVER_EXPORT(void)
omniUsdResolverSetMdlBuiltins(char const** builtins, size_t numBuiltins) OMNIUSDRESOLVER_NOEXCEPT
{
    g_builtins.clear();
    for (size_t i = 0; i < numBuiltins; i++)
    {
        g_builtins.insert(safeString(builtins[i]));
    }
}

namespace mdl_helper
{
bool IsMdlIdentifier(const std::string& assetPath)
{
    // XXX: This env var may no longer be necessary for Ar 2. This was added to Ar 1 to allow an MDL path,
    // i.e nvidia/aux_definitions.mdl, to pass through as-is all the way to resolve. This included
    // AnchorRelativePath, IsSearchPath and IsRelativePath. For Ar 2, we no longer need to worry about
    // IsSearchPath and IsRelativePath so we should just be able to return it as an identifier
    static bool enabled = TfGetEnvSetting(OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS);
    if (!enabled)
    {
        return false;
    }

    // In the current ecosystem, we really have three different ways to represent
    // MDL asset paths and how they are resolved.
    //
    // 1. A builtin MDL asset path that is a part of the MDL library. These asset paths are a part of the
    //    omni_core_materials package. See omniUsdResolverSetMdlBuiltins / g_builtins
    // 2. An MDL asset path that is authored like a search path meaning it does NOT start with ./ or ../.
    //    But, they are expected to resolve relative to the layer then via search paths i.e @Plants/Plant_A.mdl@
    // 3. An MDL asset path that is authored as a normal relative path meaning it does start with ./ or ../
    //    and just resolves relative to the layer i.e @./Plants/Plant_A.mdl@
    //
    // Sdf has internal logic when dealing with asset paths that look like search paths. It will first look
    // to see if the asset path is next to the current layer but if not it will then use search paths.
    // With a service-backed asset management system like Nucleus this can cause lots of performance problems.
    // For this reason, we have special logic for dealing with these MDL asset path. The core builtin MDL
    // asset paths, like OmniPBR.mdl or nvidia/aux_definitions.mdl, will only be resolved via search paths (i.e #1).
    // MDL asset paths that are not builtins but look like search paths (i.e #2) will be first looked for
    // relative to the layer then resolved via search paths. Finally, relative MDL asset paths will only be resolved
    // relative to the layer (i.e #3).
    if (g_builtins.find(assetPath) != g_builtins.end())
    {
        TF_DEBUG(OMNI_USD_RESOLVER_MDL).Msg("%s: %s is a builtin\n", TF_FUNC_NAME().c_str(), assetPath.c_str());
        return true;
    }

    return false;
}
} // namespace mdl_helper
