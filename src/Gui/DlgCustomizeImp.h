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


#ifndef GUI_DIALOG_DLGCUSTOMIZE_IMP_H
#define GUI_DIALOG_DLGCUSTOMIZE_IMP_H

#include <QDialog>

class QTabWidget;
class QGridLayout;
class QHBoxLayout;

namespace Gui {
namespace Dialog {

/**
 * Dialog which contains several tab pages to customize
 * the changeable toolbars and commandbars or to define
 * own macro actions.
 *
 * You can extend the existing toolbars or commandbars with
 * several commands just by drag and drop.
 * @see DlgCustomCommandsImp
 * @see DlgCustomToolbarsImp
 * @see DlgCustomCmdbarsImp
 * @see DlgCustomActionsImp
 * \author Werner Mayer
 */
class DlgCustomizeImp : public QDialog
{
    Q_OBJECT

public:
    explicit DlgCustomizeImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgCustomizeImp() override;

    static void addPage(const char* className);
    void addPage (QWidget* w);

Q_SIGNALS:
    void addMacroAction(const QByteArray&);
    void removeMacroAction(const QByteArray&);
    void modifyMacroAction(const QByteArray&);

protected:
    void changeEvent(QEvent *e) override;

private:
    /** @name for internal use only */
    //@{
    QPushButton* buttonHelp; /**< the help button */
    QPushButton* buttonClose; /**< the cancel button */
    QTabWidget* tabWidget; /**< tab widgets containing all pages */
    QGridLayout* customLayout; /**< layout */
    QHBoxLayout* layout; /** horizontal layout */
    static QList<QByteArray> _pages; /**< Name of all registered preference pages */
    //@}
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGCUSTOMIZE_IMP_H
