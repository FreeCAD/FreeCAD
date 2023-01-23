/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <App/Application.h>
#include <Base/Console.h>

#include "View3DInventorRiftViewer.h"


using namespace Gui;

View3DInventorRiftViewer::View3DInventorRiftViewer() : CoinRiftWidget()
{
    workplace = new SoGroup();

    //translation  = new SoTranslation   ;
    //translation->translation.setValue(0,-1,0);
    //workplace->addChild(translation);

    rotation1     = new SoRotationXYZ   ;
    rotation1->axis.setValue(SoRotationXYZ::X);
    rotation1->angle.setValue(-M_PI/2);
    workplace->addChild(rotation1);

    rotation2     = new SoRotationXYZ   ;
    rotation2->axis.setValue(SoRotationXYZ::Z);
    rotation2->angle.setValue(0);
    workplace->addChild(rotation2);


    scale        = new SoScale         ;
    scale->scaleFactor.setValue(0.001f,0.001f,0.001f); // scale from mm to m as needed by the Rift
    workplace->addChild(scale);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Oculus");

    this->setGeometry( hGrp->GetInt("RenderWindowPosX",100) ,
                       hGrp->GetInt("RenderWindowPosY",100) ,
                       hGrp->GetInt("RenderWindowSizeW",1920) ,
                       hGrp->GetInt("RenderWindowSizeH",1080)
                     );


    setBackgroundColor(SbColor(51,51,101));
    basePosition = SbVec3f(0.0f, 0.5f, 0.8f);
}

//void saveWinPosition(void)
//{
//
//
//
//}

View3DInventorRiftViewer::~View3DInventorRiftViewer()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Oculus");

    // remember last position on close
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
    static const float increment = 0.02; // move two centimeter per key
    static const float rotIncrement = M_PI/4; // move two 90° per key


    if (event->key() == Qt::Key_Plus) {
        scale->scaleFactor.setValue(scale->scaleFactor.getValue() * 2.0f) ;
    } else if (event->key() == Qt::Key_Minus) {
        scale->scaleFactor.setValue(scale->scaleFactor.getValue() * 0.2f) ;
    } else if (event->key() == Qt::Key_S) {
            basePosition += SbVec3f(0,0,increment) ;
    } else if (event->key() == Qt::Key_W) {
            basePosition += SbVec3f(0,0,-increment) ;
    } else if (event->key() == Qt::Key_Up) {
            basePosition += SbVec3f(0,-increment,0) ;
    } else if (event->key() == Qt::Key_Down) {
            basePosition += SbVec3f(0,increment,0) ;
    } else if (event->key() == Qt::Key_Left) {
        rotation2->angle.setValue( rotation2->angle.getValue() + rotIncrement);
    } else if (event->key() == Qt::Key_Right) {
        rotation2->angle.setValue( rotation2->angle.getValue() - rotIncrement);
    } else if (event->key() == Qt::Key_A) {
            basePosition += SbVec3f(-increment,0,0) ;
    } else if (event->key() == Qt::Key_D) {
        basePosition += SbVec3f(increment,0,0) ;
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
        ovr_Shutdown();
    }

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
