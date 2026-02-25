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

#pragma once

#include <memory>
#include <QApplication>
#include <QDialog>
#include <QMessageBox>
#include <Gui/propertyeditor/PropertyItem.h>

class QGridLayout;
class QLabel;
class QDoubleSpinBox;
class QComboBox;

namespace Gui
{
class QuantitySpinBox;

class GuiExport LocationWidget: public QWidget
{
    Q_OBJECT

public:
    LocationWidget(QWidget* parent = nullptr);
    ~LocationWidget() override;
    QSize sizeHint() const override;

    Base::Vector3d getPosition() const;
    void setPosition(const Base::Vector3d&);
    void setDirection(const Base::Vector3d& dir);
    Base::Vector3d getDirection() const;
    Base::Vector3d getUserDirection(bool* ok = nullptr) const;

private:
    void onDirectionActivated(int);

private:
    void changeEvent(QEvent*) override;
    void retranslateUi();

private:
    QGridLayout* box;
    QLabel* xLabel;
    QLabel* yLabel;
    QLabel* zLabel;
    QLabel* dLabel;
    QuantitySpinBox* xValue;
    QuantitySpinBox* yValue;
    QuantitySpinBox* zValue;
    QComboBox* dValue;
};

/** This is the abstract base dialog class that defines the interface for
 * specifying a direction vector by the user.
 * @author Werner Mayer
 */
class GuiExport LocationDialog: public QDialog
{
    Q_OBJECT

protected:
    LocationDialog(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~LocationDialog() override;

protected:
    void changeEvent(QEvent* e) override = 0;

private:
    void onDirectionActivated(int);

public:
    virtual Base::Vector3d getDirection() const = 0;
    Base::Vector3d getUserDirection(bool* ok = nullptr) const;

private:
    virtual void directionActivated(int) = 0;
};

/* TRANSLATOR Gui::LocationDialog */

/** This is the template class that implements the interface of LocationDialog.
 * The template argument can be the Ui interface class built by uic out of a
 * .ui file.
 * This class might be very useful for dialogs where a combo box is used to
 * define a direction vector by the user. For such classes the programmer doesn't
 * have to write a subclass to implement the appropriate signals/slots. Instead it's
 * possible to omit this further class and use LocationDialogUi parametrized
 * with the generated Ui class.
 * @author Werner Mayer
 */
template<class Ui>
class LocationDialogUi: public LocationDialog, public Ui
{
public:
    LocationDialogUi(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags())
        : LocationDialog(parent, fl)
    {
        this->setupUi(this);
        this->retranslate();
    }
    ~LocationDialogUi() override = default;

    void retranslate()
    {
        Ui::retranslateUi(this);

        if (this->direction->count() == 0) {
            this->direction->insertItems(
                0,
                QStringList() << QApplication::translate("Gui::LocationDialog", "X")
                              << QApplication::translate("Gui::LocationDialog", "Y")
                              << QApplication::translate("Gui::LocationDialog", "Z")
                              << QApplication::translate("Gui::LocationDialog", "User defined…")
            );

            this->direction->setCurrentIndex(2);

            // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
            this->direction->setItemData(
                0,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1, 0, 0))
            );
            this->direction->setItemData(
                1,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 1, 0))
            );
            this->direction->setItemData(
                2,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 0, 1))
            );
        }
        else {
            this->direction->setItemText(0, QApplication::translate("Gui::LocationDialog", "X"));
            this->direction->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y"));
            this->direction->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z"));
            this->direction->setItemText(
                this->direction->count() - 1,
                QApplication::translate("Gui::LocationDialog", "User defined…")
            );
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
        return Base::Vector3d(
            this->xPos->value().getValue(),
            this->yPos->value().getValue(),
            this->zPos->value().getValue()
        );
    }

    Base::Vector3d getDirection() const override
    {
        QVariant data = this->direction->itemData(this->direction->currentIndex());
        if (data.canConvert<Base::Vector3d>()) {
            return data.value<Base::Vector3d>();
        }
        else {
            return Base::Vector3d(0, 0, 1);
        }
    }

protected:
    void changeEvent(QEvent* e) override
    {
        if (e->type() == QEvent::LanguageChange) {
            this->retranslate();
        }
        else {
            QDialog::changeEvent(e);
        }
    }

private:
    void setDirection(const Base::Vector3d& dir)
    {
        if (dir.Length() < Base::Vector3d::epsilon()) {
            return;
        }

        // check if the user-defined direction is already there
        for (int i = 0; i < this->direction->count() - 1; i++) {
            QVariant data = this->direction->itemData(i);
            if (data.canConvert<Base::Vector3d>()) {
                const auto val = data.value<Base::Vector3d>();
                if (val == dir) {
                    this->direction->setCurrentIndex(i);
                    return;
                }
            }
        }

        // add a new item before the very last item
        QString display = QStringLiteral("(%1,%2,%3)").arg(dir.x).arg(dir.y).arg(dir.z);
        this->direction->insertItem(
            this->direction->count() - 1,
            display,
            QVariant::fromValue<Base::Vector3d>(dir)
        );
        this->direction->setCurrentIndex(this->direction->count() - 2);
    }
    void directionActivated(int index) override
    {
        // last item is selected to define direction by user
        if (index + 1 == this->direction->count()) {
            bool ok;
            Base::Vector3d dir = this->getUserDirection(&ok);
            if (ok) {
                if (dir.Length() < Base::Vector3d::epsilon()) {
                    QMessageBox::critical(
                        this,
                        LocationDialog::tr("Wrong direction"),
                        LocationDialog::tr("Direction must not be the null vector")
                    );
                    return;
                }

                setDirection(dir);
            }
        }
    }
};

/** This template class does basically the same as LocationDialogUi unless
 * that it doesn inherit from a widget but only from the UI_-generated class.
 * Thus, this class can be used as composition in dialog-based classes without
 * including the ui_-generated header file. The Ui_-class can simply be forward
 * declared, then.
 * @author Werner Mayer
 */
template<class Ui>
class LocationUi: public Ui
{
public:
    LocationUi(QDialog* dlg)
    {
        this->setupUi(dlg);
        this->retranslate(dlg);
    }
    ~LocationUi() = default;

    void retranslate(QDialog* dlg)
    {
        Ui::retranslateUi(dlg);

        if (this->direction->count() == 0) {
            this->direction->insertItems(
                0,
                QStringList() << QApplication::translate("Gui::LocationDialog", "X")
                              << QApplication::translate("Gui::LocationDialog", "Y")
                              << QApplication::translate("Gui::LocationDialog", "Z")
                              << QApplication::translate("Gui::LocationDialog", "User defined…")
            );

            this->direction->setCurrentIndex(2);

            // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
            this->direction->setItemData(
                0,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1, 0, 0))
            );
            this->direction->setItemData(
                1,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 1, 0))
            );
            this->direction->setItemData(
                2,
                QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 0, 1))
            );
        }
        else {
            this->direction->setItemText(0, QApplication::translate("Gui::LocationDialog", "X"));
            this->direction->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y"));
            this->direction->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z"));
            this->direction->setItemText(
                this->direction->count() - 1,
                QApplication::translate("Gui::LocationDialog", "User defined…")
            );
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
        return Base::Vector3d(
            this->xPos->value().getValue(),
            this->yPos->value().getValue(),
            this->zPos->value().getValue()
        );
    }

    Base::Vector3d getDirection() const
    {
        QVariant data = this->direction->itemData(this->direction->currentIndex());
        if (data.canConvert<Base::Vector3d>()) {
            return data.value<Base::Vector3d>();
        }
        else {
            return Base::Vector3d(0, 0, 1);
        }
    }

public:
    void setDirection(const Base::Vector3d& dir)
    {
        if (dir.Length() < Base::Vector3d::epsilon()) {
            return;
        }

        // check if the user-defined direction is already there
        for (int i = 0; i < this->direction->count() - 1; i++) {
            QVariant data = this->direction->itemData(i);
            if (data.canConvert<Base::Vector3d>()) {
                const auto val = data.value<Base::Vector3d>();
                if (val == dir) {
                    this->direction->setCurrentIndex(i);
                    return;
                }
            }
        }

        // add a new item before the very last item
        QString display = QStringLiteral("(%1,%2,%3)").arg(dir.x).arg(dir.y).arg(dir.z);
        this->direction->insertItem(
            this->direction->count() - 1,
            display,
            QVariant::fromValue<Base::Vector3d>(dir)
        );
        this->direction->setCurrentIndex(this->direction->count() - 2);
    }
    bool directionActivated(LocationDialog* dlg, int index)
    {
        // last item is selected to define direction by user
        if (index + 1 == this->direction->count()) {
            bool ok;
            Base::Vector3d dir = dlg->getUserDirection(&ok);
            if (ok) {
                if (dir.Length() < Base::Vector3d::epsilon()) {
                    QMessageBox::critical(
                        dlg,
                        LocationDialog::tr("Wrong direction"),
                        LocationDialog::tr("Direction must not be the null vector")
                    );
                    return false;
                }
                setDirection(dir);
            }
        }
        return true;
    }
};

/** This template class is a subclass of LocationDialog using LocationUi
 * and implements the pure virtual methods of its base class.
 * Other dialog-based classes can directly inherit from this class if the
 * location-interface is required. But note, in this case the ui_-header file
 * needs to be included. If this should be avoided the class LocationUi
 * must be used instead of whereas the Ui_-class can be forward declared.
 * @author Werner Mayer
 */
template<class Ui>
class LocationDialogImp: public LocationDialog
{
public:
    LocationDialogImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags())
        : LocationDialog(parent, fl)
        , ui(this)
    {}
    ~LocationDialogImp() override = default;

    Base::Vector3d getDirection() const override
    {
        return ui.getDirection();
    }

protected:
    void changeEvent(QEvent* e) override
    {
        if (e->type() == QEvent::LanguageChange) {
            ui.retranslate(this);
        }
        else {
            QDialog::changeEvent(e);
        }
    }

private:
    void directionActivated(int index) override
    {
        ui.directionActivated(this, index);
    }

protected:
    LocationUi<Ui> ui;
};

/**
 * @brief The AbstractUi class
 * Abstract base class the defines the class interface.
 * @author Werner Mayer
 */
class AbstractUi
{
public:
    virtual ~AbstractUi() = default;
    virtual void retranslate(QDialog* dlg) = 0;
    virtual void setPosition(const Base::Vector3d& v) = 0;
    virtual Base::Vector3d getPosition() const = 0;
    virtual Base::Vector3d getDirection() const = 0;
    virtual void setDirection(const Base::Vector3d& dir) = 0;
    virtual bool directionActivated(LocationDialog* dlg, int index) = 0;
    virtual boost::any get() = 0;
};

/** This is the template class that implements the interface of AbstractUi.
 * The template argument is the Ui interface class built by uic out of a
 * .ui file.
 * @author Werner Mayer
 */
template<class Ui>
class LocationImpUi: public AbstractUi
{
public:
    LocationImpUi(Ui* ui)
        : ui(ui)
    {}
    ~LocationImpUi() override = default;

    boost::any get() override
    {
        return ui;
    }

    void retranslate(QDialog* dlg) override
    {
        ui->retranslateUi(dlg);

        if (ui->direction->count() == 0) {
            ui->direction->insertItems(
                0,
                QStringList() << QApplication::translate("Gui::LocationDialog", "X")
                              << QApplication::translate("Gui::LocationDialog", "Y")
                              << QApplication::translate("Gui::LocationDialog", "Z")
                              << QApplication::translate("Gui::LocationDialog", "User defined…")
            );

            ui->direction->setCurrentIndex(2);

            // Vector3d declared to use with QVariant see Gui/propertyeditor/PropertyItem.h
            ui->direction->setItemData(0, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(1, 0, 0)));
            ui->direction->setItemData(1, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 1, 0)));
            ui->direction->setItemData(2, QVariant::fromValue<Base::Vector3d>(Base::Vector3d(0, 0, 1)));
        }
        else {
            ui->direction->setItemText(0, QApplication::translate("Gui::LocationDialog", "X"));
            ui->direction->setItemText(1, QApplication::translate("Gui::LocationDialog", "Y"));
            ui->direction->setItemText(2, QApplication::translate("Gui::LocationDialog", "Z"));
            ui->direction->setItemText(
                ui->direction->count() - 1,
                QApplication::translate("Gui::LocationDialog", "User defined…")
            );
        }
    }

    void setPosition(const Base::Vector3d& v) override
    {
        ui->xPos->setValue(v.x);
        ui->yPos->setValue(v.y);
        ui->zPos->setValue(v.z);
    }

    Base::Vector3d getPosition() const override
    {
        return Base::Vector3d(
            ui->xPos->value().getValue(),
            ui->yPos->value().getValue(),
            ui->zPos->value().getValue()
        );
    }

    Base::Vector3d getDirection() const override
    {
        QVariant data = ui->direction->itemData(ui->direction->currentIndex());
        if (data.canConvert<Base::Vector3d>()) {
            return data.value<Base::Vector3d>();
        }
        else {
            return Base::Vector3d(0, 0, 1);
        }
    }

public:
    void setDirection(const Base::Vector3d& dir) override
    {
        if (dir.Length() < Base::Vector3d::epsilon()) {
            return;
        }

        // check if the user-defined direction is already there
        for (int i = 0; i < ui->direction->count() - 1; i++) {
            QVariant data = ui->direction->itemData(i);
            if (data.canConvert<Base::Vector3d>()) {
                const auto val = data.value<Base::Vector3d>();
                if (val == dir) {
                    ui->direction->setCurrentIndex(i);
                    return;
                }
            }
        }

        // add a new item before the very last item
        QString display = QStringLiteral("(%1,%2,%3)").arg(dir.x).arg(dir.y).arg(dir.z);
        ui->direction->insertItem(
            ui->direction->count() - 1,
            display,
            QVariant::fromValue<Base::Vector3d>(dir)
        );
        ui->direction->setCurrentIndex(ui->direction->count() - 2);
    }
    bool directionActivated(LocationDialog* dlg, int index) override
    {
        // last item is selected to define direction by user
        if (index + 1 == ui->direction->count()) {
            bool ok;
            Base::Vector3d dir = dlg->getUserDirection(&ok);
            if (ok) {
                if (dir.Length() < Base::Vector3d::epsilon()) {
                    QMessageBox::critical(
                        dlg,
                        LocationDialog::tr("Wrong direction"),
                        LocationDialog::tr("Direction must not be the null vector")
                    );
                    return false;
                }
                setDirection(dir);
            }
        }
        return true;
    }

private:
    std::shared_ptr<Ui> ui;
};

/** This is a subclass of LocationDialog using AbstractUi that implements
 * the pure virtual methods of its base class.
 * Other dialog-based classes can directly inherit from this class if the
 * location-interface is required.
 * The advantage of this class compared to LocationDialogImp is that the
 * ui_-header file doesn't need to be included in the header file of its
 * sub-classes because it uses "type erasure with templates".
 * @author Werner Mayer
 */
class GuiExport LocationDialogUiImp: public LocationDialog
{
public:
    template<class T>
    LocationDialogUiImp(T* t, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags())
        : LocationDialog(parent, fl)
        , ui(new LocationImpUi<T>(t))
    {
        std::shared_ptr<T> uit = boost::any_cast<std::shared_ptr<T>>(ui->get());
        uit->setupUi(this);
        ui->retranslate(this);
    }
    ~LocationDialogUiImp() override;

    Base::Vector3d getDirection() const override;

    Base::Vector3d getPosition() const;

protected:
    void changeEvent(QEvent* e) override;

private:
    void directionActivated(int index) override;

protected:
    std::unique_ptr<AbstractUi> ui;
};

}  // namespace Gui
