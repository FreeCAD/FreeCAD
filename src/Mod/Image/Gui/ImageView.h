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

#ifndef ImageView_H
#define ImageView_H

#include <Gui/MDIView.h>
#include "GLImageBox.h"

class QSlider;
class QAction;
class QActionGroup;
class QPopupMenu;
class QToolBar;

namespace ImageGui
{

class ImageGuiExport ImageView : public Gui::MDIView
{
    Q_OBJECT

public:
    ImageView(QWidget* parent);
    virtual ~ImageView();

    const char *getName(void) const {return "ImageView";}
    void onUpdate(void){};

    bool onMsg(const char* pMsg,const char** ppReturn){ return true; }
    bool onHasMsg(const char* pMsg) const { return false; }

    virtual void clearImage();
    virtual int createImageCopy(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, int displayMode = IV_DISPLAY_RESET);
    virtual int pointImageTo(void* pSrcPixelData, unsigned long width, unsigned long height, int format, unsigned short numSigBitsPerSample, bool takeOwnership, int displayMode = IV_DISPLAY_RESET);

    virtual void EnableStatusBar(bool Enable);
    virtual void EnableToolBar(bool Enable);
    virtual void EnableMouseEvents(bool Enable);
    virtual void EnableOneToOneAction(bool Enable);
    virtual void EnableFitImageAction(bool Enable);
    virtual void EnableColorActions(bool Enable);
    virtual int createColorMap(int numEntriesReq = 0, bool Initialise = true);
    virtual void clearColorMap();
    virtual int getNumColorMapEntries() const;
    virtual int setColorMapRGBAValue(int index, float red, float green, float blue, float alpha = 1.0);
    virtual int setColorMapRedValue(int index, float value);
    virtual int setColorMapGreenValue(int index, float value);
    virtual int setColorMapBlueValue(int index, float value);
    virtual int setColorMapAlphaValue(int index, float value);

public Q_SLOTS:
    virtual void fitImage();
    virtual void oneToOneImage();

protected Q_SLOTS:
    virtual void handleColorAct( QAction* act);
    virtual void sliderValueAdjusted(int NewValue);
    virtual void drawGraphics();

protected:
    virtual void createActions();
    virtual void showOriginalColors();
    virtual void showBrightened();
    virtual void mousePressEvent(QMouseEvent* cEvent);
    virtual void mouseDoubleClickEvent(QMouseEvent* cEvent);
    virtual void mouseMoveEvent(QMouseEvent* cEvent);
    virtual void mouseReleaseEvent(QMouseEvent* cEvent);
    virtual void wheelEvent(QWheelEvent * cEvent);
    virtual void showEvent (QShowEvent * e);

    virtual void updateStatusBar();
    virtual QString createStatusBarText();

    virtual void startDrag();
    virtual void zoom(int prevX, int prevY, int currX, int currY);
    virtual void select(int currX, int currY);
    virtual void addSelect(int currX, int currY);


    enum {
        nothing = 0,
        panning,
        zooming,
        selection,
        addselection
    } _currMode;

    GLImageBox* _pGLImageBox;

    int _currX;
    int _currY;
    int dragStartWCx;
    int dragStartWCy;

    // Action groups
    QActionGroup* _pShowColActGrp;

    // Actions
    QAction* _pFitAct;
    QAction* _pOneToOneAct;
    QAction* _pShowOrigAct;
    QAction* _pShowBrightAct;

    // Menus
    QMenu* _pContextMenu;

    // Toolbars
    QToolBar* _pStdToolBar;

    // Slider for brightness adjustment
    QSlider* _pSliderBrightAdj;
    int _sliderBrightAdjVal;

    // Flag for status bar enablement
    bool _statusBarEnabled;
	bool _mouseEventsEnabled;

};

} // namespace ImageViewGui

#endif // ImageView_H
