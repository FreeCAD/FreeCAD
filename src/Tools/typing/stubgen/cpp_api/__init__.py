"""C++ API documentation generation pipeline.

The package keeps the C++ documentation stages explicit:

* ``doxygen`` runs Doxygen and produces XML.
* ``extract`` converts XML into the semantic model from ``model``.
* ``markdown`` renders API pages for the documentation site.
* ``starlight`` renders the navigation fragment.
* ``pipeline`` coordinates those stages for the command-line interface.
"""
