/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Gui/MetaTypes.h>
#include <Gui/SpinBox.h>

#ifdef Q_MOC_RUN
Q_DECLARE_METATYPE(Base::Quantity)
#endif

namespace Gui
{

class QuantitySpinBoxPrivate;
class GuiExport QuantitySpinBox: public QAbstractSpinBox, public ExpressionSpinBox
{
    Q_OBJECT

    Q_PROPERTY(QString unit READ unitText WRITE setUnitText)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        double singleStep READ singleStep WRITE setSingleStep
    )  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(double rawValue READ rawValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(Base::Quantity value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(
        QString binding READ boundToName WRITE setBoundToByName
    )                                                   // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(QString expression READ expressionText)  // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(
        bool autoNormalize READ autoNormalize WRITE setAutoNormalize
    )  // clazy:exclude=qproperty-without-notify

public:
    explicit QuantitySpinBox(QWidget* parent = nullptr);
    ~QuantitySpinBox() override;

    /// Get the current quantity
    Base::Quantity value() const;
    /// Get the current quantity without unit
    double rawValue() const;

    void normalize();
    bool isNormalized();

    /// Gives the current state of the user input, gives true if it is a valid input with correct
    /// quantity or returns false if the input is a unparsable string or has a wrong unit.
    bool hasValidInput() const;

    /** Sets the Unit this widget is working with.
     *  After setting the Unit the widget will only accept
     *  user input with this unit type. Or if the user input
     *  a value without unit, this one will be added to the resulting
     *  Quantity.
     */
    Base::Unit unit() const;
    void setUnit(const Base::Unit& unit);
    /// Set the unit property
    void setUnitText(const QString&);
    /// Get the unit property
    QString unitText();

    /// Get the value of the singleStep property
    double singleStep() const;
    /// Set the value of the singleStep property
    void setSingleStep(double val);

    /// Gets the value of the minimum property
    double minimum() const;
    /// Sets the value of the minimum property
    void setMinimum(double min);

    /// Gets the value of the maximum property
    double maximum() const;
    /// Sets the value of the maximum property
    void setMaximum(double max);

    /// Gets the number of decimals
    int decimals() const;
    /// Sets the number of decimals
    void setDecimals(int v);

    /// Checks if auto normalization is enabled
    bool autoNormalize() const;
    /// Enables or disables automatic normalization on enter
    void setAutoNormalize(bool normalize);

    /// Sets a specific unit schema to handle quantities.
    /// The system-wide schema won't be used any more.
    void setSchema(int s);

    /// Clears the schemaand again use the system-wide schema.
    void clearSchema();

    /// Gets the path of the bound property
    QString boundToName() const;
    /// Sets the path of the bound property
    void setBoundToByName(const QString& path);

    /// Gets the expression as a string
    QString expressionText() const;
    void evaluateExpression();

    /// Set the number portion selected
    void selectNumber();

    void setRange(double min, double max);
    void checkRangeInExpression(bool);
    bool isCheckedRangeInExpresion() const;

    Base::Quantity valueFromText(const QString& text) const;
    QString textFromValue(const Base::Quantity& val) const;
    void stepBy(int steps) override;
    void clear() override;
    QValidator::State validate(QString& input, int& pos) const override;
    void fixup(QString& str) const override;

    /// This is a helper function to determine the size this widget requires to fully display the text
    QSize sizeForText(const QString&) const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool event(QEvent* event) override;

    void setNumberExpression(App::NumberExpression*) override;
    void bind(const App::ObjectIdentifier& _path) override;
    bool apply(const std::string& propName) override;
    using ExpressionSpinBox::apply;

public Q_SLOTS:
    /// Sets the field with a quantity
    void setValue(const Base::Quantity& val);
    /// Set a numerical value which gets converted to a quantity with the currently set unit type
    void setValue(double);

protected Q_SLOTS:
    void userInput(const QString& text);
    void handlePendingEmit(bool updateUnit = true);

protected:
    void setExpression(std::shared_ptr<App::Expression> expr) override;
    void openFormulaDialog() override;
    void showIcon() override;
    StepEnabled stepEnabled() const override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void validateInput() override;
    void updateText(const Base::Quantity&);
    void updateEdit(const QString& text);
    void updateFromCache(bool notify, bool updateUnit = true);
    QString getUserString(const Base::Quantity& val, double& factor, QString& unitString) const;
    QString getUserString(const Base::Quantity& val) const;
    QSize sizeHintCalculator(int height) const;

Q_SIGNALS:
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(const Base::Quantity&);  // clazy:exclude=overloaded-signal
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(double);  // clazy:exclude=overloaded-signal
    /**
     * The new value is passed in \a text with unit.
     */
    void textChanged(const QString&);
    /** Gets emitted if formula dialog is about to be opened (true)
     *  or finished (false).
     */
    void showFormulaDialog(bool);
    /** Gets emitted if user confirms the value with return. This
     *  is very similar to editingFinished() but does not fire on
     *  focus out.
     */
    void returnPressed();

private:
    QScopedPointer<QuantitySpinBoxPrivate> d_ptr;
    Q_DISABLE_COPY(QuantitySpinBox)
    Q_DECLARE_PRIVATE(QuantitySpinBox)
};

}  // namespace Gui
