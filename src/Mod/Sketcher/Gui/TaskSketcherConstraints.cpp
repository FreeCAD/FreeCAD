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
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPixmapCache>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QStyledItemDelegate>
#include <QWidgetAction>
#include <boost/core/ignore_unused.hpp>
#include <cmath>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "EditDatumDialog.h"
#include "TaskSketcherConstraints.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ui_TaskSketcherConstraints.h"


// clang-format off
using namespace SketcherGui;
using namespace Gui::TaskView;
namespace sp = std::placeholders;

// Translation block for context menu: do not remove
#if 0
QT_TRANSLATE_NOOP("SketcherGui::ConstraintView", "Select Elements");
#endif

/// Inserts a QAction into an existing menu
/// ICONSTR is the string of the icon in the resource file
/// NAMESTR is the text appearing in the contextual menuAction
/// CMDSTR is the string registered in the commandManager
/// FUNC is the name of the member function to be executed on selection of the menu item
/// ACTSONSELECTION is a true/false value to activate the command only if a selection is made
#define CONTEXT_ITEM(ICONSTR, NAMESTR, CMDSTR, FUNC, ACTSONSELECTION)                              \
    QIcon icon_##FUNC(Gui::BitmapFactory().pixmap(ICONSTR));                                       \
    QAction* constr_##FUNC = menu.addAction(icon_##FUNC, tr(NAMESTR), this, SLOT(FUNC()));         \
    constr_##FUNC->setShortcut(QKeySequence(QString::fromUtf8(                                     \
        Gui::Application::Instance->commandManager().getCommandByName(CMDSTR)->getAccel())));      \
    if (ACTSONSELECTION)                                                                           \
        constr_##FUNC->setEnabled(!items.isEmpty());                                               \
    else                                                                                           \
        constr_##FUNC->setEnabled(true);
/// Defines the member function corresponding to the CONTEXT_ITEM macro
#define CONTEXT_MEMBER_DEF(CMDSTR, FUNC)                                                           \
    void ConstraintView::FUNC()                                                                    \
    {                                                                                              \
        Gui::Application::Instance->commandManager().runCommandByName(CMDSTR);                     \
    }

// helper class to store additional information about the listWidget entry.
class ConstraintItem: public QListWidgetItem
{
public:
    ConstraintItem(const Sketcher::SketchObject* s, ViewProviderSketch* sketchview, int ConstNbr)
        : QListWidgetItem(QString())
        , sketch(s)
        , sketchView(sketchview)
        , ConstraintNbr(ConstNbr)
    {
        this->setFlags(this->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);

        updateVirtualSpaceStatus();
    }
    ~ConstraintItem() override
    {}
    void setData(int role, const QVariant& value) override
    {
        if (role == Qt::EditRole)
            this->value = value;

        QListWidgetItem::setData(role, value);
    }

    QVariant data(int role) const override
    {
        if (ConstraintNbr < 0 || ConstraintNbr >= sketch->Constraints.getSize())
            return QVariant();

        const Sketcher::Constraint* constraint = sketch->Constraints[ConstraintNbr];

        // it can happen that the geometry of the sketch is tmp. invalid and thus
        // the index operator returns null.
        if (!constraint) {
            return QVariant();
        }

        if (role == Qt::EditRole) {
            if (value.isValid())
                return value;
            else
                return Base::Tools::fromStdString(
                    Sketcher::PropertyConstraintList::getConstraintName(constraint->Name,
                                                                        ConstraintNbr));
        }
        else if (role == Qt::DisplayRole) {
            QString name =
                Base::Tools::fromStdString(Sketcher::PropertyConstraintList::getConstraintName(
                    constraint->Name, ConstraintNbr));

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
                    name = QString::fromLatin1("%1 (%2)").arg(
                        name, constraint->getPresentationValue().getUserString());
                    break;
                case Sketcher::SnellsLaw: {
                    double v = constraint->getPresentationValue().getValue();
                    double n1 = 1.0;
                    double n2 = 1.0;
                    if (fabs(v) >= 1) {
                        n2 = v;
                    }
                    else {
                        n1 = 1 / v;
                    }
                    name = QString::fromLatin1("%1 (%2/%3)").arg(name).arg(n2).arg(n1);
                    break;
                }
                case Sketcher::InternalAlignment:
                    break;
                default:
                    break;
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool extended = hGrp->GetBool("ExtendedConstraintInformation", false);

            if (extended) {
                if (constraint->Second == Sketcher::GeoEnum::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3)]")
                               .arg(name)
                               .arg(constraint->First)
                               .arg(static_cast<int>(constraint->FirstPos));
                }
                else if (constraint->Third == Sketcher::GeoEnum::GeoUndef) {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5)]")
                               .arg(name)
                               .arg(constraint->First)
                               .arg(static_cast<int>(constraint->FirstPos))
                               .arg(constraint->Second)
                               .arg(static_cast<int>(constraint->SecondPos));
                }
                else {
                    name = QString::fromLatin1("%1 [(%2,%3),(%4,%5),(%6,%7)]")
                               .arg(name)
                               .arg(constraint->First)
                               .arg(static_cast<int>(constraint->FirstPos))
                               .arg(constraint->Second)
                               .arg(static_cast<int>(constraint->SecondPos))
                               .arg(constraint->Third)
                               .arg(static_cast<int>(constraint->ThirdPos));
                }
            }

            return name;
        }
        else if (role == Qt::DecorationRole) {
            static QIcon hdist(Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance"));
            static QIcon vdist(Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance"));
            static QIcon horiz(Gui::BitmapFactory().iconFromTheme("Constraint_Horizontal"));
            static QIcon vert(Gui::BitmapFactory().iconFromTheme("Constraint_Vertical"));
            // static QIcon lock ( Gui::BitmapFactory().iconFromTheme("Constraint_Lock") );
            static QIcon block(Gui::BitmapFactory().iconFromTheme("Constraint_Block"));
            static QIcon coinc(Gui::BitmapFactory().iconFromTheme("Constraint_PointOnPoint"));
            static QIcon para(Gui::BitmapFactory().iconFromTheme("Constraint_Parallel"));
            static QIcon perp(Gui::BitmapFactory().iconFromTheme("Constraint_Perpendicular"));
            static QIcon tang(Gui::BitmapFactory().iconFromTheme("Constraint_Tangent"));
            static QIcon dist(Gui::BitmapFactory().iconFromTheme("Constraint_Length"));
            static QIcon radi(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
            static QIcon dia(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            // static QIcon majradi (
            // Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Major_Radius") ); static QIcon
            // minradi ( Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Minor_Radius") );
            static QIcon angl(Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle"));
            // static QIcon ellipseXUAngl (
            // Gui::BitmapFactory().iconFromTheme("Constraint_Ellipse_Axis_Angle") );
            static QIcon equal(Gui::BitmapFactory().iconFromTheme("Constraint_EqualLength"));
            static QIcon pntoo(Gui::BitmapFactory().iconFromTheme("Constraint_PointOnObject"));
            static QIcon symm(Gui::BitmapFactory().iconFromTheme("Constraint_Symmetric"));
            static QIcon snell(Gui::BitmapFactory().iconFromTheme("Constraint_SnellsLaw"));
            static QIcon iaellipseminoraxis(Gui::BitmapFactory().iconFromTheme(
                "Constraint_InternalAlignment_Ellipse_MinorAxis"));
            static QIcon iaellipsemajoraxis(Gui::BitmapFactory().iconFromTheme(
                "Constraint_InternalAlignment_Ellipse_MajorAxis"));
            static QIcon iaellipsefocus1(
                Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_Focus1"));
            static QIcon iaellipsefocus2(
                Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment_Ellipse_Focus2"));
            static QIcon iaellipseother(
                Gui::BitmapFactory().iconFromTheme("Constraint_InternalAlignment"));

            static QIcon hdist_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance_Driven"));
            static QIcon vdist_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance_Driven"));
            static QIcon dist_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_Length_Driven"));
            static QIcon radi_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
            static QIcon dia_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            static QIcon angl_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle_Driven"));
            static QIcon snell_driven(
                Gui::BitmapFactory().iconFromTheme("Constraint_SnellsLaw_Driven"));

            auto selicon = [](const Sketcher::Constraint* constr,
                              const QIcon& normal,
                              const QIcon& driven) -> QIcon {
                if (!constr->isActive) {
                    QIcon darkIcon;
                    int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
                    darkIcon.addPixmap(normal.pixmap(w, w, QIcon::Disabled, QIcon::Off),
                                       QIcon::Normal,
                                       QIcon::Off);
                    darkIcon.addPixmap(
                        normal.pixmap(w, w, QIcon::Disabled, QIcon::On), QIcon::Normal, QIcon::On);
                    return darkIcon;
                }
                else if (constr->isDriving) {
                    return normal;
                }
                else {
                    return driven;
                }
            };

            switch (constraint->Type) {
                case Sketcher::Horizontal:
                    return selicon(constraint, horiz, horiz);
                case Sketcher::Vertical:
                    return selicon(constraint, vert, vert);
                case Sketcher::Coincident:
                    return selicon(constraint, coinc, coinc);
                case Sketcher::Block:
                    return selicon(constraint, block, block);
                case Sketcher::PointOnObject:
                    return selicon(constraint, pntoo, pntoo);
                case Sketcher::Parallel:
                    return selicon(constraint, para, para);
                case Sketcher::Perpendicular:
                    return selicon(constraint, perp, perp);
                case Sketcher::Tangent:
                    return selicon(constraint, tang, tang);
                case Sketcher::Equal:
                    return selicon(constraint, equal, equal);
                case Sketcher::Symmetric:
                    return selicon(constraint, symm, symm);
                case Sketcher::Distance:
                    return selicon(constraint, dist, dist_driven);
                case Sketcher::DistanceX:
                    return selicon(constraint, hdist, hdist_driven);
                case Sketcher::DistanceY:
                    return selicon(constraint, vdist, vdist_driven);
                case Sketcher::Radius:
                case Sketcher::Weight:
                    return selicon(constraint, radi, radi_driven);
                case Sketcher::Diameter:
                    return selicon(constraint, dia, dia_driven);
                case Sketcher::Angle:
                    return selicon(constraint, angl, angl_driven);
                case Sketcher::SnellsLaw:
                    return selicon(constraint, snell, snell_driven);
                case Sketcher::InternalAlignment:
                    switch (constraint->AlignmentType) {
                        case Sketcher::EllipseMajorDiameter:
                            return selicon(constraint, iaellipsemajoraxis, iaellipsemajoraxis);
                        case Sketcher::EllipseMinorDiameter:
                            return selicon(constraint, iaellipseminoraxis, iaellipseminoraxis);
                        case Sketcher::EllipseFocus1:
                            return selicon(constraint, iaellipsefocus1, iaellipsefocus1);
                        case Sketcher::EllipseFocus2:
                            return selicon(constraint, iaellipsefocus2, iaellipsefocus2);
                        case Sketcher::Undef:
                        default:
                            return selicon(constraint, iaellipseother, iaellipseother);
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

    Sketcher::ConstraintType constraintType() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());
        return sketch->Constraints[ConstraintNbr]->Type;
    }

    bool isEnforceable() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        const Sketcher::Constraint* constraint = sketch->Constraints[ConstraintNbr];

        switch (constraint->Type) {
            case Sketcher::None:
            case Sketcher::NumConstraintTypes:
                assert(false);
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
                return (constraint->First >= 0 || constraint->Second >= 0
                        || constraint->Third >= 0);
            case Sketcher::InternalAlignment:
                return true;
        }
        return false;
    }

    bool isDimensional() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return (sketch->Constraints[ConstraintNbr])->isDimensional();
    }

    bool isDriving() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isDriving;
    }

    bool isInVirtualSpace() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isInVirtualSpace;
    }

    bool isActive() const
    {
        assert(ConstraintNbr >= 0 && ConstraintNbr < sketch->Constraints.getSize());

        return sketch->Constraints[ConstraintNbr]->isActive;
    }

    void updateVirtualSpaceStatus()
    {
        this->setCheckState((this->isInVirtualSpace() != sketchView->getIsShownVirtualSpace())
                                ? Qt::Unchecked
                                : Qt::Checked);
    }

    const Sketcher::SketchObject* sketch;
    const ViewProviderSketch* sketchView;
    int ConstraintNbr;
    QVariant value;
};

class ExpressionDelegate: public QStyledItemDelegate
{
public:
    explicit ExpressionDelegate(QListWidget* _view)
        : QStyledItemDelegate(_view)
        , view(_view)
    {}

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

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

        ConstraintItem* item = dynamic_cast<ConstraintItem*>(view->item(index.row()));
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

    QListWidget* view;
};

/* ConstraintView list widget ------------------------------*/
ConstraintView::ConstraintView(QWidget* parent)
    : QListWidget(parent)
{
    ExpressionDelegate* delegate = new ExpressionDelegate(this);
    setItemDelegate(delegate);
}

ConstraintView::~ConstraintView()
{}

void ConstraintView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu;
    QListWidgetItem* item = currentItem();
    QList<QListWidgetItem*> items = selectedItems();

    // Cancel any in-progress operation
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    bool didRelease = SketcherGui::ReleaseHandler(doc);

    // Sync the FreeCAD selection with the selection in the ConstraintView widget
    if (didRelease && item) {
        Gui::Selection().clearSelection();
        std::string doc_name = static_cast<ConstraintItem*>(item)
                                   ->sketchView->getSketchObject()
                                   ->getDocument()
                                   ->getName();
        std::string obj_name =
            static_cast<ConstraintItem*>(item)->sketchView->getSketchObject()->getNameInDocument();

        std::vector<std::string> constraintSubNames;
        for (auto&& it : items) {
            auto ci = static_cast<ConstraintItem*>(it);
            std::string constraint_name =
                Sketcher::PropertyConstraintList::getConstraintName(ci->ConstraintNbr);
            constraintSubNames.emplace_back(constraint_name.c_str());
        }

        if (!constraintSubNames.empty())
            Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);
    }

    bool isQuantity = false;
    bool isToggleDriving = false;
    bool isActive = true;

    // Non-driving-constraints/measurements
    ConstraintItem* it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        // if its the right constraint
        if (it->isDimensional()) {

            isQuantity = true;
            if (it->isEnforceable())
                isToggleDriving = true;
        }

        isActive = it->isActive();
    }

    // This does the same as a double-click and thus it should be the first action and with bold
    // text
    QAction* change = menu.addAction(tr("Change value"), this, &ConstraintView::modifyCurrentItem);
    change->setEnabled(isQuantity);
    menu.setDefaultAction(change);

    QAction* driven =
        menu.addAction(tr("Toggle to/from reference"), this, &ConstraintView::updateDrivingStatus);
    driven->setEnabled(isToggleDriving);

    QAction* activate = menu.addAction(
        isActive ? tr("Deactivate") : tr("Activate"), this, &ConstraintView::updateActiveStatus);
    activate->setEnabled(!items.isEmpty());

    menu.addSeparator();
    QAction* show = menu.addAction(tr("Show constraints"), this, &ConstraintView::showConstraints);
    show->setEnabled(!items.isEmpty());
    QAction* hide = menu.addAction(tr("Hide constraints"), this, &ConstraintView::hideConstraints);
    hide->setEnabled(!items.isEmpty());

    menu.addSeparator();
    CONTEXT_ITEM("Sketcher_SelectElementsAssociatedWithConstraints",
                 "Select Elements",
                 "Sketcher_SelectElementsAssociatedWithConstraints",
                 doSelectConstraints,
                 true)

    QAction* rename = menu.addAction(tr("Rename"), this, &ConstraintView::renameCurrentItem);
#ifndef Q_OS_MAC// on Mac F2 doesn't seem to trigger an edit signal
    rename->setShortcut(QKeySequence(Qt::Key_F2));
#endif
    rename->setEnabled(item != nullptr);

    QAction* center =
        menu.addAction(tr("Center sketch"), this, &ConstraintView::centerSelectedItems);
    center->setEnabled(item != nullptr);

    QAction* remove = menu.addAction(tr("Delete"), this, &ConstraintView::deleteSelectedItems);
    remove->setShortcut(QKeySequence(QKeySequence::Delete));
    remove->setEnabled(!items.isEmpty());

    QAction* swap = menu.addAction(
        tr("Swap constraint names"), this, &ConstraintView::swapNamedOfSelectedItems);
    swap->setEnabled(items.size() == 2);

    menu.exec(event->globalPos());
}

CONTEXT_MEMBER_DEF("Sketcher_SelectElementsAssociatedWithConstraints", doSelectConstraints)

void ConstraintView::updateDrivingStatus()
{
    QListWidgetItem* item = currentItem();

    ConstraintItem* it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        Q_EMIT onUpdateDrivingStatus(item, !it->isDriving());
    }
}

void ConstraintView::updateActiveStatus()
{
    QListWidgetItem* item = currentItem();

    ConstraintItem* it = dynamic_cast<ConstraintItem*>(item);
    if (it) {
        Q_EMIT onUpdateActiveStatus(item, !it->isActive());
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
    Q_EMIT itemActivated(currentItem());
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
    if (!doc)
        return;

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
    QList<QListWidgetItem*> items = selectedItems();

    if (items.size() != 2)
        return;

    ConstraintItem* item1 = static_cast<ConstraintItem*>(items[0]);
    std::string escapedstr1 = Base::Tools::escapedUnicodeFromUtf8(
        item1->sketch->Constraints[item1->ConstraintNbr]->Name.c_str());
    ConstraintItem* item2 = static_cast<ConstraintItem*>(items[1]);
    std::string escapedstr2 = Base::Tools::escapedUnicodeFromUtf8(
        item2->sketch->Constraints[item2->ConstraintNbr]->Name.c_str());

    // In commit 67800ec8c (21 Jul 2015) the implementation of
    // on_listWidgetConstraints_itemChanged() has changed ensuring that a name of a constraint
    // cannot be reset any more. This leads to some inconsistencies when trying to swap "empty"
    // names.
    //
    // If names are empty then nothing should be done
    if (escapedstr1.empty() || escapedstr2.empty()) {
        Gui::TranslatedUserWarning(item1->sketch,
                                   tr("Unnamed constraint"),
                                   tr("Only the names of named constraints can be swapped."));

        return;
    }

    std::stringstream ss;
    ss << "DummyConstraint" << rand();
    std::string tmpname = ss.str();

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Swap constraint names"));
    Gui::cmdAppObjectArgs(
        item1->sketch, "renameConstraint(%d, u'%s')", item1->ConstraintNbr, tmpname.c_str());
    Gui::cmdAppObjectArgs(
        item2->sketch, "renameConstraint(%d, u'%s')", item2->ConstraintNbr, escapedstr1.c_str());
    Gui::cmdAppObjectArgs(
        item1->sketch, "renameConstraint(%d, u'%s')", item1->ConstraintNbr, escapedstr2.c_str());
    Gui::Command::commitCommand();
}

/* Filter constraints list widget ----------------------*/
ConstraintFilterList::ConstraintFilterList(QWidget* parent)
    : QListWidget(parent)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    int filterState = hGrp->GetInt("ConstraintFilterState",
                                   INT_MAX);// INT_MAX = 1111111111111111111111111111111 in binary.

    normalFilterCount = filterItems.size() - 2;// All filter but selected and associated
    selectedFilterIndex = normalFilterCount;
    associatedFilterIndex = normalFilterCount + 1;

    for (auto const& filterItem : filterItems) {
        Q_UNUSED(filterItem);
        auto it = new QListWidgetItem();

        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        addItem(it);
        bool isChecked = static_cast<bool>(filterState & 1);// get the first bit of filterState
        it->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
        filterState = filterState >> 1;// shift right to get rid of the used bit.
    }
    languageChange();

    setPartiallyChecked();
}

ConstraintFilterList::~ConstraintFilterList()
{}

void ConstraintFilterList::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
        languageChange();

    QWidget::changeEvent(e);
}

void ConstraintFilterList::languageChange()
{
    assert(static_cast<int>(filterItems.size()) == count());
    int i = 0;
    for (auto const& filterItem : filterItems) {
        auto text = QStringLiteral("  ").repeated(filterItem.second - 1)
            + (filterItem.second > 0 ? QStringLiteral("- ") : QStringLiteral(""))
            + tr(filterItem.first);
        item(i++)->setText(text);
    }
}

void ConstraintFilterList::setPartiallyChecked()
{
    /* If a group is partially checked or unchecked then we apply Qt::PartiallyChecked.
    The for-loop index is starting from the end. This way sub-groups are first set, which enables
    the bigger group to be set correctly after. Example: If we go from 0 to count, then the loop
    starts at 'All' group, which check state of 'Geometric' which is not updated yet.*/
    for (int i = normalFilterCount - 1; i >= 0; i--) {
        bool mustBeChecked = true;
        bool mustBeUnchecked = true;
        int numberOfFilterInGroup = 0;

        for (int j = 0; j < FilterValueLength; j++) {
            if (i == j)
                continue;

            if (filterAggregates[i][j]) {// if it is in group
                numberOfFilterInGroup++;
                mustBeChecked = mustBeChecked && item(j)->checkState() == Qt::Checked;
                mustBeUnchecked = mustBeUnchecked && item(j)->checkState() == Qt::Unchecked;
            }
        }
        if (numberOfFilterInGroup > 1) {// avoid groups of single filters.
            if (mustBeChecked)
                item(i)->setCheckState(Qt::Checked);
            else if (mustBeUnchecked)
                item(i)->setCheckState(Qt::Unchecked);
            else
                item(i)->setCheckState(Qt::PartiallyChecked);
        }
    }
}

FilterValueBitset ConstraintFilterList::getMultiFilter()
{
    FilterValueBitset tmpBitset;

    for (int i = 0; i < normalFilterCount; i++) {
        QListWidgetItem* it = item(i);

        if (it->checkState() == Qt::Checked)
            tmpBitset.set(i);
    }

    return tmpBitset;
}

// ----------------------------------------------------------------------------

TaskSketcherConstraints::TaskSketcherConstraints(ViewProviderSketch* sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"), tr("Constraints"), true, nullptr)
    , sketchView(sketchView)
    , inEditMode(false)
    , ui(new Ui_TaskSketcherConstraints)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    ui->listWidgetConstraints->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetConstraints->setEditTriggers(QListWidget::EditKeyPressed);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    ui->filterBox->setChecked(hGrp->GetBool("ConstraintFilterEnabled", true));
    ui->filterButton->setEnabled(ui->filterBox->isChecked());

    // Create filter button
    QWidgetAction* action = new QWidgetAction(this);
    filterList = new ConstraintFilterList(this);
    action->setDefaultWidget(filterList);
    qAsConst(ui->filterButton)->addAction(action);

    // Create local settings menu
    // FIXME there is probably a smarter way to handle this menu
    // FIXME translations aren't updated automatically at language change
    QAction* action1 = new QAction(tr("Auto constraints"), this);
    QAction* action2 = new QAction(tr("Auto remove redundants"), this);
    QAction* action3 = new QAction(tr("Show only filtered Constraints"), this);
    QAction* action4 = new QAction(tr("Extended information (in widget)"), this);
    QAction* action5 = new QAction(tr("Hide internal alignment (in widget)"), this);

    action1->setCheckable(true);
    action2->setCheckable(true);
    action3->setCheckable(true);
    action4->setCheckable(true);
    action5->setCheckable(true);

    hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    {
        QSignalBlocker block(this);
        action1->setChecked(sketchView->Autoconstraints.getValue());
        action2->setChecked(hGrp->GetBool("AutoRemoveRedundants", false));
        action3->setChecked(hGrp->GetBool("VisualisationTrackingFilter", false));
        action4->setChecked(hGrp->GetBool("ExtendedConstraintInformation", false));
        action5->setChecked(hGrp->GetBool("HideInternalAlignment", false));
    }
    hGrp->Attach(this);

    auto settingsBut = qAsConst(ui->settingsButton);

    settingsBut->addAction(action1);
    settingsBut->addAction(action2);
    settingsBut->addAction(action3);
    settingsBut->addAction(action4);
    settingsBut->addAction(action5);

    // connect needed signals

    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::itemSelectionChanged,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsItemSelectionChanged);
    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::itemActivated,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsItemActivated);
    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::itemChanged,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsItemChanged);
    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::emitCenterSelectedItems,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsEmitCenterSelectedItems);
    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::onUpdateDrivingStatus,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsUpdateDrivingStatus);
    QObject::connect(ui->listWidgetConstraints,
                     &ConstraintView::onUpdateActiveStatus,
                     this,
                     &TaskSketcherConstraints::onListWidgetConstraintsUpdateActiveStatus);
    QObject::connect(
        ui->listWidgetConstraints,
        &ConstraintView::emitHideSelection3DVisibility,
        this,
        &TaskSketcherConstraints::onListWidgetConstraintsEmitHideSelection3DVisibility);
    QObject::connect(
        ui->listWidgetConstraints,
        &ConstraintView::emitShowSelection3DVisibility,
        this,
        &TaskSketcherConstraints::onListWidgetConstraintsEmitShowSelection3DVisibility);
    QObject::connect(ui->filterBox,
                     &QCheckBox::stateChanged,
                     this,
                     &TaskSketcherConstraints::onFilterBoxStateChanged);
    QObject::connect(
        ui->filterButton, &QToolButton::clicked, ui->filterButton, &QToolButton::showMenu);
    QObject::connect(ui->showHideButton,
                     &QToolButton::clicked,
                     this,
                     &TaskSketcherConstraints::onShowHideButtonClicked);
    QObject::connect(
        ui->settingsButton, &QToolButton::clicked, ui->settingsButton, &QToolButton::showMenu);
    QObject::connect(action1,
                     &QAction::triggered,// 'triggered' is emitted only on user action. This is
                                         // defensive. See if 'toggled' is needed
                     this,
                     &TaskSketcherConstraints::onSettingsAutoConstraintsChanged);
    QObject::connect(action2,
                     &QAction::triggered,
                     this,
                     &TaskSketcherConstraints::onSettingsAutoRemoveRedundantChanged);
    QObject::connect(action3,
                     &QAction::triggered,
                     this,
                     &TaskSketcherConstraints::onSettingsRestrictVisibilityChanged);
    QObject::connect(action4,
                     &QAction::triggered,
                     this,
                     &TaskSketcherConstraints::onSettingsExtendedInformationChanged);
    QObject::connect(action5,
                     &QAction::triggered,
                     this,
                     &TaskSketcherConstraints::onSettingsHideInternalAligmentChanged);
    QObject::connect(filterList,
                     &ConstraintFilterList::itemChanged,
                     this,
                     &TaskSketcherConstraints::onFilterListItemChanged);

    //NOLINTBEGIN
    connectionConstraintsChanged = sketchView->signalConstraintsChanged.connect(
        std::bind(&SketcherGui::TaskSketcherConstraints::slotConstraintsChanged, this));
    //NOLINTEND

    this->groupLayout()->addWidget(proxy);

    multiFilterStatus = filterList->getMultiFilter();

    ui->listWidgetConstraints->setStyleSheet(QString::fromLatin1("margin-top: 0px"));

    //NOLINTBEGIN
    Gui::Application* app = Gui::Application::Instance;
    changedSketchView = app->signalChangedObject.connect(
        std::bind(&TaskSketcherConstraints::onChangedSketchView, this, sp::_1, sp::_2));
    //NOLINTEND

    slotConstraintsChanged();// Populate constraints list
    // Initialize special filters
    for (int i = filterList->normalFilterCount; i < filterList->count(); i++) {
        onFilterListItemChanged(filterList->item(i));
    }
}

TaskSketcherConstraints::~TaskSketcherConstraints()
{
    connectionConstraintsChanged.disconnect();
    App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher")
        ->Detach(this);
}

void TaskSketcherConstraints::onSettingsExtendedInformationChanged(bool value)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    if (hGrp->GetBool("ExtendedConstraintInformation", false) != value) {
        hGrp->SetBool("ExtendedConstraintInformation", value);
    }

    slotConstraintsChanged();
}

void TaskSketcherConstraints::onSettingsHideInternalAligmentChanged(bool value)
{
    // synchronise  parameter
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    if (hGrp->GetBool("HideInternalAlignment", false) != value) {
        hGrp->SetBool("HideInternalAlignment", value);
    }

    slotConstraintsChanged();
}

void TaskSketcherConstraints::onSettingsRestrictVisibilityChanged(bool value)
{
    // synchronise VisualisationTrackingFilter parameter
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    if (hGrp->GetBool("VisualisationTrackingFilter", false) != value) {
        hGrp->SetBool("VisualisationTrackingFilter", value);
    }

    // Act
    if (value)
        change3DViewVisibilityToTrackFilter();
}

void TaskSketcherConstraints::onSettingsAutoConstraintsChanged(bool value)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    Base::ConnectionBlocker block(changedSketchView);
    sketchView->Autoconstraints.setValue(value);
}

void TaskSketcherConstraints::onSettingsAutoRemoveRedundantChanged(bool value)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    if (hGrp->GetBool("AutoRemoveRedundants", false) != value) {
        hGrp->SetBool("AutoRemoveRedundants", value);
    }
}

void TaskSketcherConstraints::onChangedSketchView(const Gui::ViewProvider& vp,
                                                  const App::Property& prop)
{
    if (sketchView == &vp) {
        if (&sketchView->Autoconstraints == &prop) {
            QSignalBlocker block(qAsConst(ui->settingsButton)->actions()[0]);
            qAsConst(ui->settingsButton)
                ->actions()[0]
                ->setChecked(sketchView->Autoconstraints.getValue());
        }
    }
}

void TaskSketcherConstraints::onFilterBoxStateChanged(int val)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetBool("ConstraintFilterEnabled", val == Qt::Checked);

    ui->filterButton->setEnabled(val == Qt::Checked);
    updateList();
}

/*hide all show all button  =====================================================*/
void TaskSketcherConstraints::onShowHideButtonClicked(bool val)
{
    Q_UNUSED(val)
    bool allSelected = true;
    for (int i = 0; i < ui->listWidgetConstraints->count(); ++i) {
        QListWidgetItem* it = ui->listWidgetConstraints->item(i);
        if (!(it->isHidden()) && it->checkState() == Qt::Unchecked) {
            allSelected = false;
            break;
        }
    }
    changeFilteredVisibility(!allSelected);
}

/* Right click functionalities for constraint list view =========================*/
void TaskSketcherConstraints::onListWidgetConstraintsEmitHideSelection3DVisibility()
{
    changeFilteredVisibility(false, ActionTarget::Selected);
}
void TaskSketcherConstraints::onListWidgetConstraintsEmitShowSelection3DVisibility()
{
    changeFilteredVisibility(true, ActionTarget::Selected);
}

void TaskSketcherConstraints::changeFilteredVisibility(bool show, ActionTarget target)
{
    assert(sketchView);
    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();

    auto selecteditems = ui->listWidgetConstraints->selectedItems();

    std::vector<int> constrIds;

    for (int i = 0; i < ui->listWidgetConstraints->count(); ++i) {
        QListWidgetItem* item = ui->listWidgetConstraints->item(i);

        bool processItem = false;

        if (target == ActionTarget::All) {
            processItem = !item->isHidden();
        }
        else if (target == ActionTarget::Selected) {
            if (std::find(selecteditems.begin(), selecteditems.end(), item) != selecteditems.end())
                processItem = true;
        }

        if (processItem) {// The item is shown in the filtered list
            const ConstraintItem* it = dynamic_cast<const ConstraintItem*>(item);

            if (!it)
                continue;

            // must change state is shown and is to be hidden or hidden and must change state is
            // shown
            if ((it->isInVirtualSpace() == sketchView->getIsShownVirtualSpace() && !show)
                || (it->isInVirtualSpace() != sketchView->getIsShownVirtualSpace() && show)) {

                constrIds.push_back(it->ConstraintNbr);
            }
        }
    }

    if (!constrIds.empty()) {

        Gui::Command::openCommand(
            QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));

        std::stringstream stream;

        stream << '[';

        for (size_t i = 0; i < constrIds.size() - 1; i++) {
            stream << constrIds[i] << ",";
        }
        stream << constrIds[constrIds.size() - 1] << ']';

        std::string constrIdList = stream.str();

        try {
            Gui::cmdAppObjectArgs(
                sketch, "setVirtualSpace(%s, %s)", constrIdList, show ? "False" : "True");
        }
        catch (const Base::Exception&) {
            Gui::Command::abortCommand();

            Gui::TranslatedUserError(
                sketch, tr("Error"), tr("Impossible to update visibility tracking"));

            return;
        }

        Gui::Command::commitCommand();
    }
}

void TaskSketcherConstraints::onListWidgetConstraintsUpdateDrivingStatus(QListWidgetItem* item,
                                                                         bool status)
{
    Q_UNUSED(status);
    ConstraintItem* citem = dynamic_cast<ConstraintItem*>(item);
    if (!citem)
        return;

    Gui::Application::Instance->commandManager().runCommandByName(
        "Sketcher_ToggleDrivingConstraint");
    slotConstraintsChanged();
}

void TaskSketcherConstraints::onListWidgetConstraintsUpdateActiveStatus(QListWidgetItem* item,
                                                                        bool status)
{
    Q_UNUSED(status);
    ConstraintItem* citem = dynamic_cast<ConstraintItem*>(item);
    if (!citem)
        return;

    Gui::Application::Instance->commandManager().runCommandByName(
        "Sketcher_ToggleActiveConstraint");
    slotConstraintsChanged();
}

void TaskSketcherConstraints::onListWidgetConstraintsItemActivated(QListWidgetItem* item)
{
    ConstraintItem* it = dynamic_cast<ConstraintItem*>(item);
    if (!it)
        return;

    // if its the right constraint
    if (it->isDimensional()) {
        EditDatumDialog* editDatumDialog = new EditDatumDialog(this->sketchView, it->ConstraintNbr);
        editDatumDialog->exec(false);
        delete editDatumDialog;
    }
}

void TaskSketcherConstraints::onListWidgetConstraintsItemChanged(QListWidgetItem* item)
{
    const ConstraintItem* it = dynamic_cast<const ConstraintItem*>(item);
    if (!it || inEditMode)
        return;

    inEditMode = true;

    assert(sketchView);

    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const std::vector<Sketcher::Constraint*>& vals = sketch->Constraints.getValues();
    const Sketcher::Constraint* v = vals[it->ConstraintNbr];
    const std::string currConstraintName = v->Name;

    const std::string basename = Base::Tools::toStdString(it->data(Qt::EditRole).toString());

    std::string newName(
        Sketcher::PropertyConstraintList::getConstraintName(basename, it->ConstraintNbr));

    // we only start a rename if we are really sure the name has changed, which is:
    // a) that the name generated by the constraints is different from the text in the widget item
    // b) that the text in the widget item, basename, is not ""
    // otherwise a checkbox change will trigger a rename on the first execution, bloating the
    // constraint icons with the default constraint name "constraint1, constraint2"
    if (newName != currConstraintName && !basename.empty()) {
        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(newName.c_str());

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Rename sketch constraint"));
        try {
            Gui::cmdAppObjectArgs(
                sketch, "renameConstraint(%d, u'%s')", it->ConstraintNbr, escapedstr.c_str());
            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Gui::Command::abortCommand();

            Gui::NotifyUserError(
                sketch, QT_TRANSLATE_NOOP("Notifications", "Value Error"), e.what());
        }
    }

    // update constraint virtual space status
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));
    try {
        Gui::cmdAppObjectArgs(
            sketch,
            "setVirtualSpace(%d, %s)",
            it->ConstraintNbr,
            ((item->checkState() == Qt::Checked) != sketchView->getIsShownVirtualSpace()) ? "False"
                                                                                          : "True");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        Gui::Command::abortCommand();

        Gui::NotifyUserError(sketch, QT_TRANSLATE_NOOP("Notifications", "Value Error"), e.what());
    }

    inEditMode = false;
}

void TaskSketcherConstraints::updateSelectionFilter()
{
    // Snapshot current selection
    auto items = ui->listWidgetConstraints->selectedItems();

    selectionFilter.clear();

    for (const auto& item : items)
        selectionFilter.push_back(static_cast<ConstraintItem*>(item)->ConstraintNbr);
}

void TaskSketcherConstraints::updateAssociatedConstraintsFilter()
{
    associatedConstraintsFilter.clear();

    assert(sketchView);

    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Selection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    const Sketcher::SketchObject* Obj = sketchView->getSketchObject();
    const std::vector<Sketcher::Constraint*>& vals = Obj->Constraints.getValues();

    std::vector<std::string> constraintSubNames;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
         ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0, 4) == "Edge") {
            int GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;

            // push all the constraints
            int i = 0;
            for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                 it != vals.end();
                 ++it, ++i) {
                if ((*it)->First == GeoId || (*it)->Second == GeoId || (*it)->Third == GeoId) {
                    associatedConstraintsFilter.push_back(i);
                }
            }
        }
    }
}

void TaskSketcherConstraints::updateList()
{
    multiFilterStatus =
        filterList->getMultiFilter();// moved here in case the filter is changed programmatically.

    // enforce constraint visibility
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool visibilityTracksFilter = hGrp->GetBool("VisualisationTrackingFilter", false);

    if (visibilityTracksFilter)
        change3DViewVisibilityToTrackFilter();// it will call slotConstraintChanged via update
                                              // mechanism
    else
        slotConstraintsChanged();
}

void TaskSketcherConstraints::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    assert(sketchView);

    std::string temp;
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        auto tmpBlock = ui->listWidgetConstraints->blockSignals(true);
        ui->listWidgetConstraints->clearSelection();
        ui->listWidgetConstraints->blockSignals(tmpBlock);

        if (specialFilterMode == SpecialFilterType::Selected) {
            updateSelectionFilter();

            bool block = this->blockSelection(true);// avoid to be notified by itself
            updateList();
            this->blockSelection(block);
        }
        else if (specialFilterMode == SpecialFilterType::Associated) {
            associatedConstraintsFilter.clear();
            updateList();
        }
    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection
             || msg.Type == Gui::SelectionChanges::RmvSelection) {
        bool select = (msg.Type == Gui::SelectionChanges::AddSelection);
        // is it this object??
        if (strcmp(msg.pDocName, sketchView->getSketchObject()->getDocument()->getName()) == 0
            && strcmp(msg.pObjectName, sketchView->getSketchObject()->getNameInDocument()) == 0) {
            if (msg.pSubName) {
                QRegularExpression rx(QString::fromLatin1("^Constraint(\\d+)$"));
                QRegularExpressionMatch match;
                QString expr = QString::fromLatin1(msg.pSubName);
                boost::ignore_unused(expr.indexOf(rx, 0, &match));
                if (match.hasMatch()) {// is a constraint
                    bool ok;
                    int ConstrId = match.captured(1).toInt(&ok) - 1;
                    if (ok) {
                        int countItems = ui->listWidgetConstraints->count();
                        for (int i = 0; i < countItems; i++) {
                            ConstraintItem* item =
                                static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));
                            if (item->ConstraintNbr == ConstrId) {
                                auto tmpBlock = ui->listWidgetConstraints->blockSignals(true);
                                item->setSelected(select);
                                ui->listWidgetConstraints->blockSignals(tmpBlock);
                                break;
                            }
                        }

                        if (specialFilterMode == SpecialFilterType::Selected) {
                            updateSelectionFilter();
                            bool block =
                                this->blockSelection(true);// avoid to be notified by itself
                            updateList();
                            this->blockSelection(block);
                        }
                    }
                }
                else if (specialFilterMode == SpecialFilterType::Associated) {// is NOT a constraint
                    int geoid = Sketcher::GeoEnum::GeoUndef;
                    Sketcher::PointPos pointpos = Sketcher::PointPos::none;
                    getSelectionGeoId(expr, geoid, pointpos);

                    if (geoid != Sketcher::GeoEnum::GeoUndef
                        && pointpos == Sketcher::PointPos::none) {
                        // It is not possible to update on single addition/removal of a geometric
                        // element, as one removal may imply removing a constraint that should be
                        // added by a different element that is still selected. The necessary checks
                        // outweigh a full rebuild of the filter.
                        updateAssociatedConstraintsFilter();
                        updateList();
                    }
                }
            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here
    }
}

void TaskSketcherConstraints::OnChange(Base::Subject<const char*>& rCaller, const char* rcReason)
{
    Q_UNUSED(rCaller);
    int actNum = -1;
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    if (strcmp(rcReason, "AutoRemoveRedundants") == 0) {
        actNum = 1;
    }
    else if (strcmp(rcReason, "VisualisationTrackingFilter") == 0) {
        actNum = 2;
    }
    else if (strcmp(rcReason, "ExtendedConstraintInformation") == 0) {
        actNum = 3;
    }
    else if (strcmp(rcReason, "HideInternalAlignment") == 0) {
        actNum = 4;
    }
    if (actNum >= 0) {
        assert(actNum < static_cast<int>(ui->settingsButton->actions().size()));
        qAsConst(ui->settingsButton)->actions()[actNum]->setChecked(hGrp->GetBool(rcReason, false));
    }
}

void TaskSketcherConstraints::getSelectionGeoId(QString expr, int& geoid,
                                                Sketcher::PointPos& pointpos)
{
    QRegularExpression rxEdge(QString::fromLatin1("^Edge(\\d+)$"));
    QRegularExpressionMatch match;
    boost::ignore_unused(expr.indexOf(rxEdge, 0, &match));
    geoid = Sketcher::GeoEnum::GeoUndef;
    pointpos = Sketcher::PointPos::none;

    if (match.hasMatch()) {
        bool ok;
        int edgeId = match.captured(1).toInt(&ok) - 1;
        if (ok) {
            geoid = edgeId;
        }
    }
    else {
        QRegularExpression rxVertex(QString::fromLatin1("^Vertex(\\d+)$"));
        boost::ignore_unused(expr.indexOf(rxVertex, 0, &match));

        if (match.hasMatch()) {
            bool ok;
            int vertexId = match.captured(1).toInt(&ok) - 1;
            if (ok) {
                const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
                sketch->getGeoVertexIndex(vertexId, geoid, pointpos);
            }
        }
    }
}

void TaskSketcherConstraints::onListWidgetConstraintsEmitCenterSelectedItems()
{
    sketchView->centerSelection();
}

void TaskSketcherConstraints::onListWidgetConstraintsItemSelectionChanged()
{
    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    bool block = this->blockSelection(true);// avoid to be notified by itself
    Gui::Selection().clearSelection();

    std::vector<std::string> constraintSubNames;
    QList<QListWidgetItem*> items = ui->listWidgetConstraints->selectedItems();
    for (QList<QListWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
        std::string constraint_name(Sketcher::PropertyConstraintList::getConstraintName(
            static_cast<ConstraintItem*>(*it)->ConstraintNbr));
        constraintSubNames.push_back(constraint_name);
    }

    if (!constraintSubNames.empty())
        Gui::Selection().addSelections(doc_name.c_str(), obj_name.c_str(), constraintSubNames);

    this->blockSelection(block);
}

void TaskSketcherConstraints::change3DViewVisibilityToTrackFilter()
{
    assert(sketchView);
    // Build up ListView with the constraints
    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const std::vector<Sketcher::Constraint*>& vals = sketch->Constraints.getValues();

    std::vector<int> constrIdsToVirtualSpace;
    std::vector<int> constrIdsToCurrentSpace;

    for (std::size_t i = 0; i < vals.size(); ++i) {
        ConstraintItem* it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        bool visible = !isConstraintFiltered(it);

        // If the constraint is filteredout and it was previously shown in 3D view
        if (!visible && it->isInVirtualSpace() == sketchView->getIsShownVirtualSpace()) {
            constrIdsToVirtualSpace.push_back(it->ConstraintNbr);
        }
        else if (visible && it->isInVirtualSpace() != sketchView->getIsShownVirtualSpace()) {
            constrIdsToCurrentSpace.push_back(it->ConstraintNbr);
        }
    }

    if (!constrIdsToVirtualSpace.empty() || !constrIdsToCurrentSpace.empty()) {

        Gui::Command::openCommand(
            QT_TRANSLATE_NOOP("Command", "Update constraint's virtual space"));

        auto doSetVirtualSpace = [&sketch](const std::vector<int>& constrIds, bool isvirtualspace) {
            std::stringstream stream;

            stream << '[';

            for (size_t i = 0; i < constrIds.size() - 1; i++) {
                stream << constrIds[i] << ",";
            }
            stream << constrIds[constrIds.size() - 1] << ']';

            std::string constrIdList = stream.str();

            try {
                Gui::cmdAppObjectArgs(sketch,
                                      "setVirtualSpace(%s, %s)",
                                      constrIdList,
                                      isvirtualspace ? "True" : "False");
            }
            catch (const Base::Exception&) {
                Gui::Command::abortCommand();

                Gui::TranslatedUserError(
                    sketch, tr("Error"), tr("Impossible to update visibility tracking: "));

                return false;
            }

            return true;
        };


        if (!constrIdsToVirtualSpace.empty()) {
            bool ret = doSetVirtualSpace(constrIdsToVirtualSpace, true);
            if (!ret)
                return;
        }

        if (!constrIdsToCurrentSpace.empty()) {
            bool ret = doSetVirtualSpace(constrIdsToCurrentSpace, false);

            if (!ret)
                return;
        }

        Gui::Command::commitCommand();
    }
}

bool TaskSketcherConstraints::isConstraintFiltered(QListWidgetItem* item)
{
    assert(sketchView);
    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const std::vector<Sketcher::Constraint*>& vals = sketch->Constraints.getValues();
    ConstraintItem* it = static_cast<ConstraintItem*>(item);
    const Sketcher::Constraint* constraint = vals[it->ConstraintNbr];


    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool hideInternalAlignment = hGrp->GetBool("HideInternalAlignment", false);

    bool visible = true;

    if (ui->filterBox->checkState() == Qt::Checked) {
        // First select only the filtered one.
        switch (constraint->Type) {
            case Sketcher::Horizontal:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Horizontal);
                break;
            case Sketcher::Vertical:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Vertical);
                break;
            case Sketcher::Coincident:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Coincident);
                break;
            case Sketcher::PointOnObject:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::PointOnObject);
                break;
            case Sketcher::Parallel:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Parallel);
                break;
            case Sketcher::Perpendicular:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Perpendicular);
                break;
            case Sketcher::Tangent:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Tangent);
                break;
            case Sketcher::Equal:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Equality);
                break;
            case Sketcher::Symmetric:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Symmetric);
                break;
            case Sketcher::Block:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Block);
                break;
            case Sketcher::Distance:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Distance);
                break;
            case Sketcher::DistanceX:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::HorizontalDistance);
                break;
            case Sketcher::DistanceY:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::VerticalDistance);
                break;
            case Sketcher::Radius:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Radius);
                break;
            case Sketcher::Weight:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Weight);
                break;
            case Sketcher::Diameter:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Diameter);
                break;
            case Sketcher::Angle:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::Angle);
                break;
            case Sketcher::SnellsLaw:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::SnellsLaw);
                break;
            case Sketcher::InternalAlignment:
                visible = checkFilterBitset(multiFilterStatus, FilterValue::InternalAlignment)
                    && !hideInternalAlignment;
            default:
                break;
        }

        // Then we re-filter based on selected/associated if such mode selected.
        if (visible && specialFilterMode == SpecialFilterType::Selected) {
            visible = (std::find(selectionFilter.begin(), selectionFilter.end(), it->ConstraintNbr)
                       != selectionFilter.end());
        }
        else if (visible && specialFilterMode == SpecialFilterType::Associated) {
            visible = (std::find(associatedConstraintsFilter.begin(),
                                 associatedConstraintsFilter.end(),
                                 it->ConstraintNbr)
                       != associatedConstraintsFilter.end());
        }
    }
    else if (constraint->Type == Sketcher::InternalAlignment) {
        visible = !hideInternalAlignment;
    }

    return !visible;
}

void TaskSketcherConstraints::slotConstraintsChanged()
{
    assert(sketchView);
    // Build up ListView with the constraints
    const Sketcher::SketchObject* sketch = sketchView->getSketchObject();
    const std::vector<Sketcher::Constraint*>& vals = sketch->Constraints.getValues();

    /* Update constraint number and virtual space check status */
    for (int i = 0; i < ui->listWidgetConstraints->count(); ++i) {
        ConstraintItem* it = dynamic_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        assert(it);

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
    auto tmpBlock = ui->listWidgetConstraints->blockSignals(true);
    for (int i = 0; i < ui->listWidgetConstraints->count(); ++i) {
        ConstraintItem* it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));
        it->updateVirtualSpaceStatus();
    }
    ui->listWidgetConstraints->blockSignals(tmpBlock);

    /* Update filtering */
    for (std::size_t i = 0; i < vals.size(); ++i) {
        const Sketcher::Constraint* constraint = vals[i];
        ConstraintItem* it = static_cast<ConstraintItem*>(ui->listWidgetConstraints->item(i));

        bool visible = !isConstraintFiltered(it);

        // block signals as there is no need to invoke the
        // on_listWidgetConstraints_itemChanged() slot in
        // case a name has changed because this function gets
        // called after changing the constraint list property
        QAbstractItemModel* model = ui->listWidgetConstraints->model();
        auto tmpBlock = model->blockSignals(true);
        it->setHidden(!visible);
        it->setData(Qt::EditRole, Base::Tools::fromStdString(constraint->Name));
        model->blockSignals(tmpBlock);
    }
}

void TaskSketcherConstraints::changeEvent(QEvent* e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/* list for multi filter */
void TaskSketcherConstraints::onFilterListItemChanged(QListWidgetItem* item)
{
    int filterindex = filterList->row(item);

    auto tmpBlock = filterList->blockSignals(true);

    if (filterindex < filterList->normalFilterCount) {

        auto itemAggregate = filterAggregates[filterindex];

        /*First, if this is a group, we need to set the same state to all of its children.
        ie any filter comprised on the filter of the activated item, gets the same check state.*/
        for (int i = 0; i < filterList->normalFilterCount; i++) {
            if (itemAggregate[i])
                filterList->item(i)->setCheckState(item->checkState());
        }

        /* Now we also need to see if any modified group is all checked or all unchecked and set
         * their status accordingly*/
        filterList->setPartiallyChecked();
    }
    else if (filterindex == filterList->selectedFilterIndex) {// Selected constraints
        if (item->checkState() == Qt::Checked) {
            specialFilterMode = SpecialFilterType::Selected;
            filterList->item(filterList->associatedFilterIndex)
                ->setCheckState(Qt::Unchecked);// Disable 'associated'
            updateSelectionFilter();
        }
        else
            specialFilterMode = SpecialFilterType::None;
    }
    else {// Associated constraints
        if (item->checkState() == Qt::Checked) {
            specialFilterMode = SpecialFilterType::Associated;
            filterList->item(filterList->selectedFilterIndex)
                ->setCheckState(Qt::Unchecked);// Disable 'selected'
            updateAssociatedConstraintsFilter();
        }
        else
            specialFilterMode = SpecialFilterType::None;
    }

    filterList->blockSignals(tmpBlock);

    // Save the state of the filter.
    int filterState = 0;
    for (int i = filterList->count() - 1; i >= 0; i--) {
        bool isChecked = filterList->item(i)->checkState() == Qt::Checked;
        filterState = filterState << 1;// we shift left first, else the list is shifted at the end.
        filterState = filterState | isChecked;
    }
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    hGrp->SetInt("ConstraintFilterState", filterState);

    // if tracking, it will call slotConstraintChanged via update mechanism as Multi Filter affects
    // not only visibility, but also filtered list content, if not tracking will still update the
    // list to match the multi-filter.
    updateList();
}


#include "moc_TaskSketcherConstraints.cpp"
// clang-format on
