/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#pragma once

#include <map>
#include <string>
#include <Gui/Selection/Selection.h>

class SoDepthBuffer;
class SoGroup;
class SoNode;
class SoSeparator;

namespace Gui
{

class Document;
class SoFCUnifiedSelection;
class ViewProviderDocumentObject;

class GuiExport View3DInventorSelection
{
public:
    View3DInventorSelection(SoFCUnifiedSelection* root);
    ~View3DInventorSelection();

    void setDocument(Gui::Document* pcDocument)
    {
        guiDocument = pcDocument;
    }
    Gui::Document* getDocument() const
    {
        return guiDocument;
    }

    /**
     * @brief Update the on-top render groups for a selection change.
     *
     * A `SetPreselect` change also brings a hidden object on top, so that the
     * tree view can preview it.  View3DInventorViewer only forwards
     * `SetPreselect` when its `SubType` is `MsgSource::TreeView`, so hovering
     * the 3D view cannot reveal a hidden object this way.
     *
     * @param[in] Reason The selection change to apply.
     */
    void checkGroupOnTop(const SelectionChanges& Reason);

    /// Drop every object from the on-top selection and preselection groups.
    void clearGroupOnTop();

    /// True when the last change showed a view provider's own preselection preview.
    bool isFeaturePreviewActive() const
    {
        return featurePreviewActive;
    }

private:
    /// Whether the on-top group suppresses depth testing.
    enum class DepthOverride
    {
        Off,  ///< Keep whatever depth state the traversal already had.
        On    ///< Suppress the depth test so a hidden preview draws over the scene.
    };

    /// Turn the on-top depth override on only while a hidden preview needs it.
    void setHiddenPreviewDepthOverride(DepthOverride state);

    /// Hide the feature that is showing its own preselection preview, if any.
    void clearFeaturePreview();

    SoGroup* pcGroupOnTop;
    SoDepthBuffer* pcGroupOnTopDepth;
    SoGroup* pcGroupOnTopSel;
    SoGroup* pcGroupOnTopPreSel;
    SoFCUnifiedSelection* selectionRoot;
    std::map<std::string, SoNode*> objectsOnTop;
    std::map<std::string, SoNode*> objectsOnTopPreSel;
    Gui::Document* guiDocument = nullptr;
    ViewProviderDocumentObject* previewedFeature = nullptr;
    bool featurePreviewActive = false;
};

}  // namespace Gui
