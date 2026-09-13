"""Support package for the FreeCAD Python binding stub generator.

This package keeps the stub-generation pipeline split by responsibility so the
modules stay navigable without tracing one large script.

Module layout:
- ``model`` holds shared dataclasses, type aliases, defaults, and regexes.
- ``parsing`` holds syntax-oriented helpers for source scanning and AST reads.
- ``naming`` holds tiny shared naming helpers used across the pipeline.
- ``discovery`` inventories C++ registrations and PyCXX contexts.
- ``source_inputs`` reads binding specs plus curated source-adjacent stub files.
- sibling package ``python_api_model`` provides the shared public API model, signatures,
  normalization, diagnostics, and resolution policy; it never imports StubGen.
- ``api_extract`` and ``binding_adapter`` convert curated stubs and discovered
  binding records into that shared model.
- ``render`` emits package-shaped stubs from the normalized model.
- ``stub_support`` keeps output-only helper syntax out of the public model.
- ``module_merge`` owns package paths plus AST support filtering utilities.
- ``validation`` checks discovery facts before model construction.
- ``type_context_rules`` holds the small manual escape hatch for PyCXX
  contexts that cannot be mapped mechanically yet.
- ``generator`` coordinates the end-to-end pipeline and keeps the public entrypoints stable.
- ``cli`` wires the pipeline to the public command-line interface.

A useful reading order is ``python_api_model`` -> ``model`` -> ``parsing`` ->
``discovery`` -> ``api_extract`` -> ``binding_adapter`` -> ``stub_support`` ->
``render`` -> ``generator`` -> ``cli``.
"""
