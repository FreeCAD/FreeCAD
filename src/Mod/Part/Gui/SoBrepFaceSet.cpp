/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef FC_OS_WIN32
#define GL_GLEXT_PROTOTYPES
#endif

#ifndef _PreComp_
# include <float.h>
# include <algorithm>
# include <map>
# include <Python.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/SoPrimitiveVertex.h>
# include <Inventor/actions/SoCallbackAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/actions/SoPickAction.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/bundles/SoTextureCoordinateBundle.h>
# include <Inventor/elements/SoLazyElement.h>
# include <Inventor/elements/SoOverrideElement.h>
# include <Inventor/elements/SoCoordinateElement.h>
# include <Inventor/elements/SoGLCoordinateElement.h>
# include <Inventor/elements/SoGLCacheContextElement.h>
# include <Inventor/elements/SoGLVBOElement.h>
# include <Inventor/elements/SoLineWidthElement.h>
# include <Inventor/elements/SoPointSizeElement.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/misc/SoState.h>
# include <Inventor/misc/SoContextHandler.h>
# include <Inventor/elements/SoShapeStyleElement.h>
# include <Inventor/elements/SoCacheElement.h>
# include <Inventor/elements/SoTextureEnabledElement.h>
# ifdef FC_OS_WIN32
#  include <windows.h>
#  include <GL/gl.h>
#  include <GL/glext.h>
# else
#  ifdef FC_OS_MACOSX
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#  else
#   include <GL/gl.h>
#   include <GL/glext.h>
#  endif //FC_OS_MACOSX
# endif //FC_OS_WIN32
// Should come after glext.h to avoid warnings
# include <Inventor/C/glue/gl.h>
#endif

#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/caches/SoBoundingBoxCache.h>

#include <boost/algorithm/string/predicate.hpp>

#include "SoBrepFaceSet.h"
#include <Base/Console.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/SoFCInteractiveElement.h>
#include <Gui/ViewParams.h>

FC_LOG_LEVEL_INIT("SoBrepFaceSet",true,true);

using namespace PartGui;

SO_NODE_SOURCE(SoBrepFaceSet)

static bool makeDistinctColor(SbColor &res, const SbColor &color, const SbColor &other) {
    float h,s,v;
    color.getHSVValue(h,s,v);
    float h2,s2,v2;
    other.getHSVValue(h2,s2,v2);

    if(fabs(h-h2) > 0.07)
        return false;
    h += 0.3;
    if(h>1.0)
        h = 1.0-h;
    res.setHSVValue(h,s,1.0f);
    return true;
}

static inline SbColor makeDistinctColor(const SbColor &color, const SbColor &other) {
    SbColor c;
    return makeDistinctColor(c,color,other)?c:color;
}

static inline bool makeDistinctColor(uint32_t &res, uint32_t color, uint32_t other) {
    SbColor r, c, o;
    float t;
    o.setPackedValue(other,t);
    c.setPackedValue(color,t);
    if(!makeDistinctColor(r,c,o))
        return false;
    res = r.getPackedValue(t);
    return true;
}

#define PRIVATE(p) ((p)->pimpl)

class SoBrepFaceSet::VBO {
public:
    
    struct Buffer {
        uint32_t myvbo[2];
        std::size_t vertex_array_size;
        std::size_t index_array_size;
        bool updateVbo;
        bool vboLoaded;
    };

    static SbBool vboAvailable;
    uint32_t indice_array;
    std::map<uint32_t, Buffer> vbomap;

    VBO()
    {
        SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
        indice_array = 0;
    }

    ~VBO()
    {
        SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);

        // schedule delete for all allocated GL resources
        std::map<uint32_t, Buffer>::iterator it;
        for (it = vbomap.begin(); it != vbomap.end(); ++it) {
            void * ptr0 = (void*) ((uintptr_t) it->second.myvbo[0]);
            SoGLCacheContextElement::scheduleDeleteCallback(it->first, VBO::vbo_delete, ptr0);
            void * ptr1 = (void*) ((uintptr_t) it->second.myvbo[1]);
            SoGLCacheContextElement::scheduleDeleteCallback(it->first, VBO::vbo_delete, ptr1);
        }
    }

    bool isVboAvailable(SoGLRenderAction *action) const {
        SoState *state = action->getState();

        if(!vboAvailable || !Gui::SoGLVBOActivatedElement::get(state))
            return false;

        uint32_t flags = SoOverrideElement::getFlags(state);
        if(flags & (SoOverrideElement::NORMAL_VECTOR|SoOverrideElement::NORMAL_BINDING))
            return false;

        if(flags & (SoOverrideElement::COLOR_INDEX|
                    SoOverrideElement::DIFFUSE_COLOR|
                    SoOverrideElement::MATERIAL_BINDING|
                    SoOverrideElement::TRANSPARENCY))
        {
            auto it = vbomap.find(action->getCacheContext());
            if(it == vbomap.end())
                return false;
            auto &info = it->second;
            return info.vboLoaded && !info.updateVbo;
        }
        return true;
    }

    void render(SoGLRenderAction * action,
                bool color_override,
                const std::vector<int32_t> &render_indices,
                const SoCoordinateElement *coords,
                const int32_t *vertexindices,
                int num_vertexindices,
                const int32_t *partindices,
                const int32_t *indexoffsets,
                int num_partindices,
                const SbVec3f *normals,
                const int32_t *normindices,
                SoMaterialBundle *const materials,
                const int32_t *matindices,
                SoTextureCoordinateBundle * const texcoords,
                const int32_t *texindices,
                const int nbind,
                const int mbind,
                const int texture);

    static void context_destruction_cb(uint32_t context, void * userdata)
    {
        VBO * self = static_cast<VBO*>(userdata);

        std::map<uint32_t, Buffer>::iterator it = self->vbomap.find(context);
        if (it != self->vbomap.end()) {
#ifdef FC_OS_WIN32
            const cc_glglue * glue = cc_glglue_instance((int) context);
            PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)cc_glglue_getprocaddress(glue, "glDeleteBuffersARB");
#endif
            //cc_glglue_glDeleteBuffers(glue, buffer.size(), buffer.data());
            auto &buffer = it->second;
            glDeleteBuffersARB(2, buffer.myvbo);
            self->vbomap.erase(it);
        }
    }

    static void vbo_delete(void * closure, uint32_t contextid)
    {
        const cc_glglue * glue = cc_glglue_instance((int) contextid);
        GLuint id = (GLuint) ((uintptr_t) closure);
        cc_glglue_glDeleteBuffers(glue, 1, &id);
    }
};

SbBool SoBrepFaceSet::VBO::vboAvailable = false;

void SoBrepFaceSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepFaceSet, SoIndexedFaceSet, "IndexedFaceSet");
}

SoBrepFaceSet::SoBrepFaceSet()
{
    SO_NODE_CONSTRUCTOR(SoBrepFaceSet);
    SO_NODE_ADD_FIELD(partIndex, (-1));

    selContext = std::make_shared<SelContext>();
    selContext2 = std::make_shared<SelContext>();

    pimpl.reset(new VBO);

    partIndexSensor.attach(&partIndex);
    partIndexSensor.setData(this);
    partIndexSensor.setFunction([](void *data, SoSensor*){
        reinterpret_cast<SoBrepFaceSet*>(data)->onPartIndexChange();
    });
}

SoBrepFaceSet::~SoBrepFaceSet()
{
}

void SoBrepFaceSet::onPartIndexChange() {
    partBBoxes.clear();
    indexOffset.clear();
    partIndexMap.clear();
}

void SoBrepFaceSet::buildPartIndexCache() {
    if(partIndex.getNum()+1 == (int)indexOffset.size())
        return;

    indexOffset.resize(partIndex.getNum()+1);
    const int32_t *piptr = partIndex.getValues(0);
    int32_t c = 0;
    for(int i=0,count=partIndex.getNum();i<count;++i) {
        indexOffset[i] = c;
        c += piptr[i];
    }
    indexOffset[partIndex.getNum()] = c;
    partIndexMap.clear();
    if(partIndex.getNum() > 200) {
        c = 0;
        for(int i=0,count=partIndex.getNum();i<count;++i) {
            partIndexMap[c] = i;
            c += piptr[i];
        }
    }
}

void SoBrepFaceSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        selCounter.checkAction(hlaction);
        if (!hlaction->isHighlighted()) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
            if(ctx) {
                ctx->highlightIndex = -1;
                touch();
            }
            return;
        }

        const SoDetail* detail = hlaction->getElement();
        if (!detail) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
            ctx->highlightIndex = INT_MAX;
            ctx->highlightColor = hlaction->getColor();
            touch();
        }else {
            if (!detail->isOfType(SoFaceDetail::getClassTypeId())) {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx) {
                    ctx->highlightIndex = -1;
                    touch();
                }
            }else {
                int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                ctx->highlightIndex = index;
                ctx->highlightColor = hlaction->getColor();
                touch();
            }
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
        switch(selaction->getType()) {
        case Gui::SoSelectionElementAction::All: {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action,this,selContext);
            selCounter.checkAction(selaction,ctx);
            ctx->selectionColor = selaction->getColor();
            ctx->selectionIndex.clear();
            ctx->selectionIndex.insert(-1);
            touch();
            return;
        } case Gui::SoSelectionElementAction::None:
            if(selaction->isSecondary()) {
                if(Gui::SoFCSelectionRoot::removeActionContext(action,this))
                    touch();
            }else {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx) {
                    ctx->selectionIndex.clear();
                    ctx->colors.clear();
                    touch();
                }
            }
            return;
        case Gui::SoSelectionElementAction::Color:
            if(selaction->isSecondary()) {
                const auto &colors = selaction->getColors();
                auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(colors.empty()) {
                    if(ctx) {
                        ctx->colors.clear();
                        if(ctx->isSelectAll())
                            Gui::SoFCSelectionRoot::removeActionContext(action,this);
                        touch();
                    }
                    return;
                }
                static std::string element("Face");
                if(colors.begin()->first.empty() || colors.lower_bound(element)!=colors.end()) {
                    if(!ctx) {
                        ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action,this);
                        selCounter.checkAction(selaction,ctx);
                        ctx->selectAll();
                    }
                    if(ctx->setColors(selaction->getColors(),element))
                        touch();
                }
            }
            return;
        case Gui::SoSelectionElementAction::Remove:
        case Gui::SoSelectionElementAction::Append: {
            const SoDetail* detail = selaction->getElement();
            if (!detail || !detail->isOfType(SoFaceDetail::getClassTypeId())) {
                if(selaction->isSecondary()) {
                    // For secondary context, a detail of different type means
                    // the user may want to partial render only other type of
                    // geometry. So we call below to obtain a action context.
                    // If no secondary context exist, it will create an empty
                    // one, and an empty secondary context inhibites drawing
                    // here.
                    auto ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action,this);
                    selCounter.checkAction(selaction,ctx);
                    touch();
                }
                return;
            }
            int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
            if (selaction->getType() == Gui::SoSelectionElementAction::Append) {
                auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                selCounter.checkAction(selaction,ctx);
                ctx->selectionColor = selaction->getColor();
                if(ctx->isSelectAll())
                    ctx->selectionIndex.clear();
                if(ctx->selectionIndex.insert(index).second)
                    touch();
            }else{
                auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx && ctx->removeIndex(index))
                    touch();
            }
            break;
        } default:
            break;
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoVRMLAction::getClassTypeId()) {
        // update the materialIndex field to match with the number of triangles if needed
        SoState * state = action->getState();
        Binding mbind = this->findMaterialBinding(state);
        if (mbind == PER_PART) {
            const SoLazyElement* mat = SoLazyElement::getInstance(state);
            int numColor = 1;
            int numParts = partIndex.getNum();
            if (mat) {
                numColor = mat->getNumDiffuse();
                if (numColor == numParts) {
                    int count = 0;
                    const int32_t * indices = this->partIndex.getValues(0);
                    for (int i=0; i<numParts; i++) {
                        count += indices[i];
                    }
                    this->materialIndex.setNum(count);
                    int32_t * matind = this->materialIndex.startEditing();
                    int32_t k = 0;
                    for (int i=0; i<numParts; i++) {
                        for (int j=0; j<indices[i]; j++) {
                            matind[k++] = i;
                        }
                    }
                    this->materialIndex.finishEditing();
                }
            }
        }
    }
    // The recommended way to set 'updateVbo' is to reimplement the method 'notify'
    // but the base class made this method private so that we can't override it.
    // So, the alternative way is to write a custom SoAction class.
    else if (action->getTypeId() == Gui::SoUpdateVBOAction::getClassTypeId()) {
        for(auto &v : PRIVATE(this)->vbomap) {
            v.second.updateVbo = true;
            v.second.vboLoaded = false;
        }
    }

    inherited::doAction(action);
}

void SoBrepFaceSet::setSiblings(std::vector<SoNode*> &&s) {
    // No need to ref() here, because we only use the pointer as keys to lookup
    // selection context
    siblings = std::move(s);
}

void SoBrepFaceSet::GLRender(SoGLRenderAction *action)
{
    //SoBase::staticDataLock();
    static bool init = false;
    if (!init) {
        std::string ext = (const char*)(glGetString(GL_EXTENSIONS));
        PRIVATE(this)->vboAvailable = (ext.find("GL_ARB_vertex_buffer_object") != std::string::npos);
        init = true;
    }
    //SoBase::staticDataUnlock();

    if (this->coordIndex.getNum() < 3)
        return;

    auto state = action->getState();

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Copied from SoShape::shouldGLRender(). We are replacing it so that we
    // can do our own transparency sorting.
    // TODO: check SoShape::shouldGLRender() code in case we want to render shadow
    const SoShapeStyleElement * shapestyle = SoShapeStyleElement::get(state);
    unsigned int shapestyleflags = shapestyle->getFlags();
    if (shapestyleflags & SoShapeStyleElement::INVISIBLE)
        return;
    if (getBoundingBoxCache() && !state->isCacheOpen() && !SoCullElement::completelyInside(state)) {
        if (getBoundingBoxCache()->isValid(state)) {
            if (SoCullElement::cullTest(state, getBoundingBoxCache()->getProjectedBox())) {
                return;
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////

    selCounter.checkCache(state);

    SelContextPtr ctx2;
    std::vector<SelContextPtr> ctxs;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext(this,selContext,ctx2);
    if(ctx2 && ctx2->selectionIndex.empty())
        return;

    if(selContext2->checkGlobal(ctx)) {
        ctx = selContext2;
        SoCacheElement::invalidate(state);
    }
    if(ctx && (!ctx->selectionIndex.size() && ctx->highlightIndex<0))
        ctx.reset();

    if((!ctx2||ctx2->isSelectAll()) && ctx && ctx->isHighlightAll()) {
        // Highlight (preselect) all is done in View3DInventerViewer with a
        // dedicated GroupOnTopPreSel. We shall only render edge and point.
        // But if we have partial rendering (ctx2), then edges and points are
        // not rendered, so we have to proceed as normal
        if(!action->isRenderingDelayedPaths()) {
            if(!Gui::ViewParams::instance()->getShowHighlightEdgeOnly())
                return;
        } else if(Gui::ViewParams::instance()->getShowHighlightEdgeOnly())
            return;
    }

    // Check this node's selection state first
    int selected = 0;
    if(ctx) {
        if(ctx->isSelected())
            selected = 1;
        else if(ctx->isHighlighted() && Gui::Selection().needPickedList())
            selected = 2;
    } else if ((!ctx2 || ctx2->isSelectAll())
                && (Gui::Selection().needPickedList() 
                    || (Gui::ViewParams::instance()->getShowSelectionOnTop()
                        && !Gui::SoFCUnifiedSelection::getShowSelectionBoundingBox())))
    {
        // Check the sibling selection state
        for(auto node : siblings) {
            auto sctx = Gui::SoFCSelectionRoot::getRenderContext<Gui::SoFCSelectionContext>(node);
            if(sctx) {
                if(sctx->isSelected()) {
                    selected = 1;
                    break;
                } else if (sctx->isHighlighted() && Gui::Selection().needPickedList())
                    selected = 2;
            }
        }
    }

    // If 'ShowSelectionOnTop' is enabled, and this node is selected, and we
    // are NOT rendering on top (!isRenderingDelayedPath), and we are not
    // partial rendering.
    if(Gui::ViewParams::instance()->getShowSelectionOnTop()
            && !Gui::SoFCUnifiedSelection::getShowSelectionBoundingBox()
            && (!ctx2||ctx2->isSelectAll()) 
            && selected == 1
            && !action->isRenderingDelayedPaths())
    {
        // Then the face will be rendered in group on top, i.e. when
        // action->isRenderingDelayedPaths(). So we just return here.
        return;
    }

    // override material binding to PER_PART_INDEX to achieve
    // preselection/selection with transparency
    bool pushed = overrideMaterialBinding(action,selected,ctx,ctx2);

    if(!pushed){
        // for non transparent cases, we still use the old selection rendering
        // code, because it can override emission color, which gives a more
        // distinguishable selection highlight. The above material binding
        // override method can't, because Coin does not support per part
        // emission color, also because emission color does not really work
        // with transparency.

        // There are a few factors affects the rendering order.
        //
        // 1) For normal case, the highlight (pre-selection) is the top layer.
        // And since the default glDepthFunc is GL_LESS (can we force GL_LEQUAL
        // here?), we shall draw highlight first, then selection, then the rest
        // part.
        //
        // 2) If action->isRenderingDelayedPaths() is true, it means we are
        // rendering with depth buffer clipping turned off (always on top
        // rendering), so we shall draw the top layer last, i.e.
        // renderHighlight() last
        //
        // 3) If there is highlight but not highlight all, in order to not
        // obscure selection layer, we shall draw highlight after selection.
        //
        // Transparency complicates stuff even more, but not here. It will be
        // handled inside overrideMaterialBinding()
        //
        if(ctx && ctx->isHighlightAll()) {
            if(ctx2 && !ctx2->isSelectAll()) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action,ctx2); 
            } else
                renderHighlight(action,ctx);
            return;
        }

        if(!action->isRenderingDelayedPaths())
            renderHighlight(action,ctx);
        if(ctx && ctx->isSelected()) {
            if(ctx->isSelectAll()) {
                if(ctx2 && !ctx2->isSelectAll()) {
                    ctx2->selectionColor = ctx->selectionColor;
                    renderSelection(action,ctx2); 
                } else
                    renderSelection(action,ctx); 
                if(action->isRenderingDelayedPaths())
                    renderHighlight(action,ctx);
                return;
            }
            if(!action->isRenderingDelayedPaths())
                renderSelection(action,ctx); 
        }
        if(ctx2) {
            renderSelection(action,ctx2,false);
            if(action->isRenderingDelayedPaths()) {
                renderSelection(action,ctx); 
                renderHighlight(action,ctx);
            }
            return;
        }
    }

    // If 'ShowSelectionOnTop' is enabled, and we ARE rendering on top
    // (isRenderingDelayedPath), and we are not partial rendering (!ctx2).
    if(Gui::ViewParams::instance()->getShowSelectionOnTop()
            && !Gui::SoFCUnifiedSelection::getShowSelectionBoundingBox()
            && (!ctx2 || ctx2->isSelectAll())
            && (!ctx || (!ctx->isHighlightAll() && !ctx->isSelectAll()))
            && action->isRenderingDelayedPaths())
    {
        // Perform a depth buffer only rendering so that we can draw the
        // correct outline in SoBrepEdgeSet. But only do this if vbo is
        // available for performance reason.
        Gui::FCDepthFunc guard(GL_LEQUAL);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        renderShape(action,ctx2,false);
        glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    }

    renderShape(action,ctx2,true);

    if(pushed) {
        SbBool notify = enableNotify(FALSE);
        materialIndex.setNum(0);
        if(notify) enableNotify(notify);
        state->pop();
    }else if(action->isRenderingDelayedPaths()) {
        renderSelection(action,ctx); 
        renderHighlight(action,ctx);
    }

    if(Gui::ViewParams::instance()->getSelectionFaceWire()
            && ctx && ctx->isSelected() && !ctx->isSelectAll()) 
    {
        // If SelectionFaceWire is enabled, draw the hidden lines of each face
        // primitives
        state->push();
        SbColor c = ctx->selectionColor;
        if(!pushed) {
            // !pushed usually means no transparency, so crank up the color
            // intensity to make it more distinct.
            float h,s,v;
            c.getHSVValue(h,s,v);
            s = 1.0f;
            v = 1.0f;
            c.setHSVValue(h,s,v);
        }
        uint32_t color = c.getPackedValue(0.0f);
        Gui::SoFCSelectionRoot::setupSelectionLineRendering(state,this,&color);
        SoPolygonOffsetElement::set(state, this, 0.0f, 0.0f,
                                    SoPolygonOffsetElement::FILLED, FALSE);
        SoDrawStyleElement::set(state, this, SoDrawStyleElement::LINES);
        renderSelection(action,ctx,false); 
        state->pop();
    }
}

static inline bool isOpaque(int id, const float *trans, int numtrans, 
        const std::vector<int32_t> mindices, const std::vector<uint32_t> &packed) 
{
    if(id < (int)mindices.size())
        return (packed[mindices[id]]&0xff)==0xff;
    if(numtrans==1)
        id = 0;
    if(id < numtrans) 
        return trans[id]==0.0f;
    return true;
}

static inline bool isTranslucent(int id, const float *trans, int numtrans, 
        const std::vector<int32_t> mindices, const std::vector<uint32_t> &packed) 
{
    if(id < (int)mindices.size()) {
        uint32_t t = packed[mindices[id]] & 0xff;
        return t && t<0xff;
    }
    if(numtrans==1)
        id = 0;
    if(id < numtrans) 
        return trans[id]!=0.0f && trans[id]<1.0;
    return false;
}

static FC_COIN_THREAD_LOCAL std::vector<int> RenderIndices;

struct PartDist {
    int index;
    float dist;
    PartDist(int i, float d)
        :index(i),dist(d)
    {}
};
static FC_COIN_THREAD_LOCAL std::vector<PartDist> SortedParts;

void SoBrepFaceSet::renderShape(SoGLRenderAction *action, SelContextPtr ctx2, bool checkTransp) 
{
    SoState *state = action->getState();

    bool transparent = checkTransp 
        && (SoShapeStyleElement::get(state)->getFlags() 
            & (SoShapeStyleElement::TRANSP_TEXTURE|SoShapeStyleElement::TRANSP_MATERIAL));

    if(transparent) {
        auto element = SoLazyElement::getInstance(state);
        const float *trans = element->getTransparencyPointer();
        int numtrans = element->getNumTransparencies();

        // In case not all faces are transparent, render opaque one first, with
        // depth test enabled
        if(!action->isRenderingTranspPaths()) {
            if(SoMaterialBindingElement::get(state) != SoMaterialBindingElement::OVERALL) {
                RenderIndices.clear();
                int numparts = partIndex.getNum();
                if(ctx2 && !ctx2->isSelectAll() && ctx2->isSelected()) {
                    for(auto id : ctx2->selectionIndex) {
                        if(id<0 || id>=numparts || !isOpaque(id,trans,numtrans,matIndex,packedColors))
                            continue;
                        RenderIndices.push_back(id);
                    }

                } else {
                    for(int id=0;id<numparts;++id) {
                        if(!isOpaque(id,trans,numtrans,matIndex,packedColors))
                            continue;
                        RenderIndices.push_back(id);
                    }
                }
                if(RenderIndices.size()) {
                    Gui::FCDepthFunc guard(GL_LEQUAL);
                    renderShape(action, true);
                }
            }
            // Calling handleTransparency() here to get us queued in the
            // delayed transparency rendering paths
            action->handleTransparency(true);
        } 
        
        if(action->isRenderingDelayedPaths() || action->isRenderingTranspPaths()) {

            // We perform our own "per part" face sorting to avoid artifacts in
            // transparent face rendering, where some triangle inside a part is
            // mis-sorted.
            sortParts(state, ctx2, trans, numtrans);

            RenderIndices.clear();
            for(auto &v : SortedParts)
                RenderIndices.push_back(v.index);

            if(RenderIndices.size()) {
                // Calling handleTransparency() here will setup blending for us
                action->handleTransparency(true);
                renderShape(action,checkTransp);
            }
        }
    } else  if (ctx2 && !ctx2->isSelectAll() && ctx2->isSelected()) {
        RenderIndices.clear();
        for(auto id : ctx2->selectionIndex) {
            if(id>=0 && id<partIndex.getNum())
                RenderIndices.push_back(id);
        }
        if(RenderIndices.size()) 
            renderShape(action,checkTransp);

    } else {
        RenderIndices.clear();
        renderShape(action,checkTransp);
    }
}

bool SoBrepFaceSet::overrideMaterialBinding(
        SoGLRenderAction *action, int selected, SelContextPtr ctx, SelContextPtr ctx2) 
{
    packedColors.clear();
    matIndex.clear();

    auto state = action->getState();
    if(!selected && !ctx && !ctx2) {
        if(!(SoShapeStyleElement::get(state)->getFlags() 
                & (SoShapeStyleElement::TRANSP_TEXTURE|SoShapeStyleElement::TRANSP_MATERIAL)))
        {
            return false;
        }
    }

    auto mb = SoMaterialBindingElement::get(state);

    auto element = SoLazyElement::getInstance(state);
    const SbColor *diffuse = element->getDiffusePointer();
    if(!diffuse) return false;
    int diffuse_size = element->getNumDiffuse();

    float defaultTrans = Gui::ViewParams::instance()->getSelectionTransparency();

    const float *trans = element->getTransparencyPointer();
    int trans_size = element->getNumTransparencies();
    if(!trans || !trans_size) return false;
    float trans0=0.0;
    bool hasTransparency = false;
    for(int i=0;i<trans_size;++i) {
        if(trans[i]!=0.0) {
            hasTransparency = true;
            trans0 = trans[i]<defaultTrans?defaultTrans:trans[i];
            break;
        }
    }

    // Override material binding to PER_PART_INDEXED so that we can reuse coin
    // rendering for both selection, preselection and partial rendering. The
    // main purpose is such that selection and preselection can have correct
    // transparency, too.
    //
    // Criteria of using material binding override:
    // 1) original material binding is either overall or per_part. We can
    //    support others, but omitted here to simplify coding logic, and
    //    because it seems FC only uses these two.
    // 2) either of the following :
    //      a) has highlight or selection and Selection().needPickedList, so that
    //         any preselected/selected part automatically become transparent
    //      b) has transparency
    //      c) has color override in secondary context

    if((mb==SoMaterialBindingElement::OVERALL || 
        (mb==SoMaterialBindingElement::PER_PART && diffuse_size>=partIndex.getNum())) 
        &&
       ((selected && Gui::Selection().needPickedList()) || 
        (trans0!=0.0 && ctx && (ctx->isSelected() || ctx->isHighlighted())) ||
        (ctx2 && ctx2->colors.size())))
    {
        state->push();

        if(selected && Gui::Selection().needPickedList()) {
            hasTransparency = true;
            if(trans0 < defaultTrans) 
                trans0 = defaultTrans;
            trans_size = 1;
            if(ctx2)
                ctx2->trans0 = trans0;
        }else if(ctx2)
            ctx2->trans0 = 0.0;

        uint32_t diffuseColor = diffuse[0].getPackedValue(trans0);
        uint32_t highlightColor;
        uint32_t selectionColor;
        if(ctx) {
            highlightColor = ctx->highlightColor.getPackedValue(trans0);
            selectionColor = ctx->selectionColor.getPackedValue(trans0);
        }

        int singleColor = 0;
        if(ctx && ctx->isHighlightAll()) {
            singleColor = 1;
            diffuseColor = highlightColor;
        }else if(ctx && ctx->isSelectAll()) {
            diffuseColor = selectionColor;
            singleColor = ctx->isHighlighted()?-1:1;
        } else if(ctx2 && ctx2->isSingleColor(diffuseColor,hasTransparency)) {
            singleColor = ctx?-1:1;
        }

        if(hasTransparency) {
            // Emissive color in opengl increase objects color intensity, but
            // only works in single color.
            SbColor c(0,0,0);
            SoLazyElement::setEmissive(state, &c);
        }

        bool partialRender = ctx2 && !ctx2->isSelectAll();

        if(singleColor>0 && !partialRender) {
            //optimization for single color non-partial rendering
            SoMaterialBindingElement::set(state,SoMaterialBindingElement::OVERALL);
            SoOverrideElement::setMaterialBindingOverride(state, this, true);
            packedColors.push_back(diffuseColor);
            SoLazyElement::setPacked(state, this,1, &packedColors[0], hasTransparency);
            SoTextureEnabledElement::set(state,this,false);

            if(hasTransparency) {
                // Override transparency type. We'll do our own sorting in sortParts()
                SoLazyElement::setTransparencyType(state, SoTransparencyType::SORTED_OBJECT_BLEND);
                SoShapeStyleElement::setTransparencyType(state, SoTransparencyType::SORTED_OBJECT_BLEND);
            }
            return true;
        }

        matIndex.reserve(partIndex.getNum());

        if(partialRender) {
            packedColors.push_back(SbColor(1.0,1.0,1.0).getPackedValue(1.0));
            matIndex.resize(partIndex.getNum(),0);

            if(mb == SoMaterialBindingElement::OVERALL || singleColor) {
                packedColors.push_back(diffuseColor);
                auto cidx = packedColors.size()-1;
                for(auto idx : ctx2->selectionIndex) {
                    if(idx>=0 && idx<partIndex.getNum()) {
                        if(!singleColor && ctx2->applyColor(idx,packedColors,hasTransparency)) {
                            matIndex[idx] = packedColors.size()-1;
                            continue;
                        }
                        if(singleColor>0) {
                            uint32_t c;
                            const SbColor &c2 = idx<diffuse_size?diffuse[idx]:diffuse[0];
                            if(makeDistinctColor(c,packedColors[cidx],c2.getPackedValue(trans0))) {
                                packedColors.push_back(c);
                                matIndex[idx] = packedColors.size()-1;
                                continue;
                            }
                        }
                        matIndex[idx] = cidx;
                    }
                }
            }else{
                assert(diffuse_size >= partIndex.getNum());
                for(auto idx : ctx2->selectionIndex) {
                    if(idx>=0 && idx<partIndex.getNum()) {
                        if(!ctx2->applyColor(idx,packedColors,hasTransparency)) {
                            auto t = idx<trans_size?trans[idx]:trans0;
                            packedColors.push_back(diffuse[idx].getPackedValue(t));
                        }
                        matIndex[idx] = packedColors.size()-1;
                    }
                }
            }
        }else if(mb==SoMaterialBindingElement::OVERALL || singleColor) {
            packedColors.push_back(diffuseColor);
            matIndex.resize(partIndex.getNum(),0);

            if(ctx2 && !singleColor) {
                for(auto &v : ctx2->colors) {
                    int idx = v.first;
                    if(idx>=0 && idx<partIndex.getNum()) {
                        packedColors.push_back(ctx2->packColor(v.second,hasTransparency));
                        matIndex[idx] = packedColors.size()-1;
                    }
                }
            }
        }else{
            assert(diffuse_size >= partIndex.getNum());
            packedColors.reserve(diffuse_size+3);
            for(int i=0;i<partIndex.getNum();++i) {
                auto t = i<trans_size?trans[i]:trans0;
                matIndex.push_back(i);
                if(!ctx2 || !ctx2->applyColor(i,packedColors,hasTransparency))
                    packedColors.push_back(diffuse[i].getPackedValue(t));
            }
        }

        if(ctx && ctx->selectionIndex.size()) {
            packedColors.push_back(selectionColor);
            auto cidx = packedColors.size()-1;
            for(auto idx : ctx->selectionIndex) {
                if(idx>=0 && idx<partIndex.getNum()) {
                    uint32_t c;
                    if(makeDistinctColor(c,packedColors[cidx],packedColors[matIndex[idx]]))
                        packedColors.push_back(c);
                    matIndex[idx] = packedColors.size()-1;
                }
            }
        }
        if(ctx && ctx->highlightIndex>=0 && ctx->highlightIndex<partIndex.getNum()) {
            packedColors.push_back(highlightColor);
            makeDistinctColor(packedColors.back(),
                    packedColors.back(), packedColors[matIndex[ctx->highlightIndex]]);
            matIndex[ctx->highlightIndex] = packedColors.size()-1;
        }

        SbBool notify = enableNotify(FALSE);
        materialIndex.setValuesPointer(matIndex.size(),&matIndex[0]);
        if(notify) enableNotify(notify);

        SoMaterialBindingElement::set(state, this, SoMaterialBindingElement::PER_PART_INDEXED);
        SoOverrideElement::setMaterialBindingOverride(state, this, true);
        SoLazyElement::setPacked(state, this, packedColors.size(), &packedColors[0], hasTransparency);
        SoTextureEnabledElement::set(state,this,false);

        if(hasTransparency) {
            // Override transparency type. We'll do our own sorting in sortParts()
            SoShapeStyleElement::setTransparencyType(state, SoTransparencyType::SORTED_OBJECT_BLEND);
            SoLazyElement::setTransparencyType(state, SoTransparencyType::SORTED_OBJECT_BLEND);
        }
        return true;
    }
    return false;
}

void SoBrepFaceSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepFaceSet::getBoundingBox(SoGetBoundingBoxAction * action) {

    if (this->coordIndex.getNum() < 3)
        return;

    auto state = action->getState();
    selCounter.checkCache(state,true);

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action,this);
    if(!ctx2 || ctx2->isSelectAll()) {
        inherited::getBoundingBox(action);
        return;
    }

    if(ctx2->selectionIndex.empty())
        return;

    buildPartBBoxes(state);

    int numparts = this->partIndex.getNum();
    for(auto id : ctx2->selectionIndex) {
        if (id<0 || id >= numparts)
            break;
        if(!partBBoxes[id].isEmpty())
            action->extendBy(partBBoxes[id]);
    }
}

void SoBrepFaceSet::buildPartBBoxes(SoState *state) {
    if(partIndex.getNum() == (int)partBBoxes.size())
        return;

    partBBoxes.clear();

    int numparts = partIndex.getNum();
    if(!numparts)
        return;

    partBBoxes.resize(numparts);

    const int32_t *pindices = this->partIndex.getValues(0);
    if(!pindices)
        return;

    const int32_t *cindices = coordIndex.getValues(0);
    int numindices = coordIndex.getNum();

    auto coords = static_cast<const SoGLCoordinateElement*>(SoCoordinateElement::getInstance(state));
    if(!coords)
        return;

    const SbVec3f *coords3d = coords->getArrayPtr3();
    int numverts = coords->getNum();

    buildPartIndexCache();

    for(int id=0;id<numparts;++id) {
        auto &bbox = partBBoxes[id];

        int length = (int)pindices[id]*4;
        int start = (int)indexOffset[id]*4;

        if(start+length > numindices)
            continue;

        auto viptr = &cindices[start];
        auto viendptr = viptr + length;
        while (viptr < viendptr) {
            int v = *viptr++;
            if(v >= 0 && v <numverts) 
                bbox.extendBy(coords3d[v]);
        }
    }
}


void SoBrepFaceSet::sortParts(SoState *state, SelContextPtr ctx2, const float *trans, int numtrans) {

    SortedParts.clear();

    if (coordIndex.getNum() < 3)
        return;

    // refresh part bboxes if necessary
    buildPartBBoxes(state);

    SortedParts.reserve(partBBoxes.size());
    if(ctx2 && ctx2->isSelected() && !ctx2->isSelectAll()) {
        for(auto id : ctx2->selectionIndex) {
            if(id<0 || id>=partIndex.getNum())
                continue;
            if(!isTranslucent(id,trans,numtrans,matIndex,packedColors))
                continue;
            SbVec3f center;
            SoModelMatrixElement::get(state).multVecMatrix(partBBoxes[id].getCenter(), center);
            float dist = -SoViewVolumeElement::get(state).getPlane(0.0f).getDistance(center);
            SortedParts.emplace_back(id,dist);
        }
    } else {
        int id=-1;
        for(auto &bbox : partBBoxes) {
            ++id;
            if(!isTranslucent(id,trans,numtrans,matIndex,packedColors))
                continue;
            SbVec3f center;
            SoModelMatrixElement::get(state).multVecMatrix(bbox.getCenter(), center);
            float dist = -SoViewVolumeElement::get(state).getPlane(0.0f).getDistance(center);
            SortedParts.emplace_back(id,dist);
        }
    }
    std::sort(SortedParts.begin(),SortedParts.end(),
        [](const PartDist &a, const PartDist &b) {
            return a.dist > b.dist;
        }
    );
}

  // this macro actually makes the code below more readable  :-)
#define DO_VERTEX(idx) \
  if (mbind == PER_VERTEX) {                  \
    pointDetail.setMaterialIndex(matnr);      \
    vertex.setMaterialIndex(matnr++);         \
  }                                           \
  else if (mbind == PER_VERTEX_INDEXED) {     \
    pointDetail.setMaterialIndex(*mindices); \
    vertex.setMaterialIndex(*mindices++); \
  }                                         \
  if (nbind == PER_VERTEX) {                \
    pointDetail.setNormalIndex(normnr);     \
    currnormal = &normals[normnr++];        \
    vertex.setNormal(*currnormal);          \
  }                                         \
  else if (nbind == PER_VERTEX_INDEXED) {   \
    pointDetail.setNormalIndex(*nindices);  \
    currnormal = &normals[*nindices++];     \
    vertex.setNormal(*currnormal);          \
  }                                        \
  if (tb.isFunction()) {                 \
    vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal)); \
    if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++); \
  }                                         \
  else if (tbind != NONE) {                      \
    pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx); \
    vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++)); \
  }                                         \
  vertex.setPoint(coords->get3(idx));        \
  pointDetail.setCoordinateIndex(idx);      \
  this->shapeVertex(&vertex);

void SoBrepFaceSet::generatePrimitives(SoAction * action)
{
    generatePrimitivesRange(action,0,0,0,coordIndex.getNum());
}

void SoBrepFaceSet::generatePrimitivesRange(SoAction * action, int pstart, int fstart, int vstart, int vend) {
    if (pstart<0 || pstart>=this->partIndex.getNum() 
            || vstart<0 || vend > this->coordIndex.getNum() 
            || vend - vstart < 3)
    {
        return;
    }

    SoState * state = action->getState();

    if (this->vertexProperty.getValue()) {
        state->push();
        this->vertexProperty.getValue()->doAction(action);
    }

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    SbBool doTextures;
    SbBool sendNormals;
    SbBool normalCacheUsed;

    sendNormals = true; // always generate normals

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    cindices += vstart;

    SoTextureCoordinateBundle tb(action, false, false);
    doTextures = tb.needCoordinates();

    if (!sendNormals) nbind = OVERALL;
    else if (normalCacheUsed && nbind == PER_VERTEX) {
        nbind = PER_VERTEX_INDEXED;
    }
    else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
        nbind = PER_FACE;
    }

    if (this->getNodeType() == SoNode::VRML1) {
        // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
        // on the state.
        if (mbind == PER_VERTEX) {
            mbind = PER_VERTEX_INDEXED;
            mindices = cindices;
        }
        if (nbind == PER_VERTEX) {
            nbind = PER_VERTEX_INDEXED;
            nindices = cindices;
        }
    }

    Binding tbind = NONE;
    if (doTextures) {
        if (tb.isFunction() && !tb.needIndices()) {
            tbind = NONE;
            tindices = NULL;
        }
        // FIXME: just call inherited::areTexCoordsIndexed() instead of
        // the if-check? 20020110 mortene.
        else if (SoTextureCoordinateBindingElement::get(state) ==
                 SoTextureCoordinateBindingElement::PER_VERTEX) {
            tbind = PER_VERTEX;
            tindices = NULL;
        }
        else {
            tbind = PER_VERTEX_INDEXED;
            if (tindices == NULL)
                tindices = cindices;
            else
                tindices += vstart;
        }
    }

    if (nbind == PER_VERTEX_INDEXED) {
        if(nindices == NULL)
            nindices = cindices;
        else
            nindices += vstart;
    } else if (nbind == PER_PART_INDEXED) {
        if(nindices)
            nindices += pstart;
        else
            nbind = NONE;
    } else if (nbind == PER_FACE) {
        // TODO: add support for that
    }
            
    if (mbind == PER_VERTEX_INDEXED) {
        if(mindices == NULL)
            mindices = cindices;
        else
            mindices += vstart;
    } else if (mbind == PER_PART_INDEXED) {
        if(mindices)
            mindices += pstart;
        else
            mbind = NONE;
    } else if (mbind == PER_FACE) {
        // TODO: add support for that
    }

    TriangleShape mode = POLYGON;
    TriangleShape newmode;
    const int32_t *viptr = cindices;
    const int32_t *viendptr = viptr + (vend-vstart);
    const int32_t *piptr = this->partIndex.getValues(0) + pstart;
    int num_partindices = this->partIndex.getNum() - pstart;
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, v5 = 0, pi; // v5 init unnecessary, but kills a compiler warning.

    SoPrimitiveVertex vertex;
    SoPointDetail pointDetail;
    SoFaceDetail faceDetail;
    faceDetail.setFaceIndex(fstart);

    vertex.setDetail(&pointDetail);

    SbVec3f dummynormal(0,0,1);
    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;
    vertex.setNormal(*currnormal);

    int texidx = vstart;
    int matnr = vstart;
    int normnr = vstart;
    int trinr = 0;
    pi = piptr < piendptr ? *piptr++ : -1;
    while (pi == 0) {
        // It may happen that a part has no triangles
        pi = piptr < piendptr ? *piptr++ : -1;
        if (mbind == PER_PART)
            matnr++;
        else if (mbind == PER_PART_INDEXED)
            mindices++;
    }

    while (viptr + 2 < viendptr) {
        v1 = *viptr++;
        v2 = *viptr++;
        v3 = *viptr++;
        if (v1 < 0 || v2 < 0 || v3 < 0) {
            break;
        }
        v4 = viptr < viendptr ? *viptr++ : -1;
        if (v4  < 0) newmode = TRIANGLES;
        else {
            v5 = viptr < viendptr ? *viptr++ : -1;
            if (v5 < 0) newmode = QUADS;
            else newmode = POLYGON;
        }
        if (newmode != mode) {
            if (mode != POLYGON) this->endShape();
            mode = newmode;
            this->beginShape(action, mode, &faceDetail);
        }
        else if (mode == POLYGON) this->beginShape(action, POLYGON, &faceDetail);

        // vertex 1 can't use DO_VERTEX
        if (mbind == PER_PART) {
            if (trinr == 0) {
                pointDetail.setMaterialIndex(matnr);
                vertex.setMaterialIndex(matnr++);
            }
        }
        else if (mbind == PER_PART_INDEXED) {
            if (trinr == 0) {
                pointDetail.setMaterialIndex(*mindices);
                vertex.setMaterialIndex(*mindices++);
            }
        }
        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
            pointDetail.setMaterialIndex(matnr);
            vertex.setMaterialIndex(matnr++);
        }
        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            pointDetail.setMaterialIndex(*mindices);
            vertex.setMaterialIndex(*mindices++);
        }
        if (nbind == PER_VERTEX || nbind == PER_FACE) {
            pointDetail.setNormalIndex(normnr);
            currnormal = &normals[normnr++];
            vertex.setNormal(*currnormal);
        }
        else if (nbind == PER_FACE_INDEXED || nbind == PER_VERTEX_INDEXED) {
            pointDetail.setNormalIndex(*nindices);
            currnormal = &normals[*nindices++];
            vertex.setNormal(*currnormal);
        }

        if (tb.isFunction()) {
            vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
            if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++);
        }
        else if (tbind != NONE) {
            pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx);
            vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++));
        }
        pointDetail.setCoordinateIndex(v1);
        vertex.setPoint(coords->get3(v1));
        this->shapeVertex(&vertex);

        DO_VERTEX(v2);
        DO_VERTEX(v3);

        if (mode != TRIANGLES) {
            DO_VERTEX(v4);
            if (mode == POLYGON) {
                DO_VERTEX(v5);
                v1 = viptr < viendptr ? *viptr++ : -1;
                while (v1 >= 0) {
                    DO_VERTEX(v1);
                    v1 = viptr < viendptr ? *viptr++ : -1;
                }
                this->endShape();
            }
        }
        faceDetail.incFaceIndex();
        if (mbind == PER_VERTEX_INDEXED) {
            mindices++;
        }
        if (nbind == PER_VERTEX_INDEXED) {
            nindices++;
        }
        if (tindices) tindices++;

        trinr++;
        if (pi == trinr) {
            pi = piptr < piendptr ? *piptr++ : -1;
            while (pi == 0) {
                // It may happen that a part has no triangles
                pi = piptr < piendptr ? *piptr++ : -1;
                if (mbind == PER_PART)
                    matnr++;
                else if (mbind == PER_PART_INDEXED)
                    mindices++;
            }
            trinr = 0;
        }
    }
    if (mode != POLYGON) this->endShape();

    if (normalCacheUsed) {
        this->readUnlockNormalCache();
    }

    if (this->vertexProperty.getValue()) {
        state->pop();
    }
}

#undef DO_VERTEX

void SoBrepFaceSet::renderHighlight(SoGLRenderAction *action, SelContextPtr ctx)
{
    if(!ctx || ctx->highlightIndex < 0)
        return;

    SoState * state = action->getState();
    state->push();

    SbColor color = ctx->highlightColor;
    if(!ctx->isHighlightAll()) {
        switch(SoMaterialBindingElement::get(state)) {
        case SoMaterialBindingElement::OVERALL:
            makeDistinctColor(color,color,SoLazyElement::getDiffuse(state,0));
            break;
        case SoMaterialBindingElement::PER_PART:
            if(SoLazyElement::getInstance(state)->getNumDiffuse() > ctx->highlightIndex)
                makeDistinctColor(color,color,SoLazyElement::getDiffuse(state,ctx->highlightIndex));
            break;
        default:
            break;
        }
    }

    SoLazyElement::setEmissive(state, &color);
    // if shading is disabled then set also the diffuse color
    if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
        packedColor = color.getPackedValue(0.0);
        SoLazyElement::setPacked(state, this,1, &packedColor,false);
    }
    SoMaterialBindingElement::set(state,SoMaterialBindingElement::OVERALL);
    SoOverrideElement::setMaterialBindingOverride(state,this,true);
    SoTextureEnabledElement::set(state,this,false);

    int id = ctx->highlightIndex;
    if(id == INT_MAX) {
        RenderIndices.clear();
        renderShape(action, true);
    } else if(id < partIndex.getNum()) {
        RenderIndices.resize(1,id);
        renderShape(action, true);
    }

    state->pop();
}

void SoBrepFaceSet::renderSelection(SoGLRenderAction *action, SelContextPtr ctx, bool push)
{
    if(!ctx || !ctx->isSelected())
        return;

    RenderIndices.clear();
    if(!ctx->isSelectAll()) {
        for(auto id : ctx->selectionIndex) {
            if (id<0 || id>=partIndex.getNum())
                continue;
            RenderIndices.push_back(id);
        }
        if(RenderIndices.empty())
            return;
    }

    bool resetMatIndices = false;
    SoState * state = action->getState();

    if(push) {
        state->push();

        SbColor color = ctx->selectionColor;

        auto mb = SoMaterialBindingElement::get(state);
        if(ctx->isSelectAll())
            mb = SoMaterialBindingElement::OVERALL;
        else {
            switch(mb) {
            case SoMaterialBindingElement::OVERALL:
                makeDistinctColor(color,color,SoLazyElement::getDiffuse(state,0));
                break;
            case SoMaterialBindingElement::PER_PART:
                if(partIndex.getNum()<SoLazyElement::getInstance(state)->getNumDiffuse())
                    break;
                packedColors.resize(1,color.getPackedValue(0.0));
                matIndex.resize(partIndex.getNum(),0);
                for(int id : RenderIndices) {
                    SbColor c;
                    if(makeDistinctColor(c,color,SoLazyElement::getDiffuse(state,id))) {
                        packedColors.push_back(c.getPackedValue(0.0));
                        matIndex[id] = packedColors.size()-1;
                    }
                }
                if(packedColors.size() == 1) {
                    packedColors.clear();
                    matIndex.clear();
                    mb = SoMaterialBindingElement::OVERALL;
                } else {
                    SbBool notify = enableNotify(FALSE);
                    materialIndex.setValuesPointer(matIndex.size(),&matIndex[0]);
                    if(notify) enableNotify(notify);
                    resetMatIndices = true;
                    mb = SoMaterialBindingElement::PER_PART_INDEXED;
                    SoLazyElement::setPacked(state,this,packedColors.size(),&packedColors[0],false);
                }
                break;
            default:
                mb = SoMaterialBindingElement::OVERALL;
                break;
            }
        }

        if(mb == SoMaterialBindingElement::OVERALL) {
            SoLazyElement::setEmissive(state, &color);
            // if shading is disabled then set also the diffuse color
            if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
                packedColor = color.getPackedValue(0.0);
                SoLazyElement::setPacked(state, this,1, &packedColor,false);
            }
        }
        SoTextureEnabledElement::set(state,this,false);
        SoMaterialBindingElement::set(state,mb);
        SoOverrideElement::setMaterialBindingOverride(state,this,true);
    }

    renderShape(action,true);

    if(push) {
        if(resetMatIndices) {
            SbBool notify = enableNotify(FALSE);
            materialIndex.setNum(0);
            if(notify) enableNotify(notify);
        }
        state->pop();
    }
}

void SoBrepFaceSet::VBO::render(SoGLRenderAction * action,
                                bool color_override,
                                const std::vector<int32_t> &render_indices,
                                const SoCoordinateElement *coords,
                                const int32_t *vertexindices,
                                int num_indices,
                                const int32_t *partindices,
                                const int32_t *indexoffsets,
                                int num_partindices,
                                const SbVec3f *normals,
                                const int32_t *normalindices,
                                SoMaterialBundle *const materials,
                                const int32_t *matindices,
                                SoTextureCoordinateBundle * const texcoords,
                                const int32_t *texindices,
                                const int nbind,
                                const int mbind,
                                const int texture)
{
    auto vertexlist = static_cast<const SoGLCoordinateElement*>(coords);

    (void)texcoords; (void)texindices; (void)texture;
    const SbVec3f * coords3d = NULL;
    SbVec3f * cur_coords3d = NULL;
    coords3d = vertexlist->getArrayPtr3();
    cur_coords3d = ( SbVec3f *)coords3d;

    const int32_t *mindices = matindices;
    const int32_t *viptr = vertexindices;
    const int32_t *viendptr = viptr + num_indices;
    const int32_t *piptr = partindices;
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, pi;
    SbVec3f dummynormal(0,0,1);
    int numverts = vertexlist->getNum();

    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;

    int matnr = 0;
    int trinr = 0;

    float * vertex_array = NULL;
    GLuint * index_array = NULL;
    SbColor  mycolor1,mycolor2,mycolor3;
    float t1, t2, t3;
    SbVec3f *mynormal1 = (SbVec3f *)currnormal;
    SbVec3f *mynormal2 = (SbVec3f *)currnormal;
    SbVec3f *mynormal3 = (SbVec3f *)currnormal;
    int indice=0;
    uint32_t RGBA,R,G,B,A;
    float Rf,Gf,Bf,Af;

#ifdef FC_OS_WIN32
    const cc_glglue * glue = cc_glglue_instance(action->getCacheContext());
#endif

    uint32_t contextId = action->getCacheContext();
    auto res = this->vbomap.insert(std::make_pair(contextId,VBO::Buffer()));
    VBO::Buffer &buf = res.first->second;
    if (res.second) {
#ifdef FC_OS_WIN32
        PFNGLGENBUFFERSPROC glGenBuffersARB = (PFNGLGENBUFFERSPROC)cc_glglue_getprocaddress(glue, "glGenBuffersARB");
#endif
        glGenBuffersARB(2, buf.myvbo);
        buf.vertex_array_size = 0;
        buf.index_array_size = 0;
        buf.vboLoaded = false;
    }

    if ((buf.vertex_array_size != (sizeof(float) * num_indices * 10)) ||
        (buf.index_array_size != (sizeof(GLuint) * num_indices))) {
        if ((buf.vertex_array_size != 0 ) && ( buf.index_array_size != 0))
            buf.updateVbo = true;
    }

    // vbo loaded is defining if we must pre-load data into the VBO. When the variable is set to 0
    // it means that the VBO has not been initialized
    // updateVbo is tracking the need to update the content of the VBO which act as a buffer within
    // the graphic card
    // TODO FINISHING THE COLOR SUPPORT !

#ifdef FC_OS_WIN32
    PFNGLBINDBUFFERARBPROC glBindBufferARB = (PFNGLBINDBUFFERARBPROC) cc_glglue_getprocaddress(glue, "glBindBufferARB");
#endif

    if (!buf.vboLoaded || buf.updateVbo) {
#ifdef FC_OS_WIN32
        PFNGLMAPBUFFERARBPROC glMapBufferARB = (PFNGLMAPBUFFERARBPROC) cc_glglue_getprocaddress(glue, "glMapBufferARB");
        PFNGLGENBUFFERSPROC glGenBuffersARB = (PFNGLGENBUFFERSPROC)cc_glglue_getprocaddress(glue, "glGenBuffersARB");
        PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)cc_glglue_getprocaddress(glue, "glDeleteBuffersARB");
        PFNGLBUFFERDATAARBPROC glBufferDataARB = (PFNGLBUFFERDATAARBPROC)cc_glglue_getprocaddress(glue, "glBufferDataARB");
#endif
        // We must manage buffer size increase let's clear everything and re-init to test the
        // clearing process
        glDeleteBuffersARB(2, buf.myvbo);
        glGenBuffersARB(2, buf.myvbo);
        vertex_array = ( float * ) malloc ( sizeof(float) * num_indices * 10 );
        index_array = ( GLuint *) malloc ( sizeof(GLuint) * num_indices );
        buf.vertex_array_size = sizeof(float) * num_indices * 10;
        buf.index_array_size = sizeof(GLuint) * num_indices;
        this->vbomap[contextId] = buf;
        this->indice_array = 0;

        // Get the initial colors
        SoState * state = action->getState();

        if(mbind == PER_PART_INDEXED || mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            mycolor1 = mycolor2 = mycolor3 = SoLazyElement::getDiffuse(state,matindices[0]);
            t1 = t2 = t3 = SoLazyElement::getTransparency(state,matindices[0]);
        } else {
            mycolor1 = mycolor2 = mycolor3 = SoLazyElement::getDiffuse(state,0);
            t1 = t2 = t3 = SoLazyElement::getTransparency(state,0);
        }

        pi = piptr < piendptr ? *piptr++ : -1;
        while (pi == 0) {
           // It may happen that a part has no triangles
           pi = piptr < piendptr ? *piptr++ : -1;
           if (mbind == PER_PART)
               matnr++;
           else if (mbind == PER_PART_INDEXED)
               matindices++;
        }

        while (viptr + 2 < viendptr) {
            v1 = *viptr++;
            v2 = *viptr++;
            v3 = *viptr++;
            // This test is for robustness upon buggy data sets
            if (v1 < 0 || v2 < 0 || v3 < 0 ||
                    v1 >= numverts || v2 >= numverts || v3 >= numverts) {
                break;
            }
            v4 = viptr < viendptr ? *viptr++ : -1;
            (void)v4;

            if (mbind == PER_PART) {
                if (trinr == 0) {
                    mycolor1=SoLazyElement::getDiffuse(state,matnr);
                    t1 = t2 =t3 = SoLazyElement::getTransparency(state,matnr);
                    matnr++;
                    mycolor2=mycolor1;
                    mycolor3=mycolor1;
                }
            }
            else if (mbind == PER_PART_INDEXED) {
                if (trinr == 0) {
                    mycolor1=SoLazyElement::getDiffuse(state,*matindices);
                    t1 = t2 = t3 = SoLazyElement::getTransparency(state,*matindices);
                    matindices++;
                    mycolor2=mycolor1;
                    mycolor3=mycolor1;
                }
            }
            else if (mbind == PER_VERTEX || mbind == PER_FACE) {
                mycolor1 = SoLazyElement::getDiffuse(state,matnr);
                t1 = SoLazyElement::getTransparency(state,matnr);
                matnr++;
                if(mbind == PER_FACE) {
                    mycolor2 = mycolor3 = mycolor1;
                    t2 = t3 = t1;
                }
            }
            else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                mycolor1 = SoLazyElement::getDiffuse(state,*matindices);
                t1 = SoLazyElement::getTransparency(state,*matindices);
                matindices++;
                if(mbind == PER_FACE_INDEXED) {
                    mycolor2 = mycolor3 = mycolor1;
                    t2 = t3 = t1;
                }
            }

            if (normals) {
                if (nbind == PER_VERTEX || nbind == PER_FACE) {
                    currnormal = normals++;
                    mynormal1=(SbVec3f *)currnormal;
                }
                else if (nbind == PER_VERTEX_INDEXED || nbind == PER_FACE_INDEXED) {
                    currnormal = &normals[*normalindices++];
                    mynormal1 =(SbVec3f *) currnormal;
                }
            }
            if (mbind == PER_VERTEX) {
                mycolor2 = SoLazyElement::getDiffuse(state,matnr);
                t2 = SoLazyElement::getTransparency(state,matnr);
                ++matnr;

            } else if (mbind == PER_VERTEX_INDEXED) {
                mycolor2 = SoLazyElement::getDiffuse(state,*matindices);
                t2 = SoLazyElement::getTransparency(state,*matindices);
                ++matindices;
            }

            if (normals) {
                if (nbind == PER_VERTEX) {
                    currnormal = normals++;
                    mynormal2 = (SbVec3f *)currnormal;
                }
                else if (nbind == PER_VERTEX_INDEXED) {
                     currnormal = &normals[*normalindices++];
                    mynormal2 = (SbVec3f *)currnormal;
                 }
             }

            if (mbind == PER_VERTEX) {
                mycolor3 = SoLazyElement::getDiffuse(state,matnr);
                t3 = SoLazyElement::getTransparency(state,matnr);
                ++matnr;
            } else if (mbind == PER_VERTEX_INDEXED) {
                mycolor3 = SoLazyElement::getDiffuse(state,*matindices);
                t3 = SoLazyElement::getTransparency(state,*matindices);
                ++matindices;
            }

            if (normals) {
                if (nbind == PER_VERTEX) {
                    currnormal = normals++;
                    mynormal3 =(SbVec3f *)currnormal;
                }
                else if (nbind == PER_VERTEX_INDEXED) {
                    currnormal = &normals[*normalindices++];
                    mynormal3 = (SbVec3f *)currnormal;
                }
            }
            if (nbind == PER_VERTEX_INDEXED)
                normalindices++;

            /* We building the Vertex dataset there and push it to a VBO */
            /* The Vertex array shall contain per element vertex_coordinates[3],
            normal_coordinates[3], color_value[3] (RGBA format) */

            index_array[this->indice_array] =   this->indice_array;
            index_array[this->indice_array+1] = this->indice_array + 1;
            index_array[this->indice_array+2] = this->indice_array + 2;
            this->indice_array += 3;


            ((SbVec3f *)(cur_coords3d+v1 ))->getValue(vertex_array[indice+0],
                                                      vertex_array[indice+1],
                                                      vertex_array[indice+2]);

            ((SbVec3f *)(mynormal1))->getValue(vertex_array[indice+3],
                                               vertex_array[indice+4],
                                               vertex_array[indice+5]);

            /* We decode the Vertex1 color */
            RGBA = mycolor1.getPackedValue(t1);
            R = ( RGBA & 0xFF000000 ) >> 24 ;
            G = ( RGBA & 0xFF0000 ) >> 16;
            B = ( RGBA & 0xFF00 ) >> 8;
            A = ( RGBA & 0xFF );

            Rf = (((float )R) / 255.0);
            Gf = (((float )G) / 255.0);
            Bf = (((float )B) / 255.0);
            Af = (((float )A) / 255.0);

            vertex_array[indice+6] = Rf;
            vertex_array[indice+7] = Gf;
            vertex_array[indice+8] = Bf;
            vertex_array[indice+9] = Af;
            indice+=10;

            ((SbVec3f *)(cur_coords3d+v2))->getValue(vertex_array[indice+0],
                                                     vertex_array[indice+1],
                                                     vertex_array[indice+2]);
            ((SbVec3f *)(mynormal2))->getValue(vertex_array[indice+3],
                                               vertex_array[indice+4],
                                               vertex_array[indice+5]);

            RGBA = mycolor2.getPackedValue(t2);
            R = ( RGBA & 0xFF000000 ) >> 24 ;
            G = ( RGBA & 0xFF0000 ) >> 16;
            B = ( RGBA & 0xFF00 ) >> 8;
            A = ( RGBA & 0xFF );

            Rf = (((float )R) / 255.0);
            Gf = (((float )G) / 255.0);
            Bf = (((float )B) / 255.0);
            Af = (((float )A) / 255.0);

            vertex_array[indice+6] = Rf;
            vertex_array[indice+7] = Gf;
            vertex_array[indice+8] = Bf;
            vertex_array[indice+9] = Af;
            indice+=10;

            ((SbVec3f *)(cur_coords3d+v3))->getValue(vertex_array[indice+0],
                                                     vertex_array[indice+1],
                                                     vertex_array[indice+2]);
            ((SbVec3f *)(mynormal3))->getValue(vertex_array[indice+3],
                                               vertex_array[indice+4],
                                               vertex_array[indice+5]);

            RGBA = mycolor3.getPackedValue(t3);
            R = ( RGBA & 0xFF000000 ) >> 24 ;
            G = ( RGBA & 0xFF0000 ) >> 16;
            B = ( RGBA & 0xFF00 ) >> 8;
            A = ( RGBA & 0xFF );

            Rf = (((float )R) / 255.0);
            Gf = (((float )G) / 255.0);
            Bf = (((float )B) / 255.0);
            Af = (((float )A) / 255.0);

            vertex_array[indice+6] = Rf;
            vertex_array[indice+7] = Gf;
            vertex_array[indice+8] = Bf;
            vertex_array[indice+9] = Af;
            indice+=10;

            /* ============================================================ */
            trinr++;
            if (pi == trinr) {
                pi = piptr < piendptr ? *piptr++ : -1;
                while (pi == 0) {
                    // It may happen that a part has no triangles
                    pi = piptr < piendptr ? *piptr++ : -1;
                    if (mbind == PER_PART)
                        matnr++;
                    else if (mbind == PER_PART_INDEXED)
                    matindices++;
                }
                trinr = 0;
            }
        }

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, buf.myvbo[0]);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * indice , vertex_array, GL_DYNAMIC_DRAW_ARB);

        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, buf.myvbo[1]);
        glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(GLuint) * this->indice_array , &index_array[0], GL_DYNAMIC_DRAW_ARB);

        // glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        // glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

        buf.updateVbo = true; // set to true here avoid calling glBindBuffer below
        buf.vboLoaded = true;
        free(vertex_array);
        free(index_array);
    }

    // This is the VBO rendering code

    if (!buf.updateVbo) {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, buf.myvbo[0]);
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, buf.myvbo[1]);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3,GL_FLOAT,10*sizeof(GLfloat),0);
    glNormalPointer(GL_FLOAT,10*sizeof(GLfloat),(GLvoid *)(3*sizeof(GLfloat)));

    if(!color_override && render_indices.empty()) {
        // no color override, no out of order rendering, just render with vbo buffer

        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4,GL_FLOAT,10*sizeof(GLfloat),(GLvoid *)(6*sizeof(GLfloat)));
        glDrawElements(GL_TRIANGLES, this->indice_array, GL_UNSIGNED_INT, (void *)0);

    } else if(!color_override) {
        // no color override, but out of order rendering

        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4,GL_FLOAT,10*sizeof(GLfloat),(GLvoid *)(6*sizeof(GLfloat)));

        for(int id : render_indices) {
            uint32_t count = (uint32_t)partindices[id]*3;
            intptr_t offset = (intptr_t)indexoffsets[id]*3*4;
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)offset);
        }

    } else if(render_indices.empty()) {

        // color override only
        for(int id=0;id<num_partindices;++id) {
            if (mbind == PER_PART)
                materials->send(id, true);
            else if (mbind == PER_PART_INDEXED)
                materials->send(mindices[id], true);
            uint32_t count = (uint32_t)partindices[id]*3;
            intptr_t offset = (intptr_t)indexoffsets[id]*3*4;
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)offset);
        }

    } else {

        // color override and out of order rendering
        for(int id : render_indices) {
            if (mbind == PER_PART)
                materials->send(id, true);
            else if (mbind == PER_PART_INDEXED)
                materials->send(mindices[id], true);
            uint32_t count = (uint32_t)partindices[id]*3;
            intptr_t offset = (intptr_t)indexoffsets[id]*3*4;
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void *)offset);
        }
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    buf.updateVbo = false;
}

void SoBrepFaceSet::renderShape(SoGLRenderAction * action, bool check_override) {

    SoMaterialBundle mb(action);
    mb.sendFirst(); 

    auto state = action->getState();
    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    const int32_t * pindices;
    int numparts;
    SbBool normalCacheUsed;

    SoTextureCoordinateBundle tb(action, true, false);
    int doTextures = tb.needCoordinates()?1:0;
    SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    // just in case someone forgot
    if (!mindices) mindices = cindices;
    if (!nindices) nindices = cindices;
    pindices = this->partIndex.getValues(0);
    numparts = this->partIndex.getNum();

    buildPartIndexCache();

    // Can we use vertex buffer objects?
    if(PRIVATE(this)->isVboAvailable(action)) {
        SoState* state = action->getState();
        if (SoLazyElement::getLightModel(state) == SoLazyElement::BASE_COLOR) {
            // if no shading is set then the normals are all equal
            nbind = OVERALL;
        }

        check_override = check_override && (SoOverrideElement::getFlags(state)
                                            & (SoOverrideElement::COLOR_INDEX|
                                                SoOverrideElement::DIFFUSE_COLOR|
                                                SoOverrideElement::MATERIAL_BINDING|
                                                SoOverrideElement::TRANSPARENCY));

        PRIVATE(this)->render(action, check_override, RenderIndices, coords, cindices, numindices,
                              pindices, &indexOffset[0], numparts, normals, nindices,
                              &mb, mindices, &tb, tindices, nbind, mbind, doTextures);
        if (normalCacheUsed)
            this->readUnlockNormalCache();
        RenderIndices.clear();
        return;
    }

    if(RenderIndices.empty())
        renderFaces(coords, cindices, numindices, pindices, 0, numparts,
                normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures);
    else {
        int start = 0;
        int next = 0;
        for(int id : RenderIndices) {
            // try to render together consequtive indices
            if(next == id) {
                ++next;
                continue;
            }
            if(next!=start) {
                renderFaces(coords, cindices, numindices, pindices, start, next-start,
                        normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures);
            }
            start = id;
            next = id+1;
        }
        if(next!=start) {
            renderFaces(coords, cindices, numindices, pindices, start, next-start,
                    normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures);
        }
        RenderIndices.clear();
    }

    if (normalCacheUsed)
        this->readUnlockNormalCache();
}

void SoBrepFaceSet::renderFaces(const SoCoordinateElement *coords,
                                const int32_t *vertexindices,
                                int num_indices,
                                const int32_t *partindices,
                                int start_partindex,
                                int num_partindices,
                                const SbVec3f *normals,
                                const int32_t *normalindices,
                                SoMaterialBundle *const materials,
                                const int32_t *matindices,
                                SoTextureCoordinateBundle * const texcoords,
                                const int32_t *texindices,
                                int nbind,
                                int mbind,
                                int texture)
{
    auto vertexlist = static_cast<const SoGLCoordinateElement*>(coords);

    int matnr = 0;
    int texidx = 0;

    assert(partIndex.getNum()+1 == (int)indexOffset.size());

    int start = (int)indexOffset[start_partindex]*4;
    int length = (int)(indexOffset[start_partindex+num_partindices]
                        - indexOffset[start_partindex])*4;
    // normals
    if (nbind == PER_VERTEX_INDEXED)
        normalindices += start;
    else if(nbind == PER_VERTEX)
        normals += start;
    else
        nbind = OVERALL;

    if(mbind == PER_PART_INDEXED) {
        matindices += start_partindex;
        if(texindices)
            texindices += start_partindex;
    } else if (mbind == PER_VERTEX_INDEXED) {
        matindices += start;
        if(texindices)
            texindices += start;
    else if (mbind == PER_FACE_INDEXED)
        matindices += start/4;
        if(texindices)
            texindices += start/4;
    }

    vertexindices += start;
    num_indices = length;
    partindices += start_partindex;
    texidx = matnr = start_partindex;

    const SbVec3f * coords3d = NULL;
    coords3d = vertexlist->getArrayPtr3();

    const int32_t *viptr = vertexindices;
    const int32_t *viendptr = viptr + num_indices;
    const int32_t *piptr = partindices;
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, pi;
    SbVec3f dummynormal(0,0,1);
    int numverts = vertexlist->getNum();

    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;

    int trinr = 0;

    // Legacy code without VBO support
    pi = piptr < piendptr ? *piptr++ : -1;
    while (pi == 0) {
        // It may happen that a part has no triangles
        pi = piptr < piendptr ? *piptr++ : -1;
        if (mbind == PER_PART)
            matnr++;
        else if (mbind == PER_PART_INDEXED)
            matindices++;
    }

    glBegin(GL_TRIANGLES);
    while (viptr + 2 < viendptr) {
        v1 = *viptr++;
        v2 = *viptr++;
        v3 = *viptr++;
        if (v1 < 0 || v2 < 0 || v3 < 0 ||
            v1 >= numverts || v2 >= numverts || v3 >= numverts) {
            break;
        }
        v4 = viptr < viendptr ? *viptr++ : -1;
        (void)v4;
        /* vertex 1 *********************************************************/
        if (mbind == PER_PART) {
            if (trinr == 0)
                materials->send(matnr++, true);
        }
        else if (mbind == PER_PART_INDEXED) {
            if (trinr == 0)
                materials->send(*matindices++, true);
        }
        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
            materials->send(matnr++, true);
        }
        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            materials->send(*matindices++, true);
        }

        if (normals) {
            if (nbind == PER_VERTEX || nbind == PER_FACE) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED || nbind == PER_FACE_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v1),
                        *currnormal);
        }
        glVertex3fv((const GLfloat*) (coords3d + v1));

        /* vertex 2 *********************************************************/
        if (mbind == PER_VERTEX)
            materials->send(matnr++, true);
        else if (mbind == PER_VERTEX_INDEXED)
            materials->send(*matindices++, true);

        if (normals) {
            if (nbind == PER_VERTEX) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                            vertexlist->get3(v2),
                            *currnormal);
        }

        glVertex3fv((const GLfloat*) (coords3d + v2));

        /* vertex 3 *********************************************************/
        if (mbind == PER_VERTEX)
            materials->send(matnr++, true);
        else if (mbind == PER_VERTEX_INDEXED)
            materials->send(*matindices++, true);

        if (normals) {
            if (nbind == PER_VERTEX) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                            vertexlist->get3(v3),
                            *currnormal);
        }

        glVertex3fv((const GLfloat*) (coords3d + v3));

        if (mbind == PER_VERTEX_INDEXED)
            matindices++;

        if (nbind == PER_VERTEX_INDEXED)
            normalindices++;

        if (texture && texindices) {
            texindices++;
        }

        trinr++;
        if (pi == trinr) {
            pi = piptr < piendptr ? *piptr++ : -1;
            while (pi == 0) {
                // It may happen that a part has no triangles
                pi = piptr < piendptr ? *piptr++ : -1;
                if (mbind == PER_PART)
                    matnr++;
                else if (mbind == PER_PART_INDEXED)
                    matindices++;
            }
            trinr = 0;
        }
    }
    glEnd();
}

int SoBrepFaceSet::getPartFromFace(int index) {
    const int32_t * indices = this->partIndex.getValues(0);
    int num = this->partIndex.getNum();
    if(!indices)
        return -1;

    buildPartIndexCache();
    if(num == (int)partIndexMap.size()) {
        auto it = partIndexMap.upper_bound(index);
        if(it==partIndexMap.end())
            return num-1;
        else
            return it->second-1;
    } else {
        int count = 0;
        for (int i=0; i<num; i++) {
            count += indices[i];
            if (index < count)
                return i;
        }
        return num-1;
    }
}

SoDetail * SoBrepFaceSet::createTriangleDetail(SoRayPickAction * action,
                                               const SoPrimitiveVertex * v1,
                                               const SoPrimitiveVertex * v2,
                                               const SoPrimitiveVertex * v3,
                                               SoPickedPoint * pp)
{
    SoDetail* detail = inherited::createTriangleDetail(action, v1, v2, v3, pp);
    SoFaceDetail* face_detail = static_cast<SoFaceDetail*>(detail);
    face_detail->setPartIndex(getPartFromFace(face_detail->getFaceIndex()));
    return detail;
}

SoBrepFaceSet::Binding
SoBrepFaceSet::findMaterialBinding(SoState * const state) const
{
    Binding binding = OVERALL;
    SoMaterialBindingElement::Binding matbind =
        SoMaterialBindingElement::get(state);

    switch (matbind) {
    case SoMaterialBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoMaterialBindingElement::PER_VERTEX:
        binding = PER_VERTEX;
        break;
    case SoMaterialBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART:
        binding = PER_PART;
        break;
    case SoMaterialBindingElement::PER_FACE:
        binding = PER_FACE;
        break;
    case SoMaterialBindingElement::PER_PART_INDEXED:
        binding = PER_PART_INDEXED;
        break;
    case SoMaterialBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

SoBrepFaceSet::Binding
SoBrepFaceSet::findNormalBinding(SoState * const state) const
{
    Binding binding = PER_VERTEX_INDEXED;
    SoNormalBindingElement::Binding normbind =
        (SoNormalBindingElement::Binding) SoNormalBindingElement::get(state);

    switch (normbind) {
    case SoNormalBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoNormalBindingElement::PER_VERTEX:
        binding = PER_VERTEX;
        break;
    case SoNormalBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoNormalBindingElement::PER_PART:
        binding = PER_PART;
        break;
    case SoNormalBindingElement::PER_FACE:
        binding = PER_FACE;
        break;
    case SoNormalBindingElement::PER_PART_INDEXED:
        binding = PER_PART_INDEXED;
        break;
    case SoNormalBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

void SoBrepFaceSet::rayPick(SoRayPickAction *action) {

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action,this);
    if(ctx2 && !ctx2->isSelected())
        return;

    if (!shouldRayPick(action))
        return;

    computeObjectSpaceRay(action);

    SoState *state = action->getState();

    if (getBoundingBoxCache() && getBoundingBoxCache()->isValid(state)) {
        SbBox3f box = getBoundingBoxCache()->getProjectedBox();
        if(box.isEmpty() || !action->intersect(box,TRUE))
            return;
    }

    buildPartIndexCache();

    FC_TIME_INIT(t);

    auto pick = [&](int id) {
        this->generatePrimitivesRange(action,id,
                this->indexOffset[id], this->indexOffset[id]*4, this->indexOffset[id+1]*4);
    };

    int threshold = Gui::ViewParams::instance()->getSelectionPickThreshold();
    int numparts = partIndex.getNum();

    if(threshold && numparts && indexOffset[numparts-1]/numparts > threshold) {
        // If face per part exceeds the threshold, then force computes bbox per
        // part. The computed bboxes will be cached until partIndex changes
        buildPartBBoxes(state);
    }

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    if(!threshold || numparts!=(int)partBBoxes.size() 
            || mbind==PER_FACE || mbind==PER_FACE_INDEXED 
            || nbind==PER_FACE || nbind==PER_FACE_INDEXED ) 
    {
        generatePrimitives(action);
        FC_TIME_TRACE(t,"pick");
        return;
    }

    if(ctx2 && !ctx2->isSelectAll()) {
        for(auto id : ctx2->selectionIndex) {
            if(id<0 || id>=numparts)
                continue;
            auto &box = partBBoxes[id];
            if(box.isEmpty() || !action->intersect(box,TRUE))
                continue;
            pick(id);
        }
    } else {
        for(int id=0;id<numparts;++id) {
            auto &box = partBBoxes[id];
            if(box.isEmpty() || !action->intersect(box,TRUE))
                continue;
            pick(id);
        }
        FC_TIME_TRACE(t,"pick new");
    }
}

#undef PRIVATE
