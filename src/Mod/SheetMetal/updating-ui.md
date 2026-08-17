# Updating UnfoldOptions.ui

1. Generate the Python code from UnfoldOptions.ui by: 

    ```
    pyuic4 UnfoldOptions.ui -o myui.py
    ```

2. Copy and paste relevant codes into the `SMUnfoldTaskPanel()` class in `SheetMetalUnfolder.py`.


# TODO

* Use `UnfoldOptions.ui` file directly.
