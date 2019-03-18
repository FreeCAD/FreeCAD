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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QContextMenuEvent>
# include <QMenu>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <App/Application.h>

#include "PrefWidgets.h"
#include "FileDialog.h"
#include <cstring>

using Base::Console;
using namespace Gui;

/** Constructs a preference widget. 
 */
PrefWidget::PrefWidget()
 : WindowParameter("")
{
}

/**
 * Destroys the widget and detaches it from its parameter group.
 */
PrefWidget::~PrefWidget()
{
  if (getWindowParameter().isValid())
    getWindowParameter()->Detach(this);
}

/** Sets the preference name to \a name. */
void PrefWidget::setEntryName( const QByteArray& name )
{
  m_sPrefName = name;
}

/** Returns the widget's preference name. */
QByteArray PrefWidget::entryName() const
{
  return m_sPrefName;
}

/** Sets the preference path to \a path. */
void PrefWidget::setParamGrpPath( const QByteArray& path )
{
#ifdef FC_DEBUG
  if (getWindowParameter().isValid())
  {
    if ( paramGrpPath() != path )
      Base::Console().Warning("Widget already attached\n");
  }
#endif

  if ( paramGrpPath() != path )
  {
    if ( setGroupName( path ) )
    {
      m_sPrefGrp = path;
      assert(getWindowParameter().isValid());
      getWindowParameter()->Attach(this);
    }
  }
}

/** Returns the widget's preferences path. */
QByteArray PrefWidget::paramGrpPath() const
{
  return m_sPrefGrp;
}

/** 
 * This method is called if one ore more values in the parameter settings are changed 
 * where getParamGrp() points to. 
 * Note: This method is called for each parameter inside the parameter group. So
 * you have to filter out the appropriate parameter with the name \a sReason.
 * \a rCaller calls this method.
 */
void PrefWidget::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    Q_UNUSED(rCaller);
    if (std::strcmp(sReason,m_sPrefName) == 0)
        restorePreferences();
}

/**
 * Saves the current preferences of the widget.
 * All preference widget attached to the same parameter group are notified.
 */
void PrefWidget::onSave()
{
  savePreferences();
  if (getWindowParameter().isValid())
    getWindowParameter()->Notify( entryName() );
#ifdef FC_DEBUG
  else
    qFatal( "No parameter group specified!" );
#endif
}

/**
 * Restores the preferences of the widget.
 */
void PrefWidget::onRestore()
{
#ifdef FC_DEBUG
  if (getWindowParameter().isNull())
    qWarning( "No parameter group specified!" );
#endif
  restorePreferences();
}

// --------------------------------------------------------------------

PrefSpinBox::PrefSpinBox ( QWidget * parent )
  : QSpinBox(parent), PrefWidget()
{
}

PrefSpinBox::~PrefSpinBox()
{
}

void PrefSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  int nVal = getWindowParameter()->GetInt( entryName(), QSpinBox::value() );
  setValue( nVal );
}

void PrefSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt( entryName() , (int)value() );
}

// --------------------------------------------------------------------

PrefDoubleSpinBox::PrefDoubleSpinBox ( QWidget * parent )
  : QDoubleSpinBox(parent), PrefWidget()
{
}

PrefDoubleSpinBox::~PrefDoubleSpinBox()
{
}

void PrefDoubleSpinBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  double fVal = (double)getWindowParameter()->GetFloat( entryName() , value() );
  setValue(fVal);
}

void PrefDoubleSpinBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetFloat( entryName(), value() );
}

// --------------------------------------------------------------------

PrefLineEdit::PrefLineEdit ( QWidget * parent )
  : QLineEdit(parent), PrefWidget()
{
}

PrefLineEdit::~PrefLineEdit()
{
}

void PrefLineEdit::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  QString text = this->text();
  text = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), text.toUtf8()).c_str());
  setText(text);
}

void PrefLineEdit::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetASCII(entryName(), text().toUtf8());
}

// --------------------------------------------------------------------

PrefFileChooser::PrefFileChooser ( QWidget * parent )
  : FileChooser(parent), PrefWidget()
{
}

PrefFileChooser::~PrefFileChooser()
{
}

void PrefFileChooser::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  QString txt = QString::fromUtf8(getWindowParameter()->GetASCII(entryName(), fileName().toUtf8()).c_str());
  setFileName(txt);
}

void PrefFileChooser::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetASCII(entryName(), fileName().toUtf8());
}

// --------------------------------------------------------------------

PrefComboBox::PrefComboBox ( QWidget * parent )
  : QComboBox(parent), PrefWidget()
{
}

PrefComboBox::~PrefComboBox()
{
}

void PrefComboBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  int index = getWindowParameter()->GetInt(entryName(), currentIndex());
  setCurrentIndex(index);
}

void PrefComboBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt(entryName() , currentIndex());
}

// --------------------------------------------------------------------

PrefCheckBox::PrefCheckBox ( QWidget * parent )
  : QCheckBox(parent), PrefWidget()
{
}

PrefCheckBox::~PrefCheckBox()
{
}

void PrefCheckBox::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefCheckBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetBool( entryName(), isChecked() );
}

// --------------------------------------------------------------------

PrefRadioButton::PrefRadioButton ( QWidget * parent )
  : QRadioButton(parent), PrefWidget()
{
}

PrefRadioButton::~PrefRadioButton()
{
}

void PrefRadioButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  bool enable = getWindowParameter()->GetBool( entryName(), isChecked() );
  setChecked(enable);
}

void PrefRadioButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetBool( entryName() , isChecked() );
}

// --------------------------------------------------------------------

PrefSlider::PrefSlider ( QWidget * parent )
  : QSlider(parent), PrefWidget()
{
}

PrefSlider::~PrefSlider()
{
}

void PrefSlider::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  int nVal = getWindowParameter()->GetInt(entryName(), QSlider::value());
  setValue(nVal);
}

void PrefSlider::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  getWindowParameter()->SetInt(entryName() , (int)value());
}

// --------------------------------------------------------------------

PrefColorButton::PrefColorButton ( QWidget * parent )
  : ColorButton(parent), PrefWidget()
{
}

PrefColorButton::~PrefColorButton()
{
}

void PrefColorButton::restorePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  QColor col = color();

  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8);

  unsigned long lcol = static_cast<unsigned long>(icol);
  lcol = getWindowParameter()->GetUnsigned( entryName(), lcol );
  icol = static_cast<unsigned int>(lcol);
  int r = (icol >> 24)&0xff;
  int g = (icol >> 16)&0xff;
  int b = (icol >>  8)&0xff;

  setColor(QColor(r,g,b));
}

void PrefColorButton::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  QColor col = color();
  // (r,g,b,a) with a = 255 (opaque)
  unsigned int icol = (col.red() << 24) | (col.green() << 16) | (col.blue() << 8) | 255;
  unsigned long lcol = static_cast<unsigned long>(icol);
  getWindowParameter()->SetUnsigned( entryName(), lcol );
}

// --------------------------------------------------------------------

PrefUnitSpinBox::PrefUnitSpinBox ( QWidget * parent )
  : QuantitySpinBox(parent), PrefWidget()
{
}

PrefUnitSpinBox::~PrefUnitSpinBox()
{
}

void PrefUnitSpinBox::restorePreferences()
{
    if (getWindowParameter().isNull()) {
        Console().Warning("Cannot restore!\n");
        return;
    }

    double fVal = (double)getWindowParameter()->GetFloat( entryName() ,rawValue() );
    setValue(fVal);
}

void PrefUnitSpinBox::savePreferences()
{
    if (getWindowParameter().isNull()) {
        Console().Warning("Cannot save!\n");
        return;
    }

    double q = rawValue();
    getWindowParameter()->SetFloat( entryName(), q );
}

// --------------------------------------------------------------------

namespace Gui {
class PrefQuantitySpinBoxPrivate
{
public:
    PrefQuantitySpinBoxPrivate() :
      historySize(5)
    {
    }
    ~PrefQuantitySpinBoxPrivate()
    {
    }

    QByteArray prefGrp;
    ParameterGrp::handle handle;
    int historySize;
};
}

PrefQuantitySpinBox::PrefQuantitySpinBox (QWidget * parent)
  : QuantitySpinBox(parent), d_ptr(new PrefQuantitySpinBoxPrivate())
{
}

PrefQuantitySpinBox::~PrefQuantitySpinBox()
{
}

void PrefQuantitySpinBox::contextMenuEvent(QContextMenuEvent *event)
{
    Q_D(PrefQuantitySpinBox);

    QMenu *editMenu = lineEdit()->createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    QMenu* menu = new QMenu(QString::fromLatin1("PrefQuantitySpinBox"));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction *> actions;

    // add the history menu part...
    QStringList history = getHistory();

    for (QStringList::const_iterator it = history.begin();it!= history.end();++it) {
        actions.push_back(menu->addAction(*it));
        values.push_back(*it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *saveValueAction = menu->addAction(tr("Save value"));
    QAction *clearListAction = menu->addAction(tr("Clear list"));
    clearListAction->setDisabled(history.empty());

    // call the menu and wait until its back
    QAction *userAction = menu->exec(event->globalPos());

    // look what the user has chosen
    if (userAction == saveValueAction) {
        pushToHistory(this->text());
    }
    else if (userAction == clearListAction) {
        d->handle->Clear();
    }
    else {
        int i=0;
        for (std::vector<QAction *>::const_iterator it = actions.begin();it!=actions.end();++it,i++) {
            if (*it == userAction) {
                lineEdit()->setText(values[i]);
                break;
            }
        }
    }

    delete menu;
}

void PrefQuantitySpinBox::onSave()
{
    pushToHistory();
}

void PrefQuantitySpinBox::onRestore()
{
    setToLastUsedValue();
}

void PrefQuantitySpinBox::pushToHistory(const QString &valueq)
{
    Q_D(PrefQuantitySpinBox);

    QString val;
    if (valueq.isEmpty())
        val = this->text();
    else
        val = valueq;

    std::string value(val.toUtf8());
    if (d->handle.isValid()) {
        try {
            // do nothing if the given value is on top of the history
            std::string tHist = d->handle->GetASCII("Hist0");
            if (tHist != val.toUtf8().constData()) {
                for (int i = d->historySize -1 ; i>=0 ;i--) {
                    QByteArray hist1 = "Hist";
                    QByteArray hist0 = "Hist";
                    hist1.append(QByteArray::number(i+1));
                    hist0.append(QByteArray::number(i));
                    std::string tHist = d->handle->GetASCII(hist0);
                    if (!tHist.empty())
                        d->handle->SetASCII(hist1,tHist.c_str());
                }
                d->handle->SetASCII("Hist0",value.c_str());
            }
        }
        catch (const Base::Exception& e) {
            Console().Warning("pushToHistory: %s\n", e.what());
        }
    }
}

QStringList PrefQuantitySpinBox::getHistory() const
{
    Q_D(const PrefQuantitySpinBox);
    QStringList res;

    if (d->handle.isValid()) {
        std::string tmp;
        for (int i = 0 ; i< d->historySize ;i++) {
            QByteArray hist = "Hist";
            hist.append(QByteArray::number(i));
            tmp = d->handle->GetASCII(hist);
            if (!tmp.empty())
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }

    return res;
}

void PrefQuantitySpinBox::setToLastUsedValue()
{
    QStringList hist = getHistory();
    if (!hist.empty())
        lineEdit()->setText(hist[0]);
}

void PrefQuantitySpinBox::setParamGrpPath(const QByteArray& path)
{
    Q_D(PrefQuantitySpinBox);
    QByteArray groupPath = path;
    if (!groupPath.startsWith("User parameter:")) {
        groupPath.prepend("User parameter:BaseApp/Preferences/");
    }
    d->handle = App::GetApplication().GetParameterGroupByPath(groupPath);
    if (d->handle.isValid())
        d->prefGrp = path;
}

QByteArray PrefQuantitySpinBox::paramGrpPath() const
{
    Q_D(const PrefQuantitySpinBox);
    if (d->handle.isValid())
        return d->prefGrp;
    return QByteArray();
}

int PrefQuantitySpinBox::historySize() const
{
    Q_D(const PrefQuantitySpinBox);
    return d->historySize;
}

void PrefQuantitySpinBox::setHistorySize(int i)
{
    Q_D(PrefQuantitySpinBox);
    d->historySize = i;
}

// --------------------------------------------------------------------

PrefFontBox::PrefFontBox ( QWidget * parent )
  : QFontComboBox(parent), PrefWidget()
{
}

PrefFontBox::~PrefFontBox()
{
}

void PrefFontBox::restorePreferences()
{
  if ( getWindowParameter().isNull() )
  {
    Console().Warning("Cannot restore!\n");
    return;
  }

  QFont currFont = currentFont();                         //QFont from selector widget
  QString currName = currFont.family();
  
  std::string prefName = getWindowParameter()->GetASCII(entryName(), currName.toUtf8());  //font name from cfg file

  currFont.setFamily(QString::fromStdString(prefName));
  setCurrentFont(currFont);                               //set selector widget to name from cfg file
}

void PrefFontBox::savePreferences()
{
  if (getWindowParameter().isNull())
  {
    Console().Warning("Cannot save!\n");
    return;
  }

  QFont currFont = currentFont();
  QString currName = currFont.family();
  getWindowParameter()->SetASCII( entryName() , currName.toUtf8() );
}

#include "moc_PrefWidgets.cpp"
