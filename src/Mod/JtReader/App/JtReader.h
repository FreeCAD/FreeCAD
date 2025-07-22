/***************************************************************************
 *   Copyright (c) Juergen Riegel 2007    <juergen.riegel@web.de>          *
 *   LGPL                                                                  *
 ***************************************************************************/


#ifndef __JtReader_h__
#define __JtReader_h__


/** simple facet structure */
struct SimpleMeshFacet
{
    float p1[3], p2[3], p3[3], n[3];
};

/** Reads a JT File an build up the internal data structure
 * imports all the meshes of all Parts, recursing the Assamblies.
 */
void readFile(const char* FileName, int iLods = 0);

/** Write the read Part to a file */
void writeAsciiSTL(const char* FileName);

/** start the iterator on the result */
const SimpleMeshFacet* iterStart(void);

/** get the faces or 0 at end */
const SimpleMeshFacet* iterGetNext(void);

/** size of the result */
unsigned int iterSize(void);

/** clears the internal structure */
void clearData(void);

#endif  // __JtReader_h__
