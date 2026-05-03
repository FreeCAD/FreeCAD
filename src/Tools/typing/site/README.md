# FreeCAD Python API Site

This directory contains an Astro Starlight site scaffold for the generated
FreeCAD Python API reference.

## Usage

Install dependencies:

```sh
npm install
```

Regenerate the API content and sidebar:

```sh
npm run generate:api
```

Start the local preview:

```sh
npm run dev
```

The generated API content is written to:

- `src/content/docs/python-api/`
- `src/generated/python-api-sidebar.ts`

These files are disposable generated artifacts and are not edited by hand.

If you want linked source URLs instead of plain source paths in the generated
pages, run the generator manually and pass `--source-base-url` through to
`src/Tools/typing/generate_stubs.py docs`.
