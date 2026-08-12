# About translating Sheet Metal Workbench

## Translators:

Translations for this workbench are done by visiting the **FreeCAD-addons**
project on CrowdIn platform at <https://crowdin.com/project/freecad-addons> webpage,
then find your language, look for the **Fasteners** project, and do the translation.

> [!IMPORTANT]
> The PR system for translations is no longer accepted. Please use CrowdIn above.

## Maintainers:

> [!NOTE]
> All commands **must** be run in the `./Resources/translations/` directory.
> QT6 lupdate and lrelease must be installed and in PATH.  
> Linux: sudo apt install qt6-l10n-tools qt6-base-dev  
> Windows: pip install PySide6 / find executables under Python installation  

> [!WARNING]
> If you want to update/release the files, you need to have `lupdate`
> and `lrelease` from Qt6 installed. Using Qt5 versions is not advised
> because they're buggy.

## Pulling updated translations from Crowdin

- Log in to Crowdin at <https://crowdin.com/project/freecad-addons>
- From the menu click translations.
- Expand the Downloads pane.
- If it does not exist, create a SheetMetal bundle. In the bundle content, select only SheetMetal.ts.
- Click the download icon on the SheetMetal bundle line.
- Extract the files into `./Resources/translations/` directory. This should create a `SheetMetal` subdirectory with all the translated files.

## Compiling translations

To convert all `.ts` files under subdir `SheetMetal` to `.qm` files use this command:  

```shell
python ./compile_translations
```
- This script takes care of all renaming necessary for FreeCAD to recognize the compiled files
- The script will not accept translation files that are less than 10% completed or locales not supported by FreeCAD
- Before committing the changes, make sure there are no new language files (.qm) needed to be added to git
- After compilation, the `SheetMetal` directory can be safely removed. only the `.qm` files are needed



## Updating translations template file

To update the template file from source files you should use this command:

```shell
python ./update_translations.py
```

This updates the SheetMetal.ts file.
Once done, you can commit the changes and upload the new file to CrowdIn platform
at <https://crowdin.com/project/freecad-addons> webpage and find the **Fasteners** project.  
(From the menu select sources, search the SheetMetal.ts file and click the update button.)



## Updating translations

Some notes for maintainers to consider:

- First update the `SheetMetal.ts` file then upload it to CrowdIn
- Remember to pull the translations from CrowdIn periodically
- Make sure you're using the correct `lupdate` version
- Make sure changes are working before making a PR

<https://github.com/shaise/FreeCAD_FastenersWB>

## More information

You can read more about translating external workbenches here:

<https://wiki.freecad.org/Translating_an_external_workbench>
