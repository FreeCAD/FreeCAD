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

#ifndef GLIMAGEBOX_H
#define GLIMAGEBOX_H

#include <Mod/Image/App/ImageBase.h>
#include <QGLWidget>

namespace ImageGui
{

#define IV_DISPLAY_NOCHANGE     0       // no change to view settings when displaying a new image
#define IV_DISPLAY_FITIMAGE     1       // fit-image when displaying a new image (other settings remain the same)
#define IV_DISPLAY_RESET        2       // reset settings when displaying a new image (image will be displayed at 1:1 scale with no color map)

class ImageGuiExport GLImageBox : public QGLWidget
{
    Q_OBJECT

public:

    GLImageBox(QWidget * parent = 0, const QGLWidget * shareWidget = 0, Qt::WFlags f = 0);
    ~GLImageBox();

    Image::ImageBase *getImageBasePtr() { return &_image; }

    void redraw();

    int getImageSample(int x, int y, unsigned short sampleIndex, double &value);
    unsigned short getImageNumSamplesPerPix();
    int getImageFormat();

    void fixBasePosCurr();
    double getZoomFactor() { return _zoomFactor; }
    void setZoomFactor(double zoomFactor, bool useCentrePt = false, int ICx = 0, int ICy = 0);
    void zoom(int power, bool useCentrePt = false, int ICx = 0, int ICy = 0);
    void stretchToFit();
    void setNormal();
    void getCentrePoint(int &ICx, int &ICy);
    void relMoveWC(int WCdx, int WCdy);

    double WCToIC_X(double WidgetX);
    double WCToIC_Y(double WidgetY);
    double ICToWC_X(double ImageX);
    double ICToWC_Y(double ImageY);

    void clearImage();
    int createImageCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, int displayMode = IV_DISPLAY_RESET);
    int pointImageTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership, int displayMode = IV_DISPLAY_RESET);

    void clearColorMap();
    int createColorMap(int numEntriesReq = 0, bool Initialise = true);
    int getNumColorMapEntries() const { return _numMapEntries; }
    int setColorMapRGBAValue(int index, float red, float green, float blue, float alpha = 1.0);
    int setColorMapRedValue(int index, float value);
    int setColorMapGreenValue(int index, float value);
    int setColorMapBlueValue(int index, float value);
    int setColorMapAlphaValue(int index, float value);
    unsigned int pixValToMapIndex(double PixVal);

Q_SIGNALS:
  void drawGraphics();

private:

    void initializeGL();
    void paintGL();
    void resizeGL( int w, int h );

    void drawImage();
    void getDisplayedImageAreaSize(int &dx, int &dy);

    void getPixFormat(GLenum &pixFormat, GLenum &pixType);
    void limitCurrPos();
    void limitZoomFactor();
    void setCurrPos(int x0, int y0);
    void setToFit();
    void resetDisplay();
    int calcNumColorMapEntries();

    Image::ImageBase _image;   // the image data

    int _x0;            // image x-coordinate of top-left widget pixel
    int _y0;            // image y-coordinate of top-left widget pixel
    double _zoomFactor; // zoom factor = (num_widget_pixels / num_image_pixels)

    int _base_x0;       // defines a fixed position of x0
    int _base_y0;       // defines a fixed position of y0

    float* _pColorMap;  // a RGBA color map (to alter the intensity or colors)
    int _numMapEntries;     // number of entries in color map
    static bool haveMesa;

};


} // namespace ImageGui

#endif // GLIMAGEBOX_H
