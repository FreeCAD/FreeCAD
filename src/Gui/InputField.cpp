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
#ifndef _PreComp_
# include <QContextMenuEvent>
# include <QMenu>
#endif

#include <Base/Console.h>
#include <Base/Quantity.h>
#include <Base/Exception.h>
#include <App/Application.h>

#include "InputField.h"
#include "BitmapFactory.h"

using namespace Gui;
using namespace Base;

// --------------------------------------------------------------------

InputField::InputField ( QWidget * parent )
  : QLineEdit(parent), 
  StepSize(1.0), 
  Maximum(DOUBLE_MAX),
  Minimum(-DOUBLE_MAX),
  HistorySize(5),
  SaveSize(5)
{
    iconLabel = new QLabel(this);
    iconLabel->setCursor(Qt::ArrowCursor);
    QPixmap pixmap = BitmapFactory().pixmapFromSvg(":/icons/button_valid.svg", QSize(sizeHint().height(),sizeHint().height()));
    iconLabel->setPixmap(pixmap);
    iconLabel->setStyleSheet(QString::fromAscii("QLabel { border: none; padding: 0px; }"));
    iconLabel->hide();
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(updateIconLabel(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString::fromAscii("QLineEdit { padding-right: %1px } ").arg(iconLabel->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), iconLabel->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), iconLabel->sizeHint().height() + frameWidth * 2 + 2));

    this->setContextMenuPolicy(Qt::DefaultContextMenu);

    QObject::connect(this, SIGNAL(textChanged  (QString)),
                     this, SLOT(newInput(QString)));
}

InputField::~InputField()
{
}

void InputField::resizeEvent(QResizeEvent *)
{
    QSize sz = iconLabel->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    iconLabel->move(rect().right() - frameWidth - sz.width(),
                    (rect().bottom() + 1 - sz.height())/2);
}

void InputField::updateIconLabel(const QString& text)
{
    iconLabel->setVisible(!text.isEmpty());
}

void InputField::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *editMenu = createStandardContextMenu();
    editMenu->setTitle(tr("Edit"));
    QMenu* menu = new QMenu(QString::fromAscii("InputFieldContextmenu"));

    menu->addMenu(editMenu);
    menu->addSeparator();

    // datastructure to remember actions for values
    std::vector<QString> values;
    std::vector<QAction *> actions;

    // add the history menu part...
    std::vector<QString> history = getHistory();

    for(std::vector<QString>::const_iterator it = history.begin();it!= history.end();++it){
        actions.push_back(menu->addAction(*it));
        values.push_back(*it);
    }

    // add the save value portion of the menu
    menu->addSeparator();
    QAction *SaveValueAction = menu->addAction(tr("Save value"));
    std::vector<QString> savedValues = getSavedValues();

    for(std::vector<QString>::const_iterator it = savedValues.begin();it!= savedValues.end();++it){
        actions.push_back(menu->addAction(*it));
        values.push_back(*it);
    }

    // call the menu and wait until its back
    QAction *saveAction = menu->exec(event->globalPos());

    // look what the user has choosen
    if(saveAction == SaveValueAction)
        pushToSavedValues();
    else{
        int i=0;
        for(std::vector<QAction *>::const_iterator it = actions.begin();it!=actions.end();++it,i++)
            if(*it == saveAction)
                this->setText(values[i]);
    }

    delete menu;
}

void InputField::newInput(const QString & text)
{
    Quantity res;
    try{
        res = Quantity::parse(text);
    }catch(Base::Exception &e){
        ErrorText = e.what();
        this->setToolTip(QString::fromAscii(ErrorText.c_str()));
        QPixmap pixmap = BitmapFactory().pixmapFromSvg(":/icons/button_invalid.svg", QSize(sizeHint().height(),sizeHint().height()));
        iconLabel->setPixmap(pixmap);
        parseError(QString::fromAscii(ErrorText.c_str()));
        return;
    }

    QPixmap pixmap = BitmapFactory().pixmapFromSvg(":/icons/button_valid.svg", QSize(sizeHint().height(),sizeHint().height()));
    iconLabel->setPixmap(pixmap);
    ErrorText = "";

	if (res.getValue() > Maximum){
		res.setValue(Maximum);
		ErrorText = "Maximum reached";
	}
	if (res.getValue() < Minimum){
		res.setValue(Minimum);
		ErrorText = "Minimum reached";
	}

    this->setToolTip(QString::fromAscii(ErrorText.c_str()));
    actQuantity = res;
    double dFactor;
    res.getUserString(dFactor,actUnitStr);
    // calculate the number shown 
    actUnitValue = res.getValue()/dFactor; 
    // signaling
    valueChanged(res);
    valueChanged(res.getValue());
}

void InputField::pushToHistory(const QString &valueq)
{
    QString val;
    if(valueq.isEmpty())
        val = this->text();
    else
        val = valueq;

    // check if already in:
    std::vector<QString> hist = InputField::getHistory();
    for(std::vector<QString>::const_iterator it = hist.begin();it!=hist.end();++it)
        if( *it == val)
            return;
    
    std::string value(val.toUtf8());
    if(_handle.isValid()){
        char hist1[21];
        char hist0[21];
        for(int i = HistorySize -1 ; i>=0 ;i--){
            snprintf(hist1,20,"Hist%i",i+1);
            snprintf(hist0,20,"Hist%i",i);
            std::string tHist = _handle->GetASCII(hist0,"");
            if(tHist != "")
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Hist0",value.c_str());
    }
}

std::vector<QString> InputField::getHistory(void)
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< HistorySize ;i++){
            snprintf(hist,20,"Hist%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( tmp != "")
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }
    return res;
}

void InputField::setToLastUsedValue(void)
{
     std::vector<QString> hist = getHistory();
     if(hist.size()>0)
         this->setValue(Base::Quantity::parse(hist[0]));
}


void InputField::pushToSavedValues(const QString &valueq)
{
    std::string value;
    if(valueq.isEmpty())
        value = this->text().toUtf8().constData();
    else
        value = valueq.toUtf8().constData();

    if(_handle.isValid()){
        char hist1[21];
        char hist0[21];
        for(int i = SaveSize -1 ; i>=0 ;i--){
            snprintf(hist1,20,"Save%i",i+1);
            snprintf(hist0,20,"Save%i",i);
            std::string tHist = _handle->GetASCII(hist0,"");
            if(tHist != "")
                _handle->SetASCII(hist1,tHist.c_str());
        }
        _handle->SetASCII("Save0",value.c_str());
    }
}

std::vector<QString> InputField::getSavedValues(void)
{
    std::vector<QString> res;

    if(_handle.isValid()){
        std::string tmp;
        char hist[21];
        for(int i = 0 ; i< SaveSize ;i++){
            snprintf(hist,20,"Save%i",i);
            tmp = _handle->GetASCII(hist,"");
            if( tmp != "")
                res.push_back(QString::fromUtf8(tmp.c_str()));
            else
                break; // end of history reached
        }
    }
    return res;
}

/** Sets the preference path to \a path. */
void InputField::setParamGrpPath( const QByteArray& path )
{
   
  _handle = App::GetApplication().GetParameterGroupByPath( path);
  if (_handle.isValid())
      sGroupString = (const char*)path;
}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
    if(_handle.isValid())
        return sGroupString.c_str();
    return QByteArray();
}

/// sets the field with a quantity
void InputField::setValue(const Base::Quantity& quant)
{
    actQuantity = quant;
	// check limits
	if (actQuantity.getValue() > Maximum)
		actQuantity.setValue(Maximum);
	if (actQuantity.getValue() < Minimum)
		actQuantity.setValue(Minimum);

    if(!quant.getUnit().isEmpty())
        actUnit = quant.getUnit();

    double dFactor;
    setText(quant.getUserString(dFactor,actUnitStr));
    actUnitValue = quant.getValue()/dFactor;
}

void InputField::setUnit(const Base::Unit& unit)
{
    actUnit = unit;
}



/// get the value of the singleStep property
double InputField::singleStep(void)const
{
    return StepSize;
}

/// set the value of the singleStep property 
void InputField::setSingleStep(double s)
{
    StepSize = s;
}

/// get the value of the maximum property
double InputField::maximum(void)const
{
    return Maximum;
}

/// set the value of the maximum property 
void InputField::setMaximum(double m)
{
    Maximum = m;
	if (actQuantity.getValue() > Maximum)
		actQuantity.setValue(Maximum);
}

/// get the value of the minimum property
double InputField::minimum(void)const
{
    return Minimum;
}

/// set the value of the minimum property 
void InputField::setMinimum(double m)
{
    Minimum = m;
	if (actQuantity.getValue() < Minimum)
		actQuantity.setValue(Minimum);
}

void InputField::setUnitText(QString str)
{
    Base::Quantity quant = Base::Quantity::parse(str);
    setUnit(quant.getUnit());
}
  
QString InputField::getUnitText(void)
{
    return actUnitStr;
}

// get the value of the minimum property
int InputField::historySize(void)const
{
    return HistorySize;
}
// set the value of the minimum property 
void InputField::setHistorySize(int i)
{
    assert(i>=0);
    assert(i<100);

    HistorySize = i;
}

void InputField::selectNumber(void)
{
    QByteArray str = text().toLatin1();
    unsigned int i = 0;

    while ( (str.at(i) >= '0' && str.at(i) <= '9') || str.at(i)== ',' ||  str.at(i)== '.'||  str.at(i)== '-' ) 
        i++;

    setSelection(0,i);

}

void InputField::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
        {
            double val = actUnitValue + StepSize;
            this->setText( QString::fromUtf8("%L1 %2").arg(val).arg(actUnitStr));
            event->accept();
        }
        break;
    case Qt::Key_Down:
        {
            double val = actUnitValue - StepSize;
            this->setText( QString::fromUtf8("%L1 %2").arg(val).arg(actUnitStr));
            event->accept();
        }
        break;
    default:
        QLineEdit::keyPressEvent(event);
        break;
    }
}

void InputField::wheelEvent (QWheelEvent * event)
{
    double step = event->delta() > 0 ? StepSize : -StepSize;
    double val = actUnitValue + step;

    this->setText( QString::fromUtf8("%L1 %2").arg(val).arg(actUnitStr));
    event->accept();
}
// --------------------------------------------------------------------


#include "moc_InputField.cpp"
