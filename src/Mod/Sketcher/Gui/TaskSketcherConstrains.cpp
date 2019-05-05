/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
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
# include <cmath>
# include <QContextMenuEvent>
# include <QMenu>
# include <QRegExp>
# include <QString>
# include <QMessageBox>
# include <QStyledItemDelegate>
# include <QPainter>
# include <QPixmapCache>
# include <boost/bind.hpp>
#endif

#include "TaskSketcherConstrains.h"
#include "ui_TaskSketcherConstrains.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"

#include <Mod/Sketcher/App/SketchObject.h>

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>

using namespace SketcherGui;
using namespace Gui::TaskView;

/// Inserts a QAction into an existing menu
/// ICONSTR is the string of the icon in the resource file
/// NAMESTR is the text appearing in the contextual menuAction
/// CMDSTR is the string registered in the commandManager
/// FUNC is the name of the member function to be executed on selection of the menu item
/// ACTSONSELECTION is a true/false value to activate the command only if a selection is made
#define CONTEXT_ITEM(ICONSTR,NAMESTR,CMDSTR,FUNC,ACTSONSELECTION)                       \
QIcon icon_ ## FUNC( Gui::BitmapFactory().pixmap(ICONSTR) );                    \
    QAction* constr_ ## FUNC = menu.addAction(icon_ ## FUNC,tr(NAMESTR), this, SLOT(FUNC()),    \
        QKeySequence(QString::fromUtf8(Gui::Application::Instance->commandManager().getCommandByName(CMDSTR)->getAccel())));        \
    if(ACTSONSELECTION) constr_ ## FUNC->setEnabled(!items.isEmpty()); else constr_ ## FUNC->setEnabled(true);
/// Defines the member function corresponding to the CONTEXT_ITEM macro
#define CONTEXT_MEMBER_DEF(CMDSTR,FUNC)                             \
void ConstraintView::FUNC(){                               \
   Gui::Application::Instance->commandManager().runCommandByName(CMDSTR);}

// helper class to store additional information about the listWidget entry.
class ConstraintItem : public QListWidgetItem
{
public:
    ConstraintItem(const Sketcher::SketchObject * s, ViewProviderSketch *sketchview, int ConstNbr)
        : QListWidgetItem(QString()),
          sketch(s),
          sketchView(sketchview),
          ConstraintNbr(ConstNbr)
    {
        this->setFlags(this->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);

        updateVirtualSpaceStatus();
    }
    ~ConstraintItem()
    {
    }
    void setData(int role, const QVariant & value)
    {
        if (role == Qt::EditRole)
            this->value = value;

        QListWidgetItem::setData(role, value);
    }

    QVariant data (int role) const
    {
        if (ConstraintNbr < 0 || ConstraintNbr >= sketch->Constraints.getSize())
            return QVariant();

        const Sketcher::Constraint * constraint = sketch->Constraints[ConstraintNbr];

        // it can happen that the geometry of the sketch is tmp. invalid and thus
        // the index operator returns null.
        if (!constraint) {
            return QVariant();
        }

        if (role == Qt::EditRole) {
            if (value.isValid())
                return value;
            else
                return Base::Tools::fromStdString(Sketcher::PropertyConstraintList::getConstraintName(constraint->Name, ConstraintNbr));
        }
        else if (role == Qt::DisplayRole) {
            QString name = Base::Tools::fromStdString(Sketcher::PropertyConstraintList::getConstraintName(constraint->Name, ConstraintNbr));

            switch (constraint->Type) {
            case Sketcher::Horizontal:
            case Sketcher::Vertical:
            case Sketcher::Coincident:
            case Sketcher::PointOnObject:
            case Sketcher::Parallel:
            case Sketcher::Perpendicular:
            case Sketcher::Tangent:
            case Sketcher::Equal:
            case Sketcher::Symmetric:
            case Sketcher::Block:
                break;
            case Sketcher::Distance:
            case Sketcher::DistanceX:
            case Sketcher::DistanceY:
            case Sketcher::Radius:
            case Sketcher::Diameter:
            case Sketcher::Angle:
                name = QString::fromLatin1("%1 (%2)").arg(name).arg(constraint->getPresentationValue().getUserString());
                break;
            case Sketcher::SnellsLaw: {
                double v = constraint->getPresentationValue().getValue();
                double n1 = 1.0;
                double n2 = 1.0;
                if (fabs(v) >= 1) {
                    n2 = v;
                } else {
                    n1 = 1/v;
                }
                name = QString::fromLatin1("%1 (%2/%3)").arg(name).arg(n2).arg(n1);
                break;
            }
            case Sketcher::InternalAlignment:
                break;
            default:
                break;
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool extended = hGrp->GetBool("ExtendedConstraintInformation",false);

            if(extended) {
                if(constraint->Second == Sketcher::Constraint::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3)]").arg(name).arg(constraint->First).arg(constraint->FirstPos);
                }
                else if(constraint->Third == Sketcher::Constraint::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5)]").arg(name).arg(constraint->First).arg(constraint->FirstPos).arg(constraint->Second).arg(constraint->SecondPos);
                }
                else {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5),(%6,%7)]").arg(name).arg(constraint->First).arg(constraint->FirstPos).arg(constraint->Second).arg(constraint->SecondPos).arg(constraint->Third).arg(constraint->ThirdPos);
                }
            }

            return name;
        }
        else if (role == Qt::DecorationRole) {
            static QIcon hdist( Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance") );
            static QIcon vdist( Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance") );
            static QIcon horiz( Gui::BitmapFactory().iconFromTheme("Constraint_Horizontal") );
            static QIcon vert ( Gui::BitmapFactory().iconFromTheme("Constraint_Vertical") );
          //static QIcon lock ( Gui::BitmapFactory().iconFromTheme("Sketcher_ConstrainLock") );
            static QIcon block ( Gui::BitmapFactory().iconFromTheme("Sketcher_ConstrainBlock") );
            static QIcon coinc( Gui::BitmapFactory().iconFromTheme("Constraint_PointOnPoint") );
            static QIcon para ( Gui::BitmapFactory().iconFromTheme("Constraint_Parallel") );
            static QIcon perp ( Gui::BitmapFactory().iconFromTheme("Constraint_Perpendicular") );
            static QIcon tang ( Gui::BitmapFactory().iconFromTheme("Constraint_Tangent") );
            static QIcon dist ( Gui::BitmapFactory().iconFromTheme("Constraint_Length") );
            static QIcon radi ( Gui::BitmapFactory().iconFromTheme("Constraint_Radius") );
            static QIcon dia ( Gui::BitmapFactory().iconFromTheme("Constraint_Diameter") );
          //static QIcon majradi ( Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Major_Radius") );
          //static QIcon minradi ( Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Minor_Radius") );
            static QIcon angl ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle") );
          //static QIcon ellipseXUAngl ( Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Axis_Angle") );
            static QIcon equal( Gui::BitmapFactory().iconFromTheme("Constraint_EqualLength") );
            static QIcon pntoo( Gui::BitmapFactory().iconFromTheme("Constraint_PointOnObject") );
            static QIcon symm ( Gui::BitmapFactory().iconFromTheme("Constraint_Symmetric") );
            static QIcon snell ( Gui::BitmapFactory().iconFromTheme("Constraint_SnellsLaw") );
            static QIcon iaellipseminoraxis ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_MinorAxis") );
            static QIcon iaellipsemajoraxis ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_MajorAxis") );
            static QIcon iaellipsefocus1 ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_Focus1") );
            static QIcon iaellipsefocus2 ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_Focus2") );
            static QIcon iaellipseother ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment") );

            static QIcon hdist_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance_Driven") );
            static QIcon vdist_driven( Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance_Driven") );
            static QIcon dist_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_Length_Driven") );
            static QIcon radi_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven") );
            static QIcon dia_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven") );
            static QIcon angl_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle_Driven") );
            static QIcon snell_driven ( Gui::BitmapFactory().iconFromTheme("Constraint_SnellsLaw_Driven") );

            switch(constraint->Type){
            case Sketcher::Horizontal:
                return horiz;
            case Sketcher::Vertical:
                return vert;
            case Sketcher::Coincident:
                return coinc;
            case Sketcher::Block:
                return block;
            case Sketcher::PointOnObject:
                return pntoo;
            case Sketcher::Parallel:
                return para;
            case Sketcher::Perpendicular:
                return perp;
            case Sketcher::Tangent:
                return tang;
            case Sketcher::Equal:
                return equal;
            case Sketcher::Symmetric:
                return symm;
            case Sketcher::Distance:
                return constraint->isDriving ? dist : dist_driven;
            case Sketcher::DistanceX:
                return constraint->isDriving ? hdist : hdist_driven;
            case Sketcher::DistanceY:
                return constraint->isDriving ? vdist : vdist_driven;
            case Sketcher::Radius:
                return constraint->isDriving ? radi : radi_driven;
            case Sketcher::Diameter:
                return constraint->isDriving ? dia : dia_driven;
            case Sketcher::Angle:
                return constraint->isDriving ? angl : angl_driven;
            case Sketcher::SnellsLaw:
                return constraint->isDriving ? snell : snell_driven;
            case Sketcher::InternalAlignment:
                switch(constraint->AlignmentType){
                case Sketcher::EllipseMajorDiameter:
                    return iaellipsemajoraxis;
                case Sketcher::EllipseMinorDiameter:
                    return iaellipseminoraxis;
                case Sketcher::EllipseFocus1:
                    return iaellipsefocus1;
                case Sketcher::EllipseFocus2:
                    return iaellipsefocus2;
                case Sketcher::Undef:
                default:
                    return iaellipseother;
                }
            default:
                return QVariant();
            }
        }
        else if (role == Qt::ToolTipRole) {
            App::ObjectIdentifier path = sketch->Constraints.createPath(ConstraintNbr);
            App::PropertyExpressionEngine::ExpressionInfo expr_info = sketch->getExpression(path);

            if (expr_info.expression)
                return Base::Tools::fromStdString(expr_info.expression->toString());
            else
                return QVariant();
        }
        else
            return QListWidgetItem::data(role);
    }

    Sketcher::ConstraintType constraintType() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());
        return sketch->Constraints[ConstraintNbr]->Type;
    }

    bool isEnforceable() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        const Sketcher::Constraint * constraint = sketch->Constraints[ConstraintNbr];

        switch (constraint->Type) {
        case Sketcher::None:
        case Sketcher::NumConstraintTypes:
            assert( false );
            return false;
        case Sketcher::Horizontal:
        case Sketcher::Vertical:
        case Sketcher::Coincident:
        case Sketcher::Block:
        case Sketcher::PointOnObject:
        case Sketcher::Parallel:
        case Sketcher::Perpendicular:
        case Sketcher::Tangent:
        case Sketcher::Equal:
        case Sketcher::Symmetric:
            return true;
        case Sketcher::Distance:
        case Sketcher::DistanceX:
        case Sketcher::DistanceY:
        case Sketcher::Radius:
        case Sketcher::Diameter:
        case Sketcher::Angle:
        case Sketcher::SnellsLaw:
            return ( constraint->First >= 0 || constraint->Second >= 0 || constraint->Third >= 0 );
        case Sketcher::InternalAlignment:
            return true;
        }
        return false;
    }

    bool isDimensional() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return (sketch->Constraints[ConstraintNbr])->isDimensional();
    }

    bool isDriving() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isDriving;
    }

    bool isInVirtualSpace() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isInVirtualSpace;
    }

    void updateVirtualSpaceStatus() {
        this->setCheckState((this->isInVirtualSpace() != sketchView->getIsShownVirtualSpace())?Qt::Unchecked:Qt::Checked);
    }

    const Sketcher::SketchObject * sketch;
    const ViewProviderSketch *sketchView;
    int ConstraintNbr;
    QVariant value;
};

class ExpressionDelegate : public QStyledItemDelegate
{
public:
    ExpressionDelegate(QListWidget * _view) : view(_view) { }
protected:
    QPixmap getIcon(const char* name, const QSize& size) const
    {
        QString key = QString::fromLatin1("%1_%2x%3")
            .arg(QString::fromLatin1(name))
            .arg(size.width())
            .arg(size.height());
        QPixmap icon;
        if (QPixmapCache::find(key, icon))
            return icon;

        icon = Gui::BitmapFactory().pixmapFromSvg(name, size);
        if (!icon.isNull())
            QPixmapCache::insert(key, icon);
        return icon;
    }

    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
#if QT_VERSION >= 0x050000
        QStyleOptionViewItem options = option;
#else
        QStyleOptionViewItemV4 options = option;
#endif
        initStyleOption(&options, index);

        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

        ConstraintItem * item = dynamic_cast<ConstraintItem*>(view->item(index.row()));
        if (!item || item->sketch->Constraints.getSize() <= item->ConstraintNbr)
            return;

        App::ObjectIdentifier path = item->sketch->Constraints.createPath(item->ConstraintNbr);
        App::PropertyExpressionEngine::ExpressionInfo expr_info = item->sketch->getExpression(path);

        // in case the constraint property is invalidated it returns a null pointer
        const Sketcher::Constraint* constraint = item->sketch->Constraints[item->ConstraintNbr];
        if (constraint && constraint->isDriving && expr_info.expression) {
            // Paint pixmap
            int s = 2 * options.rect.height() / 4;
            int margin = s;
            QPixmap pixmap = getIcon(":/icons/bound-expression.svg", QSize(s, s));
            QRect r(options.rect);

            r.setTop(r.top() + (r.height() - s) / 2);
            r.setLeft(r.right() - s);
            r.setHeight(s);
            r.moveLeft(r.left() - margin);
            painter->drawPixmap(r, pixmap);
        }
    }

    QListWidget * view;
};

ConstraintView::ConstraintView(QWidget *parent)
    : QListWidget(parent)
{
    ExpressionDelegate * delegate = new ExpressionDelegate(this);
    setItemDelegate(delegate);
}

ConstraintView::~ConstraintView()
{
}

void ConstraintView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu;
    QListWidgetItem* item = currentItem();
    QList<QListWidgetItem *> items = selectedItems();

    bool isQuantity = false;
    bool isToggleDriving = false;

    // Non-driving-constraints/measurements
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        // if its the right constraint
        if (it->isDimensional()) {

            isQuantity = true;
            if (it->isEnforceable())
                isToggleDriving = true;
        }
    }

    // This does the same as a double-click and thus it should be the first action and with bold text
    QAction* change = menu.addAction(tr("Change value"), this, SLOT(modifyCurrentItem()));
    change->setEnabled(isQuantity);
    menu.setDefaultAction(change);

    QAction* driven = menu.addAction(tr("Toggle to/from reference"), this, SLOT(updateDrivingStatus()));
    driven->setEnabled(isToggleDriving);

    menu.addSeparator();
    QAction* show = menu.addAction(tr("Show constraints"), this, SLOT(showConstraints()));
    show->setEnabled(!items.isEmpty());
    QAction* hide = menu.addAction(tr("Hide constraints"), this, SLOT(hideConstraints()));
    hide->setEnabled(!items.isEmpty());

    menu.addSeparator();
    CONTEXT_ITEM("Sketcher_SelectElementsAssociatedWithConstraints","Select Elements","Sketcher_SelectElementsAssociatedWithConstraints",doSelectConstraints,true)

    QAction* rename = menu.addAction(tr("Rename"), this, SLOT(renameCurrentItem())
#ifndef Q_OS_MAC // on Mac F2 doesn't seem to trigger an edit signal
        ,QKeySequence(Qt::Key_F2)
#endif
        );
    rename->setEnabled(item != 0);

    QAction* center = menu.addAction(tr("Center sketch"), this, SLOT(centerSelectedItems()));
    center->setEnabled(item != 0);

    QAction* remove = menu.addAction(tr("Delete"), this, SLOT(deleteSelectedItems()),
        QKeySequence(QKeySequence::Delete));
    remove->setEnabled(!items.isEmpty());

    QAction* swap = menu.addAction(tr("Swap constraint names"), this, SLOT(swapNamedOfSelectedItems()));
    swap->setEnabled(items.size() == 2);

    menu.exec(event->globalPos());
}

CONTEXT_MEMBER_DEF("Sketcher_SelectElementsAssociatedWithConstraints",doSelectConstraints)

void ConstraintView::updateDrivingStatus()
{
    QListWidgetItem* item = currentItem();

    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        onUpdateDrivingStatus(item, !it->isDriving());
    }
}

void ConstraintView::showConstraints()
{
    QList<QListWidgetItem *> items = selectedItems();
    for (auto it : items) {
        if (it->checkState() != Qt::Checked)
            it->setCheckState(Qt::Checked);
    }
}

void ConstraintView::hideConstraints()
{
    QList<QListWidgetItem *> items = selectedItems();
    for (auto it : items) {
        if (it->checkState() != Qt::Unchecked)
            it->setCheckState(Qt::Unchecked);
    }
}

void ConstraintView::modifyCurrentItem()
{
    /*emit*/itemActivated(currentItem());
}

void ConstraintView::renameCurrentItem()
{
    // See also TaskSketcherConstrains::on_listWidgetConstraints_itemChanged
    QListWidgetItem* item = currentItem();
    if (item)
        editItem(item);
}

void ConstraintView::centerSelectedItems()
{
    Q_EMIT emitCenterSelectedItems();
}

void ConstraintView::deleteSelectedItems()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) return;

    doc->openTransaction("Delete");
    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
    for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(ft->getObject());
        if (vp) {
            vp->onDelete(ft->getSubNames());
        }
    }
    doc->commitTransaction();
}

void ConstraintView::swapNamedOfSelectedItems()
{
    QList<QListWidgetItem *> items = selectedItems();

    if (items.size() != 2)
        return;

    ConstraintItem * item1 = static_cast<ConstraintItem*>(items[0]);
    std::string escapedstr1 = Base::Tools::escapedUnicodeFromUtf8(item1->sketch->Constraints[item1->ConstraintNbr]->Name.c_str());
    ConstraintItem * item2 = static_cast<ConstraintItem*>(items[1]);
    std::string escapedstr2 = Base::Tools::escapedUnicodeFromUtf8(item2->sketch->Constraints[item2->ConstraintNbr]->Name.c_str());

    // In commit 67800ec8c (21 Jul 2015) the implementation of on_listWidgetConstraints_itemChanged()
    // has changed ensuring that a name of a constraint cannot be reset any more.
    // This leads to some inconsistencies when trying to swap "empty" names.
    //
    // If names are empty then nothing should be done
    if (escapedstr1.empty() || escapedstr2.empty()) {
        QMessageBox::warning(Gui::MainWindow::getInstance(), tr("Unnamed constraint"),
                             tr("Only the names of named constraints can be swapped."));
        return;
    }

    std::stringstream ss;
    ss << "DummyConstraint" << rand();
    std::string tmpname = ss.str();

    Gui::Command::openCommand("Swap constraint names");
    FCMD_OBJ_CMD2("renameConstraint(%d, u'%s')",
                            item1->sketch,
                            item1->ConstraintNbr, tmpname.c_str());
    FCMD_OBJ_CMD2("renameConstraint(%d, u'%s')",
                            item2->sketch,
                            item2->ConstraintNbr, escapedstr1.c_str());
    FCMD_OBJ_CMD2("renameConstraint(%d, u'%s')",
                            item1->sketch,
                            item1->ConstraintNbr, escapedstr2.c_str());
    Gui::Command::commitCommand();
}

// ----------------------------------------------------------------------------

TaskSketcherConstrains::TaskSketcherConstrains(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Constraints"),true, 0)
    , sketchView(sketchView), inEditMode(false)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskSketcherConstrains();
    ui->setupUi(proxy);
    ui->listWidgetConstraints->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetConstraints->setEditTriggers(QListWidget::EditKeyPressed);
    //QMetaObject::connectSlotsByName(this);

    // connecting the needed signals
    QObject::connect(
        ui->comboBoxFilter, SIGNAL(currentIndexChanged(int)),
        this              , SLOT  (on_comboBoxFilter_currentIndexChanged(int))
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemSelectionChanged()),
        this                     , SLOT  (on_listWidgetConstraints_itemSelectionChanged())
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemActivated(QListWidgetItem *)),
        this                     , SLOT  (on_listWidgetConstraints_itemActivated(QListWidgetItem *))
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(itemChanged(QListWidgetItem *)),
        this                     , SLOT  (on_listWidgetConstraints_itemChanged(QListWidgetItem *))
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(emitCenterSelectedItems()),
        this                     , SLOT  (on_listWidgetConstraints_emitCenterSelectedItems())
       );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(onUpdateDrivingStatus(QListWidgetItem *, bool)),
        this                     , SLOT  (on_listWidgetConstraints_updateDrivingStatus(QListWidgetItem *, bool))
       );
    QObject::connect(
        ui->filterInternalAlignment, SIGNAL(stateChanged(int)),
        this                     , SLOT  (on_filterInternalAlignment_stateChanged(int))
        );
    QObject::connect(
        ui->extendedInformation, SIGNAL(stateChanged(int)),
                     this                     , SLOT  (on_extendedInformation_stateChanged(int))
        );

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect(
        boost::bind(&SketcherGui::TaskSketcherConstrains::slotConstraintsChanged, this));

    this->groupLayout()->addWidget(proxy);

    this->ui->filterInternalAlignment->onRestore();
    this->ui->extendedInformation->onRestore();

    slotConstraintsChanged();
}

TaskSketcherConstrains::~TaskSketcherConstrains()
{
    this->ui->filterInternalAlignment->onSave();
    this->ui->extendedInformation->onSave();
    connectionConstraintsChanged.disconnect();
    delete ui;
}

void TaskSketcherConstrains::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    std::string temp;
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->listWidgetConstraints->blockSignals(true);
        ui->listWidgetConstraints->clearSelection ();
        ui->listWidgetConstraints->blockSignals(false);
    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection ||
             msg.Type == Gui::SelectionChanges::RmvSelection) {
        bool select = (msg.Type == Gui::SelectionChanges::AddSelection);
        // is it this object??
        if (strcmp(msg.pDocName,sketchView->getSketchObject()->getDocument()->getName())==0 &&
            strcmp(msg.pObjectName,sketchView->getSketchObject()->getNameInDocument())== 0) {
            if (msg.pSubName) {
                QRegExp rx(QString::fromLatin1("^Constraint(\\d+)$"));
                QString expr = QString::fromLatin1(msg.pSubName);
                int pos = expr.indexOf(rx);
                if (pos > -1) {
                    bool ok;
                    int ConstrId = rx.cap(1).toInt(&ok) - 1;
                    if (ok) {
                        int countItems = ui->listWidgetConstraints->count();
                        for (int i=0; i < countItems; i++) {
                            ConstraintItem* item = static_cast<ConstraintItem*>
                                (ui->listWidgetConstraints->item(i));
                            if (item->ConstraintNbr == ConstrId) {
                                if(!item->isSelected()) {
                                    ui->listWidgetConstraints->blockSignals(true);
                                    item->setSelected(select);
                                    ui->listWidgetConstraints->blockSignals(false);
                                    ui->listWidgetConstraints->scrollToItem(item);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here
    }
}

void TaskSketcherConstrains::on_comboBoxFilter_currentIndexChanged(int)
{
    slotConstraintsChanged();
}

void TaskSketcherConstrains::on_filterInternalAlignment_stateChanged(int state)
{
    Q_UNUSED(state);
    slotConstraintsChanged();
}

void TaskSketcherConstrains::on_extendedInformation_stateChanged(int state)
{
    Q_UNUSED(state);
    this->ui->extendedInformation->onSave();
    slotConstraintsChanged();
}

void TaskSketcherConstrains::on_listWidgetConstraints_emitCenterSelectedItems()
{
    sketchView->centerSelection();
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemSelectionChanged(void)
{
    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    bool block = this->blockConnection(true); // avoid to be notified by itself
    Gui::Selection().clearSelection();
    QList<QListWidgetItem *> items = ui->listWidgetConstraints->selectedItems();
    for (QList<QListWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        std::string constraint_name(Sketcher::PropertyConstraintList::getConstraintName(static_cast<ConstraintItem*>(*it)->ConstraintNbr));

        Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), constraint_name.c_str());
    }
    this->blockConnection(block);
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemActivated(QListWidgetItem *item)
{
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (!it) return;

    // if its the right constraint
    if (it->isDimensional()) {

        EditDatumDialog *editDatumDialog = new EditDatumDialog(this->sketchView, it->ConstraintNbr);
        editDatumDialog->exec(false);
        delete editDatumDialog;
    }
}

void TaskSketcherConstrains::on_listWidgetConstraints_updateDrivingStatus(QListWidgetItem *item, bool status)
{
    Q_UNUSED(status);
    ConstraintItem *citem = dynamic_cast<ConstraintItem*>(item);
    if (!citem) return;

    Gui::Application::Instance->commandManager().runCommandByName("Sketcher_ToggleDrivingConstraint");
    slotConstraintsChanged();
}

void TaskSketcherConstrains::on_listWidgetConstraints_itemChanged(QListWidgetItem *item)
{
    const ConstraintItem *it = dynamic_cast<const ConstraintItem*>(item);
    if (!it || inEditMode)
        return;

    inEditMode = true;

    assert(sketchView);

    const Sketcher::SketchObject * sketch = sketchView->getSketchObject();
    const std::vector< Sketcher::Constraint * > &vals = sketch->Constraints.getValues();
    const Sketcher::Constraint* v = vals[it->ConstraintNbr];
    const std::string currConstraintName = v->Name;

    const std::string basename = Base::Tools::toStdString(it->data(Qt::EditRole).toString());

    std::string newName(Sketcher::PropertyConstraintList::getConstraintName(basename, it->ConstraintNbr));

    // we only start a rename if we are really sure the name has changed, which is:
    // a) that the name generated by the constraints is different from the text in the widget item
    // b) that the text in the widget item, basename, is not ""
    // otherwise a checkbox change will trigger a rename on the first execution, bloating the constraint icons with the
    // default constraint name "constraint1, constraint2"
    if (newName != currConstraintName && !basename.empty()) {
        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(newName.c_str());

        Gui::Command::openCommand("Rename sketch constraint");
        try {
            FCMD_OBJ_CMD2("renameConstraint(%d, u'%s')",
                                    sketch,
                                    it->ConstraintNbr, escapedstr.c_str());
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception & e) {
            Gui::Command::abortCommand();

            QMessageBox::critical(Gui::MainWindow::getInstance(), tr("Error"),
                                  QString::fromLatin1(e.what()), QMessageBox::Ok, QMessageBox::Ok);
        }
    }

    // update constraint virtual space status
    Gui::Command::openCommand("Update constraint's virtual space");
    try {
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setVirtualSpace(%d, %s)",
                                sketch->getNameInDocument(),
                                it->ConstraintNbr,
                                ((item->checkState() == Qt::Checked) != sketchView->getIsShownVirtualSpace())?"False":"True");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception & e) {
        Gui::Command::abortCommand();

        QMessageBox::critical(Gui::MainWindow::getInstance(), tr("Error"),
                              QString::fromLatin1(e.what()), QMessageBox::Ok, QMessageBox::Ok);
    }

    inEditMode = false;
}

void TaskSketcherConstrains::slotConstraintsChanged(void)
{
    assert(sketchView);
    // Build up ListView with the constraints
    const Sketcher::SketchObject * sketch = sketchView->getSketchObject();
    const std::vector< Sketcher::Constraint * > &vals = sketch->Constraints.getValues();

    /* Update constraint number and virtual space check status */
    for (int i = 0; i <  ui->listWidgetConstraints->count(); ++i) {
        ConstraintItem * it = dynamic_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        assert(it != 0);

        it->ConstraintNbr = i;
        it->value = QVariant();
    }

    /* Remove entries, if any */
    for (std::size_t i = ui->listWidgetConstraints->count(); i > vals.size(); --i)
        delete ui->listWidgetConstraints->takeItem(i - 1);

    /* Add new entries, if any */
    for (std::size_t i = ui->listWidgetConstraints->count(); i < vals.size(); ++i)
        ui->listWidgetConstraints->addItem(new ConstraintItem(sketch, sketchView, i));

    /* Update the states */
    ui->listWidgetConstraints->blockSignals(true);
    for (int i = 0; i <  ui->listWidgetConstraints->count(); ++i) {
        ConstraintItem * it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));
        it->updateVirtualSpaceStatus();
    }
    ui->listWidgetConstraints->blockSignals(false);

    /* Update filtering */
    int Filter = ui->comboBoxFilter->currentIndex();
    for(std::size_t i = 0; i < vals.size(); ++i) {
        const Sketcher::Constraint * constraint = vals[i];
        ConstraintItem * it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));
        bool visible = true;

        /* Filter
         0 <=> All
         1 <=> Normal
         2 <=> Datums
         3 <=> Named
         4 <=> Non-Driving
        */

        bool showNormal = (Filter < 2);
        bool showDatums = (Filter < 3);
        bool showNamed = (Filter == 3 && !(constraint->Name.empty()));
        bool showNonDriving = (Filter == 4 && !constraint->isDriving);
        bool hideInternalAlignment = this->ui->filterInternalAlignment->isChecked();

        switch(constraint->Type) {
        case Sketcher::Horizontal:
        case Sketcher::Vertical:
        case Sketcher::Coincident:
        case Sketcher::PointOnObject:
        case Sketcher::Parallel:
        case Sketcher::Perpendicular:
        case Sketcher::Tangent:
        case Sketcher::Equal:
        case Sketcher::Symmetric:
        case Sketcher::Block:
            visible = showNormal || showNamed;
            break;
        case Sketcher::Distance:
        case Sketcher::DistanceX:
        case Sketcher::DistanceY:
        case Sketcher::Radius:
        case Sketcher::Diameter:
        case Sketcher::Angle:
        case Sketcher::SnellsLaw:
            visible = (showDatums || showNamed || showNonDriving);
            break;
        case Sketcher::InternalAlignment:
            visible = ((showNormal || showNamed) && !hideInternalAlignment);
        default:
            break;
        }

        // block signals as there is no need to invoke the
        // on_listWidgetConstraints_itemChanged() slot in
        // case a name has changed because this function gets
        // called after changing the constraint list property
        QAbstractItemModel* model = ui->listWidgetConstraints->model();
        bool block = model->blockSignals(true);
        it->setHidden(!visible);
        it->setData(Qt::EditRole, Base::Tools::fromStdString(constraint->Name));
        model->blockSignals(block);
    }
}

void TaskSketcherConstrains::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}



#include "moc_TaskSketcherConstrains.cpp"
