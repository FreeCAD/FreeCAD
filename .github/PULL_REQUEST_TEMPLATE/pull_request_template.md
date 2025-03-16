name: Create a PR for the main branch
description: Have you found something that does not work well, is too hard to do or is missing altogether? Please create a Problem Report.
body:
  - type: checkboxes
    id: terms
    attributes:
      label: Code of Conduct
      description: By submitting this PR, you agree to follow our [Code of Conduct](https://github.com/FreeCAD/FreeCAD/blob/main/CODE_OF_CONDUCT.md) and Contributing Guidelines](https://github.com/FreeCAD/FreeCAD/blob/main/CONTRIBUTING.md).
    options:
        - label: I agree to follow this project's Code of Conduct and Contributing Guidelines.
          required: true
