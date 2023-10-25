/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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


#ifndef GUI_CUSTOMWIDGETS_H
#define GUI_CUSTOMWIDGETS_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QWidget>

namespace Base
{
class Exception
{
};
class Unit
{
public:
    Unit();
    Unit(const QString&);
    bool isEmpty() const;
    bool operator==(const Unit&);
    bool operator!=(const Unit&);
    const QString& getString() const;

private:
    QString unit;
};

struct QuantityFormat
{
    enum NumberOption
    {
        None = 0x00,
        OmitGroupSeparator = 0x01,
        RejectGroupSeparator = 0x02
    };
    enum NumberFormat
    {
        Default = 0,
        Fixed = 1,
        Scientific = 2
    };

    using NumberOptions = int;
    NumberOptions option;
    NumberFormat format;
    int precision;
    int denominator;

    // Default denominator of minimum fractional inch. Only used in certain
    // schemas.
    static int defaultDenominator;  // i.e 8 for 1/8"

    static inline int getDefaultDenominator()
    {
        return defaultDenominator;
    }

    static inline void setDefaultDenominator(int denom)
    {
        defaultDenominator = denom;
    }

    inline int getDenominator() const
    {
        return denominator;
    }

    inline void setDenominator(int denom)
    {
        denominator = denom;
    }
    QuantityFormat();
    inline char toFormat() const
    {
        switch (format) {
            case Fixed:
                return 'f';
            case Scientific:
                return 'e';
            default:
                return 'g';
        }
    }
    static inline NumberFormat toFormat(char c, bool* ok = 0)
    {
        if (ok) {
            *ok = true;
        }
        switch (c) {
            case 'f':
                return Fixed;
            case 'e':
                return Scientific;
            case 'g':
                return Default;
            default:
                if (ok) {
                    *ok = false;
                }
                return Default;
        }
    }
};

class Quantity
{
public:
    Quantity(void);
    explicit Quantity(double Value, const Unit& unit = Unit());
    static Quantity parse(const QString&);
    void setValue(double);
    double getValue() const;
    void setUnit(const Unit&);
    Unit getUnit() const;
    const QuantityFormat& getFormat() const
    {
        return format;
    }
    void setFormat(const QuantityFormat& f)
    {
        format = f;
    }
    QString getUserString() const;
    QString getUserString(double& factor, QString& unitString) const;

private:
    double value;
    Unit unit;
    QuantityFormat format;
};
}  // namespace Base

Q_DECLARE_METATYPE(Base::Quantity)

namespace Gui
{

class UrlLabel: public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl)

public:
    UrlLabel(QWidget* parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~UrlLabel();

    QString url() const;

public Q_SLOTS:
    void setUrl(const QString& u);

protected:
    void mouseReleaseEvent(QMouseEvent*);

private:
    QString _url;
};

class LocationWidget: public QWidget
{
    Q_OBJECT

public:
    LocationWidget(QWidget* parent = 0);
    virtual ~LocationWidget();
    QSize sizeHint() const;

public Q_SLOTS:

private:
    void changeEvent(QEvent*);
    void retranslateUi();

private:
    QGridLayout* box;
    QLabel* xLabel;
    QLabel* yLabel;
    QLabel* zLabel;
    QLabel* dLabel;
    QDoubleSpinBox* xValue;
    QDoubleSpinBox* yValue;
    QDoubleSpinBox* zValue;
    QComboBox* dValue;
};

/**
 * There is a bug in QtDesigner of Qt version 4.0, 4.1 and 4.2. If a class declaration
 * is inside a namespace and it uses the Q_ENUM macro then QtDesigner doesn't handle
 * the enum(s) correctly in its property editor. This bug is fixed since Qt 4.3.0.
 */
class FileChooser: public QWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        File,
        Directory
    };

private:
    Q_ENUM(Mode)
    Q_PROPERTY(Mode mode READ mode WRITE setMode)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(QString filter READ filter WRITE setFilter)
    Q_PROPERTY(QString buttonText READ buttonText WRITE setButtonText)

public:
    FileChooser(QWidget* parent = 0);
    virtual ~FileChooser();


    QString filter() const;
    QString fileName() const;
    Mode mode() const;
    QString buttonText() const;

public Q_SLOTS:
    void setFileName(const QString& fn);
    void setMode(FileChooser::Mode m);
    void setFilter(const QString&);
    void setButtonText(const QString&);

Q_SIGNALS:
    void fileNameChanged(const QString&);
    void fileNameSelected(const QString&);

private Q_SLOTS:
    void chooseFile();

private:
    QLineEdit* lineEdit;
    QPushButton* button;
    Mode md;
    QString _filter;
};

// ------------------------------------------------------------------------------

class PrefFileChooser: public FileChooser
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefFileChooser(QWidget* parent = 0);
    virtual ~PrefFileChooser();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class AccelLineEdit: public QLineEdit
{
    Q_OBJECT

public:
    AccelLineEdit(QWidget* parent = 0);

protected:
    void keyPressEvent(QKeyEvent* e);
};

// ------------------------------------------------------------------------------

class ActionSelector: public QWidget
{
    Q_OBJECT

public:
    ActionSelector(QWidget* parent = 0);
    ~ActionSelector();

private:
    QGridLayout* gridLayout;
    QVBoxLayout* vboxLayout;
    QVBoxLayout* vboxLayout1;
    QPushButton* addButton;
    QPushButton* removeButton;
    QPushButton* upButton;
    QPushButton* downButton;
    QLabel* labelAvailable;
    QLabel* labelSelected;
    QTreeWidget* availableWidget;
    QTreeWidget* selectedWidget;
    QSpacerItem* spacerItem;
    QSpacerItem* spacerItem1;
};

// ------------------------------------------------------------------------------

class InputField: public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int historySize READ historySize WRITE setHistorySize)
    Q_PROPERTY(QString unit READ getUnitText WRITE setUnitText)
    Q_PROPERTY(double quantity READ getQuantity WRITE setValue)

public:
    InputField(QWidget* parent = 0);
    virtual ~InputField();

    void setValue(double);
    double getQuantity(void) const;
    double singleStep(void) const;
    void setSingleStep(double);
    double maximum(void) const;
    void setMaximum(double);
    double minimum(void) const;
    void setMinimum(double);
    int historySize(void) const;
    void setHistorySize(int);
    void setUnitText(QString);
    QString getUnitText(void);
    QByteArray paramGrpPath() const;
    void setParamGrpPath(const QByteArray& name);

Q_SIGNALS:
    void valueChanged(const Base::Quantity&);
    void valueChanged(double);
    void parseError(const QString& errorText);

private:
    QByteArray m_sPrefGrp;
    QString UnitStr;
    double Value;
    double Maximum;
    double Minimum;
    double StepSize;
    int HistorySize;
};

// ------------------------------------------------------------------------------

class QuantitySpinBoxPrivate;
class QuantitySpinBox: public QAbstractSpinBox
{
    Q_OBJECT

    Q_PROPERTY(QString unit READ unitText WRITE setUnitText)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(double rawValue READ rawValue WRITE setValue NOTIFY valueChanged DESIGNABLE false)
    Q_PROPERTY(double value READ rawValue WRITE setValue NOTIFY valueChanged USER true)
    // Q_PROPERTY(Base::Quantity value READ value WRITE setValue NOTIFY valueChanged USER true)

public:
    explicit QuantitySpinBox(QWidget* parent = 0);
    virtual ~QuantitySpinBox();

    /// Get the current quantity
    Base::Quantity value() const;
    /// Get the current quantity without unit
    double rawValue() const;

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
    QString unitText(void);

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

    /// Clears the schemaand again use the system-wide schema.
    void clearSchema();

    /// Set the number portion selected
    void selectNumber();

    void setRange(double min, double max);

    Base::Quantity valueFromText(const QString& text) const;
    QString textFromValue(const Base::Quantity& val) const;
    virtual void stepBy(int steps);
    virtual void clear();
    virtual QValidator::State validate(QString& input, int& pos) const;
    virtual void fixup(QString& str) const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    bool event(QEvent* event);

public Q_SLOTS:
    /// Sets the field with a quantity
    void setValue(const Base::Quantity& val);
    /// Set a numerical value which gets converted to a quantity with the currently set unit type
    void setValue(double);

protected Q_SLOTS:
    void userInput(const QString& text);
    void handlePendingEmit();

protected:
    virtual StepEnabled stepEnabled() const;
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual void focusOutEvent(QFocusEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

private:
    void updateText(const Base::Quantity&);
    void updateFromCache(bool);
    QString getUserString(const Base::Quantity& val, double& factor, QString& unitString) const;
    QString getUserString(const Base::Quantity& val) const;

Q_SIGNALS:
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(const Base::Quantity&);
    /** Gets emitted if the user has entered a VALID input
     *  Valid means the user inputted string obeys all restrictions
     *  like: minimum, maximum and/or the right Unit (if specified).
     */
    void valueChanged(double);
    /**
     * The new value is passed in \a text with unit.
     */
    void textChanged(const QString&);
    /** Gets emitted if formula dialog is about to be opened (true)
     *  or finished (false).
     */
    void showFormulaDialog(bool);

private:
    QScopedPointer<QuantitySpinBoxPrivate> d_ptr;
    Q_DISABLE_COPY(QuantitySpinBox)
    Q_DECLARE_PRIVATE(QuantitySpinBox)
};

// ------------------------------------------------------------------------------

class PrefUnitSpinBox: public QuantitySpinBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefUnitSpinBox(QWidget* parent = 0);
    virtual ~PrefUnitSpinBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefQuantitySpinBox: public QuantitySpinBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefQuantitySpinBox(QWidget* parent = 0);
    virtual ~PrefQuantitySpinBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class CommandIconView: public QListWidget
{
    Q_OBJECT

public:
    CommandIconView(QWidget* parent = 0);
    virtual ~CommandIconView();

protected:
    void startDrag(Qt::DropActions supportedActions);

protected Q_SLOTS:
    void onSelectionChanged(QListWidgetItem* item, QListWidgetItem*);

Q_SIGNALS:
    void emitSelectionChanged(const QString&);
};

// -------------------------------------------------------------

class UIntSpinBoxPrivate;
class UIntSpinBox: public QSpinBox
{
    Q_OBJECT
    Q_OVERRIDE(uint maximum READ maximum WRITE setMaximum)
    Q_OVERRIDE(uint minimum READ minimum WRITE setMinimum)
    Q_OVERRIDE(uint value READ value WRITE setValue)

public:
    UIntSpinBox(QWidget* parent);
    virtual ~UIntSpinBox();

    void setRange(uint minVal, uint maxVal);
    uint value() const;
    virtual QValidator::State validate(QString& input, int& pos) const;
    uint minimum() const;
    void setMinimum(uint value);
    uint maximum() const;
    void setMaximum(uint value);

Q_SIGNALS:
    void unsignedChanged(uint value);

public Q_SLOTS:
    void setValue(uint value);

private Q_SLOTS:
    void valueChange(int value);

protected:
    virtual QString textFromValue(int v) const;
    virtual int valueFromText(const QString& text) const;

private:
    void updateValidator();
    UIntSpinBoxPrivate* d;
};

// ------------------------------------------------------------------------------

class IntSpinBox: public QSpinBox
{
    Q_OBJECT

public:
    IntSpinBox(QWidget* parent = 0);
    virtual ~IntSpinBox();
};

// ------------------------------------------------------------------------------

class DoubleSpinBox: public QDoubleSpinBox
{
    Q_OBJECT

public:
    DoubleSpinBox(QWidget* parent = 0);
    virtual ~DoubleSpinBox();
};

// -------------------------------------------------------------

class PrefSpinBox: public QSpinBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefSpinBox(QWidget* parent = 0);
    virtual ~PrefSpinBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class ColorButton: public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QColor color READ color WRITE setColor)
    Q_PROPERTY(bool allowChangeColor READ allowChangeColor WRITE setAllowChangeColor)
    Q_PROPERTY(bool drawFrame READ drawFrame WRITE setDrawFrame)
    Q_PROPERTY(bool allowTransparency READ allowTransparency WRITE setAllowTransparency)

public:
    ColorButton(QWidget* parent = 0);
    ~ColorButton();

    void setColor(const QColor&);
    QColor color() const;

    void setAllowChangeColor(bool);
    bool allowChangeColor() const;
    void setAllowTransparency(bool);
    bool allowTransparency() const;

    void setDrawFrame(bool);
    bool drawFrame() const;

public Q_SLOTS:
    virtual void onChooseColor();

Q_SIGNALS:
    void changed();

protected:
    void paintEvent(QPaintEvent*);

private:
    QColor _col;
    bool _allowChange;
    bool _allowTransparency;
    bool _drawFrame;
};

// ------------------------------------------------------------------------------

class PrefColorButton: public ColorButton
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefColorButton(QWidget* parent = 0);
    virtual ~PrefColorButton();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefDoubleSpinBox: public QDoubleSpinBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefDoubleSpinBox(QWidget* parent = 0);
    virtual ~PrefDoubleSpinBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefLineEdit: public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefLineEdit(QWidget* parent = 0);
    virtual ~PrefLineEdit();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefComboBox: public QComboBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefComboBox(QWidget* parent = 0);
    virtual ~PrefComboBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefCheckBox: public QCheckBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefCheckBox(QWidget* parent = 0);
    virtual ~PrefCheckBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefRadioButton: public QRadioButton
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefRadioButton(QWidget* parent = 0);
    virtual ~PrefRadioButton();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefSlider: public QSlider
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefSlider(QWidget* parent = 0);
    virtual ~PrefSlider();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};

// ------------------------------------------------------------------------------

class PrefFontBox: public QFontComboBox
{
    Q_OBJECT

    Q_PROPERTY(QByteArray prefEntry READ entryName WRITE setEntryName)
    Q_PROPERTY(QByteArray prefPath READ paramGrpPath WRITE setParamGrpPath)

public:
    PrefFontBox(QWidget* parent = 0);
    virtual ~PrefFontBox();

    QByteArray entryName() const;
    QByteArray paramGrpPath() const;
    void setEntryName(const QByteArray& name);
    void setParamGrpPath(const QByteArray& name);

private:
    QByteArray m_sPrefName;
    QByteArray m_sPrefGrp;
};
}  // namespace Gui

#endif  // GUI_CUSTOMWIDGETS_H
