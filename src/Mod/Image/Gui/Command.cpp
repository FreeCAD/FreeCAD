/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   J�rgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QFileDialog>
# include <QImage>
# include <QImageReader>
# include <QMessageBox>
# include <QTextStream>
#endif

#include <time.h>
#include <sys/timeb.h>

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/BitmapFactory.h>
#include "ImageOrientationDialog.h"

#if HAVE_OPENCV
#  include "opencv2/opencv.hpp"
#endif


#include "ImageView.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using namespace ImageGui;

DEF_STD_CMD(CmdImageOpen);

CmdImageOpen::CmdImageOpen()
  : Command("Image_Open")
{
    sAppModule      = "Image";
    sGroup          = QT_TR_NOOP("Image");
    sMenuText       = QT_TR_NOOP("Open...");
    sToolTipText    = QT_TR_NOOP("Open image view");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "image-import";
}

void CmdImageOpen::activated(int iMsg)
{
    // add all supported QImage formats
    QString formats;
    QTextStream str(&formats);
    str << QObject::tr("Images") << " (";
    QList<QByteArray> qtformats = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        str << "*." << it->toLower() << " ";
    }
    str << ");;" << QObject::tr("All files") << " (*.*)";
    // Reading an image
    QString s = QFileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an image file to open"),
                                             QString::null, formats);
    if (!s.isEmpty()) {
        try{
            // load the file with the module
            Command::doCommand(Command::Gui, "import Image, ImageGui");
            Command::doCommand(Command::Gui, "ImageGui.open(\"%s\")", (const char*)s.toUtf8());
        }
        catch (const Base::PyException& e){
            // Usually thrown if the file is invalid somehow
            e.ReportException();
        }
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DEF_STD_CMD_A(CmdCreateImagePlane);

CmdCreateImagePlane::CmdCreateImagePlane()
    :Command("Image_CreateImagePlane")
{
    sAppModule      = "Image";
    sGroup          = QT_TR_NOOP("Image");
    sMenuText       = QT_TR_NOOP("Create image plane...");
    sToolTipText    = QT_TR_NOOP("Create a planar image in the 3D space");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "image-import";
}

void CmdCreateImagePlane::activated(int iMsg)
{
    QString formats;
    QTextStream str(&formats);
    str << QObject::tr("Images") << " (";
    QList<QByteArray> qtformats = QImageReader::supportedImageFormats();
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        str << "*." << it->toLower() << " ";
    }
    str << ");;" << QObject::tr("All files") << " (*.*)";
    // Reading an image
    QString s = QFileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an image file to open"),
                                             QString::null, formats);
    if (!s.isEmpty()) {

        QImage impQ(s);
        if (impQ.isNull()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Error open image"),
                QObject::tr("Could not load the choosen image"));
            return;
        }

        // ask user for orientation
        ImageOrientationDialog Dlg;

        if (Dlg.exec() != QDialog::Accepted)
            return; // canceled
        Base::Vector3d p = Dlg.Pos.getPosition();
        Base::Rotation r = Dlg.Pos.getRotation();

        std::string FeatName = getUniqueObjectName("ImagePlane");

        openCommand("Create ImagePlane");
        doCommand(Doc,"App.activeDocument().addObject('Image::ImagePlane','%s\')",FeatName.c_str());
        doCommand(Doc,"App.activeDocument().%s.ImageFile = '%s'",FeatName.c_str(),(const char*)s.toUtf8());
        doCommand(Doc,"App.activeDocument().%s.XSize = %d",FeatName.c_str(),impQ.width () );
        doCommand(Doc,"App.activeDocument().%s.YSize = %d",FeatName.c_str(),impQ.height() );
        doCommand(Doc,"App.activeDocument().%s.Placement = App.Placement(App.Vector(%f,%f,%f),App.Rotation(%f,%f,%f,%f))"
                     ,FeatName.c_str(),p.x,p.y,p.z,r[0],r[1],r[2],r[3]);
        commitCommand();
    }
}

bool CmdCreateImagePlane::isActive()
{
    return App::GetApplication().getActiveDocument();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if HAVE_OPENCV
DEF_STD_CMD(CmdImageCapturerTest);

CmdImageCapturerTest::CmdImageCapturerTest()
  : Command("Image_CapturerTest")
{
    sAppModule      = "Image";
    sGroup          = ("Image");
    sMenuText       = ("CapturerTest");
    sToolTipText    = ("test camara capturing");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "camera-photo";
}

void CmdImageCapturerTest::activated(int iMsg)
{
	using namespace cv;

    VideoCapture cap(0); // open the default camera
    if(!cap.isOpened())  // check if we succeeded
        return;

    Mat edges;
    namedWindow("edges",1);
    for(;;)
    {
        Mat frame;
        cap >> frame; // get a new frame from camera
        cvtColor(frame, edges, CV_BGR2GRAY);
        GaussianBlur(edges, edges, Size(7,7), 1.5, 1.5);
        Canny(edges, edges, 0, 30, 3);
        imshow("edges", edges);
        if(waitKey(30) >= 0) break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor

}
#endif

void CreateImageCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdImageOpen());
    rcCmdMgr.addCommand(new CmdCreateImagePlane());
#if HAVE_OPENCV
	rcCmdMgr.addCommand(new CmdImageCapturerTest());
#endif
}
