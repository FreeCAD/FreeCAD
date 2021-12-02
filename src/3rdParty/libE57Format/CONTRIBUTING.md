# How To Contribute

## Code Changes

I am happy to review any [pull requests](https://github.com/asmaloney/libE57Format/pulls). Please keep them as short as possible. Each pull request should be atomic and only address one issue. This helps with the review process.

### Formatting

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format the code. There is a cmake target (_format_) - which runs _clang-format_ on the source files. After changes have been made, and before you submit your pull request, please run the following:

```sh
cmake --build . --target format
```

## Documentation

The [documentation](https://github.com/asmaloney/libE57Format) is a bit old and could use some lovin'. You can submit changes over in the [libE57Format-docs](https://github.com/asmaloney/libE57Format-docs) repository.

## Financial

If you would like to support the project financially, you can use the **Sponsor** button at the top of the [libE57Format](https://github.com/asmaloney/libE57Format) repository page.
