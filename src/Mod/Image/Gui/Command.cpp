/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
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

//#include <Mod/Image/App/CaptureClass.h>

//#include <cv.h>
//#include <highgui.h>


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
#if 0
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
#if 0
    // Reading an image
    QString s = QFileDialog::getOpenFileName(Gui::getMainWindow(), QObject::tr("Choose an image file to open"), QString::null, 
                                             QObject::tr("Images (*.png *.xpm *.jpg *.bmp)"));
    if (s.isEmpty()) return;

    IplImage* image = cvLoadImage( 
        (const char*)s.toLatin1(),
        CV_LOAD_IMAGE_GRAYSCALE
    );
    IplImage* src = cvLoadImage( (const char*)s.toLatin1() ); //Changed for prettier show in color
    CvMemStorage* storage = cvCreateMemStorage(0);
    cvSmooth(image, image, CV_GAUSSIAN, 5, 5 );
    CvSeq* results = cvHoughCircles( 
        image, 
        storage, 
        CV_HOUGH_GRADIENT, 
        2, 
        image->width/10 
    ); 
    for( int i = 0; i < results->total; i++ ) {
        float* p = (float*) cvGetSeqElem( results, i );
        CvPoint pt = cvPoint( cvRound( p[0] ), cvRound( p[1] ) );
        cvCircle( 
            src,
            pt, 
            cvRound( p[2] ),
            CV_RGB(0xff,0,0) 
        );
    }
    cvNamedWindow( "cvHoughCircles", 1 );
    cvShowImage( "cvHoughCircles", src);
    cvWaitKey(0);
#else
    struct tm *newtime;
#if defined (_MSC_VER)
    struct _timeb tstruct;
    __int64 ltime;
#elif defined(__GNUC__)
    struct timeb tstruct;
    time_t ltime;
#endif

    char buff[100];
    Capturerer cap(Capturerer::chooseCamNum());
    cap.setCaptureWindows(true);
    for(int i = 0; i< 200;i++){
#if defined (_MSC_VER)
        _ftime( &tstruct ); 
        _time64( &ltime );
        // Obtain coordinated universal time:
        newtime = _gmtime64( &ltime ); // C4996
#elif defined(__GNUC__)
        ftime( &tstruct ); 
        time( &ltime );
        // Obtain coordinated universal time:
        newtime = gmtime( &ltime ); // C4996
#endif
        sprintf(buff,"%2d:%2d:%2d:%3d - %4d",newtime->tm_hour,newtime->tm_min,newtime->tm_sec,tstruct.millitm,i );
        if (cap.getOneCapture(buff)==27)
            break;
    }
#endif
}
#endif

void CreateImageCommands(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdImageOpen());
    rcCmdMgr.addCommand(new CmdCreateImagePlane());
  //rcCmdMgr.addCommand(new CmdImageCapturerTest());
}
