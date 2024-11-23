import argparse
import logging
import os

import build_flavor
import repoman

repoman.bootstrap()

import generate
import omni.repo.man as repo_man
import omni.repo.package

logger = logging.getLogger(os.path.basename(__file__))


def run_command(options, package_name="omni_usd_resolver"):
    platforms = ["windows-x86_64", "linux-x86_64", "linux-aarch64"]
    platform_host = omni.repo.man.get_and_validate_host_platform(["windows-x86_64", "linux-x86_64", "linux-aarch64"])
    # By default target platform equals host platform
    platform_target = options.platform_target
    if platform_target is None:
        platform_target = platform_host

    omni.repo.man.validate_platform(platform_target, platforms, "target")

    usd_flavor = options.usd_flavor
    usd_ver = options.usd_ver
    python_ver = options.python_ver

    config = "debug" if options.debug else "release"

    try:
        package_version = omni.repo.man.build_number.generate_build_number_from_file("_build/VERSION", flow_v2=True)
    except omni.repo.man.exceptions.StorageError as e:
        generate.generate_version_file()
        try:
            package_version = omni.repo.man.build_number.generate_build_number_from_file("_build/VERSION", flow_v2=True)
        except omni.repo.man.exceptions.StorageError as e:
            logger.error("Failed to generate build number from VERSION file")
            raise e

    pkg_desc = omni.repo.package.PackageDesc()
    pkg_desc.version = package_version
    pkg_desc.output_folder = "_build/packages"
    pkg_desc.remove_pycache = True
    pkg_desc.warn_if_not_exist = True
    pkg_desc.name = f"{package_name}_{usd_flavor}_{usd_ver}_py_{python_ver}.{platform_target}.{config}"

    package_dict = repo_man.get_toml_module().load("package.toml")

    pkg_desc.files = []
    pkg_desc.files.extend(
        repo_man.gather_files_from_dict_for_platform(package_dict, package_name, platform_target, [config])
    )
    pkg_desc.files_exclude = repo_man.gather_files_from_dict_for_platform(
        package_dict, package_name, platform_target, [config], keyword="files_exclude"
    )
    pkg_desc.files_strip = repo_man.gather_files_from_dict_for_platform(
        package_dict, package_name, platform_target, [config], keyword="files_strip"
    )
    pkg_desc.files_strip_exclude = repo_man.gather_files_from_dict_for_platform(
        package_dict, package_name, platform_target, [config], keyword="files_strip_exclude"
    )

    omni.repo.package.package(pkg_desc, [config])


if __name__ == "__main__" or __name__ == "__mp_main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--platform-target", dest="platform_target", required=False)
    parser.add_argument("--usd-flavor", dest="usd_flavor", default=build_flavor.get_usd_flavor(), required=False)
    parser.add_argument("--usd-ver", dest="usd_ver", default=build_flavor.get_usd_version(), required=False)
    parser.add_argument("--python-ver", dest="python_ver", default=build_flavor.get_python_version(), required=False)
    parser.add_argument("-d", "--debug", dest="debug", action="store_true", required=False)
    options, _ = parser.parse_known_args()

    run_command(options, package_name="omni_usd_resolver")
    run_command(options, package_name="symbols_omni_usd_resolver")
