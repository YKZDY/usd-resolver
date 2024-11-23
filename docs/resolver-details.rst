OmniUsdResolver Details
-----------------------

`ArResolver` was originally designed so a single studio could hook up their asset management system to USD.
For the most part, this was a fairly trivial process for a studio to set up. Very rarely did one studio's assets
need to work with a different studio's assets in USD. As USD has been incorporated into more industries those
needs to consume assets from multiple asset management systems became a necessity.
The second iteration for `ArResolver`, Ar 2.0, addressed this requirement by implementing the notion
of :ref:`Primary <Primary Resolvers>` and :ref:`URI <URI Resolvers>` Resolvers. This separation allowed
multiple `ArResolver` plugins to live in the same USD environment and Ar would dispatch resolves to the correct
`ArResolver` plugin. Even though the API between Ar 1.0 and Ar 2.0 is quite different the underlying concepts are similar.
The following sections describe these concepts and how they apply to the different versions of Ar

.. _Asset Paths:

Asset Paths
^^^^^^^^^^^

Asset Paths are a familiar part of scene description that are commonly authored in USD. They are a special type of
string which indicates to USD that they need to be identified and located by asset resolution. The documentation
describes what an `Asset <https://openusd.org/release/glossary.html#asset>`_ means in USD along with the Asset Path
which represents it.

SdfFileFormat Arguments
"""""""""""""""""""""""

`SdfFileFormat` plugins are a powerful concept in USD but are beyond the scope of this documentation. What is important
for `SdfFileFormat` plugins in regards to `ArResolver` are the `SdfFileFormat` arguments that can be authored as part of
the :ref:`Asset Path <Asset Paths>`. These `SdfFileFormat` arguments are important to the underlying `SdfFileFormat` plugin that will be used
to load the actual Asset. There are two important aspects to an :ref:`Asset Path <Asset Paths>` authored with `SdfFileFormat` arguments:

#. An `ArResolver` plugin must respect incoming :ref:`Asset Paths <Asset Paths>` that may, or may not, have `SdfFileFormat` arguments. These `SdfFileFormat` arguments are usually seen when creating the :ref:`Asset Identifier <Asset Identifiers>` from the :ref:`Asset Path <Asset Paths>` (`CreateIdentifier` / `AnchorRelativePath`) or computing the :ref:`Resolved Path <Resolved Paths>` (`Resolve` / `ResolveWithAssetInfo`)

#. The `SdfFileFormat` arguments are tied to the identity of the Asset which means that the same :ref:`Asset Path <Asset Paths>` with different `SdfFileFormat` arguments is a completely different Asset.

    Examples of Asset Paths that contain SdfFileFormat arguments:

        * @./foo/bar/asset.sff:SDF_FORMAT_ARGS:arg1=baz@
        * @./foo/bar/asset.sff:SDF_FORMAT_ARGS:arg1=boo@

The `OmniUsdResolver` will maintain the `SdfFileFormat` arguments when creating the :ref:`Asset Identifier <Asset Identifiers>` from the :ref:`Asset Path <Asset Paths>`.
So it should be expected that the returned :ref:`Asset Identifier <Asset Identifiers>` can contain the `SdfFileFormat` arguments. However, the
`SdfFileFormat` arguments will be stripped out when computing and returning the :ref:`Resolved Path <Resolved Paths>`.


.. _Asset Identifiers:

Asset Identifiers
^^^^^^^^^^^^^^^^^

Asset Identifiers are not explicitly referred to throughout USD but they are important for uniquely identifying an Asset.
In most cases, an :ref:`Asset Path <Asset Paths>` does not point to a specific Asset and requires additional information so the Asset can be
resolved. For example, a relative file path authored as an :ref:`Asset Path <Asset Paths>` requires the containing Layer :ref:`Asset Identifier <Asset Identifiers>` so
it can be properly anchored and resolved. The result returned from `CreateIdentifier` / `AnchorRelativePath` is usually
the :ref:`Asset Identifier <Asset Identifiers>` computed from the :ref:`Asset Path <Asset Paths>` and anchoring
:ref:`Asset Identifier <Asset Identifiers>`.

Ar 2.0 introduced new API around creating Asset Identifiers for new Assets. `CreateIdentifierForNewAsset` is a place for
an `ArResolver` plugin to perform any sort of initialization when an Asset is about to be created. The initialization
performed in `CreateIdentifierForNewAsset` is completely up to the plugin and can be as simple, or as complex,
as necessary for the new Asset. `OmniUsdResolver (Ar 2.0)` does not perform any special initialization in
`CreateIdentifierForNewAsset` and is functionally equivalent to `CreateIdentifier`.

.. _Resolved Paths:

Resolved Paths
^^^^^^^^^^^^^^

Resolved Paths are the computed result when an :ref:`Asset Identifier <Asset Identifiers>` is resolved. In Ar 2.0 Resolved Paths are explicitly
typed as `ArResolvedPath` but are really a wrapper around a normal `std::string`. The explicit `ArResolvedPath` type
helps inform APIs what to expect about the Asset. When an API specifies an `ArResolvedPath` it indicates to the caller,
or implementer, that the Asset is expected to have already gone through Asset Resolution.

    The lack of an explicit :ref:`Resolved Path <Resolved Paths>` type like `ArResolvedPath` in Ar 1.0 made it difficult for a `ArResolver`
    implementation to know the state of an incoming path. Everything was just a `std::string` that could be either
    an :ref:`Asset Path <Asset Paths>`, :ref:`Asset Identifier <Asset Identifiers>` or Resolved Path. The explicit type in Ar 2.0 really helped clarify the expectation
    of an incoming or outgoing path.

In a similar manor as `CreateIdentifierForNewAsset`, Ar 2.0 introduced `ResolveForNewAsset`. In most cases, the normal
call to `Resolve` / `ResolveWithAssetInfo` would perform some sort of existence check on the Asset to return the
:ref:`Resolved Path <Resolved Paths>` successfully. But for new Assets it's quite common that they might not exist, as they are still in the
process of being created, but need to resolve to some different result than the Asset Identifier. The new
`CreateIdentifierForNewAsset` / `ResolveForNewAsset` API allows for an `ArResolver` plugin to completely handle the
creation of new Assets. `OmniUsdResolver (Ar 2.0)` does not do any existence checking but does make sure to
completely resolve the URL performing any necessary normalization.

.. _Search Paths:

Search Paths
^^^^^^^^^^^^

A concept that Pixar uses for it's own Asset Resolution purposes is Search Paths. Search Paths are a special type of :ref:`Asset Path <Asset Paths>`
that require a method of "searching" to find the actual Asset. They require an :ref:`Asset Path <Asset Paths>` to be authored in a special
way and configuration that determines where these Search Paths will be searched for. The syntax to author a Search Path
as an :ref:`Asset Path <Asset Paths>` is similar to a normal relative file path, it just requires the that the :ref:`Asset Path <Asset Paths>` is authored
**without** a *./* or *../* prefix. The other requirement is a list of paths for the Search Path to be "searched" against
which need to be set on the `ArResolver`. The method to set these paths is specific to the `ArResolver` implementation.
The `ArDefaultResolver` allows for these paths to be set from an environment variable (*PXR_AR_DEFAULT_SEARCH_PATH*)
or creating the `ArDefaultResolverContext` directly.

    Example of Asset Paths authored as Search Paths:
        @vehicles/vehicle_a/asset.usd@

    Set of paths that Search Paths will be configured to search against:
        * /fast-storage-server/assets/
        * /normal-storage-server/assets/
        * /slow-storage-server/assets/

    Search Path *vehicles/vehicle_a/asset.usd* will be searched for in the following order:
        * /fast-storage-server/assets/vehicles/vehicle_a/asset.usd
        * /normal-storage-server/assets/vehicles/vehicle_a/asset.usd
        * /slow-storage-server/assets/vehicles/vehicle_a/asset.usd

`OmniUsdResolver` also supports Search Paths indirectly through the `Omniverse Client Library`. The paths used for "searching" are
set explicitly by calling `omniClientAddDefaultSearchPath (C++) <https://docs.omniverse.nvidia.com/kit/docs/client_library/latest/_build/docs/client_library/latest/function_group__resolve_1ga7189457cb9f538843d05f9c7e5e05c45.html#exhale-function-group-resolve-1ga7189457cb9f538843d05f9c7e5e05c45>`_
or `omni.client.add_default_search_path (Python) <https://docs.omniverse.nvidia.com/kit/docs/client_library/latest/docs/python.html#omni.client.add_default_search_path>`_.
The `OmniUsdResolver` will respect these configured paths when resolving a Search Path.

.. _Look Here First:

Look Here First Strategy
""""""""""""""""""""""""

Search Paths were a formal concept in Ar 1.0 that required `ArResolver` implementations to acknowledge them regardless
if they were supported or not. To make matters worse, the `Sdf` library in USD also had some of it's own logic to handle
a "Look Here First" approach for :ref:`Search Paths <Search Paths>`. This "Look Here First" strategy would simply treat the :ref:`Search Path <Search Paths>` as a
normal relative file path and create an :ref:`Asset Identifier <Asset Identifiers>` from the `SdfLayer` containing the Search Path. This
anchored :ref:`Asset Identifier <Asset Identifiers>` would then be resolved to determine existence, and if it did exist the "searching" was done.
For the most part the "Look Here First" behaved as one would expect, since they have the appearance of a relative file
path, but there were a couple problems with it:

#. Asset Resolution was not entirely handled by the underlying `ArResolver`. The "Look Here First" resolution step was done in `Sdf` while the "searching" was handled in `ArResolver`. To do this the `ArResolver` API required methods for determining if an :ref:`Asset Path <Asset Paths>` was indeed a Search Path, regardless if they were supported.
#. It assumes a file-based asset management system being hosted on a really fast file server where latency isn't too much of a concern. For cloud-based asset management systems latency is a much larger issue.

    Going back to the example above with a Search Path of:
        @vehicles/vehicle_a/asset.usd@

    Authored in the following SdfLayer:
        omniverse://server-a/scenes/scene.usd

    Set of paths that Search Paths will be configured to search against:
        * /fast-storage-server/assets/
        * /normal-storage-server/assets/
        * /slow-storage-server/assets/

    The **actual** set of paths that Search Path *vehicles/vehicle_a/asset.usd* will be searched for:
        * **omniverse://server-a/scenes/vehicles/vehicle_a/asset.usd**
        * /fast-storage-server/assets/vehicles/vehicle_a/asset.usd
        * /normal-storage-server/assets/vehicles/vehicle_a/asset.usd
        * /slow-storage-server/assets/vehicles/vehicle_a/asset.usd

**This "Look Here First" strategy is the core issue with MDL Paths for Omniverse in USD which has led to performance problems, bugs and confusion.**

.. _MDL Paths:

MDL Paths
^^^^^^^^^

Before getting into how `OmniUsdResolver` works with both MDL Paths and Search Paths to ensure that everything resolves
efficiently, its best to modify the previous Search Path example to incorporate a core MDL module to highlight
the problem:

    Example of a core MDL module authored as a Search Path:
        @nvidia/core_definitions.mdl@

    Authored in the following SdfLayer:
        omniverse://server-a/scenes/vehicles/vehicle_a/asset.usd

    Set of paths that Search Paths will be configured to search against:
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/mdl/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Volume/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/VRay/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Ue4/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Base/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/rtx/iray/
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/rtx/

    The **actual** set of paths that MDL Path *nvidia/core_definitions.mdl* will be searched for:
        * **omniverse://server-a/scenes/vehicles/vehicle_a/nvidia/core_definitions.mdl**
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/mdl/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Volume/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/VRay/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Ue4/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/core/Base/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/rtx/iray/nvidia/core_definitions.mdl
        * /local-server/share/ov/pkgs/omni_core_materials/mdl/rtx/nvidia/core_definitions.mdl

On the surface, everything with how the core MDL module *nvidia/core_definitions.mdl* is authored in USD seems fine. It's a
normal Search Path that uses the configured paths from the *omni_core_materials* package to search for the correct core
module on disk. However, the fundamental problem is the :ref:`Look Here First <Look Here First>` strategy that will **always** search for the
core MDL module relative to where the Search Path is authored. In the example above this would be
*omniverse://server-a/scenes/vehicles/vehicle_a/nvidia/core_definitions.mdl* which will fail to resolve as core MDL
modules are intended to be a part of a library shared across applications. Now, why is this failed resolve such an issue?

#. In order to resolve *omniverse://server-a/scenes/vehicles/vehicle_a/nvidia/core_definitions.mdl* the Nucleus server hosted at *server-a* needs to be consulted to determine existence. This will introduce latency, based on proximity to the server, for something that will always fail to resolve.
#. `OmniUsdResolver` does not cache failed resolves which means that every core MDL module authored will be impacted by latency. The latency is based on the `SdfLayer` where the core MDL module is authored, for an `SdfLayer` that is hosted on a cloud-based asset management system like Nucleus this latency can really impact performance.

    `OmniUsdResolver` does not cache failed resolves as there is not a good way to determine cache invalidation. Doing
    so can lead to lots of undesirable issues such as restarting the process so a previous resolve can be recomputed.
    If failed resolves need to be cached, calling code can use `ArResolverScopedCache` to control the cache lifetime
    which will respect any failed resolves.

#. The number of materials using core MDL modules in a composed USD stage can be large. With a cloud-based asset management system the number of requests can flood the server causing slow-down on the server itself.

Now that there is a better description of the problem between MDL Paths and Search Paths its a good time to look at how
`OmniUsdResolver` handles it.

OmniUsdResolver MDL Path Strategy
"""""""""""""""""""""""""""""""""

The way that `OmniUsdResolver (Ar 1.0)` must optimize for MDL Paths is much more involved than how
`OmniUsdResolver (Ar 2.0)` needs to handle it. The reason for this is that the :ref:`Look Here First <Look Here First>` strategy for Search
Paths is codified in `Sdf` for Ar 1.0. In Ar 2.0, `Sdf` no longer makes that requirement and it's up to the `ArResolver`
implementation to enforce that or not. The focus will be on `OmniUsdResolver (Ar 1.0)` to describe the solution then
compare that with how it has been improved in Ar 2.0 with `OmniUsdResolver (Ar 2.0)`.

To optimize MDL Paths in `OmniUsdResolver (Ar 1.0)` we have the following requirements:

#. Core MDL modules **should not** be resolved relative to the `SdfLayer` they are authored in.

    From the example, completely eliminate the resolve call for
    *omniverse://server-a/scenes/vehicles/vehicle_a/nvidia/core_definitions.mdl*

#. Core MDL modules **should** resolve according to the configured list of paths to be "searched".
#. User-defined MDL modules authored as Search Paths (no *./* or *../* prefix) should still use the :ref:`Look Here First <Look Here First>` strategy for backwards compatibility.

    This requirement may be dropped in the future as Asset Validation can update old Assets to correct these paths
    to normal file relative paths.

#. MDL modules authored as normal file relative paths (prefixed with *./* or *../*) should be anchored to the `SdfLayer` they are authored in.
#. Avoid making changes to `Sdf` specific to MDL modules

To satisfy the first and third requirements, `OmniUsdResolver (Ar 1.0)` needs to be bootstrapped with the core MDL
modules that should not be resolved relative to `SdfLayer` they are authored in. There are two ways that this can be
done:

    Unfortunately, there is not a better way to do this as core MDL modules can be added, removed or even versioned
    as a whole from the package that hosts them. If the third requirement from above can be dropped this will no longer
    be necessary

#. By explicitly setting the list of MDL modules paths in `omniUsdResolverSetMdlBuiltins` which is declared in `OmniUsdResolver.h`
#. Through the environment variable *OMNI_USD_RESOLVER_MDL_BUILTIN_PATHS* which are the comma-separated MDL module paths.

    The explicit call to `omniUsdResolverSetMdlBuiltins` takes priority over the environment variable.

With the core MDL modules bootstrapped, `OmniUsdResolver (Ar 1.0)` uses these paths in `AnchorRelativePath`,
`IsSearchPath` and `IsRelativePath` to quickly determine if the incoming :ref:`Asset Path <Asset Paths>` matches one of these core MDL
module paths. So when `Sdf` calls `AnchorRelativePath` with a core MDL module path it will return the path as-is,
meaning that `Sdf` has no way to anchor a core MDL module path from the `SdfLayer` it is authored in. `IsSearchPath`
will always return `false` when called with a core MDL module path but `true` when called with a user-defined MDL module
path. This is to ensure that the third requirement from above works with the :ref:`Look Here First <Look Here First>`
strategy. Finally, `IsRelativePath` will also return `false` for core MDL modules paths to prevent any normalization
in `Sdf`.

As convoluted as the logic is, the thing to remember is this:
**OmniUsdResolver (Ar 1.0) needs to ensure that the core MDL module paths are returned as-is until ResolveWithAssetInfo is called**.
`ResolveWithAssetInfo` is the process that will compute the :ref:`Search Path <Search Paths>` against the configured list of paths to be
"searched". At this point the absolute path to the MDL module, wherever it is, should be returned.

`OmniUsdResolver (Ar 2.0)` greatly simplifies this process. First, `Sdf` no longer applies the :ref:`Look Here First <Look Here First>`
strategy, that is completely handled in `ArDefaultResolver`. This means that we only need to check if something is a
core MDL module path in `CreateIdentifier`. If it is a core MDL module path `OmniUsdResolver (Ar 2.0)` will just return
it as-is. `Sdf` will then use that as the :ref:`Asset Identifier <Asset Identifiers>` and resolve it as needed.

    For whatever reason, all this logic can be turned on, or off, by setting the environment variable
    *OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS* to a truth-like, or false-like, value. i.e

        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=1
        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=0
        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=ON
        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=OFF
        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=TRUE
        * OMNI_USD_RESOLVER_MDL_BUILTIN_BYPASS=FALSE

Troubleshooting MDL Paths
"""""""""""""""""""""""""

Due to all the complexity between :ref:`Search Paths <Search Paths>` and :ref:`MDL Paths <MDL Paths>` problems do arise
and it's not always clear where the problem might be. It might be on the USD side when resolving the :ref:`MDL Path <MDL Paths>`
or it might be on the MDL side where the :ref:`Search Paths <Search Paths>` are configured. Either way, there are a
couple easy things to do to at least see where the problem might be.

First would be to make sure the :ref:`Search Paths <Search Paths>` are configured properly:

.. code-block:: python

    import omni.client

    # get all the configured search paths from omni.client
    search_paths = omni.client.get_default_search_paths()

    # print out the list of paths that will be search when resolving a MDL Path
    for search_path in search_paths:
        print(search_path)

The output list of paths gives a starting point to see if the :ref:`MDL Path <MDL Paths>` is somewhere in those
directories. If the :ref:`MDL Path <MDL Paths>` is not in any of those paths, it's pretty safe to assume that the
problem is on the configuration side of MDL.

Now if the :ref:`MDL Path <MDL Paths>` is in one of those paths the problem is more than likely on the USD side. That
is still a pretty large space which could be narrowed down further. The next step would be to see if the
:ref:`MDL Path <MDL Paths>` is being resolved correctly from `ArResolver`:

.. code-block:: python

    from pxr import Ar

    # assuming that we are running in a Omniverse Kit-based Application
    # we get the UsdStage that the MDL Path is authored on
    import omni.usd
    stage = omni.usd.get_context().get_stage()
    anchor_path = stage.GetRootLayer().resolvedPath

    # get the configured ArResolver instance
    resolver = Ar.GetResolver()

    # create the asset identifier for the MDL Path
    # we'll use the OmniPBR.mdl module as an example
    asset_path = "OmniPBR.mdl"
    asset_id = resolver.CreateIdentifier(asset_path, anchor_path)

    # check to see if the MDL Path has been anchored or not
    print(asset_id)

    # verify that the MDL Path can be resolved
    resolved_path = resolver.Resolve(asset_id)
    print(resolved_path)

If everything looks to be behaving correctly, the *asset_id* is not anchored and *resolved_path* is correct, the problem
is probably not with resolving the :ref:`MDL Path <MDL Paths>`. The problem could be related to a load-order issue where
rendering code is resolving the :ref:`MDL Path <MDL Paths>` before all the :ref:`Search Paths <Search Paths>` are
configured. Regardless, it helps narrow down where the problem might be and possibly what teams to engage with.

Another way to observe what might be happening is to enable some `TfDebug` flags for `OmniUsdResolver`. Specifically,
the `OMNI_USD_RESOLVER` flag that outputs a lot of general information when `OmniUsdResolver` is being invoked. The
messages that it writes to the console easily show the input it receives along with the output they produce. Sometimes
enabling this `TfDebug` flag will quickly point out the issue.

See :ref:`Troubleshooting <Troubleshooting>` for more details about `TfDebug` flags.

.. _Package Relative Paths:

Package-Relative Paths
^^^^^^^^^^^^^^^^^^^^^^

Package-Relative Paths are a special form of :ref:`Asset Paths <Asset Paths>` that reference an Asset within another
Asset. These paths are special in the fact that they only have meaning within an Asset whose underlying `SdfFileFormat`
plugin supports packages. The most common `SdfFileFormat` plugin that supports packages is the `UsdzFileFormat` plugin.

In most cases, developers don't need to deal with Package-Relative Paths. `Sdf` does the heavy-lifting when loading an
Asset that is represented by a `SdfFileFormat` plugin which supports packages. Conversely, when saving the Asset utility
functions are usually provided so the Asset is properly packaged. `UsdZipFileWriter` in `UsdUtils` is a perfect example
of this.

    There is no limit to the level of nesting that a Package-Relative Path can represent. But directly authoring and
    parsing of these paths should be avoided. It's encouraged to use the utility functions in `Ar` to handle any sort of
    interaction with these paths. See `ArIsPackageRelativePath`, `ArJoinPackageRelativePath`,
    `ArSplitPackageRelativePathOuter` and `ArSplitPackageRelativePathInner` in `<pxr/usd/ar/packageUtils.h>`


.. _Reading Assets:

Reading Assets
^^^^^^^^^^^^^^

Reading Assets is a very important role for an `ArResolver` plugin. When a `ArResolver` plugin returns a Resolved Path
that path may, or may not, point to a file on disk. Regardless, the Asset pointed to by :ref:`Resolved Path <Resolved Paths>` needs to be read
into memory in order for USD to load the Asset. `ArResolver` provides API for opening that :ref:`Resolved Path <Resolved Paths>` via `OpenAsset`
and returns a handle, so to speak, to an `ArAsset`. The `ArAsset` abstraction is the API that USD will use for reading
that Asset into memory.

    The API around reading Assets in `ArResolver` is mostly the same from Ar 1.0 to Ar 2.0, so no
    special distinction will be made between `OmniUsdResolver (Ar 1.0)` and `OmniUsdResolver (Ar 2.0)`.

`OmniUsdResolver` supports both file paths and URLs which can live on Nucleus or HTTP. How the Asset will be
opened for reading will depend on what the :ref:`Resolved Path <Resolved Paths>` points to. When :ref:`Resolved Path <Resolved Paths>` points to a normal file path
`OmniUsdResolver` will open the Asset for reading with `ArFilesystemAsset`. But when :ref:`Resolved Path <Resolved Paths>` points to a URL,
hosted on either Nucleus or HTTP, `OmniUsdResolver` will use `OmniUsdAsset` to read the Asset. Ultimately, the
caller to `OpenAsset` will read the Asset in the same way since both `ArFilesystemAsset` and `OmniUsdAsset` are
implemented via the `ArAsset` abstraction.

`OmniUsdAsset` provides efficient reading of Assets hosted on Nucleus or HTTP. To optimize performance with USD,
`OmniUsdAsset` will download the content of the Asset to a file on disk and return a memory-mapped buffer for that file.
The local file on disk serves multiple purposes:

#. As a caching mechanism so reads to the same Asset are not re-downloaded
#. OS level support for memory-mapped files
#. Reduce traffic to Nucleus or HTTP with subsequent reads for the Asset

A trivial example for reading an Asset hosted on Nucleus would be:

    The `ArResolver` API for reading (`OpenAsset`) and writing (`OpenAssetForWrite`) Assets are only available in C++.

.. code-block:: c++

   ArResolver& resolver = ArGetResolver();

   const std::string assetId = "omniverse://server-a/scenes/vehicles/vehicle_a/asset_metadata.dat";
   std::shared_ptr<ArAsset> asset = resolver.OpenAsset(resolver.Resolve(assetId));
   if (asset) {
       // get the buffer to read data into
       std::shared_ptr<const char> buffer = asset->GetBuffer();
       if (!buffer) {
           TF_RUNTIME_ERROR("Failed to obtain buffer for reading asset");
           return;
       }

       size_t numBytes = asset->Read(buffer.get(), asset->GetSize(), 0);
       if (numBytes == 0) {
           TF_RUNTIME_ERROR("Failed to read asset");
       }

       // buffer should now contain numBytes chars read from resolvedPath
   }

Opening UsdStage
""""""""""""""""

Now that there is a better understanding of the concepts that apply to Asset Resolution in USD, its a good time to look
at the various APIs that go into opening a UsdStage, eg. `UsdStage::Open()`:

.. mermaid::

   sequenceDiagram
        %%{init: { 'theme': 'forest' } }%%
        autonumber
        actor Alice
        Alice->>UsdStage: Open(assetPath)
        UsdStage->>SdfLayer: FindOrOpen(assetPath)
        SdfLayer->>ArResolver: ArGetResolver()
        ArResolver-->>SdfLayer: resolver
        SdfLayer->>ArResolver: CreateIdentifier(assetPath)
        ArResolver-->>SdfLayer: assetIdentifier
        SdfLayer->>ArResolver: Resolve(assetIdentifier)
        ArResolver-->>SdfLayer: resolvedPath
        SdfLayer->>SdfFileFormat: FindByExtension(resolvedPath)
        SdfFileFormat->>ArResolver: GetExtension(resolvedPath)
        ArResolver-->>SdfFileFormat: extension
        SdfFileFormat-->>SdfLayer: fileFormat
        loop TryToFindLayer
            SdfLayer->>SdfLayer: Check for Matching Layer
        end
        SdfLayer-->>UsdStage: layer
        UsdStage-->>Alice: stage
        SdfLayer->>SdfFileFormat: fileFormat->NewLayer(assetIdentifier, resolvedPath)
        SdfFileFormat-->>SdfLayer: layer
        SdfLayer->>SdfFileFormat: fileFormat->Read(layer, resolvedPath)
        SdfFileFormat->>ArResolver: resolver->OpenAsset(resolvedPath)
        ArResolver-->>SdfFileFormat: asset
        SdfFileFormat-->>SdfLayer: layer
        SdfLayer-->>UsdStage: layer
        UsdStage->>ArResolver: CreateDefaultContextForAsset(assetIdentifier) | CreateDefaultContext()
        ArResolver-->>UsdStage: resolverContext
        UsdStage->>ArResolver: BindContext(resolverContext)
        UsdStage-->>Alice: stage

The diagram above is not exhaustive in the sense of showing all the different edge cases and internal APIs called. But
it shows the sequence of events and the main collaboration between the `Usd`, `Sdf` and `Ar` APIs

Writing Assets
^^^^^^^^^^^^^^

Writing Assets is also a very important role for an `ArResolver` plugin but was not easily done in Ar 1.0. Ar 2.0
acknowledged this deficiency and added direct support to the `ArResolver` API. `OpenAssetForWrite` and `ArWritableAsset`
are the equivalent APIs for writes as `OpenAsset` and `ArAsset` are for reads. `OmniUsdResolver (Ar 2.0)` provides the
`OmniUsdWritableAsset` implementation of `ArWritableAsset` to write Assets to Nucleus (we don't support writing to HTTP). Similar to
`OpenAsset`, `OpenAssetForWrite` will use `ArFilesystemWritableAsset` for :ref:`Resolved Paths<Resolved Paths>` that
point to a normal file path.

To keeps things fast and efficient, `OmniUsdWritableAsset` does not write directly to the remote host. Instead, a
temporary file will be opened for writing when `OpenAssetForWrite` is called. All subsequent writes through
`OmniUsdWritableAsset` will write to this temporary file then it will be moved to the remote host when the Asset is
closed via `Close`. The process and API is quite simple for writing Assets and is a welcomed addition to support content
that is hosted remotely on services such as Nucleus.

An area where writing Assets deviates from reading Assets is with checking for write permission. It's not uncommon to
lock an Asset to prevent accidental writes. The `ArResolver` API exposes a method that an `ArResolver` plugin can
implement to properly check write permissions on an Asset before any writes take place. `CanWriteAssetToPath` /
`CanWriteLayerToPath` are implemented in `OmniUsdResolver` to check write permissions on an Asset and optionally report
back any reason why an Asset can not be written to.

    `OmniUsdResolver` tries to be as robust as possible for checking write permissions on an Asset, handling edge cases
    such as trying to write a file to a channel or writing a file underneath a directory that has been locked. The
    reason why a write can not occur for a given Asset can be obtained by the caller

A trivial example for writing an Asset to Nucleus would be:

    The `ArResolver` API for reading (`OpenAsset`) and writing (`OpenAssetForWrite`) Assets are only available in C++.

.. code-block:: c++

   ArResolver& resolver = ArGetResolver();

   const std::string assetId = "omniverse://server-a/scenes/vehicles/vehicle_a/asset_metadata.dat";

   // assume we are writing to a new Asset
   const ArResolvedPath resolvedPath = ArGetResolver().ResolveForNewAsset(assetPath);

   // before writing any data check that we have permission to write the Asset
   std::string reason;
   if (!resolver.CanWriteAssetToPath(resolvedPath, &reason)) {
       TF_RUNTIME_ERROR("Unable to write asset %s, reason: %s", resolvedPath.c_str(), reason.c_str());
       return;
   }

   // create a buffer that contains the data we want to write
   const size_t bufferSize = 4096;
   std::unique_ptr<char[]> buffer(new char[bufferSize]);

   // put some data into the buffer
   const std::string data = "some asset data";
   memcpy(buffer.get(), data.c_str(), data.length());

   // open the asset for writing
   auto writableAsset = ArGetResolver().OpenAssetForWrite(resolvedPath, ArResolver::WriteMode::Replace);
   if (writableAsset) {
       // write the data from our buffer to the Asset
       const size_t numBytes = writableAsset->Write(buffer.get(), data.length(), 0);
       if (numBytes == 0) {
           TF_RUNTIME_ERROR("Failed to write asset");
           return;
       }

       // close out the asset to indicate that all data has been written
       bool success = asset->Close();
       if (!success) {
           TF_RUNTIME_ERROR("Failed to close asset");
           return;
       }
   }

Creating UsdStage
"""""""""""""""""

Similar to what was examined with :ref:`Reading Assets<Reading Assets>` it's also a good point to look at the different
APIs that come into play when dealing with writes. For example when calling `UsdStage::CreateNew()`:

.. mermaid::

   sequenceDiagram
        %%{init: { 'theme': 'forest' } }%%
        autonumber
        actor Bob
        Bob->>UsdStage: CreateNew(assetPath)
        UsdStage->>SdfLayer: CreateNew(assetPath)
        SdfLayer->>ArResolver: ArGetResolver()
        ArResolver-->>SdfLayer: resolver
        SdfLayer->>ArResolver: CreateIdentifierForNewAsset(assetPath)
        ArResolver-->>SdfLayer: assetIdentifier
        SdfLayer->>ArResolver: ResolveForNewAsset(assetIdentifier)
        ArResolver-->>SdfLayer: resolvedPath
        SdfLayer->>SdfFileFormat: FindByExtension(resolvedPath)
        SdfFileFormat->>ArResolver: GetExtension(resolvedPath)
        ArResolver-->>SdfFileFormat: extension
        SdfFileFormat-->>SdfLayer: fileFormat
        loop IsPackageOrPackagedLayer
            SdfLayer->>SdfLayer: Prevent Creating Package Layers
        end
        SdfLayer-->>UsdStage: Null Layer (IsPackage)
        UsdStage-->>Bob: Null Stage
        SdfLayer->>SdfFileFormat: NewLayer(assetIdentifier, resolvedPath)
        SdfFileFormat-->>SdfLayer: layer
        SdfLayer->>ArResolver: CanWriteAssetToPath(resolvedPath)
        SdfLayer->>SdfFileFormat: WriteToFile(layer, resolvedPath)
        SdfFileFormat->>ArResolver: OpenAssetForWrite(resolvedPath)
        ArResolver-->>SdfFileFormat: asset
        SdfFileFormat-->>SdfLayer: layer
        SdfLayer-->>UsdStage: layer
        UsdStage->>ArResolver: CreateDefaultContextForAsset(assetIdentifier) | CreateDefaultContext()
        ArResolver-->>UsdStage: resolverContext
        UsdStage->>ArResolver: BindContext(resolverContext)
        UsdStage-->>Bob: stage

If compared against opening a `UsdStage` the call sequence for creating a `UsdStage isn't that different. The main APIs
are still `Usd`, `Sdf` and `Ar` but there are a few differences. Specifically the calls to
`CreateIdentifierForNewAsset` / `ResolveForNewAsset` and using `OpenAssetForWrite` / `ArWritableAsset` APIs.

Initialization
^^^^^^^^^^^^^^

The initialization for the `ArResolver` system is not overly involved but it is a common source of problems. If USD
tries to load an Asset that requires a specific `ArResolver` plugin and that plugin can not be found USD has no way to
load it. Most of the time it's related to the underlying Plugin System in USD and the nature of how it loads plugins in
`PlugRegistry`. But before getting into all that, it's good to understand the different pieces that come into play for
the `ArResolver` system.

.. _Primary Resolvers:

Primary Resolvers
^^^^^^^^^^^^^^^^^

Primary resolvers are in charge of dealing with the bulk of asset resolution within Ar. There can be only one
Primary resolver active for a given USD environment. If no suitable Primary resolver can be found, Ar will use it's own
`ArDefaultResolver` as the Primary resolver.

In the first iteration of `ArResolver` (Ar 1.0) any implementation would be considered a Primary resolver.
If your `ArResolver` plugin supported multiple URI schemes, such as the case with `OmniUsdResolver`,
there was nothing preventing that. But if your `ArResolver` was very specific and only supported a single URI scheme it
would still need to be configured as a Primary resolver. This caused problems when multiple resolver implementations
need to coexist in the same USD environment. Ar 2.0 added support for URI resolvers to address this problem.

.. _URI Resolvers:

URI Resolvers
^^^^^^^^^^^^^

Much as the name implies, URI Resolvers are `ArResolver` plugins that support specific URI schemes.
These types of resolvers are configured in plugInfo.json to specify the URI scheme(s) that they support. Internally,
Ar will inspect the asset path for a scheme and dispatch the calls to the corresponding URI resolver. If no matching
URI Resolver can be found (e.g the asset path is a normal file path) the Primary resolver will be used. This change to Ar 2.0
made support for multiple resolvers much easier. An ArResolver plugin could just specify the URI schemes it supports,
and it will work alongside any other `ArResolver` plugin. It's important to point out that a lot of `ArResolver` plugins in Ar 1.0
supported multiple URI schemes but were developed as Primary Resolvers. For instance, `OmniUsdResolver` is a Primary Resolver
but supports "omniverse://", "omni://", "https://", "file://" URI schemes. This will be an important fact when configuring
multiple resolvers in the same USD environment

URI Resolver Support in OmniUsdResolver (Ar 1.0)
""""""""""""""""""""""""""""""""""""""""""""""""

As an intermediate step to transition to Ar 2.0, the `OmniUsdResolver (Ar 1.0)` has limited support for URI Resolvers.
The API for Ar 2.0 is quite different but the underlying Plugin System that Ar uses to load different `ArResolver`
plugins is the same. From this, the `OmniUsdResolver (Ar 1.0)` tries to inspect the `PlugRegistry` for other
`ArResolver` plugins that have been defined in the environment. For all `ArResolver` plugins that declare a "uriSchemes"
field in their plugInfo.json `OmniUsdResolver (Ar 1.0)` will keep a mapping of the URI scheme to the actual loaded plugin.
When the various parts of the `OmniUsdResolver (Ar 1.0)` API are invoked, the mapping of
:ref:`URI Resolvers<URI Resolvers>` will be checked first and if a matching :ref:`URI Resolver<URI Resolvers>` is found
that call will be dispatched to the corresponding :ref:`URI Resolver<URI Resolvers>`.

    To support :ref:`URI Resolvers<URI Resolvers>` in Ar 1.0, `OmniUsdResolver` must be set as the
    :ref:`Preferred Resolver<Preferred Resolver>` in the environment. During the initialization of `ArResolver`, Ar 1.0
    makes no distinction between :ref:`Primary Resolvers<Primary Resolvers>` and :ref:`URI Resolvers<URI Resolvers>`

.. _Package Resolvers:

Package Resolvers
^^^^^^^^^^^^^^^^^

Package Resolvers are a less common form of Asset Resolution in USD but they are very important for certain
`SdfFileFormat` plugins. The most well-know `SdfFileFormat` plugin that requires a Package Resolver is the
`UsdzFileFormat` plugin. The `UsdzFileFormat` is an uncompressed archive of Assets laid out according to the Zip file
specification. This allows multiple Assets to be packaged together into a single .usdz file that is easy to transport.

Unlike the public API to `ArResolver`, which can be easily obtained by calling `ArGetResolver()`, there is no public
access to Package Resolvers. A Package Resolver only has meaning to the `SdfFileFormat` plugin that it is associated
with. For this reason, `ArResolver` will handle the appropriate calls during Asset Resolution to the corresponding
Package Resolver.

    Package Resolvers are associated with their corresponding `SdfFileFormat` plugin via extension. The extensions are
    declared through their plugInfo.json

.. _Preferred Resolver:

Preferred Resolver
^^^^^^^^^^^^^^^^^^

Configuration for multiple `ArResolver` plugins can be done in a couple different ways. The important thing to remember
is that one `ArResolver` will need to serve as the :ref:`Primary Resolver<Primary Resolvers>`. At a minimum, the `ArDefaultResolver` will be the
:ref:`Primary Resolver<Primary Resolvers>` if no `ArResolver` plugin is suitable. In an environment with multiple `ArResolver` plugins there
is not a direct way to set the :ref:`Primary Resolver<Primary Resolvers>`, one can only "hint" at what plugin should
serve as the :ref:`Primary Resolver<Primary Resolvers>`. This "hint" can be set via `ArSetPreferredResolver()` with the type name of the
`ArResolver` plugin that is declared in plugInfo.json. If `ArSetPreferredResolver()` is called multiple times, the
:ref:`Primary Resolver<Primary Resolvers>` set last will be used.

    The type name used in `ArSetPreferredResolver()` should match the name used for defining the `ArResolver` `TfType`.
    For example, with `OmniUsdResolver` we define the `TfType` with `AR_DEFINE_RESOLVER(OmniUsdResolver, ArResolver)`.
    As such, the `OmniUsdResolver` must be hinted at with `ArSetPreferredResolver("OmniUsdResolver")`

In Ar 2.0, an `ArResolver` plugin will be determined as a :ref:`Primary Resolver<Primary Resolvers>` in the absence of a
"uriSchemes" field in it's plugInfo.json. In an environment with multiple `ArResolver` plugins that can be a
:ref:`Primary Resolver<Primary Resolvers>` the one chosen will be determined in one of two ways:

#. By the last `ArResolver` plugin specified through `ArSetPreferredResolver()` before the first call to `ArGetResolver()`. So it's important that `ArSetPreferredResolver()` is called during application startup.:

    The `ArSetPreferredResolver()` can only be set with an `ArResolver` plugin that satisfies the Primary Resolver
    requirement. So any `ArResolver` plugin that declares a "uriSchemes" field in their plugInfo.json can not be set
    as the Primary Resolver.

#. If no `ArResolver` plugin is specified through `ArSetPreferredResolver()`, it will be determined by the first one found from the alphabetically sorted list of `ArGetAvailableResolvers()`.

Bootstrapping ArResolver
^^^^^^^^^^^^^^^^^^^^^^^^

The call to `ArGetResolver()` is the entry point to any Asset Resolution in USD. Even libraries within USD that sit above
the Ar library initialize `ArResolver` in this way. But as simple as it may seem, `ArGetResolver()` does perform a lot
of work to initialize the `ArResolver` instance that it returns. It requires the following steps to initialize properly:

#. Load all :ref:`Primary Resolvers <Primary Resolvers>` plugins and identify which plugin will serve as the :ref:`Primary Resolver <Primary Resolvers>`. See :ref:`Preferred Resolver<Preferred Resolver>` for how this is determined.

#. Fallback to the `ArDefaultResolver` if no :ref:`Primary Resolver <Primary Resolvers>` satisfies the criteria.

#. Set the identified :ref:`Primary Resolver <Primary Resolvers>` as the underlying resolver for the entire process.

#. Load all :ref:`URI Resolvers<URI Resolvers>` plugins uniquely mapping their declared "uriSchemes" to each loaded plugin

#. Initialize all :ref:`Package Resolvers <Package Resolvers>` indexed according to the supported extensions.

Once all these plugins have been loaded and identified a :ref:`Primary Resolver<Primary Resolvers>` `ArResolver`
instance can be returned from `ArGetResolver()`. At this point, the Asset Resolution system has been initialized and USD
can begin loading Assets.

Problems with Initialization
""""""""""""""""""""""""""""

A common problem is that **any ArResolver plugins registered with PlugRegistry after this first call to ArGetResolver will not be loaded.**
The reason that it needs to load all these plugins upon the first call to `ArGetResolver()` is due to the nature of how
`PlugRegistry` loads plugins. The first time that a plugin type needs to be loaded `PlugRegistry` will load all plugins,
and any dependencies those plugins may have, derived from that type. Once those plugins are loaded any plugins
registered with `PlugRegistry`, deriving from the same plugin type, will not be recognized.
**This is a problem with all plugins not just ArResolver plugins**! `UsdSchemaRegistry` and `SdfFileFormatRegistry`
have the same issue. So, it's **really** important to be very mindful of calling `ArGetResolver()` within application
startup.

.. _Troubleshooting:

Troubleshooting
"""""""""""""""

Identifying an initialization problem with `ArResolver` can be challenging. The problem can be as simple as some recently added
startup code making a call to `ArGetResolver()` to inspect Assets. Any `ArResolver` plugins registered after this new startup
code will just fail to load the Assets they support. When this happens it's hard to identify the exact problem as it will
seem that the `ArResolver` plugins themselves have an issue. The best way to identify a problem with initialization for
`ArResolver` is to use `TfDebug` flags that are provided for both `ArResolver` and `OmniUsdResolver`.

    Without going into too much detail, `TfDebug` flags are a great way to turn on additional diagnostics at run-time.
    Depending on their usage within a library, `TfDebug` flags can provide a lot of information without going through the,
    sometimes lengthy, process of setting up a debug environment and stepping through code. The can be turned on via
    environment variable or Python

`ArResolver` provides one `TfDebug` flag **AR_RESOLVER_INIT** which writes additional information to the console when
`ArGetResolver()` is called. This will list information like the :ref:`Primary Resolvers <Primary Resolvers>`,
:ref:`URI Resolvers <URI Resolvers>` and :ref:`Package Resolvers <Package Resolvers>` discovered. The
:ref:`Preferred Resolver <Preferred Resolver>` set and if a :ref:`Primary Resolver <Primary Resolvers>` was found that
matches it. The information that the `TfDebug` flag **AR_RESOLVER_INIT** outputs is extremely helpful to help understand
how the returned `ArResolver` instance from `ArGetResolver()` was initialized. For a lot of initialization problems it
becomes clear that an expected `ArResolver` plugin wasn't discovered or that there was a problem loading the plugin.
If an `ArResolver` plugin wasn't discovered it means that it needs to be registered earlier in the startup process or
any startup code calling `ArGetResolver()` might need to be deferred.

In a similar manner, `OmniUsdResolver` also provides multiple `TfDebug` flags to troubleshooting resolves. These aren't
as useful for initialization problems but can really help identify problems with resolving Assets. The following `TfDebug`
flags are available for `OmniUsdResolver`:

+----------------------------+--------------------------------------------------+
| TfDebug Flag               | Description                                      |
+============================+==================================================+
| OMNI_USD_RESOLVER          | OmniUsdResolver general resolve information      |
+----------------------------+--------------------------------------------------+
| OMNI_USD_RESOLVER_CONTEXT  | OmniUsdResolver Context information              |
+----------------------------+--------------------------------------------------+
| OMNI_USD_RESOLVER_MDL      | OmniUsdResolver MDL specific resolve information |
+----------------------------+--------------------------------------------------+
| OMNI_USD_RESOLVER_ASSET    | OmniUsdResolver asset read / write information   |
+----------------------------+--------------------------------------------------+