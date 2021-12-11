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
# include <QStringListModel>
# include <boost_bind_bind.hpp>
#endif

#include "TaskSketcherConstraints.h"
#include "ui_TaskSketcherConstraints.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"

#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/Gui/CommandConstraints.h>

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>

#include "Utils.h"

#include "ConstraintMultiFilterDialog.h"
#include "ConstraintSettingsDialog.h"

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
            case Sketcher::Weight:
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
                if(constraint->Second == Sketcher::GeoEnum::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3)]").arg(name).arg(constraint->First).arg(static_cast<int>(constraint->FirstPos));
                }
                else if(constraint->Third == Sketcher::GeoEnum::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5)]").arg(name).arg(constraint->First).arg(static_cast<int>(constraint->FirstPos)).arg(constraint->Second).arg(static_cast<int>(constraint->SecondPos));
                }
                else {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5),(%6,%7)]").arg(name).arg(constraint->First).arg(static_cast<int>(constraint->FirstPos)).arg(constraint->Second).arg(static_cast<int>(constraint->SecondPos)).arg(constraint->Third).arg(static_cast<int>(constraint->ThirdPos));
                }
            }

            return name;
        }
        else if (role == Qt::DecorationRole) {
            static QIcon hdist( Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance") );
            static QIcon vdist( Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance") );
            static QIcon horiz( Gui::BitmapFactory().iconFromTheme("Constraint_Horizontal") );
            static QIcon vert ( Gui::BitmapFactory().iconFromTheme("Constraint_Vertical") );
          //static QIcon lock ( Gui::BitmapFactory().iconFromTheme("Constraint_Lock") );
            static QIcon block ( Gui::BitmapFactory().iconFromTheme("Constraint_Block") );
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

            auto selicon = [](const Sketcher::Constraint * constr, const QIcon & normal, const QIcon & driven) -> QIcon {
                if(!constr->isActive) {
                    QIcon darkIcon;
                    int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
                    darkIcon.addPixmap(normal.pixmap(w, w, QIcon::Disabled, QIcon::Off), QIcon::Normal, QIcon::Off);
                    darkIcon.addPixmap(normal.pixmap(w, w, QIcon::Disabled, QIcon::On ), QIcon::Normal, QIcon::On );
                    return darkIcon;
                }
                else if (constr->isDriving) {
                    return normal;
                }
                else {
                    return driven;
                }
            };

            switch(constraint->Type){
            case Sketcher::Horizontal:
                return selicon(constraint,horiz,horiz);
            case Sketcher::Vertical:
                return selicon(constraint,vert,vert);
            case Sketcher::Coincident:
                return selicon(constraint,coinc,coinc);
            case Sketcher::Block:
                return selicon(constraint,block,block);
            case Sketcher::PointOnObject:
                return selicon(constraint,pntoo,pntoo);
            case Sketcher::Parallel:
                return selicon(constraint,para,para);
            case Sketcher::Perpendicular:
                return selicon(constraint,perp,perp);
            case Sketcher::Tangent:
                return selicon(constraint,tang,tang);
            case Sketcher::Equal:
                return selicon(constraint,equal,equal);
            case Sketcher::Symmetric:
                return selicon(constraint,symm,symm);
            case Sketcher::Distance:
                return selicon(constraint,dist,dist_driven);
            case Sketcher::DistanceX:
                return selicon(constraint,hdist,hdist_driven);
            case Sketcher::DistanceY:
                return selicon(constraint,vdist,vdist_driven);
            case Sketcher::Radius:
            case Sketcher::Weight:
                return selicon(constraint,radi,radi_driven);
            case Sketcher::Diameter:
                return selicon(constraint,dia,dia_driven);
            case Sketcher::Angle:
                return selicon(constraint,angl,angl_driven);
            case Sketcher::SnellsLaw:
                return selicon(constraint,snell,snell_driven);
            case Sketcher::InternalAlignment:
                switch(constraint->AlignmentType){
                case Sketcher::EllipseMajorDiameter:
                    return selicon(constraint,iaellipsemajoraxis,iaellipsemajoraxis);
                case Sketcher::EllipseMinorDiameter:
                    return selicon(constraint,iaellipseminoraxis,iaellipseminoraxis);
                case Sketcher::EllipseFocus1:
                    return selicon(constraint,iaellipsefocus1,iaellipsefocus1);
                case Sketcher::EllipseFocus2:
                    return selicon(constraint,iaellipsefocus2,iaellipsefocus2);
                case Sketcher::Undef:
                default:
                    return selicon(constraint,iaellipseother,iaellipseother);
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
        case Sketcher::Weight:
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

    bool isActive() const {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isActive;
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
    ExpressionDelegate(QListWidget * _view) : QStyledItemDelegate(_view), view(_view) { }
protected:
    QPixmap getIcon(const char* name, const QSize& size) const
    {
        QString key = QString::fromLatin1("%1_%2x%3")
            .arg(QString::fromLatin1(name))
            .arg(size.width())
            .arg(size.height());
        QPixmap icon;
        if (QPixmapCache::find(key, &icon))
            return icon;

        icon = Gui::BitmapFactory().pixmapFromSvg(name, size);
        if (!icon.isNull())
            QPixmapCache::insert(key, icon);
        return icon;
    }

    void paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
        QStyleOptionViewItem options = option;
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

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    bool didRelease = SketcherGui::ReleaseHandler(doc);

    // Sync the FreeCAD selection with the selection in the ConstraintView widget
    if (didRelease) {
        Gui::Selection().clearSelection();
        std::string doc_name = static_cast<ConstraintItem*>(item)->sketchView->getSketchObject()->getDocument()->getName();
        std::string obj_name = static_cast<ConstraintItem*>(item)->sketchView->getSketchObject()->getNameInDocument();

        std::vector<std::string> constraintSubNames;
        for (auto&& it : items) {
            auto ci = static_cast<ConstraintItem*>(it);
            std::string constraint_name = Sketcher::PropertyConstraintList::getConstraintName(ci->ConstraintNbr);
            constraintSubNames.push_back(constraint_name.c_str());
        }

        if(!constraintSubNames.empty())
            Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
    }

    bool isQuantity = false;
    bool isToggleDriving = false;
    bool isActive = true;

    // Non-driving-constraints/measurements
    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        // if its the right constraint
        if (it->isDimensional()) {

            isQuantity = true;
            if (it->isEnforceable())
                isToggleDriving = true;
        }

        isActive = it->isActive();
    }

    // This does the same as a double-click and thus it should be the first action and with bold text
    QAction* change = menu.addAction(tr("Change value"), this, SLOT(modifyCurrentItem()));
    change->setEnabled(isQuantity);
    menu.setDefaultAction(change);

    QAction* driven = menu.addAction(tr("Toggle to/from reference"), this, SLOT(updateDrivingStatus()));
    driven->setEnabled(isToggleDriving);

    QAction* activate = menu.addAction(isActive ? tr("Deactivate") : tr("Activate"), this, SLOT(updateActiveStatus()));
    activate->setEnabled(!items.isEmpty());

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

void ConstraintView::updateActiveStatus()
{
    QListWidgetItem* item = currentItem();

    ConstraintItem *it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        onUpdateActiveStatus(item, !it->isActive());
    }
}

void ConstraintView::showConstraints()
{
    Q_EMIT emitShowSelection3DVisibility();
}

void ConstraintView::hideConstraints()
{
    Q_EMIT emitHideSelection3DVisibility();
}

void ConstraintView::modifyCurrentItem()
{
    /*emit*/itemActivated(currentItem());
}

void ConstraintView::renameCurrentItem()
{
    // See also TaskSketcherConstraints::on_listWidgetConstraints_itemChanged
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

    doc->openTransaction("Delete constraint");
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

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Swap constraint names"));
    Gui::cmdAppObjectArgs(item1->sketch, "renameConstraint(%d, u'%s')",
                          item1->ConstraintNbr, tmpname.c_str());
    Gui::cmdAppObjectArgs(item2->sketch, "renameConstraint(%d, u'%s')",
                          item2->ConstraintNbr, escapedstr1.c_str());
    Gui::cmdAppObjectArgs(item1->sketch, "renameConstraint(%d, u'%s')",
                          item1->ConstraintNbr, escapedstr2.c_str());
    Gui::Command::commitCommand();
}

// ----------------------------------------------------------------------------

TaskSketcherConstraints::TaskSketcherConstraints(ViewProviderSketch *sketchView) :
    TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Constraints"), true, 0),
    sketchView(sketchView), inEditMode(false),
    ui(new Ui_TaskSketcherConstraints)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    ui->listWidgetConstraints->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetConstraints->setEditTriggers(QListWidget::EditKeyPressed);
    //QMetaObject::connectSlotsByName(this);

    createVisibilityButtonActions();

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
        ui->listWidgetConstraints, SIGNAL(onUpdateActiveStatus(QListWidgetItem *, bool)),
        this                     , SLOT  (on_listWidgetConstraints_updateActiveStatus(QListWidgetItem *, bool))
    );
    QObject::connect(
        ui->showAllButton, SIGNAL(clicked(bool)),
                     this                     , SLOT  (on_showAllButton_clicked(bool))
        );
    QObject::connect(
        ui->hideAllButton, SIGNAL(clicked(bool)),
                     this                     , SLOT  (on_hideAllButton_clicked(bool))
        );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(emitHideSelection3DVisibility()),
        this                     , SLOT  (on_listWidgetConstraints_emitHideSelection3DVisibility())
        );
    QObject::connect(
        ui->listWidgetConstraints, SIGNAL(emitShowSelection3DVisibility()),
        this                     , SLOT  (on_listWidgetConstraints_emitShowSelection3DVisibility())
        );
    QObject::connect(
        ui->multipleFilterButton, SIGNAL(clicked(bool)),
                     this                     , SLOT  (on_multipleFilterButton_clicked(bool))
        );
    QObject::connect(
        ui->settingsDialogButton, SIGNAL(clicked(bool)),
                     this                     , SLOT  (on_settingsDialogButton_clicked(bool))
        );
    QObject::connect(
        ui->visibilityButton, SIGNAL(clicked(bool)),
                     this                     , SLOT  (on_visibilityButton_clicked(bool))
        );

    QObject::connect(
        ui->visibilityButton->actions()[0], SIGNAL(changed()),
        this                     , SLOT  (on_visibilityButton_trackingaction_changed())
        );

    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect(
        boost::bind(&SketcherGui::TaskSketcherConstraints::slotConstraintsChanged, this));

    this->groupLayout()->addWidget(proxy);

    multiFilterStatus.set(); // Match 'All' selection, all bits set.

    slotConstraintsChanged();
}

TaskSketcherConstraints::~TaskSketcherConstraints()
{
    connectionConstraintsChanged.disconnect();
}


void TaskSketcherConstraints::createVisibilityButtonActions()
{
    QAction* action = new QAction(QString::fromLatin1("Show only filtered Constraints"),this);

    action->setCheckable(true);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool visibilityTracksFilter = hGrp->GetBool("VisualisationTrackingFilter",false);

    {
        QSignalBlocker block(this);
        action->setChecked(visibilityTracksFilter);
    }

    ui->visibilityButton->addAction(action);
}

void TaskSketcherConstraints::updateSelectionFilter()
{
    // Snapshot current selection
    auto items = ui->listWidgetConstraints->selectedItems();

    selectionFilter.clear();

    for(const auto & item : items)
        selectionFilter.push_back(static_cast<ConstraintItem*>(item)->ConstraintNbr);
}

void TaskSketcherConstraints::updateAssociatedConstraintsFilter()
{
    associatedConstraintsFilter.clear();

    assert(sketchView);

    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Selection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    const Sketcher::SketchObject * Obj = sketchView->getSketchObject();
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    std::vector<std::string> constraintSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            // push all the constraints
            int i = 0;
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it,++i)
            {
                if ((*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId) {
                    associatedConstraintsFilter.push_back(i);
                }
            }
        }
    }

    updateList();
}

void TaskSketcherConstraints::updateList()
{
    // enforce constraint visibility
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool visibilityTracksFilter = hGrp->GetBool("VisualisationTrackingFilter",false);

    if(visibilityTracksFilter)
        change3DViewVisibilityToTrackFilter(); // it will call slotConstraintChanged via update mechanism
    else
        slotConstraintsChanged();
}

void TaskSketcherConstraints::on_multipleFilterButton_clicked(bool)
{
    ConstraintMultiFilterDialog mf;

    int filterindex = ui->comboBoxFilter->currentIndex();

    if(!isFilterMatch(ConstraintFilter::SpecialFilterValue::Multiple, filterindex)) {
        ui->comboBoxFilter->setCurrentIndex(getFilterIntegral(ConstraintFilter::SpecialFilterValue::Multiple)); // Change filter to multi filter selection
    }

    mf.setMultiFilter(multiFilterStatus);

    if (mf.exec() == QDialog::Accepted) {
        multiFilterStatus = mf.getMultiFilter();

        // if tracking, it will call slotConstraintChanged via update mechanism as Multi Filter affects not only visibility, but also filtered list content, if not tracking will still update the list to match the multi-filter.
        updateList();
    }
}

void TaskSketcherConstraints::on_settingsDialogButton_clicked(bool)
{
    ConstraintSettingsDialog cs;

    QObject::connect(
        &cs, SIGNAL(emit_filterInternalAlignment_stateChanged(int)),
        this                     , SLOT  (on_filterInternalAlignment_stateChanged(int))
        );
    QObject::connect(
        &cs, SIGNAL(emit_extendedInformation_stateChanged(int)),
                     this                     , SLOT  (on_extendedInformation_stateChanged(int))
        );
    QObject::connect(
        &cs, SIGNAL(emit_visualisationTrackingFilter_stateChanged(int)),
        this                     , SLOT  (on_visualisationTrackingFilter_stateChanged(int))
        );

    cs.exec(); // The dialog reacted on any change, so the result of running the dialog is already reflected on return.
}

void TaskSketcherConstraints::changeFilteredVisibility(bool show, ActionTarget target)
{
    assert(sketchView);
    const Sketcher::SketchObject * sketch = sketchView->getSketchObject();

    auto selecteditems = ui->listWidgetConstraints->selectedItems();

    std::vector<int> constrIds;

    for(int i = 0; i < ui->listWidgetConstraints->count(); ++i)
    {
        QListWidgetItem* item = ui->listWidgetConstraints->item(i);

        bool processItem = false;

        if(target == ActionTarget::All) {
            processItem = !item->isHidden();
        }
        else if(target == ActionTarget::Selected) {
            if(std::find(selecteditems.begin(), selecteditems.end(), item) != selecteditems.end())
                processItem = true;
        }

        if(processItem) { // The item is shown in the filtered list
            const ConstraintItem *it = dynamic_cast<const ConstraintItem*>(item);

            if (!it)
                continue;

            // must change state is shown and is to be hidden or hidden and must change state is shown
            if((it->isInVirtualSpace() == sketchView->getIsShownVirtualSpace() && !show) ||
               (it->isInVirtualSpace() != sketchView->getIsShownVirtualSpace() && show)) {

                constrIds.push_back(it->ConstraintNbr);
            }
        }
    }

    if(!constrIds.empty()) {

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));

        std::stringstream stream;

        stream << '[';

        for(size_t i = 0; i < constrIds.size()-1; i++) {
            stream << constrIds[i] << ",";
        }
        stream << constrIds[constrIds.size()-1] << ']';

        std::string constrIdList = stream.str();

        try {
            Gui::cmdAppObjectArgs(sketch, "setVirtualSpace(%s, %s)",
                        constrIdList,
                        show?"False":"True");

        }
        catch (const Base::Exception & e) {
            Gui::Command::abortCommand();

            QMessageBox::critical(Gui::MainWindow::getInstance(), tr("Error"),
                        QString::fromLatin1("Impossible to update visibility tracking: ") + QString::fromLatin1(e.what()), QMessageBox::Ok, QMessageBox::Ok);

            return;
        }

        Gui::Command::commitCommand();
    }

}

void TaskSketcherConstraints::on_showAllButton_clicked(bool)
{
    changeFilteredVisibility(true);
}

void TaskSketcherConstraints::on_hideAllButton_clicked(bool)
{
    changeFilteredVisibility(false);
}

void TaskSketcherConstraints::on_listWidgetConstraints_emitHideSelection3DVisibility()
{
    changeFilteredVisibility(false, ActionTarget::Selected);
}

void TaskSketcherConstraints::on_listWidgetConstraints_emitShowSelection3DVisibility()
{
    changeFilteredVisibility(true, ActionTarget::Selected);
}

void TaskSketcherConstraints::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    assert(sketchView);

    std::string temp;
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        ui->listWidgetConstraints->blockSignals(true);
        ui->listWidgetConstraints->clearSelection();
        ui->listWidgetConstraints->blockSignals(false);

        if(isFilter(ConstraintFilter::SpecialFilterValue::Selection)) {
            updateSelectionFilter();

            bool block = this->blockSelection(true); // avoid to be notified by itself
            updateList();
            this->blockSelection(block);
        }
        else if (isFilter(ConstraintFilter::SpecialFilterValue::AssociatedConstraints)) {
            associatedConstraintsFilter.clear();
            updateList();
        }
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
                if (pos > -1) { // is a constraint
                    bool ok;
                    int ConstrId = rx.cap(1).toInt(&ok) - 1;
                    if (ok) {
                        int countItems = ui->listWidgetConstraints->count();
                        for (int i=0; i < countItems; i++) {
                            ConstraintItem* item = static_cast<ConstraintItem*>
                                (ui->listWidgetConstraints->item(i));
                            if (item->ConstraintNbr == ConstrId) {
                                ui->listWidgetConstraints->blockSignals(true);
                                item->setSelected(select);
                                ui->listWidgetConstraints->blockSignals(false);
                                break;
                            }
                        }

                        if(isFilter(ConstraintFilter::SpecialFilterValue::Selection)) {
                            updateSelectionFilter();
                            bool block = this->blockSelection(true); // avoid to be notified by itself
                            updateList();
                            this->blockSelection(block);
                        }
                    }
                }
                else if(isFilter(ConstraintFilter::SpecialFilterValue::AssociatedConstraints)) { // is NOT a constraint
                    int geoid = Sketcher::GeoEnum::GeoUndef;
                    Sketcher::PointPos pointpos = Sketcher::PointPos::none;
                    getSelectionGeoId(expr, geoid, pointpos);

                    if(geoid != Sketcher::GeoEnum::GeoUndef && pointpos == Sketcher::PointPos::none){
                        // It is not possible to update on single addition/removal of a geometric element,
                        // as one removal may imply removing a constraint that should be added by a different element
                        // that is still selected. The necessary checks outweigh a full rebuild of the filter.
                        updateAssociatedConstraintsFilter();
                    }
                }
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here
    }
}

void TaskSketcherConstraints::getSelectionGeoId(QString expr, int & geoid, Sketcher::PointPos & pointpos)
{
    QRegExp rxEdge(QString::fromLatin1("^Edge(\\d+)$"));
    int pos = expr.indexOf(rxEdge);
    geoid = Sketcher::GeoEnum::GeoUndef;
    pointpos = Sketcher::PointPos::none;

    if (pos > -1) {
        bool ok;
        int edgeId = rxEdge.cap(1).toInt(&ok) - 1;
        if (ok) {
            geoid = edgeId;
        }
    }
    else {
        QRegExp rxVertex(QString::fromLatin1("^Vertex(\\d+)$"));
        pos = expr.indexOf(rxVertex);

        if (pos > -1) {
            bool ok;
            int vertexId = rxVertex.cap(1).toInt(&ok) - 1;
            if (ok) {
                const Sketcher::SketchObject * sketch = sketchView->getSketchObject();
                sketch->getGeoVertexIndex(vertexId, geoid, pointpos);
            }
        }
    }
}

void TaskSketcherConstraints::on_comboBoxFilter_currentIndexChanged(int filterindex)
{
    selectionFilter.clear(); // reset the stored selection filter
    associatedConstraintsFilter.clear();

    if(isFilterMatch(ConstraintFilter::SpecialFilterValue::Selection, filterindex)) {
        updateSelectionFilter();
    }
    else if(isFilterMatch(ConstraintFilter::SpecialFilterValue::AssociatedConstraints, filterindex)) {
        updateAssociatedConstraintsFilter();
    }

    updateList();
}

void TaskSketcherConstraints::on_filterInternalAlignment_stateChanged(int state)
{
    Q_UNUSED(state);
    slotConstraintsChanged();
}

void TaskSketcherConstraints::on_visualisationTrackingFilter_stateChanged(int state)
{
    // Synchronise button drop state
    {
        QSignalBlocker block(this);

        if(ui->visibilityButton->actions()[0]->isChecked() != (state == Qt::Checked))
            ui->visibilityButton->actions()[0]->setChecked(state);
    }

    if(state == Qt::Checked)
        change3DViewVisibilityToTrackFilter();
}

void TaskSketcherConstraints::on_visibilityButton_trackingaction_changed()
{
    // synchronise VisualisationTrackingFilter parameter
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool visibilityTracksFilter = hGrp->GetBool("VisualisationTrackingFilter",false);

    bool bstate = ui->visibilityButton->actions()[0]->isChecked();

    if(visibilityTracksFilter != bstate) {
        hGrp->SetBool("VisualisationTrackingFilter", bstate);
    }

    // Act
    if(bstate)
        change3DViewVisibilityToTrackFilter();
}

void TaskSketcherConstraints::on_visibilityButton_clicked(bool)
{
    change3DViewVisibilityToTrackFilter();
}

void TaskSketcherConstraints::on_extendedInformation_stateChanged(int state)
{
    Q_UNUSED(state);
    slotConstraintsChanged();
}

void TaskSketcherConstraints::on_listWidgetConstraints_emitCenterSelectedItems()
{
    sketchView->centerSelection();
}

void TaskSketcherConstraints::on_listWidgetConstraints_itemSelectionChanged(void)
{
    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    bool block = this->blockSelection(true); // avoid to be notified by itself
    Gui::Selection().clearSelection();

    std::vector<std::string> constraintSubNames;
    QList<QListWidgetItem *> items = ui->listWidgetConstraints->selectedItems();
    for (QList<QListWidgetItem *>::iterator it = items.begin(); it != items.end(); ++it) {
        std::string constraint_name(Sketcher::PropertyConstraintList::getConstraintName(static_cast<ConstraintItem*>(*it)->ConstraintNbr));
        constraintSubNames.push_back(constraint_name);
    }

    if(!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);

    this->blockSelection(block);
}

void TaskSketcherConstraints::on_listWidgetConstraints_itemActivated(QListWidgetItem *item)
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

void TaskSketcherConstraints::on_listWidgetConstraints_updateDrivingStatus(QListWidgetItem *item, bool status)
{
    Q_UNUSED(status);
    ConstraintItem *citem = dynamic_cast<ConstraintItem*>(item);
    if (!citem) return;

    Gui::Application::Instance->commandManager().runCommandByName("Sketcher_ToggleDrivingConstraint");
    slotConstraintsChanged();
}

void TaskSketcherConstraints::on_listWidgetConstraints_updateActiveStatus(QListWidgetItem *item, bool status)
{
    Q_UNUSED(status);
    ConstraintItem *citem = dynamic_cast<ConstraintItem*>(item);
    if (!citem) return;

    Gui::Application::Instance->commandManager().runCommandByName("Sketcher_ToggleActiveConstraint");
    slotConstraintsChanged();
}

void TaskSketcherConstraints::on_listWidgetConstraints_itemChanged(QListWidgetItem *item)
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

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Rename sketch constraint"));
        try {
            Gui::cmdAppObjectArgs(sketch ,"renameConstraint(%d, u'%s')",
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
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));
    try {
        Gui::cmdAppObjectArgs(sketch, "setVirtualSpace(%d, %s)",
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

void TaskSketcherConstraints::change3DViewVisibilityToTrackFilter()
{
    assert(sketchView);
    // Build up ListView with the constraints
    const Sketcher::SketchObject * sketch = sketchView->getSketchObject();
    const std::vector< Sketcher::Constraint * > &vals = sketch->Constraints.getValues();

    std::vector<int> constrIdsToVirtualSpace;
    std::vector<int> constrIdsToCurrentSpace;

    for(std::size_t i = 0; i < vals.size(); ++i) {
        ConstraintItem * it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        bool visible = !isConstraintFiltered(it);

        // If the constraint is filteredout and it was previously shown in 3D view
        if( !visible && it->isInVirtualSpace() == sketchView->getIsShownVirtualSpace()) {
            constrIdsToVirtualSpace.push_back(it->ConstraintNbr);
        }
        else if( visible && it->isInVirtualSpace() != sketchView->getIsShownVirtualSpace() ) {
            constrIdsToCurrentSpace.push_back(it->ConstraintNbr);
        }
    }

    if(!constrIdsToVirtualSpace.empty() || !constrIdsToCurrentSpace.empty()) {

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));

        auto doSetVirtualSpace = [&sketch](const std::vector<int> & constrIds, bool isvirtualspace) {
            std::stringstream stream;

            stream << '[';

            for(size_t i = 0; i < constrIds.size()-1; i++) {
                stream << constrIds[i] << ",";
            }
            stream << constrIds[constrIds.size()-1] << ']';

            std::string constrIdList = stream.str();

            try {
                Gui::cmdAppObjectArgs(sketch, "setVirtualSpace(%s, %s)",
                            constrIdList,
                            isvirtualspace?"True":"False");

            }
            catch (const Base::Exception & e) {
                Gui::Command::abortCommand();

                QMessageBox::critical(Gui::MainWindow::getInstance(), tr("Error"),
                            QString::fromLatin1("Impossible to update visibility tracking: ") + QString::fromLatin1(e.what()), QMessageBox::Ok, QMessageBox::Ok);

                return false;
            }

            return true;
        };


        if(!constrIdsToVirtualSpace.empty()) {
            bool ret = doSetVirtualSpace(constrIdsToVirtualSpace, true);
            if(!ret)
                return;
        }

        if(!constrIdsToCurrentSpace.empty()) {
            bool ret = doSetVirtualSpace(constrIdsToCurrentSpace, false);

            if(!ret)
                return;
        }

        Gui::Command::commitCommand();

    }

}

bool TaskSketcherConstraints::isConstraintFiltered(QListWidgetItem * item)
{
    assert(sketchView);
    const Sketcher::SketchObject * sketch = sketchView->getSketchObject();
    const std::vector< Sketcher::Constraint * > &vals = sketch->Constraints.getValues();
    ConstraintItem * it = static_cast<ConstraintItem*>(item);
    const Sketcher::Constraint * constraint = vals[it->ConstraintNbr];

    int Filter = ui->comboBoxFilter->currentIndex();



    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool hideInternalAlignment = hGrp->GetBool("HideInternalAlignment",false);

    bool visible = true;
    bool showAll = isFilterMatch(FilterValue::All, Filter);
    bool showGeometric = isFilterMatch(FilterValue::Geometric, Filter);
    bool showDatums = (isFilterMatch(FilterValue::Datums, Filter) && constraint->isDriving);
    bool showNamed = (isFilterMatch(FilterValue::Named, Filter) && !(constraint->Name.empty()));
    bool showNonDriving = (isFilterMatch(FilterValue::NonDriving, Filter) && !constraint->isDriving);

    auto geometricVisible = [showAll, showGeometric, showNamed, Filter, this](FilterValue filtervalue) {
        return   showAll || showGeometric || showNamed || isFilterMatch(filtervalue, Filter) ||
                (isFilterMatch(SpecialFilterValue::Multiple, Filter) && checkFilterBitset(multiFilterStatus, filtervalue));
    };

    auto datumVisible = [showAll, showDatums, showNamed, showNonDriving, Filter, this](FilterValue filtervalue) {
        return   showAll || showDatums || showNamed || showNonDriving || isFilterMatch(filtervalue, Filter) ||
                (isFilterMatch(SpecialFilterValue::Multiple, Filter) && checkFilterBitset(multiFilterStatus, filtervalue));
    };

    switch(constraint->Type) {
        case Sketcher::Horizontal:
            visible = geometricVisible(FilterValue::Horizontal);
            break;
        case Sketcher::Vertical:
            visible = geometricVisible(FilterValue::Vertical);
            break;
        case Sketcher::Coincident:
            visible = geometricVisible(FilterValue::Coincident);
            break;
        case Sketcher::PointOnObject:
            visible = geometricVisible(FilterValue::PointOnObject);
            break;
        case Sketcher::Parallel:
            visible = geometricVisible(FilterValue::Parallel);
            break;
        case Sketcher::Perpendicular:
            visible = geometricVisible(FilterValue::Perpendicular);
            break;
        case Sketcher::Tangent:
            visible = geometricVisible(FilterValue::Tangent);
            break;
        case Sketcher::Equal:
            visible = geometricVisible(FilterValue::Equality);
            break;
        case Sketcher::Symmetric:
            visible = geometricVisible(FilterValue::Symmetric);
            break;
        case Sketcher::Block:
            visible = geometricVisible(FilterValue::Block);
            break;
        case Sketcher::Distance:
            visible = datumVisible(FilterValue::Distance);
            break;
        case Sketcher::DistanceX:
           visible = datumVisible(FilterValue::HorizontalDistance);
            break;
        case Sketcher::DistanceY:
           visible = datumVisible(FilterValue::VerticalDistance);
            break;
        case Sketcher::Radius:
           visible = datumVisible(FilterValue::Radius);
            break;
        case Sketcher::Weight:
           visible = datumVisible(FilterValue::Weight);
            break;
        case Sketcher::Diameter:
           visible = datumVisible(FilterValue::Diameter);
            break;
        case Sketcher::Angle:
           visible = datumVisible(FilterValue::Angle);
            break;
        case Sketcher::SnellsLaw:
           visible = datumVisible(FilterValue::SnellsLaw);
            break;
        case Sketcher::InternalAlignment:
            visible = geometricVisible(FilterValue::InternalAlignment) &&
                        (!hideInternalAlignment || isFilterMatch(FilterValue::InternalAlignment,Filter));
        default:
            break;
    }

    // Constraint Type independent, selection filter
    visible = visible || (isFilterMatch(SpecialFilterValue::Selection, Filter) &&
                std::find(selectionFilter.begin(), selectionFilter.end(), it->ConstraintNbr) != selectionFilter.end());

    // Constraint Type independent, associated Constraints Filter
    visible = visible || (isFilterMatch(SpecialFilterValue::AssociatedConstraints, Filter) &&
                std::find(associatedConstraintsFilter.begin(), associatedConstraintsFilter.end(), it->ConstraintNbr) != associatedConstraintsFilter.end());

    return !visible;
}

void TaskSketcherConstraints::slotConstraintsChanged(void)
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
    for(std::size_t i = 0; i < vals.size(); ++i) {
        const Sketcher::Constraint * constraint = vals[i];
        ConstraintItem * it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        bool visible = !isConstraintFiltered(it);

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

void TaskSketcherConstraints::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

template <class T>
bool TaskSketcherConstraints::isFilter(T filterValue) {
    return isFilterMatch(filterValue, ui->comboBoxFilter->currentIndex());
}



#include "moc_TaskSketcherConstraints.cpp"
