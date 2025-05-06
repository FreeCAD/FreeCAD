import urllib.parse
from typing import Dict, Any

class Uri:
    """
    Represents an asset URI with components.

    The URI structure is: <protocol>://<domain>/<asset_type>/<asset>/<version>?<params>
    """

    def __init__(self, uri_string: str):
        parsed = urllib.parse.urlparse(uri_string)

        self.protocol = parsed.scheme
        self.domain = parsed.netloc
        # Split path components, ignoring leading/trailing slashes
        path_components = [comp for comp in parsed.path.split('/') if comp]

        if len(path_components) < 3:
            raise ValueError(f"Invalid URI path structure: {uri_string}")

        thetype, *path, version = path_components
        self.asset_type = thetype
        self.asset = '/'.join(path)
        self.version = version

        self.params: Dict[str, list[str]] = urllib.parse.parse_qs(parsed.query)

    def __str__(self) -> str:
        path_components = [self.asset_type, self.asset, self.version]
        path = '/' + '/'.join(path_components)

        query = urllib.parse.urlencode(self.params, doseq=True) if self.params else ""

        uri_string = urllib.parse.urlunparse(
            (self.protocol, self.domain, path, "", query, "")
        )
        return uri_string

    def __repr__(self) -> str:
        return f"Uri('{str(self)}')"

    def __eq__(self, other: Any) -> bool:
        if not isinstance(other, Uri):
            return NotImplemented
        return (self.protocol == other.protocol and
                self.domain == other.domain and
                self.asset_type == other.asset_type and
                self.asset == other.asset and
                self.version == other.version and
                self.params == other.params)

    @staticmethod
    def build(protocol: str,
              domain: str | None,
              asset_type: str,
              asset: str,
              version: str | None = None,
              params: Dict[str, str | list[str]] | None = None) -> 'Uri':
        """Builds a Uri object from components."""
        uri = Uri.__new__(Uri) # Create a new instance without calling __init__
        uri.protocol = protocol
        uri.domain = domain or ""
        uri.asset_type = asset_type
        uri.asset = asset
        uri.version = version or "latest"
        uri.params = {}
        if params:
            for key, value in params.items():
                if isinstance(value, list):
                    uri.params[key] = value
                else:
                    uri.params[key] = [value]
        return uri

# Alias for type hinting
UriStr = str