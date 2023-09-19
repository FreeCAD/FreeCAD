/***************************************************************************
 *   Copyright (c) Juergen Riegel 2007    <juergen.riegel@web.de>          *
 *   LGPL                                                                  *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string.h>
#include <strstream>
#include <sys/types.h>
#include <vector>
#endif

#include <JtTk/JtkCADExporter.h>
#include <JtTk/JtkCADImporter.h>
#include <JtTk/JtkEntityFactory.h>
#include <JtTk/JtkTraverser.h>


using std::string;
using std::strstream;
using std::vector;

#include "JtReader.h"

int my_level = 0;
bool want_details = false;
int iLod = 0;
strstream InfoOut;

vector<SimpleMeshFacet> result;
vector<SimpleMeshFacet>::const_iterator resultIt;


#define indent(i)                                                                                  \
    {                                                                                              \
        for (int l = 0; l < i; l++)                                                                \
            InfoOut << "   ";                                                                      \
    }

void printXform(JtkTransform* partXform, int level)
{
    float* elements = NULL;

    indent(level);
    InfoOut << "JtkTRANSFORM\n";

    partXform->getTElements(elements);
    if (elements) {
        indent(level + 1);
        InfoOut << elements[0] << ", " << elements[1] << ", " << elements[2] << ", " << elements[3]
                << "\n";
        indent(level + 1);
        InfoOut << elements[4] << ", " << elements[5] << ", " << elements[6] << ", " << elements[7]
                << "\n";
        indent(level + 1);
        InfoOut << elements[8] << ", " << elements[9] << ", " << elements[10] << ", "
                << elements[11] << "\n";
        indent(level + 1);
        InfoOut << elements[12] << ", " << elements[13] << ", " << elements[14] << ", "
                << elements[15] << "\n";
#ifdef _DEBUG
        JtkEntityFactory::deleteMemory(elements);
#else
        delete[] elements;
#endif
    }
}

void printMaterial(JtkMaterial* partMaterial, int level)
{
    float *ambient = NULL, *diffuse = NULL, *specular = NULL, *emission = NULL, shininess = -999.0;

    indent(level);
    InfoOut << "JtkMATERIAL\n";

    partMaterial->getAmbientColor(ambient);
    if (ambient) {
        indent(level + 1);
        InfoOut << "ambient = ( " << ambient[0] << ", " << ambient[1] << ", " << ambient[2] << ", "
                << ambient[3] << " )\n";
#ifdef _DEBUG
        JtkEntityFactory::deleteMemory(ambient);
#else
        delete[] ambient;
#endif
    }

    partMaterial->getDiffuseColor(diffuse);
    if (diffuse) {
        indent(level + 1);
        InfoOut << "diffuse = ( " << diffuse[0] << ", " << diffuse[1] << ", " << diffuse[2] << ", "
                << diffuse[3] << " )\n";
#ifdef _DEBUG
        JtkEntityFactory::deleteMemory(diffuse);
#else
        delete[] diffuse;
#endif
    }

    partMaterial->getSpecularColor(specular);
    if (specular) {
        indent(level + 1);
        InfoOut << "specular = ( " << specular[0] << ", " << specular[1] << ", " << specular[2]
                << ", " << specular[3] << " )\n";
#ifdef _DEBUG
        JtkEntityFactory::deleteMemory(specular);
#else
        delete[] specular;
#endif
    }

    partMaterial->getEmissionColor(emission);
    if (emission) {
        indent(level + 1);
        InfoOut << "emission = ( " << emission[0] << ", " << emission[1] << ", " << emission[2]
                << ", " << emission[3] << " )\n";
#ifdef _DEBUG
        JtkEntityFactory::deleteMemory(emission);
#else
        delete[] emission;
#endif
    }

    partMaterial->getShininess(shininess);
    if (shininess != -999.0) {
        indent(level + 1);
        InfoOut << "shininess = " << shininess << "\n";
    }
}

void printBrep(JtkBrep* partBrep, int level)
{
    indent(level);
    InfoOut << "JtkBREP\n";
}

void printWrep(JtkWrep* partWrep, int level)
{
    indent(level);
    InfoOut << "JtkWREP\n";
}

void printShape(JtkShape* partShape, int level)
{
    indent(level);
    InfoOut << "JtkSHAPE\n";

    for (int set = 0; set < partShape->numOfSets(); set++) {
        indent(level + 1);
        InfoOut << "geom set #" << set << ":\n";

        float *vertex = NULL, *normal = NULL, *color = NULL, *texture = NULL;
        int vertexCount = -1, normCount = -1, colorCount = -1, textCount = -1;

        partShape->getInternal(vertex,
                               vertexCount,
                               normal,
                               normCount,
                               color,
                               colorCount,
                               texture,
                               textCount,
                               set);

        if (vertex && (vertexCount > 0)) {
            indent(level + 2);
            InfoOut << "vertices = ( ";

            for (int elems = 0; elems < vertexCount * 3; elems++) {
                InfoOut << ((elems != 0) ? ", " : "") << vertex[elems];
            }

            InfoOut << " )\n";

#ifdef _DEBUG
            JtkEntityFactory::deleteMemory(vertex);
#else
            delete[] vertex;
#endif
        }

        if (normal && (normCount > 0)) {
            indent(level + 2);
            InfoOut << "normals = ( ";

            for (int elems = 0; elems < normCount * 3; elems++) {
                InfoOut << ((elems != 0) ? ", " : "") << normal[elems];
            }

            InfoOut << " )\n";

#ifdef _DEBUG
            JtkEntityFactory::deleteMemory(normal);
#else
            delete[] normal;
#endif
        }

        if (color && (colorCount > 0)) {
            indent(level + 2);
            InfoOut << "color = ( ";

            for (int elems = 0; elems < colorCount * 3; elems++) {
                InfoOut << ((elems != 0) ? ", " : "") << color[elems];
            }

            InfoOut << " )\n";

#ifdef _DEBUG
            JtkEntityFactory::deleteMemory(color);
#else
            delete[] color;
#endif
        }

        if (texture && (textCount > 0)) {
            indent(level + 2);
            InfoOut << "texture = ( ";

            for (int elems = 0; elems < textCount; elems++) {
                InfoOut << ((elems != 0) ? ", " : "") << texture[elems];
            }

            InfoOut << " )\n";

#ifdef _DEBUG
            JtkEntityFactory::deleteMemory(texture);
#else
            delete[] texture;
#endif
        }
    }
}

int myPreactionCB_PrintName(JtkHierarchy* CurrNode, int level, JtkClientData*)
{
    indent(level);

    my_level++;

    switch (CurrNode->typeID()) {
        case JtkEntity::JtkNONE:
            InfoOut << "JtkNONE\n";
            break;

        case JtkEntity::JtkBREP:
            InfoOut << "JtkBREP\n";
            break;

        case JtkEntity::JtkREGION:
            InfoOut << "JtkREGION\n";
            break;

        case JtkEntity::JtkSHELL:
            InfoOut << "JtkSHELL\n";
            break;

        case JtkEntity::JtkLOOP:
            InfoOut << "JtkLOOP\n";
            break;

        case JtkEntity::JtkCOEDGE:
            InfoOut << "JtkCOEDGE\n";
            break;

        case JtkEntity::JtkEDGE:
            InfoOut << "JtkEDGE\n";
            break;

        case JtkEntity::JtkVERTEX:
            InfoOut << "JtkVERTEX\n";
            break;

        case JtkEntity::JtkNURBSSURFACE:
            InfoOut << "JtkNURBSSURFACE\n";
            break;

        case JtkEntity::JtkUVCURVE:
            InfoOut << "JtkUVCURVE\n";
            break;

        case JtkEntity::JtkXYZCURVE:
            InfoOut << "JtkXYZCURVE\n";
            break;

        case JtkEntity::JtkTRISTRIPSET:
            InfoOut << "JtkTRISTRIPSET\n";
            break;

        case JtkEntity::JtkPOINTSET:
            InfoOut << "JtkPOINTSET\n";
            break;

        case JtkEntity::JtkLINESTRIPSET:
            InfoOut << "JtkLINESTRIPSET\n";
            break;

        case JtkEntity::JtkPOLYGONSET:
            InfoOut << "JtkPOLYGONSET\n";
            break;

        case JtkEntity::JtkPOINT:
            InfoOut << "JtkPOINT\n";
            break;

        case JtkEntity::JtkMATERIAL:
            InfoOut << "JtkMATERIAL\n";
            break;

        case JtkEntity::JtkTRANSFORM:
            InfoOut << "JtkTRANSFORM\n";
            break;

        case JtkEntity::JtkPROPERTY:
            InfoOut << "JtkPROPERTY\n";
            break;

        case JtkEntity::JtkPART: {
            InfoOut << "JtkPART: ";
            InfoOut << CurrNode->name() << "\n";

            if (want_details) {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {
                    printXform(partXform, level + 1);
                }

                JtkMaterial* partMaterial = NULL;
                ((JtkPart*)CurrNode)->getMaterial(partMaterial);
                if (partMaterial) {
                    printMaterial(partMaterial, level + 1);
                }

                JtkBrep* partBrep = NULL;
                ((JtkPart*)CurrNode)->getBrep(partBrep);
                if (partBrep) {
                    printBrep(partBrep, level + 1);
                }

                JtkWrep* partWrep = NULL;
                ((JtkPart*)CurrNode)->getWrep(partWrep);
                if (partWrep) {
                    printWrep(partWrep, level + 1);
                }

                int partNumShapeLODs = -1;
                partNumShapeLODs = ((JtkPart*)CurrNode)->numPolyLODs();
                for (int lod = 0; lod < partNumShapeLODs; lod++) {
                    indent(level + 1);
                    InfoOut << "LOD#" << lod << ":\n";

                    int partNumShapes = -1;
                    partNumShapes = ((JtkPart*)CurrNode)->numPolyShapes(lod);
                    for (int shNum = 0; shNum < partNumShapes; shNum++) {
                        indent(level + 2);
                        InfoOut << "Shape#" << shNum << ":\n";

                        JtkShape* partShape = NULL;
                        ((JtkPart*)CurrNode)->getPolyShape(partShape, lod, shNum);
                        if (partShape) {
                            printShape(partShape, level + 3);
                        }
                    }
                }
            }
        } break;

        case JtkEntity::JtkASSEMBLY: {
            InfoOut << "JtkASSEMBLY: ";
            InfoOut << CurrNode->name() << "(" << ((JtkAssembly*)CurrNode)->numChildren()
                    << " children)\n";

            if (want_details) {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {
                    printXform(partXform, level + 1);
                }

                JtkMaterial* partMaterial = NULL;
                ((JtkPart*)CurrNode)->getMaterial(partMaterial);
                if (partMaterial) {
                    printMaterial(partMaterial, level + 1);
                }
            }
        } break;

        case JtkEntity::JtkINSTANCE: {
            InfoOut << "JtkINSTANCE: ";
            InfoOut << CurrNode->name() << "\n";

            if (want_details) {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {
                    printXform(partXform, level + 1);
                }

                JtkMaterial* partMaterial = NULL;
                ((JtkPart*)CurrNode)->getMaterial(partMaterial);
                if (partMaterial) {
                    printMaterial(partMaterial, level + 1);
                }
            }
        } break;


        case JtkEntity::JtkCLIENTDATA:
            InfoOut << "JtkCLIENTDATA\n";
            break;

        case JtkEntity::JtkWIRE:
            InfoOut << "JtkWIRE\n";
            break;
    }

    return (Jtk_OK);
}

void insertShapeFaces(JtkShape* partShape)
{
    for (int set = 0; set < partShape->numOfSets(); set++) {
        float *vertex = NULL, *normal = NULL, *color = NULL, *texture = NULL;
        int vertexCount = -1, normCount = -1, colorCount = -1, textCount = -1;

        partShape->getInternal(vertex,
                               vertexCount,
                               normal,
                               normCount,
                               color,
                               colorCount,
                               texture,
                               textCount,
                               set);

        if (normCount < 3) {
            return;
        }


        if (vertex && (vertexCount > 0) && normal && (normCount > 0)) {
            for (int i = 0; i < vertexCount - 2; i++) {
                SimpleMeshFacet temp;
                temp.n[0] = normal[i * 3 + 0];
                temp.n[1] = normal[i * 3 + 1];
                temp.n[2] = normal[i * 3 + 2];
                temp.p1[0] = vertex[i * 3 + 0];
                temp.p1[1] = vertex[i * 3 + 1];
                temp.p1[2] = vertex[i * 3 + 2];
                temp.p2[0] = vertex[i * 3 + 3];
                temp.p2[1] = vertex[i * 3 + 4];
                temp.p2[2] = vertex[i * 3 + 5];
                temp.p3[0] = vertex[i * 3 + 6];
                temp.p3[1] = vertex[i * 3 + 7];
                temp.p3[2] = vertex[i * 3 + 8];

                result.push_back(temp);
                /*
                          file << "  facet normal "<< normal[i*3+0] << " " << normal[i*3+1] << " "
                   << normal[i*3+2] << " " << endl; file << "    outer loop" << endl; file << "
                   vertex " << vertex[i*3+0] << " " << vertex[i*3+1] << " " << vertex[i*3+2] << " "
                   << endl; file << "      vertex " << vertex[i*3+3] << " " << vertex[i*3+4] << " "
                   << vertex[i*3+5] << " " << endl; file << "      vertex " << vertex[i*3+6] << " "
                   << vertex[i*3+7] << " " << vertex[i*3+8] << " " << endl; file << "    endloop" <<
                   endl; file << "  endfacet" << endl;
                */
            }
        }
#ifdef _DEBUG
        if (vertex) {
            JtkEntityFactory::deleteMemory(vertex);
        }
        if (vertex) {
            JtkEntityFactory::deleteMemory(normal);
        }
        if (color) {
            JtkEntityFactory::deleteMemory(color);
        }
        if (texture) {
            JtkEntityFactory::deleteMemory(texture);
        }
#else
        if (vertex) {
            delete[] vertex;
        }
        if (vertex) {
            delete[] normal;
        }
        if (color) {
            delete[] color;
        }
        if (texture) {
            delete[] texture;
        }
#endif
    }
}


int myPreactionCB_CollectFacets(JtkHierarchy* CurrNode, int level, JtkClientData*)
{
    switch (CurrNode->typeID()) {

        case JtkEntity::JtkPART: {

            {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {
                    printXform(partXform, level + 1);
                }

                int partNumShapeLODs = -1;
                partNumShapeLODs = ((JtkPart*)CurrNode)->numPolyLODs();
                for (int lod = 0; lod < partNumShapeLODs; lod++) {
                    indent(level + 1);
                    InfoOut << "LOD#" << lod << ":\n";

                    if (iLod != lod && iLod != -1) {
                        continue;
                    }

                    int partNumShapes = -1;
                    partNumShapes = ((JtkPart*)CurrNode)->numPolyShapes(lod);
                    for (int shNum = 0; shNum < partNumShapes; shNum++) {
                        indent(level + 2);
                        InfoOut << "Shape#" << shNum << ":\n";

                        JtkShape* partShape = NULL;
                        ((JtkPart*)CurrNode)->getPolyShape(partShape, lod, shNum);
                        if (partShape) {
                            insertShapeFaces(partShape);
                        }
                    }
                }
            }
        } break;

        case JtkEntity::JtkASSEMBLY: {
            InfoOut << "JtkASSEMBLY: ";
            InfoOut << CurrNode->name() << "(" << ((JtkAssembly*)CurrNode)->numChildren()
                    << " children)\n";

            {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {}
            }
        } break;

        case JtkEntity::JtkINSTANCE: {
            {
                JtkTransform* partXform = NULL;
                ((JtkPart*)CurrNode)->getTransform(partXform);
                if (partXform) {}
            }
        } break;
    }
    return (Jtk_OK);
}

/* Interface functions */

void testLicence(void)
{
    // Try to create a JtkCADExporter to test for JT write/general licensing
    JtkCADExporter* jtwriter = NULL;
    jtwriter = JtkEntityFactory::createCADExporter();
    if (!jtwriter) {
        cerr << "No export license found.\n";
        exit(1);
    }
    else {
        jtwriter->ref();
        jtwriter->unref();
        jtwriter = NULL;
    }

    // Try to create a JtkCADImporter to test for JT read licensing
    JtkCADImporter* jtreader = NULL;
    jtreader = JtkEntityFactory::createCADImporter();
    if (!jtreader) {
        cerr << "No import license found.\n";
        exit(1);
    }
    else {
        jtreader->ref();
        jtreader->unref();
        jtreader = NULL;
    }
}

void readFile(const char* FileName, int iLods)
{
    iLod = iLods;

    testLicence();

    JtkCADImporter* importer = NULL;
    importer = JtkEntityFactory::createCADImporter();
    if (importer) {
        importer->ref();
        importer->setShapeLoadOption(JtkCADImporter::JtkALL_LODS);
        importer->setBrepLoadOption(JtkCADImporter::JtkTESS_AND_BREP);
        JtkHierarchy* root = NULL;

        root = importer->import(FileName);

        if (root) {
            root->ref();
            JtkTraverser* trav = JtkEntityFactory::createTraverser();
            trav->setupPreActionCallback(myPreactionCB_CollectFacets);
            if (trav) {
                trav->ref();
                trav->traverseGraph(root);
                trav->unref();
                trav = NULL;
            }
            else {
                throw "Unable to create JtkTraverser.\n";
            }

            root->unref();
            root = NULL;
        }
        else {
            throw "Unable in find root node.  Check file...\n";
        }

        importer->unref();
        importer = NULL;
    }
    else {
        throw "Unable to create JtkCADImporter.  Check license...\n";
    }

    // Uninitialize JtTk
    // JtkEntityFactory::fini();
}


const SimpleMeshFacet* iterStart(void)
{
    resultIt = result.begin();
    if (resultIt != result.end()) {
        return &(*(resultIt));
    }
    else {
        return 0;
    }
}

const SimpleMeshFacet* iterGetNext(void)
{
    if (++resultIt != result.end()) {
        return &(*(resultIt));
    }
    else {
        return 0;
    }
}

unsigned int iterSize(void)
{
    return (unsigned int)result.size();
}


/** clears the internal structure */
void clearData(void)
{
    result.clear();
    resultIt = result.begin();
    InfoOut.clear();
    my_level = 0;
}
