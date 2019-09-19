To create an XPM file from an SVG file, you need the ImageMagick libraries.
Then run
```
convert file.svg -geometry 16x16 -colors 16 file_sm.xpm
```

The XPM icon is very small, 16x16 px in size, and you usually don't need
more than 16 colors.

Edit the xpm file manually to do small retouches, for example, setting up
the transparency and reducing the number of colors exactly to the desired ones.

The space character (empty) can be set to the color `None`,
to indicate transparency.

```
/* XPM */
static char * file_sm_xpm[] = {
"16 16 7 1",
"     c None",
".    c #BB1616",
"+    c #DE1515",
"@    c #BE1616",
```
