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

#include <Base/Console.h>
#include "View3DInventorRiftViewer.h"
#include <App/Application.h>

#define new DEBUG_CLIENTBLOCK

using namespace Gui;

View3DInventorRiftViewer::View3DInventorRiftViewer() : CoinRiftWidget()
{
    workplace = new SoGroup();
    translation  = new SoTranslation   ; 
    rotation     = new SoRotationXYZ   ;
    rotation->axis.setValue(SoRotationXYZ::X);
    rotation->angle.setValue(M_PI/4);
    workplace->addChild(rotation);


    scale        = new SoScale         ;
    scale->scaleFactor.setValue(0.01f,0.01f,0.01f); // scale from mm to m as neede by the Rift
    workplace->addChild(scale);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Oculus");
    
    this->setGeometry( hGrp->GetInt("RenderWindowPosX",100) ,
                       hGrp->GetInt("RenderWindowPosY",100) ,
                       hGrp->GetInt("RenderWindowSizeW",1920) ,
                       hGrp->GetInt("RenderWindowSizeH",1080)                   
                     );


}

//void saveWinPostion(void)
//{
//  
// 
//
//}

View3DInventorRiftViewer::~View3DInventorRiftViewer() 
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Oculus");

    // remeber last postion on close
    hGrp->SetInt("RenderWindowPosX",pos().x());
    hGrp->SetInt("RenderWindowPosY",pos().y());
    hGrp->SetInt("RenderWindowSizeW",size().width());
    hGrp->SetInt("RenderWindowSizeH",size().height());

    Base::Console().Log("pos: %d %d  size: %d %d \n",pos().x(),pos().y(),
                                                        size().width(),size().height());
}

void View3DInventorRiftViewer::setSceneGraph(SoNode *sceneGraph)
{

    workplace->addChild(sceneGraph);

    CoinRiftWidget::setSceneGraph(workplace);
}




void View3DInventorRiftViewer::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Delete
        || event->key() == Qt::Key_Backspace)) {
        ; // TODO
    } else {
        CoinRiftWidget::keyPressEvent(event);
    }
}




// static test code ================================================================================
static View3DInventorRiftViewer *window=0;

void oculusSetTestScene(View3DInventorRiftViewer *window)
{
	assert(window);
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
}


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

View3DInventorRiftViewer* oculusStart(void)
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
	if(window)
		return window;

    window = new View3DInventorRiftViewer;
    window->show();


	return window;
    //return app.exec();
}



#endif //BUILD_VR