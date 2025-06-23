import unittest
from Path.Tool.assets.uri import AssetUri


class TestPathToolAssetUri(unittest.TestCase):
    """
    Test suite for the AssetUri utility class.
    """

    def test_uri_parsing_full(self):
        uri_string = "remote://asset_id/version?" "param1=value1&param2=value2"
        uri = AssetUri(uri_string)
        self.assertEqual(uri.asset_type, "remote")
        self.assertEqual(uri.asset_id, "asset_id")
        self.assertEqual(uri.version, "version")
        self.assertEqual(uri.params, {"param1": ["value1"], "param2": ["value2"]})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"AssetUri('{uri_string}')")

    def test_uri_parsing_local(self):
        uri_string = "local://id/2?param=value"
        uri = AssetUri(uri_string)
        self.assertEqual(uri.asset_type, "local")
        self.assertEqual(uri.asset_id, "id")
        self.assertEqual(uri.version, "2")
        self.assertEqual(uri.params, {"param": ["value"]})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"AssetUri('{uri_string}')")

    def test_uri_parsing_no_params(self):
        uri_string = "file://asset_id/1"
        uri = AssetUri(uri_string)
        self.assertEqual(uri.asset_type, "file")
        self.assertEqual(uri.asset_id, "asset_id")
        self.assertEqual(uri.version, "1")
        self.assertEqual(uri.params, {})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"AssetUri('{uri_string}')")

    def test_uri_version_missing(self):
        uri_string = "foo://asset"
        uri = AssetUri(uri_string)
        self.assertEqual(uri.asset_type, "foo")
        self.assertEqual(uri.asset_id, "asset")
        self.assertIsNone(uri.version)
        self.assertEqual(uri.params, {})
        self.assertEqual(str(uri), uri_string)

    def test_uri_parsing_with_version(self):
        """
        Test parsing a URI string with asset_type, asset_id, and version.
        """
        uri_string = "test_type://test_id/1"
        uri = AssetUri(uri_string)
        self.assertEqual(uri.asset_type, "test_type")
        self.assertEqual(uri.asset_id, "test_id")
        self.assertEqual(uri.version, "1")
        self.assertEqual(uri.params, {})
        self.assertEqual(str(uri), uri_string)
        self.assertEqual(repr(uri), f"AssetUri('{uri_string}')")

    def test_uri_build_full(self):
        expected_uri_string = "local://asset_id/version?param1=value1"
        uri = AssetUri.build(
            asset_type="local", asset_id="asset_id", version="version", params={"param1": "value1"}
        )
        self.assertEqual(str(uri), expected_uri_string)
        self.assertEqual(uri.asset_type, "local")
        self.assertEqual(uri.asset_id, "asset_id")
        self.assertEqual(uri.version, "version")
        self.assertEqual(uri.params, {"param1": ["value1"]})  # parse_qs always returns list

    def test_uri_build_latest_version_no_params(self):
        expected_uri_string = "remote://id/latest"
        uri = AssetUri.build(asset_type="remote", asset_id="id", version="latest")
        self.assertEqual(str(uri), expected_uri_string)
        self.assertEqual(uri.asset_type, "remote")
        self.assertEqual(uri.asset_id, "id")
        self.assertEqual(uri.version, "latest")
        self.assertEqual(uri.params, {})

    def test_uri_equality(self):
        uri1 = AssetUri("local://asset/version")
        uri2 = AssetUri("local://asset/version")
        uri3 = AssetUri("local://asset/another_version")
        self.assertEqual(uri1, uri2)
        self.assertNotEqual(uri1, uri3)
        self.assertNotEqual(uri1, "not a uri")

    def test_uri_parsing_invalid_path_structure(self):
        """
        Test that parsing a URI string with an invalid path structure
        (more than one component) raises a ValueError.
        """
        uri_string = "local://foo/bar/1"
        with self.assertRaisesRegex(ValueError, "Invalid URI path structure:"):
            AssetUri(uri_string)


if __name__ == "__main__":
    unittest.main()
