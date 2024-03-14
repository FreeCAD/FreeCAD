# Tools

Each tool is stored as a JSON file which has the shape's path and values for all attributes of the shape.
It also includes all additional parameters and their values.

Storing a tool as a JSON file sounds great but eliminates the option of an accurate thumbnail. On the other hand,
storing each tool as a `*.fcstd` file requires more space and does not allow for generating tools. If one has an
extensive tool aresenal they might want to script the generation of tools which is easily done for a `*.json` file but
practically impossible for `*.fcstd` files.

When a tool is instantiated in a job the PDN body is created from the shape and the attributes and constraints are set
according to the values from the JSON file. All additional parameters are created as properties on the object. This
provides the correct shape and dimensions which can be used to generate a point cloud or mesh for advanced
algorithms (and potentially simulation).

# Tool Libraries

Due to each tool being stored in its own file and the storage/organization of those files being quite flexible the
importance of a tool library for organisational purposes is quite diminished. The user is free to organise their tools
in whichever directory hierarchy they see fit and can also name them as best fits their use and organisation. A
_tool library_ is nevertheless a great representation for a physical grouping of tools, such as in an automatic tool
changer.

A tool library is a (JSON) file with a mapping of tool id to the path of the tool file. As a consequence each tool
can be in multiple libraries and doesn't have an `id` of it's own. The `id` is a property of the library.

If a tool from a tool library (or an entire tool library) is added to a job it retains its `id` from the library as a
property. Adding a tool bit directly rsults in the tool getting the next free id assigned.

# Tool Controllers

They largely stay the same as they are today. As an additional feature it should be possible to _copy_ a TC, which
allows for easy feed/speed changes for the same tool.

Above requirement highlights one change though, that the `id` should be a property of the Bit, and not of the TC.
There are two requirements that are currently mapped to a single `id`. There needs to be an identification of which
TC is being used by a certain op, and which tool number to use for a `M6` command.

# Paths and Extensibility

The following directory structure is used for supplied (shipped with FreeCAD) tools:
```
  Tools
    + Bit
    + Library
    + Shape
```

Strictly speaking a user is free to store their tools wherever they want and however they want. By default the file
dialog will open the corresponding directory (depending on context), or whichever directory the user opened last.

Above directory structure with the most common default tools shipped with FreeCAD should be installed analogous to
TechDraw's templates.

## How to create a new tool

1. Set the tool's Label, this will show up in the object tree
1. Select a tool shape from the existing shape files. If your tool doesn't exist, you'll have to create a new shape,
   see below for details.
1. Each tool bit shape has its own set of parameters, fill them with the tool's values.
1. Select additional parameters
1. Save the tool under path/file that makes sense to you


## How to create a new tool bit Shape

The shape file for a tool bit is expected to contain a PD body which represents the tool as a 3d solid. The PD body
should be parametric based on a PropertyBag object so that, when the properties of the PropertyBag are changed the
solid is updated to the correct representation.

1. Create a new FreeCAD document
1. Open the `PartDesign` workbench, create a body and give the body a label you want to show up in the bit selection.
1. Open the Path workbench and (with the PD body selected) create a PropertyBag,
   menu 'Path' -> 'Utils' -> 'Property Bag'
   * this creates a PropertyBag object inside the Body (assuming it was selected)
   * add properties to which define the tool bit's shape and put those into the group 'Shape'
   * add any other properties to the bag which might be useful for the tool bit
1. Construct the body of the tool bit and assign expressions referencing properties from the PropertyBag (in the
   `Shape` Group) for all constraints.
   * Position the tip of the tool bit on the origin (0,0)
1. Save the document as a new file in the Shape directory
   * Before saving the document make sure you have _Save Thumbnail_ selected, and _Add program logo_ deselected in
     FreeCAD's preferences.
   * Also make sure to switch to _Front View_ and _Fit content to screen_
   * Whatever you see when saving the document will end up being the visual representation of tool bits with this shape

Not that 'Shape' is the only property group which has special meaning for tool bits. All other property groups are
copied verbatim to the ToolBit object when one is created.
