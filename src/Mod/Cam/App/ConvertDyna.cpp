/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


/******CONVERTDYNA.CPP******/

/******MAIN INCLUDES******/
#include "PreCompiled.h"
#include "ConvertDyna.h"
#include <cstdlib>

ReadDyna::ReadDyna(MeshCore::MeshKernel &m, const char* &inputname)
{
    //Open the file and perform standard check for file
    std::ifstream inputfile;
    inputfile.open(inputname);
    std::string line;
    if (!inputfile.is_open())  //Exists...?
    {
        std::cerr << "File not found. Exiting..." << std::endl;
        return;
    }
    getline(inputfile,line);
    if (line.find("*KEYWORD") == std::string::npos)  //Really a DYNA file...?
    {
        std::cerr << "NOT A DYNA FILE\a" << std::endl;
        return;
    }
    while (inputfile) //First find *NODE Keyword to initialize the VERTICES list (it's the main list)
    {
        getline(inputfile,line);
        if (line.find("*NODE") != std::string::npos)
        {
            std::cout << "*NODE found. Parsing... ";
            ReadNode(inputfile);
            std::cout << "done\n";
            break;
        }
    }
    inputfile.seekg(std::ios::beg);   //bring back the file pointer, you never know where those two other will spawn...
    if (inputfile.fail())
        inputfile.clear();   //...clear the badbits...
    while (inputfile) //and reread.
    {
        getline(inputfile,line);
        if (line.find("*ELEMENT_SHELL_THICKNESS") != std::string::npos)  //For Faces
        {
            std::cout << "*ELEMENT_SHELL_THICKNESS found. Parsing... ";
            ReadShellThickness(inputfile);
            std::cout << "done\n";
        }
        else if (line.find("*CONSTRAINED_ADAPTIVITY") != std::string::npos) //For constraints
        {
            std::cout << "*CONSTRAINED_ADAPTIVITY found. Parsing... ";
            ReadConstraints(inputfile);
            std::cout << "done\n";
        }
    }
    inputfile.close();
    std::cout << "Triangulating... ";
    Convert();
    std::cout << "done\nWriting into list... ";
    PutInMesh(m);
    std::cout << "done\n";
}
/*! \brief *NODE Keyword Information Parser
*/
void ReadDyna::ReadNode(std::ifstream &inputfile)
{
    std::string line;
    getline(inputfile, line);
    char *cstr;
    char Byte[16] = {0};
    char Nibble[8] = {0};
    int i = 0, j = 0;
    while (line.find("*") == std::string::npos) //while next keyword is not yet found...
    {
        if (line.find("$") != std::string::npos) //comment, ignore
        {
            getline(inputfile, line);
            continue;
        }
        VERTICES Temp;
        Temp.Constrain = false;   //Initialize both flags to false
        Temp.Constrained = false;
        cstr = new char [line.size()+1];
        strcpy(cstr, line.c_str());
        for (i = 0; i < 8; i++)
        {
            Nibble[j] = cstr[i];
            j++;
        }
        Temp.PointIndex = atoi(Nibble);  //Index
        j = 0;
        for (i = 8; i < 24; i++)
        {
            Byte[j] = cstr[i];
            j++;
        }
        Temp.Coords.push_back(atof(Byte));  //X Coords
        j = 0;

        for (i = 24; i < 40; i++)
        {
            Byte[j] = cstr[i];
            j++;
        }
        Temp.Coords.push_back(atof(Byte));  //Y Coords
        j = 0;

        for (i = 40; i < 56; i++)
        {
            Byte[j] = cstr[i];
            j++;
        }
        Temp.Coords.push_back(atof(Byte));   //Z Coords
        j = 0;

        Pointlist.insert(std::pair<unsigned int, VERTICES>(Temp.PointIndex, Temp));
        delete[] cstr;
        getline(inputfile, line);
    }
}
/*! \brief Face Reader

 Inside *ELEMENT_SHELL_THICKNESS there's the information about which points makes up a face. All other infos are ignored.

 Maybe it is in other keywords there is such infos, but I dunno...
*/
void ReadDyna::ReadShellThickness(std::ifstream &inputfile)
{
    std::map<unsigned int, VERTICES>::iterator pnt_it(Pointlist.begin());
    std::string line;
    char *c_str;
    char Info[8] = {0};
    int i = 0, j = 0;
    getline(inputfile, line);
    while (line.find("*") == std::string::npos)  //While next keyword is not yet found..
    {
        if (line.find("$") != std::string::npos) //Ignore the comments
        {
            getline(inputfile, line);
            continue;
        }
        FACE Temp;
        c_str = new char [line.size()+1];
        strcpy(c_str, line.c_str());

        for (i = 0; i < 8; i++,j++)
            Info[j] = c_str[i];
        Temp.FaceIndex = atoi(Info);    //Facet Index
        j= 0;

        for (i = 16; i < 24; i++,j++)
            Info[j] = c_str[i];
        Temp.PointIndex.push_back(atoi(Info));   //First Point
        j = 0;

        for (i = 24; i < 32; i++,j++)
            Info[j] = c_str[i];
        Temp.PointIndex.push_back(atoi(Info));   //Second Point
        j = 0;

        for (i = 32; i < 40; i++,j++)
            Info[j] = c_str[i];
        Temp.PointIndex.push_back(atoi(Info));   //Third Point
        j = 0;

        for (i = 40; i < 48; i++,j++)
            Info[j] = c_str[i];
        Temp.PointIndex.push_back(atoi(Info));   //Fourth Point
        j = 0;

        getline(inputfile,line);
        /*If you need extra info from here, extract it here*/

        for (unsigned int k = 0; k < Temp.PointIndex.size(); k++)
        {
            pnt_it = Pointlist.find(Temp.PointIndex[k]);
            (*pnt_it).second.FacetRef.push_back(Temp.FaceIndex);    //Put into the Facet Reference

        }
        Facelist.insert(std::pair<unsigned int,FACE>(Temp.FaceIndex, Temp));
        getline(inputfile,line);
    }
}

/*! \brief *CONSTRAINTS_ADATIVITY parser
*/
void ReadDyna::ReadConstraints(std::ifstream &inputfile)
{
    std::map<unsigned int, VERTICES>::iterator pnt_it(Pointlist.begin());
    std::string line;
    char *str;
    char Info[10] = {0};
    int i = 0, j = 0;
    unsigned int First;
    unsigned int Second;
    unsigned int Third;
    getline(inputfile, line);
    while (line.find("*") == std::string::npos)  //While... you know the drill
    {
        if (line.find("$") != std::string::npos) //Ignore comments...
        {
            getline(inputfile, line);
            continue;
        }
        str = new char [line.size()+1];
        strcpy(str, line.c_str());

        for (i = 0; i < 10; i++,j++)
            Info[j] = str[i];
        First = atoi(Info);   //Constrained Point

        j = 0;

        for (i = 10; i < 20; i++,j++)
            Info[j] = str[i];
        Second = atoi(Info);  //Constrainer 1

        j = 0;

        for (i = 20; i < 30; i++,j++)
            Info[j] = str[i];
        Third = atoi(Info);  //Constrainer 2

        j = 0;

        //Set all necessary flags fill the necessary std::vector's
        pnt_it = Pointlist.find(First);
        (*pnt_it).second.Constrained = true;
        (*pnt_it).second.ConstrainedBy.push_back(Second);
        (*pnt_it).second.ConstrainedBy.push_back(Third);

        pnt_it = Pointlist.find(Second);
        (*pnt_it).second.Constrain = true;
        (*pnt_it).second.Constraining.push_back(First);

        pnt_it = Pointlist.find(Third);
        (*pnt_it).second.Constrain = true;
        (*pnt_it).second.Constraining.push_back(First);

        delete[] str;
        getline(inputfile,line);
    }
}

/*! \brief Triangulator

 This will triangulate the squares, taking note of the constraining point that might come into play.

 To date, only three things are considered now: No constraints in the edges, One Constraint in the edges, and Two
 Constraints next to each other (not opposite each other!) in the edges

 Sparing some commenting... I got lost inside of it too...
*/
void ReadDyna::Convert()
{
    std::map<unsigned int, VERTICES>::iterator pnt_it(Pointlist.begin());
    std::map<unsigned int, VERTICES>::iterator constraint_it(Pointlist.begin());
    std::map<unsigned int, FACE>::iterator face_it(Facelist.begin());
    std::vector<unsigned int>::iterator ver_it;
    std::vector<unsigned int>::iterator constrainter_it;
    for ( ; face_it != Facelist.end(); face_it++) //For every face...
    {
        FACE CurFace = (*face_it).second;
        std::vector<unsigned int> AdditionalPoint;
        std::vector<unsigned int> Constrainter;
        unsigned int Doubler = 0;
        bool HaveConstraint = false;
        bool Side = false;
        for (unsigned int i = 0; i < CurFace.PointIndex.size(); i++)//...and for every point in the face...
        {
            pnt_it = Pointlist.find(CurFace.PointIndex[i]);
            if ((*pnt_it).second.Constrain)//...does it constraining any other points?
            {
                int NumOfConstraint = 0;
                for (unsigned int j = 0; j < (*pnt_it).second.Constraining.size(); j++) //Yes, so for every point it constraints...
                {
                    constraint_it = Pointlist.find((*pnt_it).second.Constraining[j]);
                    if ((constrainter_it = find(CurFace.PointIndex.begin(),CurFace.PointIndex.end(),
                                                (*constraint_it).second.PointIndex)) != CurFace.PointIndex.end())  //...does the constraint point also a part of the current face?
                    {
                        continue; //Yes, so skip it.
                    }
                    unsigned int ver = (*constraint_it).second.ConstrainedBy[0];  //No, so...
                    if (ver == CurFace.PointIndex[i]) //...the point is current point?
                        ver = (*constraint_it).second.ConstrainedBy[1]; //Yes, so move to second point in ConstrainedBy
                    if ((constrainter_it = find(CurFace.PointIndex.begin(),CurFace.PointIndex.end(),ver))
                            != CurFace.PointIndex.end()) //So, the point we are checking, is it a part of the current face?
                    {
                        HaveConstraint = true;
                        NumOfConstraint++; //Yes, so...
                        if ((ver_it = find(AdditionalPoint.begin(), AdditionalPoint.end(),(*pnt_it).second.Constraining[j]))
                                == AdditionalPoint.end()) //...have the additional point been added to the list?
                        {
                            //Yes, push it back, and push the current point index too!!!
                            AdditionalPoint.push_back((*pnt_it).second.Constraining[j]);
                            Constrainter.push_back(i);
                        }
                    }
                }
                if (NumOfConstraint == 2) //One point constraining two point...?
                {
                    Side = true;
                    Doubler = i;   //Set the doubler~
                }
            }
        }
        if (!HaveConstraint) //No Constraints, so...
        {
            /*  ------
                |    /|
                |   / |
             |  /  |
             | /   |
                -------
             or so...
            */
            STLINDEX Temp;
            Temp.PointIndex.push_back(CurFace.PointIndex[0]);
            Temp.PointIndex.push_back(CurFace.PointIndex[1]);
            Temp.PointIndex.push_back(CurFace.PointIndex[3]);
            Stllist.push_back(Temp);
            Temp.PointIndex.clear();

            Temp.PointIndex.push_back(CurFace.PointIndex[1]);
            Temp.PointIndex.push_back(CurFace.PointIndex[2]);
            Temp.PointIndex.push_back(CurFace.PointIndex[3]);
            Stllist.push_back(Temp);

        }
        else
        {
            //Houston, we have some constraints...
            switch (AdditionalPoint.size())  //"How much" asks Houston
            {
            case 1: //1 constrain is the reply
            {
                /* ____
                   | /|
                   |/ |
                   |\ |
                   | \|
                   ----
                */
                STLINDEX Temp;
                int checker;
                if (Constrainter[0] != 0)
                {
                    Temp.PointIndex.push_back(CurFace.PointIndex[Constrainter[0]]);
                    Temp.PointIndex.push_back(AdditionalPoint[0]);
                    Temp.PointIndex.push_back(CurFace.PointIndex[Constrainter[0] - 1]);
                    Stllist.push_back(Temp);
                    Temp.PointIndex.clear();

                    Temp.PointIndex.push_back(AdditionalPoint[0]);

                    checker = Constrainter[0] - 2;
                    if (checker < 0)
                        Temp.PointIndex.push_back(CurFace.PointIndex[Constrainter[0]+2]);


                    else Temp.PointIndex.push_back(CurFace.PointIndex[checker]);


                    checker = Constrainter[0] - 1;
                    if (checker < 0)
                        Temp.PointIndex.push_back(CurFace.PointIndex[3]);
                    else Temp.PointIndex.push_back(CurFace.PointIndex[checker]);


                    Stllist.push_back(Temp);
                    Temp.PointIndex.clear();

                    Temp.PointIndex.push_back(AdditionalPoint[0]);

                    Temp.PointIndex.push_back(CurFace.PointIndex[Constrainter[0] + 1]);

                    checker = Constrainter[0] + 2;
                    if (checker >= 4)
                    {
                        Temp.PointIndex.push_back(CurFace.PointIndex[Constrainter[0] - 2]);

                    }
                    else
                    {
                        Temp.PointIndex.push_back(CurFace.PointIndex[checker]);

                    }
                    Stllist.push_back(Temp);
                }
                else  //Seems Constrainter[0] == 0 have it's own... possibilities
                {

                    Temp.PointIndex.push_back(AdditionalPoint[0]);
                    Temp.PointIndex.push_back(CurFace.PointIndex[2]);
                    Temp.PointIndex.push_back(CurFace.PointIndex[3]);

                    Stllist.push_back(Temp);
                    Temp.PointIndex.clear();

                    Temp.PointIndex.push_back(AdditionalPoint[0]);
                    Temp.PointIndex.push_back(CurFace.PointIndex[1]);
                    Temp.PointIndex.push_back(CurFace.PointIndex[2]);

                    Stllist.push_back(Temp);
                    Temp.PointIndex.clear();

                    Temp.PointIndex.push_back(AdditionalPoint[0]);

                    std::map<unsigned int, VERTICES>::iterator pnt = Pointlist.find(AdditionalPoint[0]);
                    std::vector<unsigned int> temp = (*pnt).second.ConstrainedBy;
                    std::vector<unsigned int>::iterator vec = find(temp.begin(), temp.end(), CurFace.PointIndex[1]);
                    if (vec != temp.end())
                    {
                        Temp.PointIndex.push_back(CurFace.PointIndex[3]);
                        Temp.PointIndex.push_back(CurFace.PointIndex[0]);
                        Stllist.push_back(Temp);
                    }
                    else
                    {
                        Temp.PointIndex.push_back(CurFace.PointIndex[0]);
                        Temp.PointIndex.push_back(CurFace.PointIndex[1]);
                        Stllist.push_back(Temp);
                    }
                }
                break;
            }

            case 2:  //2 Constraints is the reply
            {
                /* This one i can't draw... sorry
                */
                STLINDEX temp;
                if (Doubler != 0 && Doubler != 3)
                {
                    int checker;
                    temp.PointIndex.push_back(AdditionalPoint[1]);
                    temp.PointIndex.push_back(AdditionalPoint[0]);
                    temp.PointIndex.push_back(CurFace.PointIndex[Doubler]);

                    Stllist.push_back(temp);


                    checker = Doubler + 2;
                    temp.PointIndex[0] = AdditionalPoint[0];
                    temp.PointIndex[1] = AdditionalPoint[1];

                    if (checker >= 4)
                    {
                        temp.PointIndex[2] = CurFace.PointIndex[Doubler-2];

                    }
                    else
                    {
                        temp.PointIndex[2] = CurFace.PointIndex[Doubler+2];

                    }
                    Stllist.push_back(temp);

                    checker = Doubler - 2;
                    temp.PointIndex[0] = AdditionalPoint[0];

                    if (checker < 0)
                        temp.PointIndex[1] = CurFace.PointIndex[Doubler+2];
                    else
                        temp.PointIndex[1] = CurFace.PointIndex[Doubler-2];

                    temp.PointIndex[2] = CurFace.PointIndex[Doubler-1];

                    Stllist.push_back(temp);


                    temp.PointIndex[0] = AdditionalPoint[1];

                    checker = Doubler + 1;
                    temp.PointIndex[1] = CurFace.PointIndex[Doubler+1];

                    checker = Doubler + 2;
                    if (checker >= 4)
                        temp.PointIndex[2] = CurFace.PointIndex[Doubler-2];
                    else
                        temp.PointIndex[2] = CurFace.PointIndex[Doubler+2];


                    Stllist.push_back(temp);

                }
                else if (Doubler == 0)
                {
                    temp.PointIndex.push_back(AdditionalPoint[0]);
                    temp.PointIndex.push_back(AdditionalPoint[1]);
                    temp.PointIndex.push_back(CurFace.PointIndex[0]);

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = AdditionalPoint[1];
                    temp.PointIndex[1] = AdditionalPoint[0];
                    temp.PointIndex[2] = CurFace.PointIndex[2];

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = CurFace.PointIndex[1];
                    temp.PointIndex[1] = CurFace.PointIndex[2];
                    temp.PointIndex[2] = AdditionalPoint[0];

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = CurFace.PointIndex[2];
                    temp.PointIndex[1] = CurFace.PointIndex[3];
                    temp.PointIndex[2] = AdditionalPoint[1];

                    Stllist.push_back(temp);
                }
                else
                {
                    temp.PointIndex.push_back(AdditionalPoint[0]);
                    temp.PointIndex.push_back(AdditionalPoint[1]);
                    temp.PointIndex.push_back(CurFace.PointIndex[3]);

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = AdditionalPoint[1];
                    temp.PointIndex[1] = AdditionalPoint[0];
                    temp.PointIndex[2] = CurFace.PointIndex[1];

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = CurFace.PointIndex[1];
                    temp.PointIndex[1] = CurFace.PointIndex[2];
                    temp.PointIndex[2] = AdditionalPoint[1];

                    Stllist.push_back(temp);

                    temp.PointIndex[0] = CurFace.PointIndex[0];
                    temp.PointIndex[1] = CurFace.PointIndex[1];
                    temp.PointIndex[2] = AdditionalPoint[0];

                    Stllist.push_back(temp);

                }
                break;

            }
            case 3:
            {
                std::cout << "3 Constraints" << std::endl;
                break;
            }
            case 4:
            {
                std::cout << "4 Constraints" << std::endl;
                break;
            }
            default: //Have constraints, but no constraining points, or more than 5...? BUGSPAWN
            {
                std::cout << AdditionalPoint.size() << std::endl;
                std::cout << "Blah...?" << std::endl;
                break;
            }
            }
        }
    }
}

/*! \brief Loading into Mesh Function...
*/
void ReadDyna::PutInMesh(MeshCore::MeshKernel &mesh)
{
    std::map<unsigned int,VERTICES>::iterator pnt_it(Pointlist.begin());
    Base::Vector3f Points[3];
    MeshCore::MeshBuilder builder(mesh);
    builder.Initialize(Stllist.size());
    for (unsigned int i = 0; i < Stllist.size();i++)
    {
        for (unsigned int j = 0; j < 3; j++)
        {
            pnt_it = Pointlist.find(Stllist[i].PointIndex[j]);
            Base::Vector3f Temp((float)(*pnt_it).second.Coords[0],(float)(*pnt_it).second.Coords[1],(float)(*pnt_it).second.Coords[2]);
            Points[j] = Temp;
        }
        MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);
        Face.CalcNormal();
        builder.AddFacet(Face);
    }
    builder.Finish();
}

