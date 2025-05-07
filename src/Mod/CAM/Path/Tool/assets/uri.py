from __future__ import annotations
import urllib.parse
from urllib.parse import uses_params
from typing import Dict, Any, Mapping

class AssetUri:
    """
    Represents an asset URI with components.

    The URI structure is: <protocol>://<domain>/<asset_type>/<asset>/<version>?<params>
    """

    def __init__(self, uri_string: str):
        scheme = uri_string.split(":", 1)[0]
        if scheme not in uses_params:
            uses_params.append(scheme)

        parsed = urllib.parse.urlparse(uri_string)
        scheme = parsed.scheme if parsed.scheme else scheme

        self.protocol = scheme
        self.domain = parsed.netloc
        # Split path components, ignoring leading/trailing slashes
        path_components = [comp for comp in parsed.path.split('/') if comp]

        if len(path_components) < 2:
            raise ValueError(f"Invalid URI path structure: {uri_string}")
        elif len(path_components) == 2:
            self.asset_type, *path = path_components
            self.version = None
        else:
            self.asset_type, *path, self.version = path_components
        self.asset = '/'.join(path)

        self.params: Dict[str, list[str]] = urllib.parse.parse_qs(parsed.query)

    def __str__(self) -> str:
        path_components = [self.asset_type, self.asset]
        if self.version is not None:
            path_components.append(self.version)
        path = '/' + '/'.join(path_components)

        query = urllib.parse.urlencode(self.params, doseq=True) if self.params else ""

        uri_string = urllib.parse.urlunparse(
            (self.protocol, self.domain, path, "", query, "")
        )
        return uri_string

    def __repr__(self) -> str:
        return f"AssetUri('{str(self)}')"

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, AssetUri):
            return NotImplemented
        return (self.protocol == other.protocol and
                self.domain == other.domain and
                self.asset_type == other.asset_type and
                self.asset == other.asset and
                self.version == other.version and
                self.params == other.params)

    @classmethod
    def is_uri(cls, uri: AssetUri | str) -> bool:
        """Checks if the given string is a valid URI."""
        if isinstance(uri, AssetUri):
            return True

        try:
            AssetUri(uri)
        except ValueError:
            return False
        return True

    @staticmethod
    def build(protocol: str,
              domain: str | None,
              asset_type: str,
              asset: str,
              version: str | None = "latest",
              params: Mapping[str, str | list[str]] | None = None) -> AssetUri:
        """Builds a Uri object from components."""
        uri = AssetUri.__new__(AssetUri) # Create a new instance without calling __init__
        uri.protocol = protocol
        uri.domain = domain or ""
        uri.asset_type = asset_type
        uri.asset = asset
        uri.version = version
        uri.params = {}
        if params:
            for key, value in params.items():
                if isinstance(value, list):
                    uri.params[key] = value
                else:
                    uri.params[key] = [value]
        return uri
