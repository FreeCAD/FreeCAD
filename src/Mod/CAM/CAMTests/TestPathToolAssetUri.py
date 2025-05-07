import unittest
from Path.Tool.assets.uri import Uri


class TestPathToolAssetUri(unittest.TestCase):
    """
    Test suite for the Uri utility class.
    """

    def test_uri_parsing_full(self):
        uri_string = ("remote://domain.com/asset_type/asset/version?"
                      "param1=value1&param2=value2")
        uri = Uri(uri_string)
        self.assertEqual(uri.protocol, "remote")
        self.assertEqual(uri.domain, "domain.com")
        self.assertEqual(uri.asset_type, "asset_type")
        self.assertEqual(uri.asset, "asset")
        self.assertEqual(uri.version, "version")
        self.assertEqual(uri.params, {"param1": ["value1"], "param2": ["value2"]})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"Uri('{uri_string}')")

    def test_uri_parsing_local(self):
        uri_string = "local:/type/id/v2?param=value"
        uri = Uri(uri_string)
        self.assertEqual(uri.protocol, "local")
        self.assertEqual(uri.domain, "")
        self.assertEqual(uri.asset_type, "type")
        self.assertEqual(uri.asset, "id")
        self.assertEqual(uri.version, "v2")
        self.assertEqual(uri.params, {"param": ["value"]})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"Uri('{uri_string}')")

        uri_string2 = "local:///type/id/v2?param=value"
        uri = Uri(uri_string2)
        self.assertEqual(uri.protocol, "local")
        self.assertEqual(uri.domain, "")
        self.assertEqual(uri.asset_type, "type")
        self.assertEqual(uri.asset, "id")
        self.assertEqual(uri.version, "v2")
        self.assertEqual(uri.params, {"param": ["value"]})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"Uri('{uri_string}')")

    def test_uri_parsing_no_params(self):
        uri_string = "file:///path/to/asset/v1.0"
        uri = Uri(uri_string)
        self.assertEqual(uri.protocol, "file")
        self.assertEqual(uri.domain, "")
        self.assertEqual(uri.asset_type, "path")
        self.assertEqual(uri.asset, "to/asset")
        self.assertEqual(uri.version, "v1.0")
        self.assertEqual(uri.params, {})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"Uri('{uri_string}')")

    def test_uri_version_missing(self):
        uri_string = "local://domain/asset_type/asset"
        uri = Uri(uri_string)
        self.assertEqual(uri.protocol, "local")
        self.assertEqual(uri.domain, "domain")
        self.assertEqual(uri.asset_type, "asset_type")
        self.assertEqual(uri.asset, "asset")
        self.assertIsNone(uri.version)
        self.assertEqual(uri.params, {})
        self.assertEqual(str(uri), uri_string)

    def test_uri_build_full(self):
        uri = Uri.build("local", "domain", "asset_type", "asset",
                        version="version", params={"param1": "value1"})
        expected_uri_string = "local://domain/asset_type/asset/version?param1=value1"
        self.assertEqual(str(uri), expected_uri_string)
        self.assertEqual(uri.protocol, "local")
        self.assertEqual(uri.domain, "domain")
        self.assertEqual(uri.asset_type, "asset_type")
        self.assertEqual(uri.asset, "asset")
        self.assertEqual(uri.version, "version")
        self.assertEqual(uri.params, {"param1": ["value1"]}) # parse_qs always returns list

    def test_uri_build_latest_version_no_params(self):
        uri = Uri.build("remote", "another.domain", "type", "id")
        expected_uri_string = "remote://another.domain/type/id/latest"
        self.assertEqual(str(uri), expected_uri_string)
        self.assertEqual(uri.protocol, "remote")
        self.assertEqual(uri.domain, "another.domain")
        self.assertEqual(uri.asset_type, "type")
        self.assertEqual(uri.asset, "id")
        self.assertEqual(uri.version, "latest")
        self.assertEqual(uri.params, {})

    def test_uri_build_empty_domain(self):
        """
        Test that Uri.build with empty domain incorrectly populates attributes
        and generates a malformed string representation.
        """
        protocol = "test_protocol"
        asset_type = "test_type"
        asset = "test_asset"
        version = "1"

        # Build URI with empty domain (None)
        uri = Uri.build(protocol, None, asset_type, asset, version)

        # Assert that attributes are incorrectly populated (based on observed behavior)
        self.assertEqual(uri.protocol, protocol)
        self.assertEqual(uri.domain, "")
        self.assertEqual(uri.asset_type, asset_type)
        self.assertEqual(uri.asset, asset)
        self.assertEqual(uri.version, version)

        expected_string = f"{protocol}:/{asset_type}/{asset}/{version}"
        self.assertEqual(str(uri), expected_string)

        # Assert that the 'path' attribute is missing (based on observed AttributeError)
        self.assertFalse(hasattr(uri, 'path'))


    def test_uri_equality(self):
        uri1 = Uri("local://domain/type/asset/version")
        uri2 = Uri("local://domain/type/asset/version")
        uri3 = Uri("local://domain/type/asset/another_version")
        self.assertEqual(uri1, uri2)
        self.assertNotEqual(uri1, uri3)
        self.assertNotEqual(uri1, "not a uri")
