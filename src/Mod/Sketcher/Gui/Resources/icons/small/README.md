To create an XPM file from an SVG file, you need the ImageMagick libraries.
Then run
```
convert file.svg -geometry 16x16 -colors 8 file_sm.xpm
```

The XPM icon is very small, 16x16 px in size, and we usually don't need
more than 8 colors.

Edit the xpm file manually to do small retouches, for example, setting up
the transparency and reducing the number of colors exactly to the desired ones.

An XPM image has a header that defines the number of columns, number of rows,
number of colors, and number of characters per pixel.
The first rows have the colors definition, so they must match the number
of colors, while the rest corresponds to the actual bitmap image.

The space character (empty) can be set to the color `None`,
to indicate transparency.

```
/* XPM */
static char *file_sm_xpm[] = {
/* columns rows colors chars-per-pixel */
"16 16 3 1 ",
"  c None",
". c #D71414",
"+ c #AA1919",
/* pixels */
"                ",
"  +          +  ",
" +.+        +.+ ",
"  +.+      +.+  ",
"   +        +   ",
"      ++++      ",
"     +....+     ",
"     +...++     ",
"     +..+++     ",
"     +.++.+     ",
"      ++++      ",
"   +        +   ",
"  +.+      +.+  ",
" +.+        +.+ ",
"  +          +  ",
"                "
};
```
