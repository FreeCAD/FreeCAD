/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef TESTGUI_UNITTESTIMP_H
#define TESTGUI_UNITTESTIMP_H

#include "ui_UnitTest.h"

#include <Base/PyObjectBase.h>

namespace TestGui {

class UnitTestDialog : public QDialog, public Ui_UnitTest
{
    Q_OBJECT

public:
    void showErrorDialog(const char* title, const char* message);
    void addUnitTest(const QString& unit);
    void setUnitTest(const QString& unit);
    QString getUnitTest() const;
    void setStatusText(const QString& text);
    void setProgressFraction(float fraction, const QString& = QString::null);
    void clearErrorList();
    void insertError(const QString& failure, const QString& details);
    void setRunCount(int);
    void setFailCount(int);
    void setErrorCount(int);
    void setRemainCount(int);
    void reset();
    void reject();

    static UnitTestDialog* instance();
    static void destruct();
    static bool hasInstance();

protected:
    UnitTestDialog(QWidget* parent = 0, Qt::WFlags f = 0);
    ~UnitTestDialog();
    void setProgressColor(const QColor& col);

public Q_SLOTS:
    void on_treeViewFailure_itemDoubleClicked (QTreeWidgetItem * item, int column);
    void on_helpButton_clicked();
    void on_aboutButton_clicked();
    void on_startButton_clicked();

private:
    static UnitTestDialog* _instance;
};

} // namespace TestGui


#endif // TESTGUI_UNITTESTIMP_H

