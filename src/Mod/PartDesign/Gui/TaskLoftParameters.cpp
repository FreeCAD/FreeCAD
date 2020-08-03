/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QAction>
# include <QInputDialog>
# include <QList>
# include <QRegExp>
# include <QTextEdit>
# include <QTimer>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "ui_TaskLoftParameters.h"
#include "TaskLoftParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskLoftParameters */

TaskLoftParameters::TaskLoftParameters(ViewProviderLoft *LoftView, bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_Additive_Loft",tr("Loft parameters"))
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskLoftParameters();
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->buttonProfileBase, SIGNAL(toggled(bool)),
            this, SLOT(onProfileButton(bool)));
    connect(ui->buttonRefAdd, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonAdd(bool)));
    connect(ui->buttonRefRemove, SIGNAL(toggled(bool)),
            this, SLOT(onRefButtonRemvove(bool)));
    connect(ui->checkBoxRuled, SIGNAL(toggled(bool)),
            this, SLOT(onRuled(bool)));
    connect(ui->checkBoxClosed, SIGNAL(toggled(bool)),
            this, SLOT(onClosed(bool)));
    connect(ui->checkBoxUpdateView, SIGNAL(toggled(bool)),
            this, SLOT(onUpdateView(bool)));

    QAction* remove = new QAction(tr("Remove"), this);
    remove->setShortcut(QString::fromLatin1("Del"));
    ui->listWidgetReferences->addAction(remove);
    ui->listWidgetReferences->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(remove, SIGNAL(triggered()), this, SLOT(onDeleteSection()));

    this->groupLayout()->addWidget(proxy);

    // Temporarily prevent unnecessary feature recomputes
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(true);

    //add the profiles
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(LoftView->getObject());
    App::DocumentObject* profile = loft->Profile.getValue();
    if (profile) {
        Gui::Application::Instance->showViewProvider(profile);

        QString label = QString::fromUtf8(profile->Label.getValue());
        ui->profileBaseEdit->setText(label);
    }

    for (auto obj : loft->Sections.getValues()) {
        Gui::Application::Instance->showViewProvider(obj);

        QString label = QString::fromUtf8(obj->Label.getValue());
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(label);
        item->setData(Qt::UserRole, QByteArray(obj->getNameInDocument()));
        ui->listWidgetReferences->addItem(item);
    }

    // activate and de-activate dialog elements as appropriate
    for (QWidget* child : proxy->findChildren<QWidget*>())
        child->blockSignals(false);

    updateUI(0);
}

TaskLoftParameters::~TaskLoftParameters()
{
    delete ui;
}

void TaskLoftParameters::updateUI(int index)
{
    Q_UNUSED(index);
}

void TaskLoftParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == none)
        return;

    if (msg.Type == Gui::SelectionChanges::AddSelection) {
        if (referenceSelected(msg)) {
            App::Document* document = App::GetApplication().getDocument(msg.pDocName);
            App::DocumentObject* object = document ? document->getObject(msg.pObjectName) : nullptr;
            if (object) {
                QString label = QString::fromUtf8(object->Label.getValue());
                if (selectionMode == refProfile) {
                    ui->profileBaseEdit->setText(label);
                }
                else if (selectionMode == refAdd) {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setText(label);
                    item->setData(Qt::UserRole, QByteArray(msg.pObjectName));
                    ui->listWidgetReferences->addItem(item);
                }
                else if (selectionMode == refRemove) {
                    removeFromListWidget(ui->listWidgetReferences, label);
                }
            }

            clearButtons();
            //static_cast<ViewProviderLoft*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        }

        clearButtons();
        exitSelectionMode();
    }
}

bool TaskLoftParameters::referenceSelected(const Gui::SelectionChanges& msg) const {

    if (msg.Type == Gui::SelectionChanges::AddSelection && selectionMode != none) {

        if (strcmp(msg.pDocName, vp->getObject()->getDocument()->getName()) != 0)
            return false;

        // not allowed to reference ourself
        const char* fname = vp->getObject()->getNameInDocument();
        if (strcmp(msg.pObjectName, fname) == 0)
            return false;

        //every selection needs to be a profile in itself, hence currently only full objects are
        //supported, not individual edges of a part

        //change the references
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        App::DocumentObject* obj = loft->getDocument()->getObject(msg.pObjectName);

        if (selectionMode == refProfile) {
            loft->Profile.setValue(obj);
            return true;
        }
        else if (selectionMode == refAdd || selectionMode == refRemove) {
            // now check the sections
            std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
            std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);

            if (selectionMode == refAdd) {
                if (f == refs.end())
                    refs.push_back(obj);
                else
                    return false; // duplicate selection
            }
            else if (selectionMode == refRemove) {
                if (f != refs.end())
                    refs.erase(f);
                else
                    return false;
            }

            static_cast<PartDesign::Loft*>(vp->getObject())->Sections.setValues(refs);
            return true;
        }
    }

    return false;
}

void TaskLoftParameters::removeFromListWidget(QListWidget* widget, QString name) {

    QList<QListWidgetItem*> items = widget->findItems(name, Qt::MatchExactly);
    if (!items.empty()) {
        for (QList<QListWidgetItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
            QListWidgetItem* item = widget->takeItem(widget->row(*it));
            delete item;
        }
    }
}

void TaskLoftParameters::onDeleteSection()
{
    // Delete the selected profile
    int row = ui->listWidgetReferences->currentRow();
    QListWidgetItem* item = ui->listWidgetReferences->item(row);
    if (item) {
        QByteArray data = item->data(Qt::UserRole).toByteArray();
        ui->listWidgetReferences->takeItem(row);
        delete item;

        // search inside the list of sections
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        std::vector<App::DocumentObject*> refs = loft->Sections.getValues();
        App::DocumentObject* obj = loft->getDocument()->getObject(data.constData());
        std::vector<App::DocumentObject*>::iterator f = std::find(refs.begin(), refs.end(), obj);
        if (f != refs.end()) {
            refs.erase(f);
            loft->Sections.setValues(refs);

            //static_cast<ViewProviderLoft*>(vp)->highlightReferences(false, true);
            recomputeFeature();
        }
    }
}

void TaskLoftParameters::clearButtons() {

    ui->buttonRefAdd->setChecked(false);
    ui->buttonRefRemove->setChecked(false);
}

void TaskLoftParameters::exitSelectionMode() {

    selectionMode = none;
    Gui::Selection().clearSelection();
}

void TaskLoftParameters::changeEvent(QEvent * /*e*/)
{
}

void TaskLoftParameters::onClosed(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Closed.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onRuled(bool val) {
    static_cast<PartDesign::Loft*>(vp->getObject())->Ruled.setValue(val);
    recomputeFeature();
}

void TaskLoftParameters::onProfileButton(bool checked)
{
    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refProfile;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}

void TaskLoftParameters::onRefButtonAdd(bool checked) {
    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refAdd;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}

void TaskLoftParameters::onRefButtonRemvove(bool checked) {

    if (checked) {
        Gui::Selection().clearSelection();
        selectionMode = refRemove;
        //static_cast<ViewProviderLoft*>(vp)->highlightReferences(true, true);
    }
}
////////////////////////////////////////////////////////////////////////////////////

TaskLoftWireOrders::TaskLoftWireOrders(ViewProviderLoft *LoftView, bool /*newObj*/, QWidget *parent)
    : TaskSketchBasedParameters(LoftView, parent, "PartDesign_Additive_Loft",tr("Loft Wire Orders"))

{
    svp = nullptr;
    setupUI();

    QMetaObject::connectSlotsByName(this);

    connect(previewCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(onPreviewCheckBoxToggled(bool)));
    connect(enabledCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(onEnabledCheckBoxToggled(bool)));
    connect(upButton, SIGNAL(clicked()),
            this, SLOT(onUpButton()));
    connect(downButton, SIGNAL(clicked()),
            this, SLOT(onDownButton()));
    connect(swapButton, SIGNAL(clicked()),
            this, SLOT(onSwapButton()));
    connect(sketchBox, SIGNAL(currentRowChanged(int)),
            this, SLOT(onSketchBoxRowChanged(int)));
    connect(wireBox, SIGNAL(itemSelectionChanged()),
            this, SLOT(onWireBoxSelectionChanged()));
    connectModObject = App::GetApplication().signalChangedObject.connect(boost::bind(&TaskLoftWireOrders::slotChangedObject,this,_1,_2));
    connect(sketchBox, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(onSketchBoxItemDoubleClicked(QListWidgetItem*)));

    updateUI(0, 0);

}
TaskLoftWireOrders::~TaskLoftWireOrders()
{
    connectModObject.disconnect();
}

void TaskLoftWireOrders::openTransaction(const char* title)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc){
        doc->openTransaction(title);
    }
}
void TaskLoftWireOrders::commitTransaction()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc){
        doc->commitTransaction();
    }
}

void TaskLoftWireOrders::slotChangedObject(const App::DocumentObject&, const App::Property& prop)
{
    if (prop.getTypeId() == App::PropertyStringList::getClassTypeId()){
        updateUI(0,0);
    }
}

void TaskLoftWireOrders::onSketchBoxItemDoubleClicked(QListWidgetItem* item)
{
    int currentRow = sketchBox->currentRow();
    QString itemText = item->text();
    QString prompt;
    int idx = itemText.lastIndexOf(QString::fromLatin1(":"));
    if (idx == -1){
        Base::Console().Error("Invalid Wire Orders Property\n");
        return;
    }
    QString sketchLabel = itemText.left(idx+2); //colon and space ": "
    prompt = itemText.mid(idx+2);
    QString result = QInputDialog::getText(this, tr("Wire Orders"), tr("Enter new wire order for\n")+sketchLabel,
                                        QLineEdit::Normal, prompt);
    if (!result.isEmpty()){
        PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
        std::vector<std::string> propVal = loft->WireOrders.getValues();
        if ((int) propVal.size()-1 >= sketchBox->currentRow()){
            propVal[sketchBox->currentRow()] = (sketchLabel+result).toStdString();
            openTransaction("Edit Wire Orders");
            loft->WireOrders.setValues(propVal);
            commitTransaction();
            WaitCursor wc;
            recomputeFeature();
            updateUI(currentRow, 0);
        }
    }
}

void TaskLoftWireOrders::blockSignals(bool blocked)
{
    for (QWidget* child : this->findChildren<QWidget*>())
        child->blockSignals(blocked);
}

void TaskLoftWireOrders::onSketchBoxRowChanged(int currentRow)
{

    if (sketchBox->count() == 0 || currentRow < 0){
        updateUI(0, 0);
    } else {
        updateUI(currentRow, 0);
        flashSelected();
    }
}

void TaskLoftWireOrders::onWireBoxSelectionChanged()
{
    swapButton->setEnabled(false);
    upButton->setEnabled(false);
    downButton->setEnabled(false);
    int count = wireBox->count();
    int numSelected = wireBox->selectionModel()->selectedIndexes().count();
    int currentRow = 0;
    switch(numSelected){
    case 1:
        currentRow = wireBox->selectionModel()->selectedIndexes()[0].row();
        if (currentRow >= 1 && currentRow <= count-1)
            upButton->setEnabled(true);
        if (currentRow >= 0 && currentRow <= count-2)
            downButton->setEnabled(true);
        break;
    case 2:
        swapButton->setEnabled(true);
        upButton->setEnabled(false);
        downButton->setEnabled(false);
        break;
    default:
        //leave them all disabled
        break;
    }
}
void TaskLoftWireOrders::unFlashSelected()
{
    if (svp){
        svp->LineColor.setValue(oldProfileFlashColor);
        svp = nullptr;
    }
    updateUI(sketchBox->currentRow(), wireBox->currentRow(), /*bFlash=*/false);
}

/** turns the object selected in the sketchBox lime green momentarily */
void TaskLoftWireOrders::flashSelected(){
    int sketchRow = sketchBox->currentRow();
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    Part::Feature* profile = nullptr;
    if (sketchRow == 0){
        profile = static_cast<Part::Feature*>(loft->Profile.getValue());
    } else {
        auto sections = loft->Sections.getValues();
        if ((int)sections.size() >= sketchRow)
            profile = static_cast<Part::Feature*>(sections[sketchRow-1]);
    }
    if (!profile)
        return;
    svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(profile));
    if (svp == NULL)
        return;
    App::Color svp_color = svp->LineColor.getValue();
    if (svp_color != App::Color(0.0,1.0,0.0)){
          oldProfileFlashColor = svp_color;
    }
    svp->LineColor.setValue(App::Color(0.0,1.0,0.0)); //lime green
    QTimer::singleShot(500, this, SLOT(unFlashSelected()));
}

void TaskLoftWireOrders::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    Q_UNUSED(msg);
}

void TaskLoftWireOrders::onPreviewCheckBoxToggled(bool checked) {
     static_cast<ViewProviderLoft*>(vp)->makeTemporaryVisible(checked);
}

void TaskLoftWireOrders::onEnabledCheckBoxToggled(bool checked)
{
    if (!checked){
        downButton->setEnabled(false);
        upButton->setEnabled(false);
        swapButton->setEnabled(false);
    }
    updateUI(0,0);
}

QColor TaskLoftWireOrders::getQColor(int which_color)
{
    App::Color thiscolor = getColor(which_color);
    int r = (int) (thiscolor.r * 255);
    int g = (int) (thiscolor.g * 255);
    int b = (int) (thiscolor.b * 255);
    return QColor(r, g, b);
}

App::Color TaskLoftWireOrders::getColor(int which_color)
{
    /** predefine a few basic colors, then in unlikely
     *  event we need more generate randomly
     */
    std::vector<App::Color> colors =
    {
        App::Color(1.0, 0.0, 0.0), //red
        App::Color(0.0, 1.0, 0.0), //lime
        App::Color(0.0, 0.0, 1.0), //blue
        App::Color(1.0, 0.4, 0.0), //orange
        App::Color(1.0, 1.0, 0.0), //yellow
        App::Color(1.0, 0.0, 1.0), //magenta
        App::Color(0.0, 1.0, 1.0), //cyan
        App::Color(0.0, 0.5, 0.0), //green
        App::Color(0.5, 0.0, 0.0), //maroon
        App::Color(0.5, 0.5, 0.0), //olive
        App::Color(0.0, 0.5, 0.5), //teal
        App::Color(0.5, 0.0, 0.5), //purple
        App::Color(0.0, 0.0, 0.5), //navy
        App::Color(0.0, 0.0, 0.0), //black
        App::Color(1.0, 1.0, 1.0), //white
        App::Color(1.0, 0.75, 0.8), //pink
        App::Color(0.545, 0.27, 0.08), //saddle brown
        App::Color(0.933, 0.510, 0.933), //violet
        App::Color(0.294, 0, 0.510) //indigo
    };

    if(which_color < (int) colors.size()){
        return colors[which_color];
    } else {
        float r, g, b;
        srand(which_color); //seeding with this ensures we always
                            // get the same color for this int
        r = float(rand())/RAND_MAX;
        g = float(rand())/RAND_MAX;
        b = float(rand())/RAND_MAX;
        return App::Color(r, g, b);
    }
}
void TaskLoftWireOrders::onSwapButton()
{
    if ((int)wireBox->selectionModel()->selectedIndexes().size() != 2)
        return;
    int idxA = wireBox->selectionModel()->selectedIndexes()[0].row();
    int idxB = wireBox->selectionModel()->selectedIndexes()[1].row();

    //rather than modify the list directly, we change
    //the WireOrders property and update UI to reset the listbox and wire colors
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    std::vector<std::vector<int>> wireOrders = loft->getWireOrders();
    int currentSketch = sketchBox->currentRow();
    if (currentSketch < 0 || !(currentSketch < (int)wireOrders.size()))
        return;
    int tmp = wireOrders[currentSketch][idxA];
    wireOrders[currentSketch][idxA] = wireOrders[currentSketch][idxB];
    wireOrders[currentSketch][idxB] = tmp;
    openTransaction("Swap wires");
    loft->setWireOrders(wireOrders);
    commitTransaction();
    Gui::WaitCursor waitCursor;
    recomputeFeature();
    updateUI(currentSketch, 0);

}
void TaskLoftWireOrders::onUpButton()
{
    QList<QListWidgetItem *> items = wireBox->selectedItems();
    if (items.count() != 1)
        return;
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    int row = wireBox->currentRow();
    if (row <= 0)
        return;

    //rather than modify the list directly, we change
    //the WireOrders property and update UI to reset the listbox and wire colors
    std::vector<std::vector<int>> wireOrders = loft->getWireOrders();
    int currentSketch = sketchBox->currentRow();
    if (currentSketch < 0 || !(currentSketch < (int)wireOrders.size()))
        return;
    int tmp = wireOrders[currentSketch][row];
    wireOrders[currentSketch].insert(wireOrders[currentSketch].begin() + row - 1, tmp);
    wireOrders[currentSketch].erase(wireOrders[currentSketch].begin() + row + 1);
    openTransaction("Move wire up");
    loft->setWireOrders(wireOrders);
    commitTransaction();
    Gui::WaitCursor waitCursor;
    recomputeFeature();
    updateUI(currentSketch, row - 1);

}
void TaskLoftWireOrders::onDownButton()
{
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    QList<QListWidgetItem *> items = wireBox->selectedItems();
    if (items.count() != 1)
        return;
    int row = wireBox->currentRow();
    if (row == wireBox->count() - 1)
        return;

    //rather than modify the list directly, we change
    //the WireOrders property and update UI to reset the listbox and wire colors
    std::vector<std::vector<int>> wireOrders = loft->getWireOrders();
    int currentSketch = sketchBox->currentRow();
    if (currentSketch < 0 || !(currentSketch < (int)wireOrders.size()))
        return;
    int tmp = wireOrders[currentSketch][row];
    wireOrders[currentSketch].erase(wireOrders[currentSketch].begin() + row);
    wireOrders[currentSketch].insert(wireOrders[currentSketch].begin() + row + 1, tmp);
    openTransaction("Move wire down");
    loft->setWireOrders(wireOrders);
    commitTransaction();
    Gui::WaitCursor wc;
    recomputeFeature();
    updateUI(currentSketch, row + 1);
}

void TaskLoftWireOrders::updateUI(int sketchindex, int wireindex, bool bFlash)
{
    blockSignals(true);
    sketchBox->clear();
    wireBox->clear();
    legendBox->clear();
    blockSignals(false);

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    static_cast<ViewProviderLoft*>(vp)->cleanup(); //sets all colors back to original defaults
    if (enabledCheckBox->checkState() != Qt::Checked){
        return;
    }

    std::vector<std::vector<int>> wireOrders = loft->getWireOrders();
    if (wireOrders.empty()){
        Gui::WaitCursor wc;
        recomputeFeature(); //sets wire orders to defaults
        wireOrders = loft->getWireOrders();
        if (wireOrders.empty()){
            return;
        }
    }
    //highlight profile wires each a different color
    //profile is in wireOrders[0], then sections follow
    Part::Feature* profile = static_cast<Part::Feature*>(loft->Profile.getValue());
    if (profile) {
        Gui::Application::Instance->showViewProvider(profile);
        int ii=0;
        if (!wireOrders.empty()){
            for (int wire : wireOrders[0]){
                static_cast<ViewProviderLoft*>(vp)->highlightWire(true, profile, wire, getColor(ii++));
            }
        }
    }

    //highlight the section wires each a different color
    int ii=1; //which section, profile=0
    for(auto section : loft->Sections.getValues()){
        int jj=0; //color by index, wire by value
        if (ii < (int) wireOrders.size()){
            for (int wire : wireOrders[ii++]){
                static_cast<ViewProviderLoft*>(vp)->highlightWire(true, dynamic_cast<Part::Feature*>(section), wire, getColor(jj++));
            }
        }
    }
    blockSignals(true);
    // for sketchBox listbox use strings from WireOrders property directly
    for (std::string propstr : loft->WireOrders.getValues()) {
        QString label = QString::fromUtf8(propstr.c_str());
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(label);
        item->setToolTip(tr("Double click to edit"));
        sketchBox->addItem(item);
    }
    // for wirebox listbox we use the wireOrders int values
    if ((int)wireOrders.size() > sketchindex){
        int ii=0;
        for (int wire : wireOrders[sketchindex]){
            QString label = QString::number(wire);
            QListWidgetItem* item = new QListWidgetItem();
            item->setTextAlignment(Qt::AlignHCenter);
            item->setText(label);
            wireBox->addItem(item);
            QString legendLabel = QString::fromLatin1(" ");
            QListWidgetItem* legendItem = new QListWidgetItem();
            legendItem->setText(legendLabel);
            legendItem->setBackground(QBrush(getQColor(ii++)));
            legendBox->addItem(legendItem);
        }
        sketchBox->setCurrentRow(sketchindex);
        blockSignals(false); //so onWireBoxSelectionChanged() can enable/disable buttons
        wireBox->setCurrentRow(wireindex);
        if (bFlash) //only unFlashSelected() calls this with bFlash=false
            flashSelected();
    }

    blockSignals(false);
}

void TaskLoftWireOrders::changeEvent(QEvent * /*e*/)
{
}

void TaskLoftWireOrders::setupUI()
{

    QGridLayout* grid = new QGridLayout;
    this->groupLayout()->addLayout(grid);
    enabledCheckBox = new QCheckBox(tr("Enabled"));
    enabledCheckBox->setChecked(false);
    sketchLabel = new QLabel(tr("Sketches"));
    wiresLabel = new QLabel(tr("Wires"));
    grid->addWidget(sketchLabel, 0, 0, Qt::AlignHCenter);
    grid->addWidget(enabledCheckBox, 0, 1, Qt::AlignHCenter);
    grid->addWidget(wiresLabel, 0, 2, Qt::AlignHCenter);
    previewCheckBox = new QCheckBox(tr("Preview"));
    previewCheckBox->setChecked(true);
    static_cast<ViewProviderLoft*>(vp)->makeTemporaryVisible(true); //managed by previewCheckBox
    grid->addWidget(previewCheckBox, 0, 3);
    sketchBox = new QListWidget();
    legendBox = new QListWidget(); //holds color bar reference
    wireBox = new QListWidget();
    legendBox->setSelectionMode(QAbstractItemView::NoSelection); //selected bars would change colors
    wireBox->setSelectionMode(QAbstractItemView::ExtendedSelection); //allow multiple selection for swapping
    grid->addWidget(sketchBox, 1, 0);
    grid->addWidget(legendBox, 1, 1);
    grid->addWidget(wireBox, 1, 2, Qt::AlignHCenter);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 3);
    grid->setColumnStretch(2, 4);
    grid->setColumnStretch(3, 3);

    QGridLayout* buttonGrid = new QGridLayout;
    upButton = new QPushButton();
    upButton->setIcon(BitmapFactory().iconFromTheme("button_up"));
    upButton->setToolTip(tr("Move selected wire up"));
    upButton->setEnabled(false);
    downButton = new QPushButton();
    downButton->setIcon(BitmapFactory().iconFromTheme("button_down"));
    downButton->setToolTip(tr("Move selected wire down"));
    downButton->setEnabled(false);
    swapButton = new QPushButton();
    swapButton->setIcon(BitmapFactory().iconFromTheme("button_sort"));
    swapButton->setToolTip(tr("Swap 2 selected wires"));
    swapButton->setEnabled(false);

    buttonGrid->addWidget(upButton, 0, 0);
    buttonGrid->addWidget(downButton, 1, 0);
    buttonGrid->addWidget(swapButton, 2, 0);
    grid->addLayout(buttonGrid, 1, 3);

}


///////////////////////////////////////////////////////////////////////
//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgLoftParameters::TaskDlgLoftParameters(ViewProviderLoft *LoftView,bool newObj)
   : TaskDlgSketchBasedParameters(LoftView)
{
    assert(LoftView);
    parameter  = new TaskLoftParameters(LoftView,newObj);
    Content.push_back(parameter);

    wireorder = new TaskLoftWireOrders(LoftView,newObj);
    Content.push_back(wireorder);

    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    oldOrders = loft->getWireOrders(); //to restore on user cancel
}

TaskDlgLoftParameters::~TaskDlgLoftParameters()
{
}


bool TaskDlgLoftParameters::accept()
{
    // TODO Fill this with commands (2015-09-11, Fat-Zer)
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(vp->getObject());
    std::string loftName = pcLoft->getNameInDocument();
    Gui::Command::doCommand(Gui::Command::Gui, "App.ActiveDocument.%s.ViewObject.Visibility=True", loftName.c_str());

    for(App::DocumentObject* obj : pcLoft->Sections.getValues()) {
        FCMD_OBJ_HIDE(obj);
    }
    static_cast<ViewProviderLoft*>(vp)->cleanup(); //resets default line colors

    return TaskDlgSketchBasedParameters::accept ();
}

bool TaskDlgLoftParameters::reject()
{
    PartDesign::Loft* loft = static_cast<PartDesign::Loft*>(vp->getObject());
    loft->setWireOrders(oldOrders);

    static_cast<ViewProviderLoft*>(vp)->cleanup(); //resets default line colors
    return TaskDlgSketchBasedParameters::reject();
}


//==== calls from the TaskView ===============================================================


#include "moc_TaskLoftParameters.cpp"
