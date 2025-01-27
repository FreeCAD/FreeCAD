# libE57Format Testing

Testing uses the [GoogleTest](https://github.com/google/googletest) framework. The documentation for it may be [found here](https://google.github.io/googletest/).

## Turning Testing On

To turn testing on, set the CMake option `E57_BUILD_TEST` to ON.

## Testing Data

Currently, the testing data is found in another repo: [libE57Format-test-data](https://github.com/asmaloney/libE57Format-test-data).

As we build out testing, the number & size of the test data might grow very quickly. For this reason, the test data is kept separate from the main repo. This gives us some flexibility depending on how large the data set becomes. If ends up being too big for a git repo, for example, we can make it available as a zip download somewhere. It will also let us manage the data in CI better so it doesn't have to be downloaded every time CI is run.

CMake will attempt to find this test data in the following locations:

- in this test directory (`./libE57Format-test-data`)
- in the parent directory (`../libE57Format-test-data`)
- in the grandparent directory (`../../libE57Format-test-data`)

If it is found, `E57_TEST_DATA_PATH` will be set automatically.

If not, you can set `E57_TEST_DATA_PATH` manually to point at the directory containing `libE57Format-test-data`.

A couple of minor tests can run without this test data, but for more complete coverage, it should be present.

## Adding New Tests

If the tests being added require data from the testing data repository, then the name of the test suite needs to end with `Data`. These tests will be skipped if the data files are not available.

e.g.

```cpp
TEST( SimpleWriter, WriteFoo )
{
   // Will always run
}

TEST( SimpleWriterData, WriteFoo )
{
   // Will only run if the test data is available
}
```
