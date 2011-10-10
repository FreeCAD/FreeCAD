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


#ifndef GUI_SOFCINTERACTIVEELEMENT_H
#define GUI_SOFCINTERACTIVEELEMENT_H

#ifndef __InventorAll__
# include "InventorAll.h"
#endif

class QGLWidget;

namespace Gui {
/**
 * @author Werner Mayer
 */
class GuiExport SoFCInteractiveElement : public SoReplacedElement {
  typedef SoReplacedElement inherited;

  SO_ELEMENT_HEADER(SoFCInteractiveElement);

public:
  static void initClass(void);

  virtual void init(SoState * state);
  static void set(SoState * const state, SoNode * const node, SbBool mode);
  static SbBool get(SoState * const state);
  static const SoFCInteractiveElement * getInstance(SoState * state);

protected:
  virtual ~SoFCInteractiveElement();
  virtual void setElt(SbBool mode);

private:
  SbBool interactiveMode;
};

class GuiExport SoGLWidgetElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoGLWidgetElement);

public:
  static void initClass(void);

  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state, const SoElement * prevTopElement);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

  static  void set(SoState * state, QGLWidget * window);
  static  void get(SoState * state, QGLWidget *& window);

protected:
  virtual ~SoGLWidgetElement();

protected:
  QGLWidget * window;
};

class GuiExport SoGLRenderActionElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoGLRenderActionElement);

public:
  static void initClass(void);

  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual void pop(SoState * state, const SoElement * prevTopElement);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

  static  void set(SoState * state, SoGLRenderAction * action);
  static  void get(SoState * state, SoGLRenderAction * & action);

protected:
  virtual ~SoGLRenderActionElement();

protected:
  SoGLRenderAction * glRenderAction;
};

class GuiExport SoGLWidgetNode : public SoNode {
    typedef SoNode inherited;

    SO_NODE_HEADER(SoGLWidgetNode);

public:
    static void initClass(void);
    SoGLWidgetNode(void);

    QGLWidget * window;

    virtual void doAction(SoAction * action);
    virtual void GLRender(SoGLRenderAction * action);

protected:
    virtual ~SoGLWidgetNode();
};

} // namespace Gui

#endif // GUI_SOFCINTERACTIVEELEMENT_H
