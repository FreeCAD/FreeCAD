"""Compatibility shim for the bSDD network client module."""

from BimBsdd import (
    BSDD_API_BASE_URL,
    BSDD_API_URL_KEY,
    BSDD_IFC_DICTIONARY_URI,
    BSDD_INACTIVE_KEY,
    BSDD_PREFERENCES_PATH,
    BSDD_PREVIEW_KEY,
    BSDD_TEST_KEY,
    BsddNetworkClient,
    BsddSettings,
    get_bsdd_network_client,
)

__all__ = [
    "BSDD_API_BASE_URL",
    "BSDD_API_URL_KEY",
    "BSDD_IFC_DICTIONARY_URI",
    "BSDD_INACTIVE_KEY",
    "BSDD_PREFERENCES_PATH",
    "BSDD_PREVIEW_KEY",
    "BSDD_TEST_KEY",
    "BsddNetworkClient",
    "BsddSettings",
    "get_bsdd_network_client",
]
