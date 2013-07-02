/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MOUSESELECTION_H
#define MOUSESELECTION_H

#include <vector>
#include <Inventor/SbLinear.h>
#include <Inventor/SbVec2f.h>
#include <QCursor>

// forwards
class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QPaintEvent;
class QResizeEvent;
class SoEvent;
class SbViewportRegion;
class SoMouseButtonEvent;
class SoLocation2Event;
class SoKeyboardEvent;

namespace Gui {
class View3DInventorViewer;

/**
 * The mouse selection base class
 * In derived classes you must implement the methods @ref initialize() and @ref terminate()
 * For all drawing stuff you just have to reimplement the @ref draw() method. 
 * In general you need not to do anything else.
 * \author Werner Mayer and Jürgen Riegel
 */
class GuiExport AbstractMouseSelection
{
public:
    enum { Continue=0, Restart=1, Finish=2, Cancel=3 };

    AbstractMouseSelection();
    virtual ~AbstractMouseSelection(void){}
    /// implement this in derived classes
    virtual void initialize() = 0;
    /// implement this in derived classes
    virtual void terminate () = 0;
    void grabMouseModel(Gui::View3DInventorViewer*);
    void releaseMouseModel(void);
    const std::vector<SbVec2s>& getPositions() const { return _clPoly; }
    SbBool isInner() const { return m_bInner; }

    void redraw();

    /** @name Mouse events*/
    //@{
    int handleEvent(const SoEvent * const ev, const SbViewportRegion& vp);
    //@}

protected:
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos ){ return 0; };
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos ){ return 0; };
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e )                   { return 0; };

    /// drawing stuff
    virtual void draw (){};

protected:
    Gui::View3DInventorViewer*_pcView3D;
    QCursor m_cPrevCursor;
    int  m_iXold, m_iYold;
    int  m_iXnew, m_iYnew;
    SbBool m_bInner;
    SbBool mustRedraw;
    std::vector<SbVec2s> _clPoly;
};

// -----------------------------------------------------------------------------------

/**
 * The standard mouse selection class
 * \author Jürgen Riegel
 */
class GuiExport BaseMouseSelection : public AbstractMouseSelection
{
public:
    BaseMouseSelection();
    virtual ~BaseMouseSelection(){}
};

// -----------------------------------------------------------------------------------

/**
 * The poly picker mouse selection class
 * Create a polygon
 * \author Werner Mayer
 */
class GuiExport PolyPickerSelection : public BaseMouseSelection
{
public:
    PolyPickerSelection();
    virtual ~PolyPickerSelection();

    /// set the new mouse cursor
    virtual void initialize();
    /// do nothing
    virtual void terminate();

protected:
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos );
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos );
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e );

    /// draw the polygon
    virtual void draw ();
    virtual int popupMenu();

protected:
    std::vector<QPoint> _cNodeVector;
    int  m_iRadius, m_iNodes;
    bool m_bWorking;
};

// -----------------------------------------------------------------------------------

/**
 * The poly clip mouse model class
 * Create a polygon
 * \author Werner Mayer
 */
class GuiExport PolyClipSelection : public PolyPickerSelection
{
public:
    PolyClipSelection();
    virtual ~PolyClipSelection();

protected:
    virtual int popupMenu();
};

// -----------------------------------------------------------------------------------

/**
 * The brush selection class
 * \author Werner Mayer
 */
class GuiExport BrushSelection : public BaseMouseSelection
{
public:
    BrushSelection();
    virtual ~BrushSelection();

    /// set the new mouse cursor
    virtual void initialize();
    /// do nothing
    virtual void terminate();

    // Settings
    void setColor(float r, float g, float b, float a=0);
    void setLineWidth(float);

protected:
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos );
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos );
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e );

    /// draw the polygon
    virtual void draw ();
    virtual int popupMenu();

protected:
    std::vector<QPoint> _cNodeVector;
    int  m_iNodes;
    bool m_bWorking;

private:
    float r,g,b,a,l;
};

// -----------------------------------------------------------------------------------

/**
 * The selection mouse model class
 * Draws a rectangle for selection
 * \author Werner Mayer
 */
class GuiExport RectangleSelection : public BaseMouseSelection 
{
public:
    RectangleSelection();
    virtual ~RectangleSelection();

    /// do nothing
    virtual void initialize();
    /// do nothing
    virtual void terminate();

protected:
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos );
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos );
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e );

    /// draw the rectangle
    virtual void draw ();

private:
    bool m_bWorking;
};

// -----------------------------------------------------------------------------------

/**
 * The selection mouse model class
 * Draws a rectangle for selection
 * \author Werner Mayer
 */
class GuiExport RubberbandSelection : public BaseMouseSelection 
{
public:
    RubberbandSelection();
    virtual ~RubberbandSelection();

    /// do nothing
    virtual void initialize();
    /// do nothing
    virtual void terminate();

protected:
    virtual int mouseButtonEvent( const SoMouseButtonEvent * const e, const QPoint& pos );
    virtual int locationEvent   ( const SoLocation2Event   * const e, const QPoint& pos );
    virtual int keyboardEvent   ( const SoKeyboardEvent    * const e );

    /// draw the rectangle
    virtual void draw ();

private:
    class Private;
    Private* d;
};

// -----------------------------------------------------------------------------------

/**
 * The box zoom mouse model class
 * Draws a rectangle for box zooming
 * \author Werner Mayer
 */
class GuiExport BoxZoomSelection : public RubberbandSelection 
{
public:
    BoxZoomSelection();
    ~BoxZoomSelection();
    void terminate();
};

} // namespace Gui

#endif // MOUSESELECTION_H 
