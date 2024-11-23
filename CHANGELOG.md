2.2.0
---------
* OMPE-5673: Update omnitrace-sdk-cpp to 1.7.4ed9eb88
* OMPE-25031: Add SECURITY.md
* OMPE-27259: Use the host toolchain for MSVC on Windows
* OSEC-1991: Cleanup unnecessary references
* OSEC-1991: Support distribution branch when making a release

2.1.0
---------
* OMPE-5370: Resurrect OmniUsdWrapperFileFormat for Alembic URLs
* Removed hard dependency on usd_build_bom
* Added Omniverse Product Sample License

2.0.0
-----
* OM-122154: Remove support for Ar 1
* OM-122188: Remove support for .live files (they have moved to a new project)
* OM-82718: use nv-22_11 that has removed usdMdl, for security
* OM-123307: don't replace all occurrences of 'release' in usd package
* OM-123481: Support Maya 2025 maya-usd 0.27.0 (USD 0.23.11 Py3.11)
* OM-123555: Support Max 2025 max-usd 0.27.0 (USD 0.23.11 Py3.11)
* OM-118826: Migrate to usd_build_bom and Gitlab CI
* OMPE-10977: Bump -fabi-version=14 to support _GLIBCXX_USE_CXX11_ABI=1
* OMPE-15188: Fixed repo symstore
* OMPE-15493: Fix OpenAssetForWrite for WriteMode::Update
* OMPE-16448: Disable base url when resolving MDL builtins

1.42.0
------
* Update to client-library v2.45.0
* OM-121079: Drop Python 3.7 Support
* OM-121079: Update carbonite to 160.8
* OM-121079: Update pxr-23_11 to use code signed binaries

1.41.0
----------
* OM-117444: Update carbonite to 160.6
* OM-117444: Update to client-library v2.44.0
* OM-117444: Update flatbuffers and generate OmniObjectsDelta header directly
* OM-116913: Update omnitrace to omnitrace-sdk-cpp
* OM-117073: Support 3DS Max USD Plugin 0.6.0
* OSEC-1100: Code sign Windows binaries
* OM-118538: Added pxr-23_11 with py310 and nopy flavors
* OM-117771: Add flavor blender-23_11 for Python 3.11

1.40.0
------
* Fixed packaging include folder
* OM-116162: Updated blender-23_05 USD package to include usdShaders
* OM-113705: Allow USD to be specified by a root path for local builds
* OM-89820: Added pxr-23_08 with py310 and nopy flavors

1.39.0
------
* DPEP-1660 Added Maxon USD 23.08 nopy build
* OM-104140: Added bentley-23_08 nopy flavor

1.38.0
------
* OM-115998: Fixed large variant crash in Live V2

1.37.0
------

1.36.0
------
* OM-114394: Remove local flatbuffer verification (the server verifies updates).
* OM-114609: Update Houdini USD names
* OM-112012: Merge omni_spectree into omni_usd_live

1.35.0
----------
* OM-111425: Fix Houdini USD debug build
* OM-111804: Update carbonite to 155.1
* OM-95454: Enable CI for Max 2023
* OMFP-2612: Address Checkmarx warnings.
* OM-112043: Fix CreateIdentifier with an empty anchorAssetPath
* OMFP-2679: Update Python 3.10 to 3.10.13+nv3
* OM-113233: Add houdini-23_08 flavor

1.34.0
------
* OM-101289: Updated maya dependencies to use maya-usd 0.24.0.7+tc.4e2edc45
* Update nv-usd 22.11 for OpenSSL 3.0
* OM-109769: Allow user / password to be set from env var in run_tests

1.33.0
----------
* Update to client-library v2.37.0
* OM-110037: Use persistent carb setting as default for live version

1.32.0
----------
* Update to client-library v2.36.0
* Update documentation
* OM-104608: Added pxr-23_02 with py39 flavor
* OM-103440: Added pxr-23_05 with py310 and nopy flavors
* OM-103440: Added pxr-22_11 nopy flavor
* OM-100967: Added unity-22_11 with py310 flavor
* OM-104707: Added python hello world sample
* OM-107728: Added carb setting to toggle between v1 and v2 of the file format converter

1.31.0
------
* OM-OM-92405: Update nv-usd 22.11

1.30.0
----------
* Update to client-library v2.32.0
* OM-92575: Update Python deps to resolve security vulnerabilities
* OM-95841: Update to client-library v2.31.3, remove Maya from test_live_tree
* OM-95910: Add missing omni_usd_live_v* libraries to package.toml
* OM-95531: Added profiler to samples and unit tests
* OM-95027: Added flavor blender-23_05, for Blender's USD 23.05
* OM-95566: Update `_GetModificationTimestamp` to use version with omniverse URLs

1.29.0
----------
* Update to client-library v2.31.0
* OM-90671: Add nv-usd 22.11 with py39 support
* OM-88978: Add maya-21_11 linux flavors using new maya-usd packages
* OM-85020: Fix debug build linking to release version of TBB
* OM-84960: Removed Python 2.7 support.
* OM-54732: Use .live2 extension for multi-threaded live file format
* OM-87181: Added Pixar USD 23.02 Python 3.10 build
* OM-93621: Remove support for SDF_SUPPORTS_FIELD_HANDLES
* OM-92825: Fix CreateIdentifier for search paths in Ar 2.0
* OM-93060: Update maya-usd packages for SWIPAT

1.28.0
----------
* Update to client-library v2.29.0
* OM-88993: Delegate calls to GetModificationTimestamp() for URI resolvers in Ar 1.0
* OM-88978: Add maya-21_11 and maya-22_11 flavors using new maya-usd packages
* OM-54732: Initial folder restructure to support omni_spec library
* OM-54732: Use TF_DEBUG over DEBUG_OUTPUT in live_v1
* OM-54732: Move OmniUsdResolver python bindings into its own dir

1.27.0
----------
* Update to client-library v2.28.0
* OM-74408: Add nv-usd 22.11 with py310 support
* OM-82586: Update to houdini-usd v19.5.534
* HUB-615: Close files in Hub when USD is finished with them.

1.26.0
------
* Update to client-library v2.27.5

1.25.0
------
* Update to client-library v2.27.4
* OM-85437: Error writing to server

1.24.0
----------
* Update to client-library v2.27.3
* OM-72165: Build for Blender's USD 22.11
* OM-81574: Add the USD package's bin/ directory to the PATH for tests
* OM-80852: Add windows 22.08 AR2.0 nopy monolithic TBB 2020.3 stock layout builds

1.23.0
----------
* Update to client-library v2.26.0
* OM-81276: Add IsInert verification to usd-resolver
* OM-80873: Anonymous layer identifier change by OmniUsdWrapperFileFormat
* OM-77527: Check write permissions in CanWriteLayerToPath / CanWriteAssetToPath
* OM-81574: Add linux and windows 22.11 AR2.0 py39 usdview layout builds

1.22.0
----------
* Update to client-library v2.25.0

1.21.0
----------
* Update to client-library v2.24.0
* Fix undefined behavior with Ar 1.0 multiple URI resolvers when specifying more than one URI scheme.

1.20.0
----------
* Update to client-library v2.23.1
* HUB-439: Update Python dependencies.

1.19.0
------
* Update to client-library v2.22.0
* OM-67688: Add omniUsdResolverSetMdlBuiltins and omni.usd_resolver.set_mdl_builtins
* Added NoPy and Py37 builds for Isotropix
* Add Py39 builds of 22.08 for Unreal Engine 5.1

1.18.0
------
* Update to client-library v2.21.0
* OM-67700: Revert stripping SDF_FORMAT_ARGS from AnchorRelativePath
* OM-66285: Security Update in Carbonite SDK and Omnitrace dependencies
* OM-60387: Added missingURLs test to verify that OmniUsdResolver does not hang

1.17.0
------
* Update to client-library v2.20.0

1.16.0
------
* Update to client-library v2.19.0
* OM-65631: Moved Python to redist.packman.xml
* OM-27116: Update to nv-usd 20.08 to build 2531.5c573672 for displayName metadata support on prims
* OM-54444: Python 3.10.5 build

1.15.0
------
* Update to client-library v2.18.0
* OM-63841: Update to nv-usd 20.08 to build 2474.da4596af for URL-encoded layer identifier support
* OM-63507: Strip :SDF_FORMAT_ARGS: in FetchToLocalResolvedPath

1.14.0
------
* OM-62560: Update to houdini_usd-19.0.720.1 (USD 21.08) and houdini_usd-19.5.368.1 (USD 22.05)
* OM-58293: Added file size info to usd resolver state callbacks

1.13.0
------
* Update to client-library v2.17.0
* OM-58159: Integration Tests for Additional Resolver Implementations
* OM-61860: Added support for custom URI resolvers in Ar 1.0

1.12.0
------
* Update to client-library v2.16.0
* Added omniUsdResolverGetVersionString() function and Python binding
* Changed nv-usd 20.08 to build 2404.6bf74556 so that it matches Kit 103.5
* OM-58159: Integration Tests for Additional Resolver Implementations

1.11.0
------
* Update to client-library v2.15.0
* OM-60035: Use explicit core MDL material paths for bypass
* OM-60573: Prevent resolves with no extension to call SdfFileFormat::FindByExtension

1.10.0
------
* Update to client-library v2.14.0
* CC-836: Don't fetch packages (usdz, etc) during Resolve
* OM-57318: Add support for Houdini 19.5 - USD 22.05 AR 2 - Python 3.9
* CC-769: Added omniUsdResolverRegisterStateCallback
* OM-51275: Add python3.7 to pxr-21_11 flavor

1.9.0
-----
* Update to client-library v2.13.0
* OM-55418: Skip SdfFileFormats that are packages from OmniUsdWrapperFileFormat
* OM-57476: Fixed an issue that Houdini omni-usd-resolver was built against degbug lib files on Linux.
* OM-57674: Fix crashes that can occur when the usd-resolver DLL/SO is unloaded
* OM-53578: Supporting Ar 2.0 and added pxr-22_05b-ar2 flavor (python 3.7 only)
* OM-51275: Added pxr-21_11 flavor (python 3.9 only)
* OM-57786: Added prefixed-nv-20_08 flavor (python 3.7 windows only)
* OM-47199: Fix MDL handling for non-builtin asset paths

1.8.0
-----
* Update to client-library v2.12.0
* Add support for Houdini on Linux
* CC-709: Handle Nucleus returning sequence=0 from "update_object" (fixes a desync in live mode)
* CC-547: Fix layer reload with live layers
* CC-575: Fix layer->Clear() with live layers

1.7.0
-----
* Update to client-library v2.11.1
* OM-53998: Fix for resolving assets inside USDZ archives

1.6.0
-----
* Update to client-library v2.11.0
* Include omni_usd_resolver.lib in packman package
* CC-710: Add support for SdfTimeCode, and fixed crash when attempting to read an unsupported type

1.5.0
-----
* OM-47199: pathological subscribe_list() calls to the server when loading scenes
* OM-47199: Adds tests for OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS
* OM-47199: Updated README for testing MRs against kit

1.4.0
-----
* Update to client-library v2.10.0
* CC-566: Added documentation! http://omniverse-docs.s3-website-us-east-1.amazonaws.com/usd_resolver/

1.3.0
-----
* Update to client-library v2.9.0
* Include python bindings in package.

1.2.0
-----
* Update to client-library v2.8.0
* OM-49309: Unable to find correct identifier from within a SdfFileFormat plugin
* CC-501: Use omniClientBreakUrlAbsolute
* CC-504: Add omniUsdResolverSetCheckpointMessage function. This function allows users to set the checkpoint message
          that will be used for atomic checkpoints when saving files. Python bindings are also created for this funciton.

1.1.0
-----
* Update to client-library v2.7.0
* OM-48244: Added Python 3.9 support.
* OM-45532: Fix layer->Reload not sending change notifications for non-live omniverse files.
* OM-48916: Fix SdfLayer::CreateNew after SdfLayer::FindOrOpen while an ArResolverScopeCache is active.
* CC-436: Fix loading a layer from local disk with % characters in their file names.

1.0.0
-----
* Update to client-library v2.6.0
* CC-333: Split omni-usd-resolver from client-library v2.5.0
* CC-380: Fix OpenAsAnonymous
* CC-438: Fix GetExtension with non-layers
