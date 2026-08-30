"""Python API documentation and public-model pipeline.

The package keeps Python API stages explicit:

* ``extract`` reads curated source-adjacent stubs into the semantic model.
* ``model`` defines the normalized public API representation.
* ``markdown`` renders Python API pages for the documentation site.
* ``starlight`` renders the navigation fragment.
* ``pipeline`` coordinates those stages for the command-line interface.
"""
