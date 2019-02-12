/***************************************************************************
 *                                                                         *
 *   This is a QGLWidget displaying an image or portion of an image in a   *
 *   box.                                                                  *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <cmath>
# include <QDebug>
# include <QOpenGLDebugMessage>
# include <QOpenGLContext>
# include <QOpenGLFunctions>
# include <QSurfaceFormat>
# include <QMessageBox>
# include <QPainter>
#endif

#if defined(__MINGW32__)
# include <GL/gl.h>
# include <GL/glext.h>
#elif defined (FC_OS_MACOSX)
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# include <GLKit/GLKMatrix4.h>
#elif defined (FC_OS_WIN32)
# include <Windows.h>
# include <GL/gl.h>
# include <GL/glu.h>
# if defined(_MSC_VER) && _MSC_VER >= 1910
# include <GL/glext.h>
# endif
#else
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

#include "OpenGLImageBox.h"

using namespace ImageGui;

#if defined(Q_CC_MSVC)
#pragma warning(disable:4305) // init: truncation from const double to float
#endif

bool GLImageBox::haveMesa = false;

/*
Notes:
+ Using QGLWidget with Qt5 still works fine
+ But QGLWidget is marked as deprecated and should be replaced with QOpenGLWidget
+ When opening one or more image views (based on QOpenGLWidget) everything works fine
  but as soon as a 3d view based on QGLWidget is opened the content becomes black and
  from then on will never render normally again.
+ This problem is caused by QuarterWidget::paintEvent!!!
+ https://groups.google.com/forum/?_escaped_fragment_=topic/coin3d-discuss/2SVG6ZxOWy4#!topic/coin3d-discuss/2SVG6ZxOWy4

+ Using a QSurfaceFormat to switch on double buffering doesn't seem to have any effect
    QSurfaceFormat format;
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    setFormat(format);
+ Directly swapping in paintGL doesn't work either
    QOpenGLContext::currentContext()->swapBuffers(QOpenGLContext::currentContext()->surface());
+ Check for OpenGL errors with: GLenum err = glGetError(); // GL_NO_ERROR
+ http://retokoradi.com/2014/04/21/opengl-why-is-your-code-producing-a-black-window/
+ http://forum.openscenegraph.org/viewtopic.php?t=15177
+ implement GLImageBox::renderText
+ See http://doc.qt.io/qt-5/qtquick-scenegraph-openglunderqml-example.html
*/

/* TRANSLATOR ImageGui::GLImageBox */

// Constructor
GLImageBox::GLImageBox(QWidget * parent, Qt::WindowFlags f)
    : QOpenGLWidget(parent, f)
{
    // uses default display format for the OpenGL rendering context
    // (double buffering is enabled)

    // enable mouse tracking when moving even if no buttons are pressed
    setMouseTracking(true);

    // initialise variables
    _x0 = 0;
    _y0 = 0;
    _zoomFactor = 1.0;
    _base_x0 = 0;
    _base_y0 = 0;
    _pColorMap = 0;
    _numMapEntries = 0;

#if defined(_DEBUG) && 0
    QSurfaceFormat format;
    format.setOption(QSurfaceFormat::DebugContext);
    this->setFormat(format);
#endif
}


// Destructor
GLImageBox::~GLImageBox()
{
    delete [] _pColorMap;
}

void GLImageBox::handleLoggedMessage(const QOpenGLDebugMessage &message)
{
    qDebug() << message;
}

// Set up the OpenGL rendering state
void GLImageBox::initializeGL()
{
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
  //QColor c(Qt::black);
    QPalette p = this->palette();
    QColor c(p.color(this->backgroundRole()));		// Let OpenGL clear to black
    f->glClearColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());     // Let OpenGL clear to black
    static bool init = false;
    if (!init) {
        init = true;
        std::string ver = (const char*)(glGetString(GL_VERSION));
        haveMesa = (ver.find("Mesa") != std::string::npos);
    }

#if defined(_DEBUG) && 0
    QOpenGLContext *context = QOpenGLContext::currentContext();
    if (context->hasExtension(QByteArrayLiteral("GL_KHR_debug"))) {
        QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);
        connect(logger, &QOpenGLDebugLogger::messageLogged, this, &GLImageBox::handleLoggedMessage);

        if (logger->initialize())
            logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
    }
#endif
}


// Update the viewport
void GLImageBox::resizeGL( int w, int h )
{
    glViewport( 0, 0, (GLint)w, (GLint)h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
#if defined (FC_OS_MACOSX)
    GLKMatrix4 orthoMat = GLKMatrix4MakeOrtho(0, width() - 1, height() - 1, 0, -1, 1);
    glLoadMatrixf(orthoMat.m);
#else
    gluOrtho2D(0, width() - 1, height() - 1, 0);
#endif
    glMatrixMode(GL_MODELVIEW);
}

// Redraw (current image)
void GLImageBox::redraw()
{
    update();
}


// Paint the box
void GLImageBox::paintGL()
{
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_DEPTH_TEST);

    // clear background (in back buffer)
    //glDrawBuffer(GL_BACK); // this is an invalid call!
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Draw the image
    drawImage();

    // Emit a signal for owners to draw any graphics that is needed.
    if (_image.hasValidData() == true)
        drawGraphics();

    // flush the OpenGL graphical pipeline
    glFinish();
    glPopAttrib();

    // Double buffering is used so we need to swap the buffers
    // There is no need to explicitly call this function because it is 
    // done automatically after each widget repaint, i.e. each time after paintGL() has been executed
    // swapBuffers();
}

// Draw the image
void GLImageBox::drawImage()
{
    if (_image.hasValidData() == false)
        return;

    // Gets the size of the displayed image area using the current display settings 
    // (in units of image pixels)
    int dx, dy;
    getDisplayedImageAreaSize(dx, dy);

   // Draw the visible image region with the correct position and zoom
    if ((dx > 0) && (dy > 0))
    {
        // Get top left image pixel to display
        int tlx = std::max<int>(0, _x0);
        int tly = std::max<int>(0, _y0);

        // Get pointer to first pixel in source image rectangle
        unsigned char* pPix = (unsigned char *)(_image.getPixelDataPtr());
        pPix += (unsigned long)(_image.getNumBytesPerPixel()) * (tly * _image.getWidth() + tlx);

        // Draw in the back buffer, using the following parameters
        //glDrawBuffer(GL_BACK); // this is an invalid call!
        glPixelStorei(GL_UNPACK_ROW_LENGTH, _image.getWidth()); // defines number of pixels in a row
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // defines byte alignment of rows
        glPixelZoom(_zoomFactor, -_zoomFactor); // defines the zoom factors to draw at

        // set current raster position to coincide with top left image pixel to display
        // the first pixel is always displayed in full when zoomed in
         // round to nearest widget pixel that coincides with top left corner of top left image pixel to display
        int xx = (int)floor(ICToWC_X(tlx - 0.5) + 0.5);
        int yy = (int)floor(ICToWC_Y(tly - 0.5) + 0.5);
        glRasterPos2f(xx, yy);

        // Compute scale to stretch number of significant bits to full range
        // e.g. stretch 12 significant bits to 16-bit range: 0-4095 -> 0-65535, therefore scale = 65535/4095
        double scale = (pow(2.0, _image.getNumBitsPerSample()) - 1.0) / (pow(2.0, _image.getNumSigBitsPerSample()) - 1.0);
        glPixelTransferf(GL_RED_SCALE, (float)scale);
        glPixelTransferf(GL_GREEN_SCALE, (float)scale);
        glPixelTransferf(GL_BLUE_SCALE, (float)scale);

        // Load the color map if present
        if (_pColorMap != 0)
        {
            if (!haveMesa) glPixelTransferf(GL_MAP_COLOR, 1.0);
            glPixelMapfv(GL_PIXEL_MAP_R_TO_R, _numMapEntries, _pColorMap);
            glPixelMapfv(GL_PIXEL_MAP_G_TO_G, _numMapEntries, _pColorMap + _numMapEntries);
            glPixelMapfv(GL_PIXEL_MAP_B_TO_B, _numMapEntries, _pColorMap + _numMapEntries * 2);
            glPixelMapfv(GL_PIXEL_MAP_A_TO_A, _numMapEntries, _pColorMap + _numMapEntries * 3);
        }
        else
        {
            glPixelTransferf(GL_MAP_COLOR, 0.0);
            glPixelMapfv(GL_PIXEL_MAP_R_TO_R, 0, NULL);
            glPixelMapfv(GL_PIXEL_MAP_G_TO_G, 0, NULL);
            glPixelMapfv(GL_PIXEL_MAP_B_TO_B, 0, NULL);
            glPixelMapfv(GL_PIXEL_MAP_A_TO_A, 0, NULL);
        }

        // Get the pixel format
        GLenum pixFormat;
        GLenum pixType;
        getPixFormat(pixFormat, pixType);

        // Draw the defined source rectangle
        glDrawPixels(dx, dy, pixFormat, pixType, (GLvoid *)pPix);
        glFlush();
    }
}

// Gets the size of the displayed image area using the current display settings 
// (in units of image pixels)
void GLImageBox::getDisplayedImageAreaSize(int &dx, int &dy)
{
    if (_image.hasValidData() == false)
    {
        dx = 0;
        dy = 0;
    }
    else
    {
        // Make sure drawing position and zoom factor are valid
        limitCurrPos();
        limitZoomFactor();

        // Image coordinates of top left widget pixel = (_x0, _y0)
        // Get image coordinates of bottom right widget pixel
        int brx = (int)ceil(WCToIC_X(width() - 1));
        int bry = (int)ceil(WCToIC_Y(height() - 1));

        // Find the outer coordinates of the displayed image area
        int itlx = std::max<int>(_x0, 0);
        int itly = std::max<int>(_y0, 0);
        int ibrx = std::min<int>(brx, (int)(_image.getWidth()) - 1);
        int ibry = std::min<int>(bry, (int)(_image.getHeight()) - 1);
        if ((itlx >= (int)(_image.getWidth())) || 
            (itly >= (int)(_image.getHeight())) || 
            (ibrx < 0) || 
            (ibry < 0))
        {
            dx = 0;
            dy = 0;
        }
        dx = ibrx - itlx + 1;
        dy = ibry - itly + 1;
    }
}

// Gets the value of an image sample at the given image pixel position
// Returns 0 for valid value or -1 if coordinates or sample index are out of range or 
// if there is no image data
int GLImageBox::getImageSample(int x, int y, unsigned short sampleIndex, double &value)
{
    return (_image.getSample(x, y, sampleIndex, value));
}

// Gets the number of samples per pixel for the image
unsigned short GLImageBox::getImageNumSamplesPerPix()
{
    return (_image.getNumSamples());
}

// Gets the format (color space format) of the image
int GLImageBox::getImageFormat()
{
    return (_image.getFormat());
}


// Get the OpenGL pixel format and pixel type from the image properties
void GLImageBox::getPixFormat(GLenum &pixFormat, GLenum &pixType)
{
    switch(_image.getFormat())
    {
        case IB_CF_GREY8:
            pixFormat = GL_LUMINANCE;
            pixType = GL_UNSIGNED_BYTE;
            break;
        case IB_CF_GREY16:
            pixFormat = GL_LUMINANCE;
            pixType = GL_UNSIGNED_SHORT;
            break;
        case IB_CF_GREY32:
            pixFormat = GL_LUMINANCE;
            pixType = GL_UNSIGNED_INT;
            break;
        case IB_CF_RGB24:
            pixFormat = GL_RGB;
            pixType = GL_UNSIGNED_BYTE;
            break;
#ifndef FC_OS_CYGWIN
        case IB_CF_BGR24:
            pixFormat = GL_BGR_EXT;
            pixType = GL_UNSIGNED_BYTE;
            break;
        case IB_CF_RGB48:
            pixFormat = GL_RGB;
            pixType = GL_UNSIGNED_SHORT;
            break;
        case IB_CF_BGR48:
            pixFormat = GL_BGR_EXT;
            pixType = GL_UNSIGNED_SHORT;
            break;
#endif
        case IB_CF_RGBA32:
            pixFormat = GL_RGBA;
            pixType = GL_UNSIGNED_BYTE;
            break;
        case IB_CF_RGBA64:
            pixFormat = GL_RGBA;
            pixType = GL_UNSIGNED_SHORT;
            break;
#ifndef FC_OS_CYGWIN
        case IB_CF_BGRA32:
            pixFormat = GL_BGRA_EXT;
            pixType = GL_UNSIGNED_BYTE;
            break;
        case IB_CF_BGRA64:
            pixFormat = GL_BGRA_EXT;
            pixType = GL_UNSIGNED_SHORT;
            break;
#endif
        default:
            // Should never happen
            pixFormat = GL_LUMINANCE;
            pixType = GL_UNSIGNED_BYTE;
            QMessageBox::warning((QWidget *)this, tr("Image pixel format"),
                tr("Undefined type of colour space for image viewing"));
            return;
    }
}

// Limits the current position (centre of top left image pixel)
// Currently we don't limit it!
void GLImageBox::limitCurrPos()
{
    if (_image.hasValidData() == false)
        return;

    /*
    if (_x0 < 0)
        _x0 = 0;
    else if (_x0 >= (int)(_image.getWidth()))
        _x0 = _image.getWidth() - 1;
    if (_y0 < 0)
        _y0 = 0;
    else if (_y0 >= (int)(_image.getHeight()))
        _y0 = _image.getHeight() - 1;
    */
}

// Limits the current zoom factor from 1:64 to 64:1
void GLImageBox::limitZoomFactor()
{
    if (_zoomFactor > 64.0)
        _zoomFactor = 64.0;
    else if (_zoomFactor < (1.0 / 64.0))
        _zoomFactor = 1.0 / 64.0;
}

// Set the current position (centre of top left image pixel coordinates)
// This function does not redraw (call redraw afterwards)
void GLImageBox::setCurrPos(int x0, int y0)
{
    _x0 = x0;
    _y0 = y0;
    limitCurrPos();
}

// Fixes a base position at the current position
void GLImageBox::fixBasePosCurr()
{
    if (_image.hasValidData() == false)
    {
        _base_x0 = 0;
        _base_y0 = 0;
    }
    else
    {
        _base_x0 = _x0;
        _base_y0 = _y0;
    }
}

// Set the current zoom factor
// Option to centre the zoom at a given image point or not
// This function does not redraw (call redraw afterwards)
void GLImageBox::setZoomFactor(double zoomFactor, bool useCentrePt, int ICx, int ICy)
{
    if ((useCentrePt == false) || (_image.hasValidData() == false))
    {
        _zoomFactor = zoomFactor;
        limitZoomFactor();
    }
    else
    {
        // Set new zoom factor
        _zoomFactor = zoomFactor;
        limitZoomFactor();

        // get centre position of widget in image coordinates
        int ix, iy;
        getCentrePoint(ix, iy);

        // try to shift the current position so that defined centre point is in the middle of the widget
        // (this can be modified by the limitCurrPos function)
        setCurrPos(_x0 - ix + ICx, _y0 - iy + ICy);
    }
}

// Stretch or shrink the image to fit the view (although the zoom factor is limited so a 
// very small or very big image may not fit completely (depending on the size of the view)
// This function redraws
void GLImageBox::stretchToFit()
{
    if (_image.hasValidData() == false)
        return;

    setToFit();
    update();
}

// Sets the settings needed to fit the image into the view (although the zoom factor is limited so a 
// very small or very big image may not fit completely (depending on the size of the view)
// This function does not redraw (call redraw afterwards)
void GLImageBox::setToFit()
{
    if (_image.hasValidData() == false)
        return;

    // Compute ideal zoom factor to fit the image
    double zoomX = (double)width() / (double)(_image.getWidth());
    double zoomY = (double)height() / (double)(_image.getHeight());
    if (zoomX > zoomY)
        _zoomFactor = zoomY;
    else
        _zoomFactor = zoomX;
    limitZoomFactor();

    // set current position to top left image pixel
    setCurrPos(0, 0);
}

// Sets the normal viewing position and zoom = 1
// If the image is smaller than the widget then the image is centred 
// otherwise we view the top left part of the image
// This function does not redraw (call redraw afterwards)
void GLImageBox::setNormal()
{
    if (_image.hasValidData() == false)
        return;

    if (((int)(_image.getWidth()) < width()) && ((int)(_image.getHeight()) < height()))
    {
        setZoomFactor(1.0, true, _image.getWidth() / 2, _image.getHeight() / 2);
    }
    else
    {
        _zoomFactor = 1;
        setCurrPos(0, 0);
    }
}

// Gets the image coordinates of the centre point of the widget 
void GLImageBox::getCentrePoint(int &ICx, int &ICy)
{
    ICx = (int)floor(WCToIC_X((double)(width() - 1) / 2.0) + 0.5);
    ICy = (int)floor(WCToIC_Y((double)(height() - 1) / 2.0) + 0.5);
}

// Moves the image by a relative amount (in widget pixel units) from the base position
// First use fixBasePosCurr() to fix the base position at a position
void GLImageBox::relMoveWC(int WCdx, int WCdy)
{
    double ICdx = WCdx / _zoomFactor;
    double ICdy = WCdy / _zoomFactor;
    setCurrPos(_base_x0 - (int)floor(ICdx + 0.5), _base_y0 - (int)floor(ICdy + 0.5));
    update();
}

// Computes an image x-coordinate from the widget x-coordinate
// Note: (_x0,_y0) is the centre of the image pixel displayed at the top left of the widget
// therefore (_x0 - 0.5, _y0 - 0.5) is the top left coordinate of this pixel which will 
// theoretically coincide with widget coordinate (-0.5,-0.5)
// Zoom = 4:    Widget(0,0) = Image(_x0 - 0.375,_y0 - 0.375)
// Zoom = 2:    Widget(0,0) = Image(_x0 - 0.250,_y0 - 0.250)
// Zoom = 1:    Widget(0,0) = Image(_x0,_y0)
// Zoom = 0.5:  Widget(0,0) = Image(_x0 + 0.500,_y0 + 0.500)
// Zoom = 0.25: Widget(0,0) = Image(_x0 + 1.500,_y0 + 1.500)
double GLImageBox::WCToIC_X(double WidgetX)
{
    return ((double)_x0 - 0.5 + (WidgetX + 0.5) / _zoomFactor);
}

// Computes an image y-coordinate from the widget y-coordinate
// Note: (_x0,_y0) is the centre of the image pixel displayed at the top left of the widget
// therefore (_x0 - 0.5, _y0 - 0.5) is the top left coordinate of this pixel which will 
// theoretically coincide with widget coordinate (-0.5,-0.5)
// Zoom = 4:    Widget(0,0) = Image(_x0 - 0.375,_y0 - 0.375)
// Zoom = 2:    Widget(0,0) = Image(_x0 - 0.250,_y0 - 0.250)
// Zoom = 1:    Widget(0,0) = Image(_x0,_y0)
// Zoom = 0.5:  Widget(0,0) = Image(_x0 + 0.500,_y0 + 0.500)
// Zoom = 0.25: Widget(0,0) = Image(_x0 + 1.500,_y0 + 1.500)
double GLImageBox::WCToIC_Y(double WidgetY)
{
    return ((double)_y0 - 0.5 + (WidgetY + 0.5) / _zoomFactor);
}

// Computes a widget x-coordinate from an image x-coordinate
// Note: (_x0,_y0) is the centre of the image pixel displayed at the top left of the widget
// therefore (_x0 - 0.5, _y0 - 0.5) is the top left coordinate of this pixel which will 
// theoretically coincide with widget coordinate (-0.5,-0.5)
// Zoom = 4:    Widget(0,0) = Image(_x0 - 0.375,_y0 - 0.375)
// Zoom = 2:    Widget(0,0) = Image(_x0 - 0.250,_y0 - 0.250)
// Zoom = 1:    Widget(0,0) = Image(_x0,_y0)
// Zoom = 0.5:  Widget(0,0) = Image(_x0 + 0.500,_y0 + 0.500)
// Zoom = 0.25: Widget(0,0) = Image(_x0 + 1.500,_y0 + 1.500)
double GLImageBox::ICToWC_X(double ImageX)
{
    return ((ImageX - (double)_x0 + 0.5) * _zoomFactor - 0.5);
}

// Computes a widget y-coordinate from an image y-coordinate
// Note: (_x0,_y0) is the centre of the image pixel displayed at the top left of the widget
// therefore (_x0 - 0.5, _y0 - 0.5) is the top left coordinate of this pixel which will 
// theoretically coincide with widget coordinate (-0.5,-0.5)
// Zoom = 4:    Widget(0,0) = Image(_x0 - 0.375,_y0 - 0.375)
// Zoom = 2:    Widget(0,0) = Image(_x0 - 0.250,_y0 - 0.250)
// Zoom = 1:    Widget(0,0) = Image(_x0,_y0)
// Zoom = 0.5:  Widget(0,0) = Image(_x0 + 0.500,_y0 + 0.500)
// Zoom = 0.25: Widget(0,0) = Image(_x0 + 1.500,_y0 + 1.500)
double GLImageBox::ICToWC_Y(double ImageY)
{
    return ((ImageY - (double)_y0 + 0.5) * _zoomFactor - 0.5);
}


// Clears the image data
void GLImageBox::clearImage()
{
    _image.clear();
    resetDisplay();
}

// Load image by copying the pixel data
// The image object will take ownership of the copied pixel data
// (the source image is still controlled by the caller)
// If numSigBitsPerSample = 0 then the full range is assumed to be significant
// displayMode ... controls the initial display of the image, one of:
//                      IV_DISPLAY_NOCHANGE  ... no change to view settings when displaying a new image
//                      IV_DISPLAY_FITIMAGE  ... fit-image when displaying a new image (other settings remain the same)
//                      IV_DISPLAY_RESET     ... reset settings when displaying a new image (image will be displayed at 1:1 scale with no color map)
// This function does not redraw (call redraw afterwards)
// Returns:
//		 0 for OK
//		-1 for invalid color format
//		-2 for memory allocation error
int GLImageBox::createImageCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, int displayMode)
{
    // Copy image
    int ret = _image.createCopy(pSrcPixelData, width, height, format, numSigBitsPerSample);

    // Set display settings depending on mode
    if (displayMode == IV_DISPLAY_RESET)
    {
        // reset drawing settings (position, scale, colour mapping) if requested
        resetDisplay();
    }
    else if (displayMode == IV_DISPLAY_FITIMAGE)
    {
        // compute stretch to fit settings
        setToFit();
    }
    else // if (displayMode == IV_DISPLAY_NOCHANGE)
    {
        // use same settings
        limitCurrPos();
        limitZoomFactor();
    }
    return ret;
}

// Make the image object point to another image source
// If takeOwnership is false then:
//      This object will not own (control) or copy the pixel data
//      (the source image is still controlled by the caller)
// Else if takeOwnership is true then:
//      This object will take ownership (control) of the pixel data
//      (the source image is not (should not be) controlled by the caller anymore)
//      In this case the memory must have been allocated with the new operator (because this class will use the delete operator)
// If numSigBitsPerSample = 0 then the full range is assumed to be significant
// displayMode ... controls the initial display of the image, one of:
//                      IV_DISPLAY_NOCHANGE  ... no change to view settings when displaying a new image
//                      IV_DISPLAY_FITIMAGE  ... fit-image when displaying a new image (other settings remain the same)
//                      IV_DISPLAY_RESET     ... reset settings when displaying a new image (image will be displayed at 1:1 scale with no color map)
// This function does not redraw (call redraw afterwards)
// Returns:
//		 0 for OK
//		-1 for invalid color format
int GLImageBox::pointImageTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership, int displayMode)
{
    // Point to image
    int ret = _image.pointTo(pSrcPixelData, width, height, format, numSigBitsPerSample, takeOwnership);

    // Set display settings depending on mode
    if (displayMode == IV_DISPLAY_RESET)
    {
        // reset drawing settings (position, scale, colour mapping) if requested
        resetDisplay();
    }
    else if (displayMode == IV_DISPLAY_FITIMAGE)
    {
        // compute stretch to fit settings
        setToFit();
    }
    else // if (displayMode == IV_DISPLAY_NOCHANGE)
    {
        // use same settings
        limitCurrPos();
        limitZoomFactor();
    }
    return ret;
}

// Reset display settings
void GLImageBox::resetDisplay()
{
    clearColorMap();
    setNormal(); // re-draws as well
}

// Clears the color map
void GLImageBox::clearColorMap()
{
    delete [] _pColorMap;
    _pColorMap = 0;
    _numMapEntries = 0;
}

// Calculate the number of color map entries to use
int GLImageBox::calcNumColorMapEntries()
{
    // Get the maximum number of map entries that the system supports
    // Get the number of bits per sample for the image if it exists and compute the number of pixel values
    // Return the fewer amount of entries
    GLint maxMapEntries;
    glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &maxMapEntries);
    int NumEntries = maxMapEntries;
    if (_image.hasValidData() == true)
        NumEntries = (int)std::min<double>(pow(2.0, (double)(_image.getNumSigBitsPerSample())), (double)maxMapEntries);
    return NumEntries;
}

// Creates a color map (All red entries come first, then green, then blue, then alpha)
// returns 0 for OK, -1 for memory allocation error
// numRequestedEntries ... requested number of map entries (used if not greater than maximum possible or number of intensity values)
// Initialise ... flag to initialise the map to a linear scale or not
int GLImageBox::createColorMap(int numEntriesReq, bool Initialise)
{
    // Get the number of map entries to use
    int maxNumEntries = calcNumColorMapEntries();
    int numEntries;
    if (numEntriesReq <= 0)
        numEntries = maxNumEntries;
    else
        numEntries = std::min<int>(numEntriesReq, maxNumEntries);

    // Clear and re-create the color map if it's not the desired size
    if (numEntries != _numMapEntries)
    {
        clearColorMap();
        _numMapEntries = numEntries;

        // Create the color map (RGBA)
        try
        {
            _pColorMap = new float[4 * _numMapEntries];
        }
        catch(...)
        {
            clearColorMap();
            return -1;
        }
    }

    // Initialise the color map if requested
    // (All red entries come first, then green, then blue, then alpha)
    if (Initialise == true)
    {
        // For each RGB channel
        int arrayIndex = 0;
        for (int chan = 0; chan < 3; chan++)
        {
            for (int in = 0; in < _numMapEntries; in++)
            {
                _pColorMap[arrayIndex] = (float)in / (float)(_numMapEntries - 1);
                arrayIndex++;
            }
        }
        // For alpha channel
        for (int in = 0; in < _numMapEntries; in++)
        {
            _pColorMap[arrayIndex] = 1.0;
            arrayIndex++;
        }
    }

    return 0;
}

// Sets a color map RGBA value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map RGBA entry
// red ... intensity value for this red entry (range 0 to 1)
// green ... intensity value for this green entry (range 0 to 1)
// blue ... intensity value for this blue entry (range 0 to 1)
// alpha ... value for this alpha entry (range 0 to 1)
int GLImageBox::setColorMapRGBAValue(int index, float red, float green, float blue, float alpha)
{
    if ((index < 0) || (index >= _numMapEntries) || 
        (red < 0.0) || (red > 1.0) || 
        (green < 0.0) || (green > 1.0) || 
        (blue < 0.0) || (blue > 1.0) || 
        (alpha < 0.0) || (alpha > 1.0))
        return -1;

    _pColorMap[index] = red;
    _pColorMap[_numMapEntries + index] = green;
    _pColorMap[_numMapEntries * 2 + index] = blue;
    _pColorMap[_numMapEntries * 3 + index] = alpha;
    return 0;
}

// Sets a color map red value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map red entry
// value ... intensity value for this red entry (range 0 to 1)
int GLImageBox::setColorMapRedValue(int index, float value)
{
    if ((index < 0) || (index >= _numMapEntries) || (value < 0.0) || (value > 1.0))
        return -1;

    _pColorMap[index] = value;
    return 0;
}

// Sets a color map green value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map green entry
// value ... intensity value for this green entry (range 0 to 1)
int GLImageBox::setColorMapGreenValue(int index, float value)
{
    if ((index < 0) || (index >= _numMapEntries) || (value < 0.0) || (value > 1.0))
        return -1;

    _pColorMap[_numMapEntries + index] = value;
    return 0;
}

// Sets a color map blue value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map blue entry
// value ... intensity value for this blue entry (range 0 to 1)
int GLImageBox::setColorMapBlueValue(int index, float value)
{
    if ((index < 0) || (index >= _numMapEntries) || (value < 0.0) || (value > 1.0))
        return -1;

    _pColorMap[_numMapEntries * 2 + index] = value;
    return 0;
}

// Sets a color map alpha value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map alpha entry
// value ... value for this alpha entry (range 0 to 1)
int GLImageBox::setColorMapAlphaValue(int index, float value)
{
    if ((index < 0) || (index >= _numMapEntries) || (value < 0.0) || (value > 1.0))
        return -1;

    _pColorMap[_numMapEntries * 3 + index] = value;
    return 0;
}

// Helper function to convert a pixel's value (of a sample) to the color map index (i.e. the map index that will be used for that pixel value)
unsigned int GLImageBox::pixValToMapIndex(double PixVal)
{
    if (_pColorMap != NULL)
    {
        double MaxVal = pow(2.0, _image.getNumBitsPerSample()) - 1.0;
        double Scale = (pow(2.0, _image.getNumBitsPerSample()) - 1.0) / (pow(2.0, _image.getNumSigBitsPerSample()) - 1.0);
        double PixVal01 = Scale * PixVal / MaxVal;
        int numMapEntries = getNumColorMapEntries();
        unsigned int MapIndex = (unsigned int)floor(0.5 + PixVal01 * (double)(numMapEntries - 1));
        return MapIndex;
    }
    else
    {
        return 0;
    }
}

// https://learnopengl.com/?_escaped_fragment_=In-Practice/Text-Rendering#!In-Practice/Text-Rendering
void GLImageBox::renderText(int x, int y, const QString& str, const QFont& fnt)
{
    if (str.isEmpty() || !isValid())
        return;

    //glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    //glPushAttrib(GL_ALL_ATTRIB_BITS);

#if 0
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLfloat color[4];
    glGetFloatv(GL_CURRENT_COLOR, &color[0]);
    QColor col;
    col.setRgbF(color[0], color[1], color[2],color[3]);

    QFont font(fnt);
    font.setStyleHint(QFont::Times, QFont::PreferAntialias);

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    painter.setFont(font);
    painter.setPen(col);
    painter.drawText(x, y, str);
    painter.end();
#else
    GLfloat color[4];
    glGetFloatv(GL_CURRENT_COLOR, &color[0]);
    QColor col;
    col.setRgbF(color[0], color[1], color[2],color[3]);

    QFont font(fnt);
    font.setStyleHint(QFont::Times, QFont::PreferAntialias);

    QPainterPath textPath;
    textPath.addText(x, y, font, str);

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    painter.setBrush(col);
    painter.setPen(Qt::NoPen);
    painter.drawPath(textPath);
    painter.end();
#endif
    //glPopAttrib();
    //glPopClientAttrib();
}

#include "moc_OpenGLImageBox.cpp"
