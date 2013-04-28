/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_INPUTVECTOR_H
#define GUI_INPUTVECTOR_H

#include <cfloat>
#include <QDialog>
#include <QMessageBox>
#include <QApplication> 

#include <Gui/propertyeditor/PropertyItem.h>

class QGridLayout;
class QLabel;
class QDoubleSpinBox;
class QComboBox;

namespace Gui {

class GuiExport LocationWidget : public QWidget
{
    Q_OBJECT

public:
    LocationWidget (QWidget * parent = 0);
    virtual ~LocationWidget();
    QSize sizeHint() const;

    Base::Vector3d getPosition() const;
    void setPosition(const Base::Vector3d&);
    void setDirection(const Base::Vector3d& dir);
    Base::Vector3d getDirection() const;
    Base::Vector3d getUserDirection(bool* ok=0) const;

private Q_SLOTS:
    void on_direction_activated(int);

private:
    void changeEvent(QEvent*);
    void retranslateUi();

private:
    QGridLayout *box;
    QLabel *xLabel;
    QLabel *yLabel;
    QLabel *zLabel;
    QLabel *dLabel;
    QDoubleSpinBox *xValue;
    QDoubleSpinBox *yValue;
    QDoubleSpinBox *zValue;
    QComboBox *dValue;
};

/** This is the base dialog class that defines the interface for
 * specifying a direction vector by the user.
 * @author Werner Mayer
 */
class GuiExport LocationDialog : public QDialog
{
    Q_OBJECT

protected:
    LocationDialog(QWidget* parent = 0, Qt::WFlags fl = 0);
    virtual ~LocationDialog();

protected:
    virtual void changeEvent(QEvent *e) = 0;

private Q_SLOTS:
    void on_direction_activated(int);

public:
    virtual Base::Vector3d getDirection() const = 0;
    Base::Vector3d getUserDirection(bool* ok=0) const;

private:
    virtual void directionActivated(int) = 0;
};

/* TRANSLATOR Gui::LocationDialog */

/** This is the template class that implements the interface of LocationDialog.
 * The template argument can be the Ui interface class built by uic out of a
 * .ui file.
 * This class might be very useful for dialogs where a combo box is used to
 * define a direction vector by the user. For such classes the programmer don't
 * to write a subclass to implement the appropriate singals/slots. Instead it's
 * possible to omit this further class and use LocationInterface parametrized
 * with the generated Ui class.
 * @author Werner Mayer
 */
template <class Ui>
class LocationInterface : public LocationDialog, public Ui
{
public:
    LocationInterface(QWidget* parent = 0, Qt::WFlags fl = 0)  : LocationDialog(parent, fl)
    {
        this->setupUi(this);
        this->retranslate();
    }
    virtual ~LocationInterface(){}

    void retranslate()
    {
        Ui::retranslateUi(this);

        if (this->direction->count() == 0) {
            this->direction->insertItems(0, QStringList()
             << QApplication::translate("Gui::LocationDialog", "X", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "Y", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "Z", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "User defined...", 0, 
                QApplication::UnicodeUTF8)
            );

            this->direction->setCurrentIndex(2);

            // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
            this->direction->setItemData(0, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1,0,0)));
            this->direction->setItemData(1, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,1,0)));
            this->direction->setItemData(2, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,0,1)));
        }
        else {
            this->direction->setItemText(0, QApplication::translate("Gui::LocationDialog", "X", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(this->direction->count()-1,
                QApplication::translate("Gui::LocationDialog", "User defined...", 0,
                QApplication::UnicodeUTF8));
        }
    }

    Base::Vector3d getPosition() const
    {
        return Base::Vector3d(this->xPos->value(),
                              this->yPos->value(),
                              this->zPos->value());
    }

    Base::Vector3d getDirection() const
    {
        QVariant data = this->direction->itemData (this->direction->currentIndex());
        if (data.canConvert<Base::Vector3d>()) {
            return data.value<Base::Vector3d>();
        }
        else {
            return Base::Vector3d(0,0,1);
        }
    }

protected:
    void changeEvent(QEvent *e)
    {
        if (e->type() == QEvent::LanguageChange) {
            this->retranslate();
        }
        else {
            QDialog::changeEvent(e);
        }
    }

private:
    void directionActivated(int index)
    {
        // last item is selected to define direction by user
        if (index+1 == this->direction->count()) {
            bool ok;
            Base::Vector3d dir = this->getUserDirection(&ok);
            if (ok) {
                if (dir.Length() < FLT_EPSILON) {
                    QMessageBox::critical(this, LocationDialog::tr("Wrong direction"),
                        LocationDialog::tr("Direction must not be the null vector"));
                    return;
                }

                // check if the user-defined direction is already there
                for (int i=0; i<this->direction->count()-1; i++) {
                    QVariant data = this->direction->itemData (i);
                    if (data.canConvert<Base::Vector3d>()) {
                        const Base::Vector3d val = data.value<Base::Vector3d>();
                        if (val == dir) {
                            this->direction->setCurrentIndex(i);
                            return;
                        }
                    }
                }

                // add a new item before the very last item
                QString display = QString::fromAscii("(%1,%2,%3)")
                    .arg(dir.x)
                    .arg(dir.y)
                    .arg(dir.z);
                this->direction->insertItem(this->direction->count()-1, display,
                    QVariant::fromValue<Base::Vector3d>(dir));
                this->direction->setCurrentIndex(this->direction->count()-2);
            }
        }
    }
};

/** This template class does basically the same as LocationInterface unless
 * that the Ui class is used as composition not as further base class.
 * This class acts as a small wrapper class around the UI_-generated classes
 * by Qt for which the location interface is needed. This class can be used
 * as composition in dialog-based classes without including the ui_-generated
 * header file. The Ui_-class can simply be forward declared.
 * @author Werner Mayer
 */
template <class Ui>
class LocationInterfaceComp : public Ui
{
public:
    LocationInterfaceComp(QDialog *dlg)
    {
        this->setupUi(dlg);
        this->retranslate(dlg);
    }
    ~LocationInterfaceComp()
    {
    }

    void retranslate(QDialog *dlg)
    {
        Ui::retranslateUi(dlg);

        if (this->direction->count() == 0) {
            this->direction->insertItems(0, QStringList()
             << QApplication::translate("Gui::LocationDialog", "X", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "Y", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "Z", 0, QApplication::UnicodeUTF8)
             << QApplication::translate("Gui::LocationDialog", "User defined...", 0, 
                QApplication::UnicodeUTF8)
            );

            this->direction->setCurrentIndex(2);

            // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
            this->direction->setItemData(0, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1,0,0)));
            this->direction->setItemData(1, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,1,0)));
            this->direction->setItemData(2, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0,0,1)));
        }
        else {
            this->direction->setItemText(0, QApplication::translate("Gui::LocationDialog", "X", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z", 0,
                QApplication::UnicodeUTF8));
            this->direction->setItemText(this->direction->count()-1,
                QApplication::translate("Gui::LocationDialog", "User defined...", 0,
                QApplication::UnicodeUTF8));
        }
    }

    void setPosition(const Base::Vector3d& v)
    {
        this->xPos->setValue(v.x);
        this->yPos->setValue(v.y);
        this->zPos->setValue(v.z);
    }

    Base::Vector3d getPosition() const
    {
        return Base::Vector3d(this->xPos->value(),
                              this->yPos->value(),
                              this->zPos->value());
    }

    Base::Vector3d getDirection() const
    {
        QVariant data = this->direction->itemData (this->direction->currentIndex());
        if (data.canConvert<Base::Vector3d>()) {
            return data.value<Base::Vector3d>();
        }
        else {
            return Base::Vector3d(0,0,1);
        }
    }

public:
    void setDirection(const Base::Vector3d& dir)
    {
        if (dir.Length() < FLT_EPSILON) {
            return;
        }

        // check if the user-defined direction is already there
        for (int i=0; i<this->direction->count()-1; i++) {
            QVariant data = this->direction->itemData (i);
            if (data.canConvert<Base::Vector3d>()) {
                const Base::Vector3d val = data.value<Base::Vector3d>();
                if (val == dir) {
                    this->direction->setCurrentIndex(i);
                    return;
                }
            }
        }

        // add a new item before the very last item
        QString display = QString::fromAscii("(%1,%2,%3)")
            .arg(dir.x)
            .arg(dir.y)
            .arg(dir.z);
        this->direction->insertItem(this->direction->count()-1, display,
            QVariant::fromValue<Base::Vector3d>(dir));
        this->direction->setCurrentIndex(this->direction->count()-2);
    }
    bool directionActivated(LocationDialog* dlg, int index)
    {
        // last item is selected to define direction by user
        if (index+1 == this->direction->count()) {
            bool ok;
            Base::Vector3d dir = dlg->getUserDirection(&ok);
            if (ok) {
                if (dir.Length() < FLT_EPSILON) {
                    QMessageBox::critical(dlg, LocationDialog::tr("Wrong direction"),
                        LocationDialog::tr("Direction must not be the null vector"));
                    return false;
                }
                setDirection(dir);
            }
        }
        return true;
    }
};

/** This template class is a subclass of LocationDialog using LocationInterfaceComp
 * which implements the pure virtual method directionActivated().
 * Other dialog-based classes can directly inherit from this class if the
 * location-interface is required. But note, in this case the ui_-header file
 * needs to be included. If this should be avoided the class LocationInterfaceComp
 * must be used instead of whereas the Ui_-class can be forward declared.
 * @author Werner Mayer
 */
template <class Ui>
class LocationDialogComp : public LocationDialog
{
public:
    LocationDialogComp(QWidget* parent = 0, Qt::WFlags fl = 0)
      : LocationDialog(parent, fl), ui(this)
    {
    }
    virtual ~LocationDialogComp()
    {
        // no need to delete child widgets, Qt does it all for us
    }

    Base::Vector3d getDirection() const
    {
        return ui.getDirection();
    }

protected:
    void changeEvent(QEvent *e)
    {
        if (e->type() == QEvent::LanguageChange) {
            ui.retranslate(this);
        }
        else {
            QDialog::changeEvent(e);
        }
    }

private:
    void directionActivated(int index)
    {
        ui.directionActivated(this,index);
    }

protected:
    LocationInterfaceComp<Ui> ui;
};

} // namespace Gui

#endif // GUI_INPUTVECTOR_H
