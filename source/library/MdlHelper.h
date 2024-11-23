// SPDX-FileCopyrightText: Copyright (c) 2020-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
// SPDX-License-Identifier: LicenseRef-NvidiaProprietary
//
// NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
// property and proprietary rights in and to this material, related
// documentation and any modifications thereto. Any use, reproduction,
// disclosure or distribution of this material and related documentation
// without an express license agreement from NVIDIA CORPORATION or
// its affiliates is strictly prohibited.

#include <string>

namespace mdl_helper
{
/// \brief Determines if the provided assetPath is a valid MDL identifier.
/// A valid MDL identifier is an assetPath with the following properties:
///     1. It does not start with a ".", "../" or "/"
///     2. It is not a URL such as omniverse://path.mdl or https://path.mdl
///     3. It ends with .mdl
/// \note This requires calling omniUsdResolverSetMdlBuiltins
/// If not, this function will return false
/// \param assetPath the path to determine if it is a MDL identifier
/// \return true if the assetPath is an MDL identifier. Otherwise, false.

bool IsMdlIdentifier(const std::string& assetPath);

} // namespace mdl_helper
