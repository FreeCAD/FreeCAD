/***************************************************************************
 *   Copyright (c) 2005 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef GUI_SOFCUNIFIEDSELECTION_H
#define GUI_SOFCUNIFIEDSELECTION_H

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoLightModel.h>
#include "View3DInventorViewer.h"
#include <list>

class SoFullPath;
class SoPickedPoint;
class SoDetail;


namespace Gui {

class Document;
class ViewProviderDocumentObject;

/**  Unified Selection node
 *  This is the new selection node for the 3D Viewer which will 
 *  gradually remove all the low level selection nodes in the view
 *  provider. The handling of the highlighting and the selection will
 *  be unified here. 
 *  \author Jürgen Riegel
 */
class GuiExport SoFCUnifiedSelection : public SoSeparator {
    typedef SoSeparator inherited;

    SO_NODE_HEADER(Gui::SoFCUnifiedSelection);

public:
    static void initClass(void);
    static void finish(void);
    SoFCUnifiedSelection(void);
    void applySettings();

    enum HighlightModes {
        AUTO, ON, OFF
    };

    const char* getFileFormatName(void) const;
    void write(SoWriteAction * action);

    SoSFColor colorHighlight;
    SoSFColor colorSelection;
    SoSFEnum highlightMode;
    SoSFEnum selectionMode;
    SoSFBool selectionRole;

    virtual void doAction(SoAction *action);
    //virtual void GLRender(SoGLRenderAction * action);

    virtual void handleEvent(SoHandleEventAction * action);
    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    //virtual void GLRenderInPath(SoGLRenderAction * action);
    //static  void turnOffCurrentHighlight(SoGLRenderAction * action);

    bool checkSelectionStyle(int type, ViewProvider *vp);

    static bool hasHighlight();

    friend class View3DInventorViewer;
protected:
    virtual ~SoFCUnifiedSelection();
    //virtual void redrawHighlighted(SoAction * act, SbBool flag);
    //virtual SbBool readInstance(SoInput *  in, unsigned short  flags); 

private:
    //static void turnoffcurrent(SoAction * action);
    //void setOverride(SoGLRenderAction * action);
    //SbBool isHighlighted(SoAction *action);
    //SbBool preRender(SoGLRenderAction *act, GLint &oldDepthFunc);
    static int getPriority(const SoPickedPoint* p);

    struct PickedInfo {
        const SoPickedPoint *pp;
        ViewProviderDocumentObject *vpd;
        std::string element;
        PickedInfo():pp(0),vpd(0)
        {}
    };

    bool setHighlight(const PickedInfo &);
    bool setHighlight(SoFullPath *path, const SoDetail *det, 
            ViewProviderDocumentObject *vpd, const char *element, float x, float y, float z);
    bool setSelection(const std::vector<PickedInfo> &, bool ctrlDown=false);

    std::vector<PickedInfo> getPickedList(SoHandleEventAction* action, bool singlePick) const;

    Gui::Document       *pcDocument;

    static SoFullPath * currenthighlight;
    SoFullPath * detailPath;

    SbBool setPreSelection;

    // -1 = not handled, 0 = not selected, 1 = selected
    int32_t preSelection;
    SoColorPacker colorpacker;
};

class GuiExport SoFCPathAnnotation : public SoSeparator {
    typedef SoSeparator inherited;

    SO_NODE_HEADER(Gui::SoFCPathAnnotation);
public:
    static void initClass(void);
    static void finish(void);
    SoFCPathAnnotation();

    void setPath(SoPath *);

    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    virtual void GLRender(SoGLRenderAction * action);
    virtual void GLRenderInPath(SoGLRenderAction * action);

protected:
    virtual ~SoFCPathAnnotation();

protected:
    SoPath *path;
};

class GuiExport SoFCSelectionRoot : public SoSeparator {
    typedef SoSeparator inherited;

    SO_NODE_HEADER(Gui::SoFCSelectionRoot);
  
public:
    static void initClass(void);
    static void finish(void);
    SoFCSelectionRoot(bool secondary=false);

    virtual void GLRenderBelowPath(SoGLRenderAction * action);
    virtual void GLRender(SoGLRenderAction * action);
    virtual void GLRenderInPath(SoGLRenderAction * action);
    virtual void GLRenderOffPath(SoGLRenderAction * action);

    virtual void doAction(SoAction *action);

    /** Returns selection context for rendering. 
     *
     * @param node: the querying node
     * @param def: default context if none is found
     * @param secCtx: optional, for querying secondary context
     *
     * @return Returned the primary context for selection, and the context is
     * always stored in the first encounted SoFCSelectionRoot in the path. It
     * is keyed using the entires sequence of SoFCSelectionRoot along the path
     * to \c node, replacing the first SoFCSelectionRoot with the given node. 
     *
     * @return Secondary context returned in \c secCtx is for customized
     * highlighting, and is not affected by mouse event. The highlight is
     * applied manually using SoSelectionElementAction. It is stored in the
     * last encountered SoFCSelectionRoot, and is keyed using the querying 
     * \c node and (if there are more than one SoFCSelectionRoot along the
     * path) the first SoFCSelectionRoot. The reason is so that any link to a
     * node (new links means additional SoFCSelectionRoot added in front) with
     * customized subelement highlight will also show the highlight.
     *
     * @note For simplicity reason, currently secondary context is only freed
     * when the storage SoFCSSelectionRoot node is freed.
     */
    template<class T>
    static std::shared_ptr<T> getRenderContext(SoNode *node,  
                    std::shared_ptr<T> def, std::shared_ptr<T> *ctx2 = 0) 
    {
        ContextPtr _ctx2;
        auto ctx = std::static_pointer_cast<T>(getContext(node,def,ctx2?&_ctx2:0));
        if(ctx2) 
            *ctx2 = std::static_pointer_cast<T>(_ctx2);
        return ctx;
    }

    /** Get the selection context for an action. 
     *
     * @param action: the action. SoSelectionElementAction has any option to
     * query for secondary context. \sa getRenderContext for detail about
     * secondary context
     * @param node: the querying node
     * @param def: default context if none is found
     *
     * @return If no SoFCSelectionRoot is found in the current path of action,
     * \c def is returned. Otherwise a selection context returned. A new one
     * will be created if none is found.
     */
    template<class T>
    static std::shared_ptr<T> getActionContext(
            SoAction *action, SoNode *node, std::shared_ptr<T> def) 
    {
        ContextPtr pdef(def);
        ContextPtr *pctx = getContext(action,node,&pdef);
        if(pctx == &pdef)
            return def;
        // make a new context if there is none
        auto &ctx = *pctx;
        if(!ctx) ctx = std::make_shared<T>();
        return std::static_pointer_cast<T>(ctx);
    }

    static void removeActionContext(SoAction *action, SoNode *node) {
        getContext(action,node,0);
    }

    static SoNode *getCurrentRoot(bool front, SoNode *def);

    void resetContext();

protected:
    virtual ~SoFCSelectionRoot();

    typedef std::shared_ptr<void> ContextPtr;

    static ContextPtr getContext(SoNode *node, ContextPtr def, ContextPtr *ctx2);
    static ContextPtr *getContext(SoAction *action, SoNode *node, ContextPtr *pdef);

    //selection root node stack during rendering
    typedef std::vector<SoFCSelectionRoot*> Stack;
    static Stack SelStack; // stack for non-secondary-only nodes
    static Stack SelStack2; // stack for all selection root nodes

    typedef std::map<Stack,ContextPtr> ContextMap;
    ContextMap contextMap;
    ContextMap contextMap2;//holding secondary context

    bool pushed;//to prevent double push into the stack
    bool secondary;//indicate if this node for secondary context only
};

/**
 * @author Werner Mayer
 */
class GuiExport SoHighlightElementAction : public SoAction
{
    SO_ACTION_HEADER(SoHighlightElementAction);

public:
    SoHighlightElementAction ();
    ~SoHighlightElementAction();

    void setHighlighted(SbBool);
    SbBool isHighlighted() const;
    void setColor(const SbColor&);
    const SbColor& getColor() const;
    void setElement(const SoDetail*);
    const SoDetail* getElement() const;

    static void initClass();

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);

private:
    SbBool _highlight;
    SbColor _color;
    const SoDetail* _det;
};

/**
 * @author Werner Mayer
 */
class GuiExport SoSelectionElementAction : public SoAction
{
    SO_ACTION_HEADER(SoSelectionElementAction);

public:
    enum Type {None, Append, Remove, All};

    SoSelectionElementAction (Type, bool secondary = false);
    ~SoSelectionElementAction();

    Type getType() const;
    void setColor(const SbColor&);
    const SbColor& getColor() const;
    void setElement(const SoDetail*);
    const SoDetail* getElement() const;

    bool isSecondary() const {return _secondary;}

    static void initClass();

protected:
    virtual void beginTraversal(SoNode *node);

private:
    static void callDoAction(SoAction *action,SoNode *node);

private:
    Type _type;
    SbColor _color;
    const SoDetail* _det;
    bool _secondary;
};

/**
 * @author Werner Mayer
 */
class GuiExport SoVRMLAction : public SoAction
{
    SO_ACTION_HEADER(SoVRMLAction);

public:
    SoVRMLAction();
    ~SoVRMLAction();
    void setOverrideMode(SbBool);
    SbBool isOverrideMode() const;

    static void initClass();

private:
    SbBool overrideMode;
    std::list<int> bindList;
    static void callDoAction(SoAction *action,SoNode *node);

};


} // namespace Gui

#endif // !GUI_SOFCUNIFIEDSELECTION_H
