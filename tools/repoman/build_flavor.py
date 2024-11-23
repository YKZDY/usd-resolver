import os
from typing import Optional


def get_build_flavor(usd_flavor: Optional[str] = None, usd_ver: Optional[str] = None):
    # In most cases the usd_flavor will correspond to a vendor specific build of USD
    # such as nv-usd, usd, houdini_usd, maya_usd, etc. In these cases we will follow the naming convention
    # of the usd_build_bom which is to concatenate the usd_flavor with the usd_ver.
    # 'local' is a special case supported by the usd_build_bom to indicate a locally managed build
    # of USD.
    flavor = usd_flavor or get_usd_flavor()
    ver = usd_ver or get_usd_version()
    return flavor if is_local(flavor) else f"{flavor}_{ver}"


def get_usd_flavor() -> str:
    # these defaults need to should align with the usd_build_bom
    return os.getenv("OMNI_USD_FLAVOR", "usd")


def get_usd_version() -> str:
    # these defaults need to should align with the usd_build_bom
    return os.getenv("OMNI_USD_VER", "24.05")


def get_python_version() -> str:
    # these defaults need to should align with the usd_build_bom
    return os.getenv("OMNI_PYTHON_VER", "3.10")


def is_local(usd_flavor: Optional[str] = None) -> bool:
    flavor = usd_flavor or get_usd_flavor()
    return flavor == "local"
