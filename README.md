# Omniverse USD Resolver

## Overview

Omniverse USD Resolver is an implementation of OpenUSD's ArResolver plugin abstraction. It's sole purpose is to 
identify and resolve an asset in OpenUSD against a backing asset management system.

## Documentation

The latest documentation can be found at http://omniverse-docs.s3-website-us-east-1.amazonaws.com/usd_resolver/

## Downloading

You can get the latest build from Packman.
There are seperate packages for each usd flavor, usd version, python version, platform, and configuration.

They are all named:
omni_usd_resolver_{usd_flavor}_{usd_version}_py_{python_version}.{platform}.{config}

usd_flavor / usd_version is one of:
* usd 24.05
* usd 24.03
* usd 23.11
* nv-usd 22.11
* usd-minimal 24.05
* (see `repo_usd` for the full list)

python_version is one of:
* 0
* 3.10
* 3.11

platform is one of:
* windows-x86_64
* linux-x86_64
* linux-aarch64

config is one of:
* release
* debug

All packages use the same versioning scheme:
`{major}.{minor}.{patch}`

## Initializing

### USD, Python & Client Library

The package includes `redist.packman.xml` which point to the versions of USD, Python and the Omniverse Client Library
that this plugin was built against. You can include it in your own packman.xml file like this:

```
<project toolsVersion="5.0">
  <import path="../_build/target-deps/omni_usd_resolver/deps/redist.packman.xml">
  </import>
  <dependency name="usd_${config}" linkPath="../_build/usd-deps/usd/${config}">
  </dependency>
  <dependency name="python" linkPath="../_build/usd-deps/python">
  </dependency>
  <dependency name="omni_client_library" linkPath="../_build/target-deps/omni_client_library">
  </dependency>
</project>
```

NOTE: This must be in packman.xml file that is pulled _after_ pulling this package.
You can't put it in the same packman.xml as the one that pulls this package.

This requires packman 6.4 or later.

### Registration

You must either copy the omni_usd_resolver plugin to the default USD plugin location, or register
the plugin location at application startup using `PXR_NS::PlugRegistry::GetInstance().RegisterPlugins`.

Be sure to package both the library (.dll or .so) and the "plugInfo.json" file. Be sure to keep the
folder structure the same for the "plugInfo.json" file. It should look like this:

- omni_usd_resolver.dll or libomni_usd_resolver.so
- usd/omniverse/resources/plugInfo.json

If you use `RegisterPlugins`, provide it the path to the "resources" folder.
Otherwise, you can copy the entire 'debug' or 'release' folders into the standard USD folder structure.

## References

- [OpenUSD API Docs](https://openusd.org/release/api/index.html)
- [OpenUSD User Docs](https://openusd.org/release/index.html)
- [NVIDIA OpenUSD Resources and Learning](https://developer.nvidia.com/usd)
- [NVIDIA OpenUSD Code Samples](https://github.com/NVIDIA-Omniverse/OpenUSD-Code-Samples)

## License

The NVIDIA Omniverse USD Resolver is governed by the [NVIDIA Agreements | Enterprise Software | NVIDIA Software License Agreement and NVIDIA Agreements](https://www.nvidia.com/en-us/agreements/enterprise-software/nvidia-software-license-agreement/) and [NVIDIA Agreements | Enterprise Software | Product Specific Terms for Omniverse](https://www.nvidia.com/en-us/agreements/enterprise-software/product-specific-terms-for-omniverse)