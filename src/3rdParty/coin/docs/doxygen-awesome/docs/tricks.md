# Tips & Tricks

[TOC]

## Diagrams with Graphviz {#tricks-graphviz}

To get the best-looking class diagrams for your documentation, generate them with Graphviz as vector graphics with transparent background:

```
# Doxyfile
HAVE_DOT = YES
DOT_IMAGE_FORMAT = svg
DOT_TRANSPARENT = YES
```

In case `INTERACTIVE_SVG = YES` is set in the Doxyfile, all user-defined dotgraphs must be wrapped with the `interactive_dotgraph` CSS class for them to be rendered correctly:

```md
<div class="interactive_dotgraph">

\dotfile graph.dot

</div>
```

@note Both the default overflow scrolling behavior in this theme and the interactive editor enabled by `INTERACTIVE_SVG` are unsatisfying workarounds IMHO. Consider designing your graphs to be narrow enough to fit the page to avoid scrolling.

## Disable Dark Mode {#tricks-darkmode}

If you don't want the theme to automatically switch to dark mode depending on the browser preference,
you can disable dark mode by adding the `light-mode` class to the HTML tag in the header template:

```html
<html xmlns="http://www.w3.org/1999/xhtml" class="light-mode">
```

The same can be done to always enable dark mode:

```html
<html xmlns="http://www.w3.org/1999/xhtml" class="dark-mode">
```


@warning This only works if you don't use the dark-mode toggle extension.

## Choosing Sidebar Width {#tricks-sidebar}

If you have enabled the sidebar-only theme variant, make sure to carefully choose a proper width for your sidebar.
It should be wide enough to hold the icon, project title and version number. If the content is too wide, it will be
cut off.

```css
html {
    /* Make sure sidebar is wide enough to contain the page title (logo + title + version) */
    --side-nav-fixed-width: 335px;
}
```

The chosen width should also be set in the Doxyfile:

```
# Doxyfile
TREEVIEW_WIDTH = 335
```

## Formatting Tables {#tricks-tables}

By default tables in this theme are left-aligned and as wide as required to fit their content.
Those properties can be changed for individual tables.

### Centering

Tables can be centered by wrapping them in the `<center>` HTML tag.

<div class="tabbed">

- <span class="tab-title">Code</span>
    ```md
    <center>
        | This table | is centered          |
        |------------|----------------------|
        | test 1     | test 2               |
    </center>
    ```
- <span class="tab-title">Result</span>
    <center>
        | This table | is centered |
        |------------|----------------------|
        | test 1     | test 2               |
    </center>

</div>



### Full Width

To make tables span the full width of the page, no matter how wide the content is, wrap the table in the `full_width_table` CSS class.

@warning Apply with caution! This breaks the overflow scrolling of the table. Content might be cut off on small screens!

<div class="tabbed">

- <span class="tab-title">Code</span>
    ```md
    <div class="full_width_table">
        | This table | spans the full width |
        |------------|----------------------|
        | test 1     | test 2               |
    </div>
    ```
- <span class="tab-title">Result</span>
    <div class="full_width_table">
        | This table | spans the full width |
        |------------|----------------------|
        | test 1     | test 2               |
    </div>

</div>

<div class="section_buttons">

| Previous                          |                                   Next |
|:----------------------------------|---------------------------------------:|
| [Customization](customization.md) | [Example](https://jothepro.github.io/doxygen-awesome-css/class_my_library_1_1_example.html) |

</div>