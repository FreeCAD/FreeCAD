/***************************************************************************
 *   Copyright (c) 2014 Juergen Riegel <FreeCAD@juergen-riegel.net>		   *
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


#include "PreCompiled.h"


#if BUILD_VR

#include "View3DInventorRiftViewer.h"

#define new DEBUG_CLIENTBLOCK

using namespace Gui;

static View3DInventorRiftViewer *window=0;


void oculusStop()
{
    //SoDB::finish();
	if(window){
		delete window;
		window = 0;
	}

    ovr_Shutdown();
}

bool oculusUp(void)
{
	return window!=0;
}

int oculusStart(void)
{
    //SoDB::init();

    //QApplication app(argc, argv);
    //qAddPostRoutine(cleanup);

    // Moved here because of https://developer.oculusvr.com/forums/viewtopic.php?f=17&t=7915&p=108503#p108503
    // Init libovr.
    if (!ovr_Initialize()) {
        qDebug() << "Could not initialize Oculus SDK.";
        return 0;
    }

    window = new View3DInventorRiftViewer;
    window->show();

    // An example scene.
    static const char * inlineSceneGraph[] = {
        "#Inventor V2.1 ascii\n",
        "\n",
        "Separator {\n",
        " Rotation { rotation 1 0 0 0.3 }\n",
        " Cone { }\n",
        " BaseColor { rgb 1 0 0 }\n",
        " Scale { scaleFactor .7 .7 .7 }\n",
        " Cube { }\n",
        "\n",
        " DrawStyle { style LINES }\n",
        " ShapeHints { vertexOrdering COUNTERCLOCKWISE }\n",
        " Coordinate3 {\n",
        " point [\n",
        " -2 -2 1.1, -2 -1 1.1, -2 1 1.1, -2 2 1.1,\n",
        " -1 -2 1.1, -1 -1 1.1, -1 1 1.1, -1 2 1.1\n",
        " 1 -2 1.1, 1 -1 1.1, 1 1 1.1, 1 2 1.1\n",
        " 2 -2 1.1, 2 -1 1.1, 2 1 1.1, 2 2 1.1\n",
        " ]\n",
        " }\n",
        "\n",
        " Complexity { value 0.7 }\n",
        " NurbsSurface {\n",
        " numUControlPoints 4\n",
        " numVControlPoints 4\n",
        " uKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        " vKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        " }\n",
        "}\n",
        NULL
    };

    SoInput in;
    in.setStringArray(inlineSceneGraph);

    window->setSceneGraph(SoDB::readAll(&in));

	return 1;
    //return app.exec();
}

#endif //BUILD_VR