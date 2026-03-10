// SPDX-License-Identifier: LGPL-2.1-or-later
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

#pragma once

#include <Inventor/SbColor.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFString.h>
#include <vector>
#include <FCGlobal.h>

class SoSFString;
class SoSFColor;

namespace Gui
{

class SelectionChanges;

/**
 * The SoFCPreselectionAction class is used to inform an SoFCSelection node
 * whether an object gets preselected.
 * @author Jürgen Riegel
 */
class GuiExport SoFCPreselectionAction: public SoAction
{
    SO_ACTION_HEADER(SoFCPreselectionAction);

public:
    SoFCPreselectionAction(const SelectionChanges& SelCh);
    ~SoFCPreselectionAction() override;

    static void initClass();
    static void finish();

    const SelectionChanges& SelChange;

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCSelectionAction class is used to inform an SoFCSelection node
 * whether an object gets selected.
 * @author Jürgen Riegel
 */
class GuiExport SoFCSelectionAction: public SoAction
{
    SO_ACTION_HEADER(SoFCSelectionAction);

public:
    SoFCSelectionAction(const SelectionChanges& SelCh);
    ~SoFCSelectionAction() override;

    static void initClass();
    static void finish();

    const SelectionChanges& SelChange;

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCEnableSelectionAction class is used to inform an SoFCSelection node
 * whether selection is enabled or disabled.
 * @author Werner Mayer
 */
class GuiExport SoFCEnableSelectionAction: public SoAction
{
    SO_ACTION_HEADER(SoFCEnableSelectionAction);

public:
    SoFCEnableSelectionAction(const SbBool& sel);
    ~SoFCEnableSelectionAction() override;

    SbBool enabled;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCEnablePreselectionAction class is used to inform an SoFCSelection node
 * whether preselection is enabled or disabled.
 * @author Werner Mayer
 */
class GuiExport SoFCEnablePreselectionAction: public SoAction
{
    SO_ACTION_HEADER(SoFCEnablePreselectionAction);

public:
    SoFCEnablePreselectionAction(const SbBool& sel);
    ~SoFCEnablePreselectionAction() override;

    SbBool enabled;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCSelectionColorAction class is used to inform an SoFCSelection node
 * which selection color is used.
 * @author Werner Mayer
 */
class GuiExport SoFCSelectionColorAction: public SoAction
{
    SO_ACTION_HEADER(SoFCSelectionColorAction);

public:
    SoFCSelectionColorAction(const SoSFColor& col);
    ~SoFCSelectionColorAction() override;

    SoSFColor selectionColor;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCHighlightColorAction class is used to inform an SoFCSelection node
 * which highlight color is used.
 * @author Werner Mayer
 */
class GuiExport SoFCHighlightColorAction: public SoAction
{
    SO_ACTION_HEADER(SoFCHighlightColorAction);

public:
    SoFCHighlightColorAction(const SoSFColor& col);
    ~SoFCHighlightColorAction() override;

    SoSFColor highlightColor;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCDocumentAction class is used to inform an SoFCSelection node
 * when a document has been renamed.
 * @author Werner Mayer
 */
class GuiExport SoFCDocumentAction: public SoAction
{
    SO_ACTION_HEADER(SoFCDocumentAction);

public:
    SoFCDocumentAction(const SoSFString& docName);
    ~SoFCDocumentAction() override;

    SoSFString documentName;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

/**
 * The SoFCDocumentObjectAction class is used to get the name of the document,
 * object and component at a certain position of an SoFCSelection node.
 * @author Werner Mayer
 */
class GuiExport SoFCDocumentObjectAction: public SoAction
{
    SO_ACTION_HEADER(SoFCDocumentObjectAction);

public:
    SoFCDocumentObjectAction();
    ~SoFCDocumentObjectAction() override;

    void setHandled();
    SbBool isHandled() const;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);

public:
    SbString documentName;
    SbString objectName;
    SbString componentName;

private:
    SbBool _handled {false};
};

/**
 * The SoGLSelectAction class is used to get all data under a selected area.
 * @author Werner Mayer
 */
class GuiExport SoGLSelectAction: public SoAction
{
    SO_ACTION_HEADER(SoGLSelectAction);

public:
    SoGLSelectAction(const SbViewportRegion& region, const SbViewportRegion& select);
    ~SoGLSelectAction() override;

    void setHandled();
    SbBool isHandled() const;
    const SbViewportRegion& getViewportRegion() const;

    static void initClass();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);

public:
    std::vector<unsigned long> indices;

private:
    const SbViewportRegion& vpregion;
    const SbViewportRegion& vpselect;
    SbBool _handled {false};
};

/**
 * @author Werner Mayer
 */
class GuiExport SoVisibleFaceAction: public SoAction
{
    SO_ACTION_HEADER(SoVisibleFaceAction);

public:
    SoVisibleFaceAction();
    ~SoVisibleFaceAction() override;

    void setHandled();
    SbBool isHandled() const;

    static void initClass();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);

private:
    SbBool _handled {false};
};

class SoBoxSelectionRenderActionP;
/**
 * The SoBoxSelectionRenderAction class renders the scene with highlighted boxes around selections.
 * @author Werner Mayer
 */
class GuiExport SoBoxSelectionRenderAction: public SoGLRenderAction
{
    using inherited = SoGLRenderAction;

    SO_ACTION_HEADER(SoBoxSelectionRenderAction);

public:
    SoBoxSelectionRenderAction();
    SoBoxSelectionRenderAction(const SbViewportRegion& viewportregion);
    ~SoBoxSelectionRenderAction() override;

    static void initClass();

    void apply(SoNode* node) override;
    void apply(SoPath* path) override;
    void apply(const SoPathList& pathlist, SbBool obeysrules = false) override;
    void setVisible(SbBool b)
    {
        hlVisible = b;
    }
    SbBool isVisible() const
    {
        return hlVisible;
    }
    void setColor(const SbColor& color);
    const SbColor& getColor();
    void setLinePattern(unsigned short pattern);
    unsigned short getLinePattern() const;
    void setLineWidth(const float width);
    float getLineWidth() const;

protected:
    SbBool hlVisible;

private:
    void constructorCommon();
    void drawBoxes(SoPath* pathtothis, const SoPathList* pathlist);

    SoBoxSelectionRenderActionP* pimpl;
};

/**
 * Helper class no notify nodes to update VBO.
 * @author Werner Mayer
 */
class GuiExport SoUpdateVBOAction: public SoAction
{
    SO_ACTION_HEADER(SoUpdateVBOAction);

public:
    SoUpdateVBOAction();
    ~SoUpdateVBOAction() override;

    static void initClass();
    static void finish();

protected:
    void beginTraversal(SoNode* node) override;

private:
    static void callDoAction(SoAction* action, SoNode* node);
};

}  // namespace Gui
