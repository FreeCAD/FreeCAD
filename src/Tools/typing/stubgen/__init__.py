"""Support package for the FreeCAD Python binding stub generator.

This package keeps the stub-generation pipeline split by responsibility so the
modules stay navigable without tracing one large script.

Module layout:
- ``model`` holds shared dataclasses, type aliases, defaults, and regexes.
- ``parsing`` holds syntax-oriented helpers for source scanning and AST reads.
- ``naming`` holds tiny shared naming helpers used across the pipeline.
- ``signature_parser`` holds reusable Python callable-signature parsing helpers.
- ``api_model`` holds the neutral public API model shared by future renderers.
- ``api_extract`` builds that neutral API model from curated stub inputs.
- ``api_markdown`` renders package-shaped Markdown pages from that model.
- ``discovery`` inventories C++ registrations and PyCXX contexts.
- ``source_inputs`` reads binding specs plus curated source-adjacent stub files.
- ``render`` formats individual stub fragments and inventory skeletons.
- ``module_merge`` owns package paths plus module-body and support-node merges.
- ``class_merge`` owns class alias planning and public class stub assembly.
- ``type_context_rules`` holds the small manual escape hatch for PyCXX
  contexts that cannot be mapped mechanically yet.
- ``generator`` coordinates the end-to-end pipeline and keeps the public entrypoints stable.
- ``cli`` wires the pipeline to the public command-line interface.

A useful reading order is ``model`` -> ``parsing`` -> ``signature_parser`` ->
``api_model`` -> ``api_extract`` -> ``api_markdown`` -> ``naming`` ->
``discovery`` -> ``source_inputs`` -> ``render`` -> ``module_merge`` ->
``class_merge`` -> ``type_context_rules`` -> ``generator`` -> ``cli``.
"""
