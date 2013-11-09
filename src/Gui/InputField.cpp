/***************************************************************************
 *   Copyright (c) 2013 Juergen Riegel                                     *
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

#include <Base/Console.h>
#include <Base/Quantity.h>
#include <Base/Exception.h>
#include <App/Application.h>

#include "InputField.h"
using namespace Gui;
using namespace Base;

// --------------------------------------------------------------------

InputField::InputField ( QWidget * parent )
  : QLineEdit(parent)
{
    this->setContextMenuPolicy(Qt::DefaultContextMenu);

    QObject::connect(this, SIGNAL(textChanged  (QString)),
        	         this, SLOT(newInput(QString)));
}

InputField::~InputField()
{
}

void InputField::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();
    QAction *saveAction = menu->addAction(tr("My Menu Item"));
    //...
    QAction *saveAction2 = menu->exec(event->globalPos());
    delete menu;
}

void InputField::newInput(const QString & text)
{
    Quantity res;
    try{
        res = Quantity::parse(text.toAscii());
    }catch(Base::Exception &e){
        ErrorText = e.what();
        this->setToolTip(QString::fromAscii(ErrorText.c_str()));
        QPalette *palette = new QPalette();
	    palette->setColor(QPalette::Base,QColor(255,200,200));
	    setPalette(*palette);
        parseError(QString::fromAscii(ErrorText.c_str()));
        return;
    }
    QPalette *palette = new QPalette();
	palette->setColor(QPalette::Base,QColor(200,255,200));
	setPalette(*palette);
    ErrorText = "";
    this->setToolTip(QString::fromAscii(ErrorText.c_str()));
    // signaling 
    valueChanged(res);

}

void InputField::pushToHistory(std::string value)
{
    if(_handle.isValid()){
        _handle->SetASCII("Hist1",_handle->GetASCII("Hist0","").c_str());
        _handle->SetASCII("Hist0",value.c_str());
    }
}

std::vector<std::string> InputField::getHistory(void)
{
    std::vector<std::string> res;

    if(_handle.isValid()){
        std::string tmp;
        tmp = _handle->GetASCII("Hist0","");
        if( tmp != ""){
            res.push_back(tmp);
            tmp = _handle->GetASCII("Hist1","");
            if( tmp != ""){
                res.push_back(tmp);
                //tmp = _handle->GetASCII("Hist2","");
            }
        }
    }
    return res;
}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath( const QByteArray& path )
{
   
  _handle = App::GetApplication().GetParameterGroupByPath( path);
  if(_handle.isValid())
      sGroupString = path;

}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
  if(_handle.isValid())
      return sGroupString.c_str();
}

/// sets the field with a quantity
void InputField::setValue(const Base::Quantity& quant)
{
    actQuantity = quant;
    if(!quant.getUnit().isEmpty())
        actUnit = quant.getUnit();

    setText(QString::fromAscii(quant.getUserString().c_str()));
}

void InputField::setUnit(const Base::Unit& unit)
{
    actUnit = unit;
}



/// get the value of the singleStep property
double InputField::singleStep(void)const
{
    return 0.0;
}

/// set the value of the singleStep property 
void InputField::setSingleStep(double)
{

}

/// get the value of the maximum property
double InputField::maximum(void)const
{
    return 0.0;
}

/// set the value of the maximum property 
void InputField::setMaximum(double)
{

}

/// get the value of the minimum property
double InputField::minimum(void)const
{
    return 0.0;
}

/// set the value of the minimum property 
void InputField::setMinimum(double)
{

}


// --------------------------------------------------------------------


#include "moc_InputField.cpp"
