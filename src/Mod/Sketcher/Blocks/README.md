# Sketcher Blocks

Each `.txt` file is a geometry snapshot in the format produced by Sketcher's
**Copy Elements** command. To create a block, select at least two edges while
editing a sketch, then choose **Create Block** from the right-click menu or
the Sketcher tools menu. Choose a name in the save dialog, which starts in the
default block library folder. The sketch and clipboard remain unchanged.
Alternatively, save Copy Elements clipboard text as UTF-8. Insert Block can
browse to that file.
Files in the user's `Mod/Sketcher/Blocks` directory also appear in the library;
subdirectories provide categories. User entries take precedence over bundled
entries with the same relative name.

The loader accepts the geometry-building statements emitted by Copy Elements.
It preserves construction geometry. Copied constraints are accepted as part of
the format; the inserted geometry is a snapshot controlled by a Group handle.
The loader does not execute arbitrary Python scripts.

The Group constraint stores its source in `File` metadata and the chosen width
or height mode in `FileHeight`. Select the Group in the constraints list and
choose **Reload From File** to update it. Reload retains the handle, its
dimensions and the Group's name. It replaces all
members, so constraints attached directly to those old members are removed.
The stored geometry remains usable when the source file is unavailable.

## Bundled marks

The nine text entries are geometry conversions of the library originally
submitted with the Symbol tool. The conversion changes the file format, not
the designs. References for the three marks raised during review:

- **CE:** the European Commission provides the mark for reproduction on its
  [CE marking page](https://single-market-economy.ec.europa.eu/single-market/goods/ce-marking_en).
  Applying it to a product is subject to the applicable conformity requirements.
- **FCC:** the FCC permits manufacturers to use the conformity logo voluntarily
  on compliant devices; see paragraph 18 of
  [FCC 17-93](https://docs.fcc.gov/public/attachments/FCC-17-93A1.pdf).
- **Recycling:** Gary Anderson stated that his winning design was intended for
  the public domain. The original interview and account of the withdrawn
  trademark application appear on page 2 of
  [Resource Recycling, May 1999](https://logoblink.com/img/2008/03/recycling_symbol_garyanderson.pdf).

These files are drawing aids. Including a mark in a sketch does not establish
product conformity or certify a recycling claim.
