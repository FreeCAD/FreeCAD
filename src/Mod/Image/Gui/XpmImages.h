/***************************************************************************
 *                                                                         *
 *   This is a header file containing xpm images for the image view.       *
 *                                                                         *
 *   Author:    Graeme van der Vlugt                                       *
 *   Copyright: Imetric 3D GmbH                                            *
 *   Year:      2004                                                       *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 ***************************************************************************/

#ifndef XpmImages_H
#define XpmImages_H

namespace ImageGui
{

/* XPM */
static const char *image_stretch[]={
"16 16 2 1",
". c #000000",
"# c #ffffff",
"................",
".##############.",
".#....####....#.",
".#..########..#.",
".#.#.######.#.#.",
".#.##.####.##.#.",
".#####.##.#####.",
".######..######.",
".######..######.",
".#####.##.#####.",
".#.##.####.##.#.",
".#.#.######.#.#.",
".#..########..#.",
".#....####....#.",
".##############.",
"................"};

/* XPM */
static const char *image_oneToOne[]={
"16 16 2 1",
". c #000000",
"# c #ffffff",
"................",
".##############.",
".##############.",
".###.#######.##.",
".##..######..##.",
".#.#.#####.#.##.",
".###.##..###.##.",
".###.##..###.##.",
".###.#######.##.",
".###.##..###.##.",
".###.##..###.##.",
".###.#######.##.",
".##...#####...#.",
".##############.",
".##############.",
"................"};

/* XPM */
static const char *image_orig[]={
"16 16 5 1",
". c #000000",
"b c #000080",
"a c #008000",
"c c #404040",
"# c #800000",
"................",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
"................",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
"................"};

/* XPM */
static const char *image_bright[]={
"16 16 5 1",
". c #000000",
"b c #0000ff",
"a c #00ff00",
"c c #c0c0c0",
"# c #ff0000",
"................",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
".#####aaaabbbbb.",
"................",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
".cccccccccccccc.",
"................"};

} // namespace ImageGui

#endif // XpmImages_H
