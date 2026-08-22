# FreeCAD API Site

This directory contains an Astro Starlight site scaffold for the generated
FreeCAD Python and C++ API reference.

## Usage

Install dependencies:

```sh
npm install
```

Regenerate the API content and sidebars:

```sh
npm run generate:api
```

Start the local preview:

```sh
npm run dev
```

The generated API content is written to:

- `src/content/docs/python-api/`
- `src/content/docs/cpp-api/`
- `src/generated/python-api-sidebar.ts`
- `src/generated/cpp-api-sidebar.ts`
- `src/content/docs/python-api-manifest.json`
- `src/content/docs/cpp-api-manifest.json`

These files are disposable generated artifacts and are not edited by hand.

If you want linked source URLs instead of plain source paths in the generated
pages, run the generator manually and pass `--source-base-url` through to
`src/Tools/typing/generate_stubs.py docs` or
`src/Tools/typing/generate_stubs.py cpp-docs`.
