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

#include <list>
#include <unordered_map>
#include <unordered_set>

#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/nodes/SoSeparator.h>

#include "SoFCSelectionContext.h"
#include "View3DInventorViewer.h"


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
    using inherited = SoSeparator;

    SO_NODE_HEADER(Gui::SoFCUnifiedSelection);

public:
    static void initClass();
    static void finish();
    SoFCUnifiedSelection();
    void applySettings();

    enum HighlightModes {
        AUTO, ON, OFF
    };

    const char* getFileFormatName() const override;
    void write(SoWriteAction * action) override;

    SoSFColor colorHighlight;
    SoSFColor colorSelection;
    SoSFEnum highlightMode;
    SoSFEnum selectionMode;
    SoSFBool selectionRole;
    SoSFBool useNewSelection;

    void doAction(SoAction *action) override;
    //virtual void GLRender(SoGLRenderAction * action);

    void handleEvent(SoHandleEventAction * action) override;
    void GLRenderBelowPath(SoGLRenderAction * action) override;
    //virtual void GLRenderInPath(SoGLRenderAction * action);
    //static  void turnOffCurrentHighlight(SoGLRenderAction * action);

    static bool hasHighlight();

    friend class View3DInventorViewer;

protected:
    ~SoFCUnifiedSelection() override;
    //virtual void redrawHighlighted(SoAction * act, SbBool flag);
    //virtual SbBool readInstance(SoInput *  in, unsigned short  flags);

private:
    //static void turnoffcurrent(SoAction * action);
    //void setOverride(SoGLRenderAction * action);
    //SbBool isHighlighted(SoAction *action);
    //SbBool preRender(SoGLRenderAction *act, GLint &oldDepthFunc);
    static int getPriority(const SoPickedPoint* p);

    struct PickedInfo {
        const SoPickedPoint *pp{nullptr};
        ViewProviderDocumentObject *vpd{nullptr};
        std::string element;
    };

    bool setHighlight(const PickedInfo &);
    bool setHighlight(SoFullPath *path, const SoDetail *det,
            ViewProviderDocumentObject *vpd, const char *element, float x, float y, float z);
    bool setSelection(const std::vector<PickedInfo> &, bool ctrlDown=false);

    std::vector<PickedInfo> getPickedList(SoHandleEventAction* action, bool singlePick) const;

    Gui::Document       *pcDocument{nullptr};

    static SoFullPath * currenthighlight;
    SoFullPath * detailPath;

    SbBool setPreSelection;

    // -1 = not handled, 0 = not selected, 1 = selected
    int32_t preSelection;
    SoColorPacker colorpacker;
};

class GuiExport SoFCPathAnnotation : public SoSeparator {
    using inherited = SoSeparator;

    SO_NODE_HEADER(Gui::SoFCPathAnnotation);
public:
    static void initClass();
    static void finish();
    SoFCPathAnnotation();

    void setPath(SoPath *);
    SoPath *getPath() {return path;}
    void setDetail(SoDetail *d);
    SoDetail *getDetail() {return det;}

    void GLRenderBelowPath(SoGLRenderAction * action) override;
    void GLRender(SoGLRenderAction * action) override;
    void GLRenderInPath(SoGLRenderAction * action) override;

    void getBoundingBox(SoGetBoundingBoxAction * action) override;

protected:
    ~SoFCPathAnnotation() override;

protected:
    SoPath *path;
    SoTempPath *tmpPath;
    SoDetail *det;
};

class GuiExport SoFCSeparator : public SoSeparator {
    using inherited = SoSeparator;

    SO_NODE_HEADER(Gui::SoFCSeparator);

public:
    static void initClass();
    static void finish();
    explicit SoFCSeparator(bool trackCacheMode=true);

    void GLRenderBelowPath(SoGLRenderAction * action) override;

    static void setCacheMode(CacheEnabled mode) {
        CacheMode = mode;
    }
    static CacheEnabled getCacheMode() {
        return CacheMode;
    }

private:
    bool trackCacheMode;
    static CacheEnabled CacheMode;
};

class GuiExport SoFCSelectionRoot : public SoFCSeparator {
    using inherited = SoFCSeparator;

    SO_NODE_HEADER(Gui::SoFCSelectionRoot);

public:
    static void initClass();
    static void finish();
    explicit SoFCSelectionRoot(bool trackCacheMode=false);

    void GLRenderBelowPath(SoGLRenderAction * action) override;
    void GLRenderInPath(SoGLRenderAction * action) override;

    void doAction(SoAction *action) override;
    void pick(SoPickAction * action) override;
    void rayPick(SoRayPickAction * action) override;
    void handleEvent(SoHandleEventAction * action) override;
    void search(SoSearchAction * action) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction * action) override;
    void getBoundingBox(SoGetBoundingBoxAction * action) override;
    void getMatrix(SoGetMatrixAction * action) override;
    void callback(SoCallbackAction *action) override;

    template<class T>
    static std::shared_ptr<T> getRenderContext(SoNode *node, std::shared_ptr<T> def = std::shared_ptr<T>()) {
        return std::dynamic_pointer_cast<T>(getNodeContext(SelStack,node,def));
    }

    /** Returns selection context for rendering.
     *
     * @param node: the querying node
     * @param def: default context if none is found
     * @param ctx2: secondary context output
     *
     * @return Returned the primary context for selection, and the context is
     * always stored in the first encountered SoFCSelectionRoot in the path. It
     * is keyed using the entire sequence of SoFCSelectionRoot along the path
     * to \c node, replacing the first SoFCSelectionRoot with the given node.
     *
     * @return Secondary context returned in \c ctx2 is for customized
     * highlighting, and is not affected by mouse event. The highlight is
     * applied manually using SoSelectionElementAction. It is stored in the
     * last encountered SoFCSelectionRoot, and is keyed using the querying
     * \c node and (if there are more than one SoFCSelectionRoot along the
     * path) the first SoFCSelectionRoot. The reason is so that any link to a
     * node (new links means additional SoFCSelectionRoot added in front) with
     * customized subelement highlight will also show the highlight. Secondary
     * context can be chained, which why the secondary context type must provide
     * an function called merge() for getRenderContext() to merge the context.
     * See SoFCSelectionContext::merge() for an implementation of merging multiple
     * context.
     *
     * @note For simplicity reason, currently secondary context is only freed
     * when the storage SoFCSSelectionRoot node is freed.
     */
    template<class T>
    static std::shared_ptr<T> getRenderContext(SoNode *node, std::shared_ptr<T> def, std::shared_ptr<T> &ctx2)
    {
        ctx2 = std::dynamic_pointer_cast<T>(getNodeContext2(SelStack,node,T::merge));
        return std::dynamic_pointer_cast<T>(getNodeContext(SelStack,node,def));
    }

    /** Get the selection context for an action.
     *
     * @param action: the action. SoSelectionElementAction has any option to
     * query for secondary context. \sa getRenderContext for detail about
     * secondary context
     * @param node: the querying node
     * @param def: default context if none is found, only used if querying
     * non-secondary context
     * @param create: create a new context if none is found
     *
     * @return If no SoFCSelectionRoot is found in the current path of action,
     * \c def is returned. Otherwise a selection context returned. A new one
     * will be created if none is found.
     */
    template<class T>
    static std::shared_ptr<T> getActionContext(
            SoAction *action, SoNode *node, std::shared_ptr<T> def=std::shared_ptr<T>(), bool create=true)
    {
        auto res = findActionContext(action,node,create,false);
        if(!res.second) {
            if(res.first)
                return std::shared_ptr<T>();
            // default context is only applicable for non-secondary context query
            return def;
        }
        // make a new context if there is none
        auto &ctx = *res.second;
        if(ctx) {
            auto ret = std::dynamic_pointer_cast<T>(ctx);
            if(!ret)
                ctx.reset();
        }
        if(!ctx && create)
            ctx = std::make_shared<T>();
        return std::static_pointer_cast<T>(ctx);
    }

    static bool removeActionContext(SoAction *action, SoNode *node) {
        return findActionContext(action,node,false,true).second!=0;
    }

    template<class T>
    static std::shared_ptr<T> getSecondaryActionContext(SoAction *action, SoNode *node) {
        auto it = ActionStacks.find(action);
        if(it == ActionStacks.end())
            return std::shared_ptr<T>();
        return std::dynamic_pointer_cast<T>(getNodeContext2(it->second,node,T::merge));
    }

    static void checkSelection(bool &sel, SbColor &selColor, bool &hl, SbColor &hlColor);

    static void moveActionStack(SoAction *from, SoAction *to, bool erase);

    static SoNode *getCurrentRoot(bool front, SoNode *def);

    void resetContext();

    static bool checkColorOverride(SoState *state);

    bool hasColorOverride() const {
        return overrideColor;
    }

    void setColorOverride(App::Color c) {
        overrideColor = true;
        colorOverride = SbColor(c.r,c.g,c.b);
        transOverride = c.a;
    }

    void removeColorOverride() {
        overrideColor = false;
    }

    enum SelectStyles {
        Full, Box, PassThrough
    };
    SoSFEnum selectionStyle;

    static bool renderBBox(SoGLRenderAction *action, SoNode *node, SbColor color);

protected:
    ~SoFCSelectionRoot() override;

    void renderPrivate(SoGLRenderAction *, bool inPath);
    bool _renderPrivate(SoGLRenderAction *, bool inPath);

    class Stack : public std::vector<SoNode*> {
    public:
        std::unordered_set<SoNode*> nodeSet;
        size_t offset = 0;
    };

    static SoFCSelectionContextBasePtr getNodeContext(
            Stack &stack, SoNode *node, SoFCSelectionContextBasePtr def);
    static SoFCSelectionContextBasePtr getNodeContext2(
            Stack &stack, SoNode *node, SoFCSelectionContextBase::MergeFunc *merge);
    static std::pair<bool,SoFCSelectionContextBasePtr*> findActionContext(
            SoAction *action, SoNode *node, bool create, bool erase);

    static Stack SelStack;
    static std::unordered_map<SoAction*,Stack> ActionStacks;
    struct StackComp {
        bool operator()(const Stack &a, const Stack &b) const;
    };

    using ContextMap = std::map<Stack,SoFCSelectionContextBasePtr,StackComp>;
    ContextMap contextMap;
    ContextMap contextMap2;//holding secondary context

    struct SelContext: SoFCSelectionContextBase {
    public:
        SbColor selColor;
        SbColor hlColor;
        bool selAll = false;
        bool hlAll = false;
        bool hideAll = false;
        static MergeFunc merge;
    };
    using SelContextPtr = std::shared_ptr<SelContext>;
    using ColorStack = std::vector<SbColor>;
    static ColorStack SelColorStack;
    static ColorStack HlColorStack;
    static SoFCSelectionRoot *ShapeColorNode;
    bool overrideColor = false;
    SbColor colorOverride;
    float transOverride = 0.0f;
    SoColorPacker shapeColorPacker;

    bool doActionPrivate(Stack &stack, SoAction *);
};

/**
 * @author Werner Mayer
 */
class GuiExport SoHighlightElementAction : public SoAction
{
    SO_ACTION_HEADER(SoHighlightElementAction);

public:
    SoHighlightElementAction ();
    ~SoHighlightElementAction() override;

    void setHighlighted(SbBool);
    SbBool isHighlighted() const;
    void setColor(const SbColor&);
    const SbColor& getColor() const;
    void setElement(const SoDetail*);
    const SoDetail* getElement() const;

    static void initClass();

protected:
    void beginTraversal(SoNode *node) override;

private:
    static void callDoAction(SoAction *action,SoNode *node);

private:
    SbBool _highlight{false};
    SbColor _color;
    const SoDetail* _det{nullptr};
};

/**
 * @author Werner Mayer
 */
class GuiExport SoSelectionElementAction : public SoAction
{
    SO_ACTION_HEADER(SoSelectionElementAction);

public:
    enum Type {None, Append, Remove, All, Color, Hide, Show};

    explicit SoSelectionElementAction (Type=None, bool secondary = false);
    ~SoSelectionElementAction() override;

    Type getType() const;
    void setType(Type type) {
        _type = type;
    }

    void setColor(const SbColor&);
    const SbColor& getColor() const;
    void setElement(const SoDetail*);
    const SoDetail* getElement() const;

    bool isSecondary() const {return _secondary;}
    void setSecondary(bool enable) {
        _secondary = enable;
    }

    const std::map<std::string,App::Color> &getColors() const {
        return _colors;
    }
    void setColors(const std::map<std::string,App::Color> &colors) {
        _colors = colors;
    }
    void swapColors(std::map<std::string,App::Color> &colors) {
        _colors.swap(colors);
    }

    static void initClass();

protected:
    void beginTraversal(SoNode *node) override;

private:
    static void callDoAction(SoAction *action,SoNode *node);

private:
    Type _type;
    SbColor _color;
    const SoDetail* _det{nullptr};
    std::map<std::string,App::Color> _colors;
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
    ~SoVRMLAction() override;
    void setOverrideMode(SbBool);
    SbBool isOverrideMode() const;

    static void initClass();

private:
    SbBool overrideMode{true};
    std::list<int> bindList;
    static void callDoAction(SoAction *action,SoNode *node);

};


} // namespace Gui

#endif // !GUI_SOFCUNIFIEDSELECTION_H
