# How To Contribute

These are some of the things you can do to contribute to the project:

## 💰 Financial

Given that I'm an independent developer without funding, financial support is appreciated. If you would like to support the project financially (especially if you sell a product which uses this library), you can use the [sponsors page](https://github.com/sponsors/asmaloney) for one-off or recurring support, or we can arrange <a href="mailto:asmaloney@gmail.com?subject=libE57Format B2B Sponsorship">B2B invoicing</a> of some kind. Thank you!

## ✍ Write About The Project

If you find the project useful, spread the word! Articles, mastodon posts, tweets, blog posts, instagram photos - whatever you're into. Please include a referral back to the repository page: https://github.com/asmaloney/libE57Format

## ⭐️ Add a Star

If you found this project useful, please consider starring it! It helps me gauge how useful this project is.

## ☝ Raise Issues

If you run into something which doesn't work as expected, raising [an issue](https://github.com/asmaloney/libE57Format/issues) with all the relevant information to reproduce it would be helpful.

## 🐞 Bug Fixes & 🧪 New Things

I am happy to review any [pull requests](https://github.com/asmaloney/libE57Format/pulls). Please keep them as short as possible. Each pull request should be atomic and only address one issue. This helps with the review process.

Note that I will not accept everything, but I welcome discussion. If you are proposing a big change, please raise it as [an issue](https://github.com/asmaloney/libE57Format/issues) first for discussion.

### Formatting

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to format the code. There is a cmake target (_e57-clang-format_) - which runs _clang-format_ on the source files. After changes have been made, and before you submit your pull request, please run the following:

```sh
cmake --build . --target e57-clang-format
```

## 📖 Documentation

The [documentation](https://github.com/asmaloney/libE57Format) is a bit old and could use some lovin'. You can submit changes over in the [libE57Format-docs](https://github.com/asmaloney/libE57Format-docs) repository.
