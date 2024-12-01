/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
#include <algorithm>
#include <iomanip>
#include <ios>
#include <sstream>
#include <QCursor>
#include <QMenu>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/sensors/SoIdleSensor.h>
#endif

#include <boost/range/adaptors.hpp>

#include <App/Annotation.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/SoFCColorBar.h>
#include <Gui/SoFCColorBarNotifier.h>
#include <Gui/SoFCSelection.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Widgets.h>

#include <Mod/Mesh/App/FeatureMeshCurvature.h>
#include <Mod/Mesh/App/MeshFeature.h>

#include "ViewProviderCurvature.h"


using namespace Mesh;
using namespace MeshGui;
using namespace std;

bool ViewProviderMeshCurvature::addflag = false;

PROPERTY_SOURCE(MeshGui::ViewProviderMeshCurvature, Gui::ViewProviderDocumentObject)

ViewProviderMeshCurvature::ViewProviderMeshCurvature()
{
    // NOLINTBEGIN
    pcColorRoot = new SoSeparator();
    pcColorRoot->ref();
    pcColorMat = new SoMaterial;
    pcColorMat->ref();
    pcColorStyle = new SoDrawStyle();
    pcColorRoot->addChild(pcColorStyle);
    // simple color bar
    pcColorBar = new Gui::SoFCColorBar;
    pcColorBar->Attach(this);
    Gui::SoFCColorBarNotifier::instance().attach(pcColorBar);
    pcColorBar->ref();
    pcColorBar->setRange(-0.5f, 0.5f, 3);
    pcLinkRoot = new SoGroup;
    pcLinkRoot->ref();
    // NOLINTEND

    App::Material mat;
    const SbColor* cols {};
    // NOLINTBEGIN
    if (pcColorMat->ambientColor.getNum() == 1) {
        cols = pcColorMat->ambientColor.getValues(0);
        mat.ambientColor.setPackedValue(cols[0].getPackedValue());
    }
    if (pcColorMat->diffuseColor.getNum() == 1) {
        cols = pcColorMat->diffuseColor.getValues(0);
        mat.diffuseColor.setPackedValue(cols[0].getPackedValue());
    }
    if (pcColorMat->emissiveColor.getNum() == 1) {
        cols = pcColorMat->emissiveColor.getValues(0);
        mat.emissiveColor.setPackedValue(cols[0].getPackedValue());
    }
    if (pcColorMat->specularColor.getNum() == 1) {
        cols = pcColorMat->specularColor.getValues(0);
        mat.specularColor.setPackedValue(cols[0].getPackedValue());
    }
    if (pcColorMat->shininess.getNum() == 1) {
        const float* shiny = pcColorMat->shininess.getValues(0);
        mat.shininess = shiny[0];
    }
    if (pcColorMat->transparency.getNum() == 1) {
        const float* trans = pcColorMat->transparency.getValues(0);
        mat.transparency = trans[0];
    }
    // NOLINTEND

    ADD_PROPERTY(TextureMaterial, (mat));
    SelectionStyle.setValue(1);  // BBOX
}

ViewProviderMeshCurvature::~ViewProviderMeshCurvature()
{
    pcColorRoot->unref();
    pcColorMat->unref();
    deleteColorBar();
    pcLinkRoot->unref();
}

void ViewProviderMeshCurvature::onChanged(const App::Property* prop)
{
    if (prop == &TextureMaterial) {
        const App::Material& Mat = TextureMaterial.getValue();
        pcColorMat->ambientColor.setValue(Mat.ambientColor.r,
                                          Mat.ambientColor.g,
                                          Mat.ambientColor.b);
        pcColorMat->specularColor.setValue(Mat.specularColor.r,
                                           Mat.specularColor.g,
                                           Mat.specularColor.b);
        pcColorMat->emissiveColor.setValue(Mat.emissiveColor.r,
                                           Mat.emissiveColor.g,
                                           Mat.emissiveColor.b);
        pcColorMat->shininess.setValue(Mat.shininess);
        pcColorMat->transparency.setValue(Mat.transparency);
    }

    ViewProviderDocumentObject::onChanged(prop);
}

void ViewProviderMeshCurvature::hide()
{
    inherited::hide();
    pcColorStyle->style = SoDrawStyle::INVISIBLE;
}

void ViewProviderMeshCurvature::show()
{
    inherited::show();
    pcColorStyle->style = SoDrawStyle::FILLED;
}

void ViewProviderMeshCurvature::init(const Mesh::PropertyCurvatureList* prop)
{
    // NOLINTBEGIN(readability-magic-numbers)
    std::vector<float> aMinValues;
    std::vector<float> aMaxValues;
    const std::vector<Mesh::CurvatureInfo>& fCurvInfo = prop->getValues();
    aMinValues.reserve(fCurvInfo.size());
    aMaxValues.reserve(fCurvInfo.size());

    for (const auto& jt : fCurvInfo) {
        aMinValues.push_back(jt.fMinCurvature);
        aMaxValues.push_back(jt.fMaxCurvature);
    }

    if (aMinValues.empty() || aMaxValues.empty()) {
        return;  // no values inside
    }

    float fMin = *std::min_element(aMinValues.begin(), aMinValues.end());
    float fMax = *std::max_element(aMinValues.begin(), aMinValues.end());

    // histogram over all values
    std::map<int, int> aHistogram;
    for (float aMinValue : aMinValues) {
        int grp = (int)(10.0F * (aMinValue - fMin) / (fMax - fMin));
        aHistogram[grp]++;
    }

    float fRMin = -1.0F;
    for (const auto& mIt : aHistogram) {
        if ((float)mIt.second / (float)aMinValues.size() > 0.15F) {
            fRMin = float(mIt.first) * (fMax - fMin) / 10.0F + fMin;
            break;
        }
    }

    fMin = *std::min_element(aMaxValues.begin(), aMaxValues.end());
    fMax = *std::max_element(aMaxValues.begin(), aMaxValues.end());

    // histogram over all values
    aHistogram.clear();
    for (float aMaxValue : aMaxValues) {
        int grp = (int)(10.0F * (aMaxValue - fMin) / (fMax - fMin));
        aHistogram[grp]++;
    }

    float fRMax = 1.0F;
    for (auto rIt2 = aHistogram.rbegin(); rIt2 != aHistogram.rend(); ++rIt2) {
        if ((float)rIt2->second / (float)aMaxValues.size() > 0.15F) {
            fRMax = float(rIt2->first) * (fMax - fMin) / 10.0F + fMin;
            break;
        }
    }
    // NOLINTEND(readability-magic-numbers)

    float fAbs = std::max<float>(fabs(fRMin), fabs(fRMax));
    fRMin = -fAbs;
    fRMax = fAbs;
    fMin = fRMin;
    fMax = fRMax;
    pcColorBar->setRange(fMin, fMax, 3);
}

void ViewProviderMeshCurvature::slotChangedObject(const App::DocumentObject& Obj,
                                                  const App::Property& Prop)
{
    // we get this for any object for that a property has changed. Thus, we must regard that object
    // which is linked by our link property
    App::DocumentObject* object = dynamic_cast<Mesh::Curvature*>(pcObject)->Source.getValue();
    if (object == &Obj) {
        if (auto meshObject = dynamic_cast<Mesh::Feature*>(object)) {
            const Mesh::PropertyMeshKernel& mesh = meshObject->Mesh;
            if ((&mesh) == (&Prop)) {
                const Mesh::MeshObject& kernel = mesh.getValue();
                pcColorMat->diffuseColor.setNum((int)kernel.countPoints());
                pcColorMat->transparency.setNum((int)kernel.countPoints());
                // make sure to recompute the feature
                dynamic_cast<Mesh::Curvature*>(pcObject)->Source.touch();
            }
        }
    }
}

void ViewProviderMeshCurvature::deleteColorBar()
{
    Gui::SoFCColorBarNotifier::instance().detach(pcColorBar);
    pcColorBar->Detach(this);
    pcColorBar->unref();
}

void ViewProviderMeshCurvature::attach(App::DocumentObject* pcFeat)
{
    // creates the standard viewing modes
    inherited::attach(pcFeat);
    attachDocument(pcFeat->getDocument());

    auto flathints = new SoShapeHints;
    flathints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    flathints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    auto pcColorShadedRoot = new SoGroup();
    pcColorShadedRoot->addChild(flathints);

    // color shaded
    auto pcFlatStyle = new SoDrawStyle();
    pcFlatStyle->style = SoDrawStyle::FILLED;
    pcColorShadedRoot->addChild(pcFlatStyle);

    auto pcMatBinding = new SoMaterialBinding;
    pcMatBinding->value = SoMaterialBinding::PER_VERTEX_INDEXED;
    pcColorShadedRoot->addChild(pcColorMat);
    pcColorShadedRoot->addChild(pcMatBinding);
    pcColorShadedRoot->addChild(pcLinkRoot);

    addDisplayMaskMode(pcColorShadedRoot, "ColorShaded");

    // Check for an already existing color bar
    auto node = findFrontRootOfType(Gui::SoFCColorBar::getClassTypeId());
    if (auto pcBar = dynamic_cast<Gui::SoFCColorBar*>(node)) {
        float fMin = pcColorBar->getMinValue();
        float fMax = pcColorBar->getMaxValue();

        // Attach to the foreign color bar and delete our own bar
        pcBar->Attach(this);
        pcBar->ref();
        pcBar->setRange(fMin, fMax, 3);
        pcBar->Notify(0);
        deleteColorBar();
        pcColorBar = pcBar;
    }

    pcColorRoot->addChild(pcColorBar);
}

void ViewProviderMeshCurvature::updateData(const App::Property* prop)
{
    // set to the expected size
    if (auto link = dynamic_cast<const App::PropertyLink*>(prop)) {
        auto object = link->getValue<Mesh::Feature*>();
        Gui::coinRemoveAllChildren(this->pcLinkRoot);
        if (object) {
            const Mesh::MeshObject& kernel = object->Mesh.getValue();
            pcColorMat->diffuseColor.setNum((int)kernel.countPoints());
            pcColorMat->transparency.setNum((int)kernel.countPoints());

            // get the view provider of the associated mesh feature
            App::Document* rDoc = pcObject->getDocument();
            Gui::Document* pDoc = Gui::Application::Instance->getDocument(rDoc);
            if (auto view = dynamic_cast<ViewProviderMesh*>(pDoc->getViewProvider(object))) {
                this->pcLinkRoot->addChild(view->getHighlightNode());

                auto mesh = dynamic_cast<Mesh::Feature*>(view->getObject());
                Base::Placement plm = mesh->Placement.getValue();
                ViewProviderMesh::updateTransform(plm, pcTransform);
            }
        }
    }
    else if (auto curv = dynamic_cast<const Mesh::PropertyCurvatureList*>(prop)) {
        if (curv->getSize() < 3) {  // invalid array
            return;
        }
        setActiveMode();
    }
}

SoSeparator* ViewProviderMeshCurvature::getFrontRoot() const
{
    return pcColorRoot;
}

void ViewProviderMeshCurvature::setVertexCurvatureMode(int mode)
{
    using PropertyMap = std::map<std::string, App::Property*>;
    PropertyMap Map;
    pcObject->getPropertyMap(Map);

    auto it = std::find_if(Map.begin(), Map.end(), [](const PropertyMap::value_type& v) {
        Base::Type type = v.second->getTypeId();
        return (type == Mesh::PropertyCurvatureList::getClassTypeId());
    });

    if (it == Map.end()) {
        return;  // cannot display this feature type due to missing curvature property
    }

    auto pCurvInfo = dynamic_cast<Mesh::PropertyCurvatureList*>(it->second);

    // curvature values
    std::vector<float> fValues = pCurvInfo->getCurvature(mode);
    pcColorMat->diffuseColor.setNum(int(fValues.size()));
    pcColorMat->transparency.setNum(int(fValues.size()));

    SbColor* diffcol = pcColorMat->diffuseColor.startEditing();
    float* transp = pcColorMat->transparency.startEditing();

    for (auto const& value : fValues | boost::adaptors::indexed(0)) {
        App::Color c = pcColorBar->getColor(value.value());
        // NOLINTBEGIN
        diffcol[value.index()].setValue(c.r, c.g, c.b);
        transp[value.index()] = c.transparency();
        // NOLINTEND
    }

    pcColorMat->diffuseColor.finishEditing();
    pcColorMat->transparency.finishEditing();

    // In order to apply the transparency changes the IndexFaceSet node must be touched
    touchShapeNode();
}

void ViewProviderMeshCurvature::touchShapeNode()
{
    SoSearchAction searchAction;
    searchAction.setType(SoIndexedFaceSet::getClassTypeId());
    searchAction.setInterest(SoSearchAction::FIRST);
    searchAction.apply(pcLinkRoot);
    SoPath* selectionPath = searchAction.getPath();
    if (selectionPath) {
        selectionPath->getTail()->touch();
    }
}

QIcon ViewProviderMeshCurvature::getIcon() const
{
    static QPixmap px = Gui::BitmapFactory().pixmap(":/icons/Mesh_Tree_Curvature_Plot.svg");
    return px;
}

void ViewProviderMeshCurvature::setDisplayMode(const char* ModeName)
{
    if (strcmp("Mean curvature", ModeName) == 0) {
        setVertexCurvatureMode(Mesh::PropertyCurvatureList::MeanCurvature);
        setDisplayMaskMode("ColorShaded");
    }
    else if (strcmp("Gaussian curvature", ModeName) == 0) {
        setVertexCurvatureMode(Mesh::PropertyCurvatureList::GaussCurvature);
        setDisplayMaskMode("ColorShaded");
    }
    else if (strcmp("Maximum curvature", ModeName) == 0) {
        setVertexCurvatureMode(Mesh::PropertyCurvatureList::MaxCurvature);
        setDisplayMaskMode("ColorShaded");
    }
    else if (strcmp("Minimum curvature", ModeName) == 0) {
        setVertexCurvatureMode(Mesh::PropertyCurvatureList::MinCurvature);
        setDisplayMaskMode("ColorShaded");
    }
    else if (strcmp("Absolute curvature", ModeName) == 0) {
        setVertexCurvatureMode(Mesh::PropertyCurvatureList::AbsCurvature);
        setDisplayMaskMode("ColorShaded");
    }

    inherited::setDisplayMode(ModeName);
}

const char* ViewProviderMeshCurvature::getDefaultDisplayMode() const
{
    return "Absolute curvature";
}

std::vector<std::string> ViewProviderMeshCurvature::getDisplayModes() const
{
    std::vector<std::string> StrList = inherited::getDisplayModes();

    // add modes
    StrList.emplace_back("Absolute curvature");
    StrList.emplace_back("Mean curvature");
    StrList.emplace_back("Gaussian curvature");
    StrList.emplace_back("Maximum curvature");
    StrList.emplace_back("Minimum curvature");

    return StrList;
}

void ViewProviderMeshCurvature::OnChange(Base::Subject<int>& /*rCaller*/, int /*rcReason*/)
{
    setActiveMode();
}

namespace MeshGui
{

class Annotation
{
public:
    Annotation(Gui::ViewProviderDocumentObject* vp,
               const QString& s,
               const SbVec3f& p,
               const SbVec3f& n)
        : vp(vp)
        , s(s)
        , p(p)
        , n(n)
    {}

    static void run(void* data, SoSensor* sensor)
    {
        auto self = static_cast<Annotation*>(data);
        self->show();
        delete self;
        delete sensor;
    }

    void show()
    {
        App::Document* doc = vp->getObject()->getDocument();

        auto groups = doc->getObjectsOfType<App::DocumentObjectGroup>();
        App::DocumentObjectGroup* group = nullptr;
        std::string internalname = "CurvatureGroup";
        for (const auto& it : groups) {
            if (internalname == it->getNameInDocument()) {
                group = it;
                break;
            }
        }
        if (!group) {
            group = dynamic_cast<App::DocumentObjectGroup*>(
                doc->addObject("App::DocumentObjectGroup", internalname.c_str()));
        }

        auto anno = dynamic_cast<App::AnnotationLabel*>(
            group->addObject("App::AnnotationLabel", internalname.c_str()));
        QStringList lines = s.split(QLatin1String("\n"));
        std::vector<std::string> text;
        for (const auto& line : lines) {
            text.emplace_back((const char*)line.toLatin1());
        }
        anno->LabelText.setValues(text);
        std::stringstream str;
        str << "Curvature info (" << group->Group.getSize() << ")";
        anno->Label.setValue(str.str());
        anno->BasePosition.setValue(p[0], p[1], p[2]);
        anno->TextPosition.setValue(n[0], n[1], n[2]);
    }

private:
    Gui::ViewProviderDocumentObject* vp;
    QString s;
    SbVec3f p;
    SbVec3f n;
};

}  // namespace MeshGui

void ViewProviderMeshCurvature::curvatureInfoCallback(void* ud, SoEventCallback* n)
{
    auto view = static_cast<Gui::View3DInventorViewer*>(n->getUserData());
    const SoEvent* ev = n->getEvent();
    if (ev->getTypeId() == SoMouseButtonEvent::getClassTypeId()) {
        const auto mbe = static_cast<const SoMouseButtonEvent*>(ev);  // NOLINT

        // Mark all incoming mouse button events as handled, especially, to deactivate the
        // selection node
        n->getAction()->setHandled();
        if (mbe->getButton() == SoMouseButtonEvent::BUTTON2
            && mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            // context-menu
            QMenu menu;
            QAction* fl = menu.addAction(QObject::tr("Annotation"));
            fl->setCheckable(true);
            fl->setChecked(addflag);
            QAction* cl = menu.addAction(QObject::tr("Leave info mode"));
            QAction* id = menu.exec(QCursor::pos());
            if (fl == id) {
                addflag = fl->isChecked();
            }
            else if (cl == id) {
                view->setEditing(false);
                view->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
                view->setRedirectToSceneGraph(false);
                view->setSelectionEnabled(true);
                view->removeEventCallback(SoEvent::getClassTypeId(), curvatureInfoCallback, ud);
            }
        }
        else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1
                 && mbe->getState() == SoButtonEvent::UP) {
            const SoPickedPoint* point = n->getPickedPoint();
            if (!point) {
                Base::Console().Message("No facet picked.\n");
                return;
            }

            n->setHandled();

            // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
            // really from the mesh we render and not from any other geometry
            Gui::ViewProvider* vp = view->getViewProviderByPathFromTail(point->getPath());
            if (auto self = dynamic_cast<ViewProviderMeshCurvature*>(vp)) {
                const SoDetail* detail = point->getDetail(point->getPath()->getTail());
                if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
                    const auto facedetail = static_cast<const SoFaceDetail*>(detail);  // NOLINT
                    // get the curvature info of the three points of the picked facet
                    int index1 = facedetail->getPoint(0)->getCoordinateIndex();
                    int index2 = facedetail->getPoint(1)->getCoordinateIndex();
                    int index3 = facedetail->getPoint(2)->getCoordinateIndex();
                    std::string info = self->curvatureInfo(true, index1, index2, index3);
                    QString text = QString::fromLatin1(info.c_str());
                    if (addflag) {
                        SbVec3f pt = point->getPoint();
                        SbVec3f nl = point->getNormal();
                        auto anno = new Annotation(self, text, pt, nl);
                        auto sensor = new SoIdleSensor(Annotation::run, anno);
                        sensor->schedule();
                    }
                    else {
                        Gui::ToolTip::showText(QCursor::pos(), text);
                    }
                }
            }
        }
    }
    else if (ev->getTypeId().isDerivedFrom(SoLocation2Event::getClassTypeId())) {
        const SoPickedPoint* point = n->getPickedPoint();
        if (!point) {
            return;
        }
        n->setHandled();

        // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
        // really from the mesh we render and not from any other geometry
        Gui::ViewProvider* vp = view->getViewProviderByPathFromTail(point->getPath());
        if (auto self = dynamic_cast<ViewProviderMeshCurvature*>(vp)) {
            const SoDetail* detail = point->getDetail(point->getPath()->getTail());
            if (detail && detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
                const auto facedetail = static_cast<const SoFaceDetail*>(detail);  // NOLINT
                // get the curvature info of the three points of the picked facet
                int index1 = facedetail->getPoint(0)->getCoordinateIndex();
                int index2 = facedetail->getPoint(1)->getCoordinateIndex();
                int index3 = facedetail->getPoint(2)->getCoordinateIndex();
                std::string info = self->curvatureInfo(false, index1, index2, index3);
                Gui::getMainWindow()->setPaneText(1, QString::fromLatin1(info.c_str()));
            }
        }
    }
}

std::string
ViewProviderMeshCurvature::curvatureInfo(bool detail, int index1, int index2, int index3) const
{
    // get the curvature info of the three points of the picked facet
    App::Property* prop = pcObject->getPropertyByName("CurvInfo");
    std::stringstream str;
    if (auto curv = dynamic_cast<Mesh::PropertyCurvatureList*>(prop)) {
        const Mesh::CurvatureInfo& cVal1 = (*curv)[index1];
        const Mesh::CurvatureInfo& cVal2 = (*curv)[index2];
        const Mesh::CurvatureInfo& cVal3 = (*curv)[index3];
        float fVal1 = 0.0F;
        float fVal2 = 0.0F;
        float fVal3 = 0.0F;

        bool print = true;
        std::string mode = getActiveDisplayMode();
        if (mode == "Minimum curvature") {
            fVal1 = cVal1.fMinCurvature;
            fVal2 = cVal2.fMinCurvature;
            fVal3 = cVal3.fMinCurvature;
        }
        else if (mode == "Maximum curvature") {
            fVal1 = cVal1.fMaxCurvature;
            fVal2 = cVal2.fMaxCurvature;
            fVal3 = cVal3.fMaxCurvature;
        }
        else if (mode == "Gaussian curvature") {
            fVal1 = cVal1.fMaxCurvature * cVal1.fMinCurvature;
            fVal2 = cVal2.fMaxCurvature * cVal2.fMinCurvature;
            fVal3 = cVal3.fMaxCurvature * cVal3.fMinCurvature;
        }
        else if (mode == "Mean curvature") {
            // NOLINTBEGIN(readability-magic-numbers)
            fVal1 = 0.5F * (cVal1.fMaxCurvature + cVal1.fMinCurvature);
            fVal2 = 0.5F * (cVal2.fMaxCurvature + cVal2.fMinCurvature);
            fVal3 = 0.5F * (cVal3.fMaxCurvature + cVal3.fMinCurvature);
            // NOLINTEND(readability-magic-numbers)
        }
        else if (mode == "Absolute curvature") {
            fVal1 = fabs(cVal1.fMaxCurvature) > fabs(cVal1.fMinCurvature) ? cVal1.fMaxCurvature
                                                                          : cVal1.fMinCurvature;
            fVal2 = fabs(cVal2.fMaxCurvature) > fabs(cVal2.fMinCurvature) ? cVal2.fMaxCurvature
                                                                          : cVal2.fMinCurvature;
            fVal3 = fabs(cVal3.fMaxCurvature) > fabs(cVal3.fMinCurvature) ? cVal3.fMaxCurvature
                                                                          : cVal3.fMinCurvature;
        }
        else {
            print = false;
        }

        if (print) {
            if (!detail) {
                str << mode << ": <" << fVal1 << ", " << fVal2 << ", " << fVal3 << ">";
            }
            else {
                const int prec = 5;
                str.setf(std::ios::fixed | std::ios::showpoint);
                str.precision(prec);
                str << mode << std::endl
                    << "v1: " << std::setw(prec) << fVal1 << std::endl
                    << "v2: " << std::setw(prec) << fVal2 << std::endl
                    << "v3: " << std::setw(prec) << fVal3;
            }
        }
        else if (!detail) {
            str << "No curvature mode set";
        }
    }

    return str.str();
}
