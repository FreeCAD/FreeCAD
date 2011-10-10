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


#ifndef GUI_PREFWIDGETS_H
#define GUI_PREFWIDGETS_H

#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <Base/Parameter.h>
#include "Widgets.h"
#include "Window.h"
#include "SpinBox.h"
#include "FileDialog.h"

namespace Gui {
class CommandManager;
class WidgetFactoryInst;

/** The preference widget class.
 * If you want to extend a QWidget class to save/restore its data
 * you just have to derive from this class and implement the methods 
 * restorePreferences() and savePreferences().
 *
 * To restore and save the settings of any widgets in own dialogs you have
 * call onRestore() e.g. in the dialog's constructor and call onSave() e.g.
 * in accept() for each widget you want to enable this mechanism. 
 * 
 * For more information of how to use these widgets in normal container widgets 
 * which are again in a dialog refer to the description of Gui::Dialog::DlgPreferencesImp.
 *
 * \author Werner Mayer
 */
class GuiExport PrefWidget : public WindowParameter
{
public:
  virtual void setEntryName( const QByteArray& name );
  virtual QByteArray entryName() const;

  virtual void setParamGrpPath( const QByteArray& path );
  virtual QByteArray paramGrpPath() const;

  virtual void OnChange(Base::Subject<const char*> &rCaller, const char * sReason);
  void onSave();
  void onRestore();

protected:
  /** Restores the preferences
   * Must be reimplemented in any subclasses.
   */
  virtual void restorePreferences() = 0;
  /** Save the preferences
   * Must be reimplemented in any subclasses.
   */
  virtual void savePreferences()    = 0;

  PrefWidget();
  virtual ~PrefWidget();

private:
  QByteArray m_sPrefName;
  QByteArray m_sPrefGrp;

  // friends
  friend class Gui::WidgetFactoryInst;
};

/** The PrefSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefSpinBox : public QSpinBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefSpinBox ( QWidget * parent = 0 );
  virtual ~PrefSpinBox();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/** The PrefDoubleSpinBox class.
 * \author Werner Mayer
 */
class GuiExport PrefDoubleSpinBox : public QDoubleSpinBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefDoubleSpinBox ( QWidget * parent = 0 );
  virtual ~PrefDoubleSpinBox();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefLineEdit class.
 * \author Werner Mayer
 */
class GuiExport PrefLineEdit : public QLineEdit, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefLineEdit ( QWidget * parent = 0 );
  virtual ~PrefLineEdit();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefFileChooser class.
 * \author Werner Mayer
 */
class GuiExport PrefFileChooser : public FileChooser, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefFileChooser ( QWidget * parent = 0 );
  virtual ~PrefFileChooser();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefComboBox class.
 * \author Werner Mayer
 */
class GuiExport PrefComboBox : public QComboBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefComboBox ( QWidget * parent = 0 );
  virtual ~PrefComboBox();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefCheckBox class.
 * \author Werner Mayer
 */
class GuiExport PrefCheckBox : public QCheckBox, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefCheckBox ( QWidget * parent = 0 );
  virtual ~PrefCheckBox();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefRadioButton class.
 * \author Werner Mayer
 */
class GuiExport PrefRadioButton : public QRadioButton, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefRadioButton ( QWidget * parent = 0 );
  virtual ~PrefRadioButton();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefSlider class.
 * \author Werner Mayer
 */
class GuiExport PrefSlider : public QSlider, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefSlider ( QWidget * parent = 0 );
  virtual ~PrefSlider();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

/**
 * The PrefColorButton class.
 * \author Werner Mayer
 */
class GuiExport PrefColorButton : public ColorButton, public PrefWidget
{
  Q_OBJECT

  Q_PROPERTY( QByteArray prefEntry READ entryName     WRITE setEntryName     )
  Q_PROPERTY( QByteArray prefPath  READ paramGrpPath  WRITE setParamGrpPath  )

public:
  PrefColorButton ( QWidget * parent = 0 );
  virtual ~PrefColorButton();

  // PROPERTIES
  // getters
  QByteArray entryName    () const;
  QByteArray paramGrpPath () const;
  // setters
  void  setEntryName     ( const QByteArray& name );
  void  setParamGrpPath  ( const QByteArray& name );

protected:
  // restore from/save to parameters
  void restorePreferences();
  void savePreferences();
};

} // namespace Gui

#endif // GUI_PREFWIDGETS_H
