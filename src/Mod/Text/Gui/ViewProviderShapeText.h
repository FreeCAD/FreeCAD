/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#ifndef TEXTGUI_ViewProviderShapeText_H
#define TEXTGUI_ViewProviderShapeText_H

#include <Inventor/SoRenderManager.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <QCoreApplication>
#include <boost_signals2.hpp>

#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderFeaturePython.h>
#include <Mod/Part/Gui/ViewProvider2DObject.h>
#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/Text/TextGlobal.h>


namespace Text {
class ShapeText;
}

namespace TextGui {

class TextGuiExport ViewProviderShapeText : public PartGui::ViewProvider2DObject,
                                            public PartGui::ViewProviderAttachExtension,
                                            public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(TextGui::ViewProviderShapeText)

    PROPERTY_HEADER_WITH_OVERRIDE(TextGui::ViewProviderShapeText);

private:
    struct VPRender
    {
        ViewProviderShapeText* vp;
        SoRenderManager* renderMgr;
    };

public:
    ViewProviderShapeText();
    ~ViewProviderShapeText() override;

    App::PropertyPythonObject TempoVis;
    App::PropertyBool HideDependent;
    App::PropertyBool ShowLinks;
    App::PropertyBool ShowSupport;
    App::PropertyBool RestoreCamera;
    App::PropertyBool ForceOrtho;
    App::PropertyBool SectionView;
    App::PropertyString EditingWorkbench;

public:
    Text::ShapeText* getShapeText() const;

    bool isSelectable() const override;
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;

    int getViewOrientationFactor() const;

    void attach(App::DocumentObject* pcFeat) override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    const char* getTransactionText() const override
    {
        return nullptr;
    }
    bool doubleClicked() override;

    boost::signals2::signal<void()> signalConstraintsChanged;
    boost::signals2::signal<void(const QString& state, const QString& msg, const QString& url,
                                 const QString& linkText)>
        signalSetUp;
    boost::signals2::signal<void()> signalElementsChanged;

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    void unsetEditViewer(Gui::View3DInventorViewer*) override;
    static void camSensCB(void* data, SoSensor*);// camera sensor callback
    void onCameraChanged(SoCamera* cam);

    void slotUndoDocument(const Gui::Document&);
    void slotRedoDocument(const Gui::Document&);
    void forceUpdateData();

private:
    Base::Placement getEditingPlacement() const;

private:
    boost::signals2::connection connectUndoDocument;
    boost::signals2::connection connectRedoDocument;

    std::string editDocName;
    std::string editObjName;
    std::string editSubName;

    SoNodeSensor cameraSensor;
    int viewOrientationFactor;
};

using ViewProviderPython = Gui::ViewProviderFeaturePythonT<ViewProviderShapeText>;

} //namespace TextGui


#endif // TEXTGUI_ViewProviderShapeText_H
