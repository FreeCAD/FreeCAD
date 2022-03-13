## FreeCAD SearchBar

This FreeCAD mod adds a search bar for tools, document objects and preferences.

⚠️ Several issues related to the C++ memory management interacting badly with Python's have caused lots of segfaults during development. ⚠️

⚠️️ Most of these should now be solved, but save your work often and proceed with caution while testing this extension. ⚠️

### Extensibility

It can be extended by other mods, by adding a new result provider.

### Usage

The search bar appears next to the [`What's this?`](https://wiki.freecad.org/Std_WhatsThis) tool <a href="https://wiki.freecad.org/Std_WhatsThis"><img src="https://user-images.githubusercontent.com/4140247/156215976-5dfadb0c-cac4-44b2-8ad4-b67462a5f7fa.png" alt="drawing" width="20px" height="20px"/></a> in FreeCAD's default File toolbar.

![Screenshot of the search bar, with results in its drop-down menu and extra info about the result in a separate pane](screenshot.png)

![Animation showing how to initially load all workbenches using the first entry in the search bar](animAopt.gif)
![Animation showing how to navigate the search results with the up and down keys and select code examples from the results](animB2op.gif)

### Installation

#### Automatic Install

Install **SearchBar** addon via the FreeCAD Addon Manager from the **Tools** :arrow_right: **Addon Manager** dropdown menu. 

#### Manual Install

<details>
<summary>Expand for instructions on manual installation of SearchBar</summary>

Clone the GIT repository or extract the `.zip` downloaded from GitHub to the following location:
  * Linux: `~/.FreeCAD/Mod/SearchBar`
  * macOS: `/Users/user_name/Library/Preferences/FreeCAD/Mod/SearchBar`
  * Windows: `C:\Users\user_name\AppData\Roaming\FreeCAD\Mod\SearchBar`

### Uninstallation

* Remove the folder which was cloned during installation:
  * Linux: `~/.FreeCAD/Mod/SearchBar`
  * macOS: `/Users/user_name/Library/Preferences/FreeCAD/Mod/SearchBar`
  * Windows: `C:\Users\user_name\AppData\Roaming\FreeCAD\Mod\SearchBar`
* Optional: Remove the cache (\~30MB) `\~/.FreeCAD/Cache_SearchBarMod` or equivalent on other platforms

</details>

### Development

* `InitGui.py` adds an instance of `SearchBoxLight` to the GUI.
* `SearchBoxLight` is a hollowed-out implementation of a search box, it loads everything lazily.

### Feedback

To report bugs or feature enhancements, please open a ticket in the [issue queue](https://github.com/SuzanneSoy/SearchBar/issues). Best place to discuss feedback or issues in on the [dedicated FreeCAD forum discussion]() for SearchBar. 

### License [![License: CC0 v1.0.](https://img.shields.io/badge/license-CC0-blue.svg)](https://creativecommons.org/publicdomain/zero/1.0/)
See [LICENSE](LICENSE).
This repository is in the public domain.
