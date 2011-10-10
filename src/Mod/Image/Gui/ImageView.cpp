/***************************************************************************
 *                                                                         *
 *   This is a view displaying an image or portion of an image in a box.   *
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
# include <QAction>
# include <QApplication>
# include <QMenu>
# include <QMouseEvent>
# include <QSlider>
# include <QStatusBar>
# include <QToolBar>
# include <cmath>
#endif

#include "ImageView.h"
#include "GLImageBox.h"
#include "../App/ImageBase.h"
#include "XpmImages.h"

using namespace ImageGui;


/* TRANSLATOR ImageGui::ImageView */

ImageView::ImageView(QWidget* parent)
  : MDIView(0, parent)
{
  // enable mouse tracking when moving even if no buttons are pressed
  setMouseTracking(true);

  // enable the mouse events
  _mouseEventsEnabled = true; 

  // Create the default status bar for displaying messages
  EnableStatusBar(true);

  // Create an OpenGL widget for displaying images
  _pGLImageBox = new GLImageBox(this);
  setCentralWidget(_pGLImageBox);

  _currMode = nothing;
  _currX = 0;
  _currY = 0;

  // Create the actions, menus and toolbars
  createActions();

  // connect other slots
  connect(_pGLImageBox, SIGNAL(drawGraphics()), this, SLOT(drawGraphics()));
}

ImageView::~ImageView()
{
  // No need to delete _pGLImageBox or other widgets as this gets done automatically by QT
}

// Create the action groups, actions, menus and toolbars
void ImageView::createActions()
{
  // Create actions
  _pFitAct = new QAction(this);
  _pFitAct->setText(tr("&Fit image"));
  _pFitAct->setIcon(QPixmap(image_stretch));
  _pFitAct->setStatusTip(tr("Stretch the image to fit the view"));
  connect(_pFitAct, SIGNAL(triggered()), this, SLOT(fitImage()));

  _pOneToOneAct = new QAction(this);
  _pOneToOneAct->setText(tr("&1:1 scale"));
  _pOneToOneAct->setIcon(QPixmap(image_oneToOne));
  _pOneToOneAct->setStatusTip(tr("Display the image at a 1:1 scale"));
  connect(_pOneToOneAct, SIGNAL(triggered()), this, SLOT(oneToOneImage()));

  // Create an action group for the exclusive color actions
  _pShowColActGrp = new QActionGroup (this);
  _pShowColActGrp->setExclusive(true);
  connect(_pShowColActGrp, SIGNAL(triggered(QAction*)), this, SLOT(handleColorAct(QAction*)));

  _pShowOrigAct = new QAction(_pShowColActGrp);
  _pShowOrigAct->setCheckable(true);
  _pShowOrigAct->setText(tr("&Original color"));
  _pShowOrigAct->setIcon(QPixmap(image_orig));
  _pShowOrigAct->setStatusTip(tr("Display the image with its original color(s)"));

  _pShowBrightAct = new QAction(_pShowColActGrp);
  _pShowBrightAct->setCheckable(true);
  _pShowBrightAct->setText(tr("&Brightened color"));
  _pShowBrightAct->setIcon(QPixmap(image_bright));
  _pShowBrightAct->setStatusTip(tr("Display the image with brightened color(s)"));

  // Create the menus and add the actions
  _pContextMenu = new QMenu(this);
  _pContextMenu->addAction(_pFitAct);
  _pContextMenu->addAction(_pOneToOneAct);
  _pContextMenu->addAction(_pShowOrigAct);
  _pContextMenu->addAction(_pShowBrightAct);

  // Create the toolbars and add the actions
  _pStdToolBar = this->addToolBar(tr("Standard"));
  _pStdToolBar->addAction(_pFitAct);
  _pStdToolBar->addAction(_pOneToOneAct);
  _pStdToolBar->addAction(_pShowOrigAct);
  _pStdToolBar->addAction(_pShowBrightAct);

  // Add a slider to the toolbar (for brightness adjustment)
  _sliderBrightAdjVal = 10;
  _pSliderBrightAdj = new QSlider(Qt::Horizontal, _pStdToolBar);
  _pSliderBrightAdj->setRange(0, 100);
  _pSliderBrightAdj->setValue(_sliderBrightAdjVal);
  _pSliderBrightAdj->setPageStep(10);
  _pStdToolBar->addWidget(_pSliderBrightAdj);
  _pSliderBrightAdj->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  _pSliderBrightAdj->setEnabled(false);
  connect(_pSliderBrightAdj, SIGNAL(valueChanged(int)), this, SLOT(sliderValueAdjusted(int)));

  // Set the original color action to ON
  _pShowOrigAct->setChecked(true);
}

// Enable or disable the status bar
void ImageView::EnableStatusBar(bool Enable)
{
  if (Enable == true)
  {
    // Create the default status bar for displaying messages and disable the gripper
    _statusBarEnabled = true;
    statusBar()->setSizeGripEnabled( false );
    statusBar()->showMessage(tr("Ready..."));
  }
  else
  {
    // Delete the status bar
    _statusBarEnabled = false;
    QStatusBar *pStatusBar = statusBar();
    delete pStatusBar;
  }
}

// Enable or disable the toolbar
void ImageView::EnableToolBar(bool Enable)
{
  _pStdToolBar->setShown(Enable);
}

// Enable or disable the mouse events
void ImageView::EnableMouseEvents(bool Enable)
{
	_mouseEventsEnabled = Enable;
}

// Enable (show) or disable (hide) the '1:1' action
// Current state (zoom, position) is left unchanged
void ImageView::EnableOneToOneAction(bool Enable)
{
  _pOneToOneAct->setVisible(Enable);
}

// Enable (show) or disable (hide) the 'fit image' action
// Current state (zoom, position) is left unchanged
void ImageView::EnableFitImageAction(bool Enable)
{
  _pFitAct->setVisible(Enable);
}

// Enable (show) or disable (hide) the color actions (_pShowOrigAct and _pShowBrightAct)
// If enabling:
//      image is redisplayed with either original or brightened colors (depending on which state the toggle buttons are in)
// If disabling
//      color map is left as is and image is not redisplayed
// This function should be used to hide the color actions when a derived class implements its own color map scheme
void ImageView::EnableColorActions(bool Enable)
{
  if (Enable == true)
  {
    _pShowOrigAct->setVisible(true);
    _pShowBrightAct->setVisible(true);
    if (_pShowBrightAct->isChecked() == true)
      showBrightened();
    else
      showOriginalColors();
  }
  else
  {
    _pShowOrigAct->setVisible(false);
    _pShowBrightAct->setVisible(false);
    _pSliderBrightAdj->setVisible(false);
  }
}

// Slot function to fit (stretch/shrink) the image to the view size
void ImageView::fitImage()
{
  _pGLImageBox->stretchToFit();
}


// Slot function to display the image at a 1:1 scale"
void ImageView::oneToOneImage()
{
  _pGLImageBox->setNormal();
  _pGLImageBox->redraw();
   updateStatusBar();
}

// Slot function to handle the color actions
void ImageView::handleColorAct( QAction* act)
{
  if (act == _pShowOrigAct)
  {
    _pSliderBrightAdj->setEnabled(false);
    showOriginalColors();
  }
  else if (act == _pShowBrightAct)
  {
    _pSliderBrightAdj->setEnabled(true);
    showBrightened();
  }
}

// Show the original colors (no enhancement)
// but image will be scaled for the number of significant bits
// (i.e if 12 significant bits (in 16-bit image) a value of 4095 will be shown as white)
void ImageView::showOriginalColors()
{
  _pGLImageBox->clearColorMap();
  _pGLImageBox->redraw();
}

// Show the image with a brightness adjustment
void ImageView::showBrightened()
{
    if (createColorMap(0, false) == 0)
    {
        // Fill the color map with the preset enhancement
        int numMapEntries = getNumColorMapEntries();
        double expValue = (_sliderBrightAdjVal / 1000.0) * 256 / numMapEntries;
        for (int in = 0; in < numMapEntries; in++)
        {
            double out = 1.0 - exp (-(double)in * expValue);
            setColorMapRedValue(in, (float)out);
            setColorMapGreenValue(in, (float)out);
            setColorMapBlueValue(in, (float)out);
            setColorMapAlphaValue(in, 1.0);
        }

        // redraw
        _pGLImageBox->redraw();
    }
}

// Slot function to adjust the brightness slider's value
void ImageView::sliderValueAdjusted(int NewValue)
{
    _sliderBrightAdjVal = NewValue;
    if (_pShowBrightAct->isChecked() == true)
        showBrightened();
}

// Create a color map
// (All red entries come first, then green, then blue, then alpha)
// returns 0 for OK, -1 for memory allocation error
// numRequestedEntries ... requested number of map entries (used if not greater than system maximum or 
//                         if not greater than the maximum number of intensity values in the current image).
//                         Pass zero to use the maximum possible. Always check the actual number of entries
//                         created using getNumColorMapEntries() after a call to this method.
// Initialise ... flag to initialise the map to a linear scale or not
int ImageView::createColorMap(int numEntriesReq, bool Initialise)
{
    return (_pGLImageBox->createColorMap(numEntriesReq, Initialise));
}

// Gets the number of entries in the color map (number of entries for each color)
int ImageView::getNumColorMapEntries() const
{
    return (_pGLImageBox->getNumColorMapEntries());
}

// Clears the color map
void ImageView::clearColorMap()
{
    _pGLImageBox->clearColorMap();
}

// Sets a color map RGBA value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map RGBA entry
// red ... intensity value for this red entry (range 0 to 1)
// green ... intensity value for this green entry (range 0 to 1)
// blue ... intensity value for this blue entry (range 0 to 1)
// alpha ... intensity value for this alpha entry (range 0 to 1)
int ImageView::setColorMapRGBAValue(int index, float red, float green, float blue, float alpha)
{
    return (_pGLImageBox->setColorMapRGBAValue(index, red, green, blue, alpha));
}

// Sets a color map red value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map red entry
// value ... intensity value for this red entry (range 0 to 1)
int ImageView::setColorMapRedValue(int index, float value)
{
    return (_pGLImageBox->setColorMapRedValue(index, value));
}

// Sets a color map green value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map green entry
// value ... intensity value for this green entry (range 0 to 1)
int ImageView::setColorMapGreenValue(int index, float value)
{
    return (_pGLImageBox->setColorMapGreenValue(index, value));
}

// Sets a color map blue value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map blue entry
// value ... intensity value for this blue entry (range 0 to 1)
int ImageView::setColorMapBlueValue(int index, float value)
{
    return (_pGLImageBox->setColorMapBlueValue(index, value));
}

// Sets a color map alpha value
// (All red entries come first, then green, then blue, then alpha)
// index ... index of color map alpha entry
// value ... intensity value for this alpha entry (range 0 to 1)
int ImageView::setColorMapAlphaValue(int index, float value)
{
    return (_pGLImageBox->setColorMapAlphaValue(index, value));
}

// Clears the image data
void ImageView::clearImage()
{
    _pGLImageBox->clearImage();
    _pGLImageBox->redraw(); // clears view
	updateStatusBar();
}

// Load image by copying the pixel data
// The image object inside this view object will take ownership of the copied pixel data
// (the source image is still controlled by the caller)
// If numSigBitsPerSample = 0 then the full range is assumed to be significant
// displayMode ... controls the initial display of the image, one of:
//                      IV_DISPLAY_NOCHANGE  ... no change to view settings when displaying a new image
//                      IV_DISPLAY_FITIMAGE  ... fit-image when displaying a new image (other settings remain the same)
//                      IV_DISPLAY_RESET     ... reset settings when displaying a new image (image will be displayed at 1:1 scale with no color map)
// Returns:
//		 0 for OK
//		-1 for invalid color format
//		-2 for memory allocation error
int ImageView::createImageCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, int displayMode)
{
    int ret = _pGLImageBox->createImageCopy(pSrcPixelData, width, height, format, numSigBitsPerSample, displayMode);
    if (_pShowBrightAct->isChecked() == true)
        showBrightened();
    else
        showOriginalColors();
	updateStatusBar();
    return ret;
}

// Make the image object inside this view object point to another image source
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
// Returns:
//		 0 for OK
//		-1 for invalid color format
int ImageView::pointImageTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership, int displayMode)
{
    int ret = _pGLImageBox->pointImageTo(pSrcPixelData, width, height, format, numSigBitsPerSample, takeOwnership, displayMode);
    if (_pShowBrightAct->isChecked() == true)
        showBrightened();
    else
        showOriginalColors();
	updateStatusBar();
    return ret;
}

// Mouse press event
void ImageView::mousePressEvent(QMouseEvent* cEvent)
{
   if (_mouseEventsEnabled == true)
   {
      // Mouse event coordinates are relative to top-left of image view (including toolbar!)
      // Get current cursor position relative to top-left of image box
      QPoint offset = _pGLImageBox->pos();
      int box_x = cEvent->x() - offset.x();
      int box_y = cEvent->y() - offset.y();
      _currX = box_x;
      _currY = box_y;
      switch(cEvent->buttons())
      {
          case Qt::MidButton:
              _currMode = panning;
              this->setCursor(QCursor(Qt::ClosedHandCursor));
              startDrag();
              break;
          //case Qt::LeftButton | Qt::MidButton:
          //    _currMode = zooming;
          //    break;
          case Qt::LeftButton:
              if (cEvent->modifiers() & Qt::ShiftModifier)
                  _currMode = addselection;
              else
                _currMode = selection;
              break;
          case Qt::RightButton:
               _pContextMenu->exec(cEvent->globalPos());
              break;
          default:
              _currMode = nothing;
      }
   }
}

void ImageView::mouseDoubleClickEvent(QMouseEvent* cEvent)
{
   if (_mouseEventsEnabled == true)
   {
       // Mouse event coordinates are relative to top-left of image view (including toolbar!)
       // Get current cursor position relative to top-left of image box
       QPoint offset = _pGLImageBox->pos();
       int box_x = cEvent->x() - offset.x();
       int box_y = cEvent->y() - offset.y();
       _currX = box_x;
       _currY = box_y;
       if(cEvent->button() == Qt::MidButton)
       {
           double icX = _pGLImageBox->WCToIC_X(_currX);
           double icY = _pGLImageBox->WCToIC_Y(_currY);
           //int pixX = (int)floor(icX + 0.5);
           //int pixY = (int)floor(icY + 0.5);
           _pGLImageBox->setZoomFactor(_pGLImageBox->getZoomFactor(), true, (int)floor(icX + 0.5), (int)floor(icY + 0.5));
           _pGLImageBox->redraw();
		   updateStatusBar();
       }
   }
}

// Mouse move event
void ImageView::mouseMoveEvent(QMouseEvent* cEvent)
{
    QApplication::flush();

   // Mouse event coordinates are relative to top-left of image view (including toolbar!)
   // Get current cursor position relative to top-left of image box
   QPoint offset = _pGLImageBox->pos();
   int box_x = cEvent->x() - offset.x();
   int box_y = cEvent->y() - offset.y();
   if (_mouseEventsEnabled == true)
   {
       switch(_currMode)
       {
           case nothing:
               break;
           case panning:
               _pGLImageBox->relMoveWC(box_x - dragStartWCx, box_y - dragStartWCy);
               break;
           case zooming:
               zoom(_currX, _currY, box_x, box_y);
               break;
           default:
               break;
       }
   }
   _currX = box_x;
   _currY = box_y;

   // Update the status bar
   updateStatusBar();
}

// Mouse release event
void ImageView::mouseReleaseEvent(QMouseEvent* cEvent)
{
   if (_mouseEventsEnabled == true)
   {
       // Mouse event coordinates are relative to top-left of image view (including toolbar!)
       // Get current cursor position relative to top-left of image box
       QPoint offset = _pGLImageBox->pos();
       int box_x = cEvent->x() - offset.x();
       int box_y = cEvent->y() - offset.y();
       switch(_currMode)
       {
           case selection:
               select(box_x, box_y);
               break;
           case addselection:
               addSelect(box_x, box_y);
               break;
           case panning:
               this->unsetCursor();
               break;
           default:
               break;
       }
       _currMode = nothing;
   }
}

// Mouse wheel event
void ImageView::wheelEvent(QWheelEvent * cEvent)
{
   if (_mouseEventsEnabled == true)
   {
       // Mouse event coordinates are relative to top-left of image view (including toolbar!)
       // Get current cursor position relative to top-left of image box
       QPoint offset = _pGLImageBox->pos();
       int box_x = cEvent->x() - offset.x();
       int box_y = cEvent->y() - offset.y();

       // Zoom around centrally displayed image point
       int numTicks = cEvent->delta() / 120;
       int ICx, ICy;
       _pGLImageBox->getCentrePoint(ICx, ICy);
       _pGLImageBox->setZoomFactor(_pGLImageBox->getZoomFactor() / pow(2.0, (double)numTicks), true, ICx, ICy);
       _pGLImageBox->redraw();
       _currX = box_x;
       _currY = box_y;

       // Update the status bar
       updateStatusBar();
   }
}

void ImageView::showEvent (QShowEvent * e)
{
    _pGLImageBox->setFocus();
}

// Update the status bar with the image parameters for the current mouse position
void ImageView::updateStatusBar()
{
    if (_statusBarEnabled == true)
	{
        // Create the text string to display in the status bar
        QString txt = createStatusBarText();

        // Update status bar with new text
        statusBar()->showMessage(txt);
	}
}

// Create the text to display in the status bar.
// Gets called by updateStatusBar()
// Override this function in a derived class to add your own text
QString ImageView::createStatusBarText()
{
    // Get some image parameters
    //unsigned short numImageSamples = _pGLImageBox->getImageNumSamplesPerPix();
    double zoomFactor = _pGLImageBox->getZoomFactor();
    double icX = _pGLImageBox->WCToIC_X(_currX);
    double icY = _pGLImageBox->WCToIC_Y(_currY);
    int pixX = (int)floor(icX + 0.5);
    int pixY = (int)floor(icY + 0.5);
    int colorFormat = _pGLImageBox->getImageFormat();

   // Create text for status bar
    QString txt;
    if ((colorFormat == IB_CF_GREY8) || 
        (colorFormat == IB_CF_GREY16) || 
        (colorFormat == IB_CF_GREY32))
    {
        double grey_value;
        if (_pGLImageBox->getImageSample(pixX, pixY, 0, grey_value) == 0)
            txt = QString::fromAscii("x,y = %1,%2  |  %3 = %4  |  %5 = %6")
                  .arg(icX,0,'f',2).arg(icY,0,'f',2)
                  .arg(tr("grey")).arg((int)grey_value)
                  .arg(tr("zoom")).arg(zoomFactor,0,'f',1);
        else
            txt = QString::fromAscii("x,y = %1  |  %2 = %3")
                  .arg(tr("outside image")).arg(tr("zoom")).arg(zoomFactor,0,'f',1);
    }
    else if ((colorFormat == IB_CF_RGB24) || 
             (colorFormat == IB_CF_RGB48))
    {
        double red, green, blue;
        if ((_pGLImageBox->getImageSample(pixX, pixY, 0, red) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 1, green) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 2, blue) != 0))
            txt = QString::fromAscii("x,y = %1  |  %2 = %3")
                  .arg(tr("outside image")).arg(tr("zoom")).arg(zoomFactor,0,'f',1);
        else
            txt = QString::fromAscii("x,y = %1,%2  |  rgb = %3,%4,%5  |  %6 = %7")
                  .arg(icX,0,'f',2).arg(icY,0,'f',2)
                  .arg((int)red).arg((int)green).arg((int)blue)
                  .arg(tr("zoom")).arg(zoomFactor,0,'f',1);
    }
    else if ((colorFormat == IB_CF_BGR24) || 
             (colorFormat == IB_CF_BGR48))
    {
        double red, green, blue;
        if ((_pGLImageBox->getImageSample(pixX, pixY, 0, blue) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 1, green) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 2, red) != 0))
            txt = QString::fromAscii("x,y = %1  |  %2 = %3")
                  .arg(tr("outside image")).arg(tr("zoom")).arg(zoomFactor,0,'f',1);
        else
            txt = QString::fromAscii("x,y = %1,%2  |  rgb = %3,%4,%5  |  %6 = %7")
                  .arg(icX,0,'f',2).arg(icY,0,'f',2)
                  .arg((int)red).arg((int)green).arg((int)blue)
                  .arg(tr("zoom")).arg(zoomFactor,0,'f',1);
    }
    else if ((colorFormat == IB_CF_RGBA32) || 
             (colorFormat == IB_CF_RGBA64))
    {
        double red, green, blue, alpha;
        if ((_pGLImageBox->getImageSample(pixX, pixY, 0, red) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 1, green) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 2, blue) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 3, alpha) != 0))
            txt = QString::fromAscii("x,y = %1  |  %2 = %3")
                  .arg(tr("outside image")).arg(tr("zoom")).arg(zoomFactor,0,'f',1);
        else
            txt = QString::fromAscii("x,y = %1,%2  |  rgba = %3,%4,%5,%6  |  %7 = %8")
                  .arg(icX,0,'f',2).arg(icY,0,'f',2)
                  .arg((int)red).arg((int)green).arg((int)blue).arg((int)alpha)
                  .arg(tr("zoom")).arg(zoomFactor,0,'f',1);
    }
    else if ((colorFormat == IB_CF_BGRA32) || 
             (colorFormat == IB_CF_BGRA64))
    {
        double red, green, blue, alpha;
        if ((_pGLImageBox->getImageSample(pixX, pixY, 0, blue) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 1, green) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 2, red) != 0) ||
            (_pGLImageBox->getImageSample(pixX, pixY, 3, alpha) != 0))
            txt = QString::fromAscii("x,y = %1  |  %2 = %3")
                  .arg(tr("outside image")).arg(tr("zoom")).arg(zoomFactor,0,'f',1);
        else
            txt = QString::fromAscii("x,y = %1,%2  |  rgba = %3,%4,%5,%6  |  %7 = %8")
                  .arg(icX,0,'f',2).arg(icY,0,'f',2)
                  .arg((int)red).arg((int)green).arg((int)blue).arg((int)alpha)
                  .arg(tr("zoom")).arg(zoomFactor,0,'f',1);
    }

    return txt;
}

// Starts a mouse drag in the image - stores some initial positions
void ImageView::startDrag()
{
    _pGLImageBox->fixBasePosCurr(); // fixes current image position as base position
    dragStartWCx = _currX;
    dragStartWCy = _currY;
}

// Zoom the image using vertical mouse movement to define a zoom factor
void ImageView::zoom(int prevX, int prevY, int currX, int currY)
{
    // Check we have more of a vertical shift than a hz one
    int dx = currX - prevX;
    int dy = currY - prevY;
    if (abs(dy) > abs(dx))
    {
        // Get centrally displayed image point
        int ICx, ICy;
        _pGLImageBox->getCentrePoint(ICx, ICy);

        // Compute zoom factor multiplier
        double zoomFactorMultiplier = 1.05;
        if (currY > prevY)
            zoomFactorMultiplier = 0.95;

        // Zoom around centrally displayed image point
        _pGLImageBox->setZoomFactor(_pGLImageBox->getZoomFactor() * zoomFactorMultiplier, true, ICx, ICy);
        _pGLImageBox->redraw();
    }
}

// Select at the given position
void ImageView::select(int currX, int currY)
{
    // base class implementation does nothing
    // override this method and implement selection capability if required
}

// Add selection at the given position
void ImageView::addSelect(int currX, int currY)
{
    // base class implementation does nothing
    // override this method and implement selection capability if required
}

// Draw any 2D graphics necessary
// Use GLImageBox::ICToWC_X and ICToWC_Y methods to transform image coordinates into widget coordinates (which 
// must be used by the OpenGL vertex commands).
void ImageView::drawGraphics()
{
    // base class implementation does nothing

    // override this method and implement OpenGL drawing commands to draw any needed graphics on top of the image

    /* Example: draw a red line from image coordinates (100,100) to (120,120)
    glColor3ub((GLubyte)255, (GLubyte)0, (GLubyte)0);
    glBegin(GL_LINES);
    glVertex2d(_pGLImageBox->ICToWC_X(100.0), _pGLImageBox->ICToWC_Y(100.0));
    glVertex2d(_pGLImageBox->ICToWC_X(120.0), _pGLImageBox->ICToWC_Y(120.0));
    glEnd();
    */
}

#include "moc_ImageView.cpp"


