# customtitlebarkit

A small Qt 6 Widgets library that lets a `QMainWindow` draw its own title bar, so that toolbars,
menus and other widgets can be placed in it, like many current desktop applications do.

Author: Benjamin Nauck. License: LGPL-2.1-or-later (see the SPDX headers of the files).

## How it works

`CustomTitleBarWindow` is a `QMainWindow`. In `Mode::Native` it behaves exactly like a plain
`QMainWindow`. In `Mode::Custom` it reserves the height of a title bar at the top of the window and
shows a `TitleBarWidget` over it, with a left and a right area for the application's widgets.

The platform specific parts are in `src/platform`:

- Windows: the client area covers the title bar, the window keeps the window styles of the system
  (resizing, snapping, maximizing), and the title bar draws its own window buttons.
- macOS: the title bar keeps the traffic light buttons (with the title bar API of Qt 6.10 or
  newer, in a frameless window before).
- Linux: the window is frameless; the window buttons follow the desktop (order and icons) where it
  can be detected.
- A generic frameless backend with its own window buttons can be forced with the environment
  variable `CUSTOMTITLEBARKIT_MODE=generic` (`native` turns the custom title bar off).

`MenuIntegration` decides where the menu bar goes. `FoldableMenuIntegration` folds the menus behind
an application button in the title bar; they can be pinned to stay shown.

The icons of the window buttons are in `resources` and can be replaced with style sheets
(`qproperty-icon`, object names `minimizeButton`, `maximizeButton`, `restoreButton`,
`closeButton`).

## Use in FreeCAD

`Gui::MainWindow` derives from `CustomTitleBarWindow`. The custom title bar is off by default and
is turned on in Preferences > Display > UI (parameter `BaseApp/Preferences/MainWindow/CustomTitleBar`,
read at startup). The toolbars of the menu bar areas are then placed in the title bar.

`pixi.toml` and `examples` are only used to build and try the library on its own.
