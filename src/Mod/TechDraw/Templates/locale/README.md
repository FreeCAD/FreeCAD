This folder (`locale`) contains translations for [TechDraw workbench templates](https://wiki.freecad.org/TechDraw_Templates) in the parent `Templates` folder.
The name of each `locale` subfolder represents a language, which follows [IETF BCP 47 standardized codes](https://en.wikipedia.org/wiki/IETF_language_tag). The original TechDraw templates in the parent folder are written using American English (`en-US`).

As such, the most basic name for a locale subfolder will include an [ISO 639 language code](https://en.wikipedia.org/wiki/ISO_639) (e.g. `de` for German). If it's necessary, additional subtags can be added to describe language variants. For instance variants spoken in a particular country, or a specific script. Those subtags are combinable and are based in other standards.

The most common additional subtag is an additional country code to describe a regional variant of the language (e.g. `de-DE` for German spoken in Germany, `es-AR` for Spanish spoken in Argentina, or `zh-CN` for Simplified Chinese in Mainland China). Country subtags are based on [the ISO 3166-1 standard's country codes](https://en.wikipedia.org/wiki/ISO_3166-1).

To add a translation:

1. Add a folder named `ll` or `ll-CC` (where `ll` is a 2-letter or 3-letter ISO 639 language code, and `CC` is a 2-letter ISO 3166-1 country code).
2. Copy over the TechDraw templates in the parent `Templates` folder that you want to translate to your new folder.
3. [Translate away!](https://wiki.freecad.org/TechDraw_Templates)
4. [Submit a PR (GitHub Pull Request)](https://freecad.github.io/DevelopersHandbook/gettingstarted/#submitting-a-pr) to get your Tech Draw template translations included in FreeCAD.

Use the [FreeCAD forum](https://forum.freecad.org/) if you need further help.