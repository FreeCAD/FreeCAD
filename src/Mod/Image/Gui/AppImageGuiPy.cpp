/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
# include <QIcon>
# include <QImage>
# include <QFileInfo>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "ImageView.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Interpreter.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>

namespace ImageGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ImageGui")
    {
        add_varargs_method("open",&Module::open
        );
        add_varargs_method("insert",&Module::open
        );
        initialize("This module is the ImageGui module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object open(const Py::Tuple& args)
    {
        char* Name;
        const char* DocName=nullptr;
        if (!PyArg_ParseTuple(args.ptr(), "et|s","utf-8",&Name,&DocName))
            throw Py::Exception();

        std::string EncodedName = std::string(Name);
        PyMem_Free(Name);

        QString fileName = QString::fromUtf8(EncodedName.c_str());
        QFileInfo file(fileName);

        // Load image from file into a QImage object
        QImage imageq(fileName);

        // Extract image into a general RGB format recognised by the ImageView class
        int format = IB_CF_RGB24;
        unsigned char *pPixelData = nullptr;
        if (!imageq.isNull()) {
            pPixelData = new unsigned char[3 * (unsigned long)imageq.width() * (unsigned long)imageq.height()];
            unsigned char *pPix = pPixelData;
            for (int r = 0; r < imageq.height(); r++) {
                for (int c = 0; c < imageq.width(); c++) {
                    QRgb rgb = imageq.pixel(c,r);
                    *pPix = (unsigned char)qRed(rgb);
                    *(pPix + 1) = (unsigned char)qGreen(rgb);
                    *(pPix + 2) = (unsigned char)qBlue(rgb);
                    pPix += 3;
                }
            }
        }
        else {
            throw Py::Exception(PyExc_IOError, "Could not load image file");
        }

        // Displaying the image in a view.
        // This ImageView object takes ownership of the pixel data (in 'pointImageTo') so we don't need to delete it here
        ImageView* iView = new ImageView(Gui::getMainWindow());
        iView->setWindowIcon( Gui::BitmapFactory().pixmap("colors") );
        iView->setWindowTitle(file.fileName());
        iView->resize( 400, 300 );
        Gui::getMainWindow()->addWindow( iView );
        iView->pointImageTo((void *)pPixelData, (unsigned long)imageq.width(), (unsigned long)imageq.height(), format, 0, true);

        return Py::None();
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace ImageGui
