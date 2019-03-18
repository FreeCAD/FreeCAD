/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <sstream>
# include <QByteArray>
# include <QContextMenuEvent>
# include <QHeaderView>
# include <QInputDialog>
# include <QMessageBox>
# include <QMenu>
# include <QTreeWidget>
#endif

#include "ui_DlgParameter.h"
#include "DlgParameterImp.h"
#include "DlgInputDialogImp.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "SpinBox.h"

#include <Base/Parameter.h>
#include <Base/Exception.h>
#include <App/Application.h>



using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgParameterImp */

/**
 *  Constructs a DlgParameterImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgParameterImp::DlgParameterImp( QWidget* parent,  Qt::WindowFlags fl )
  : QDialog(parent, fl|Qt::WindowMinMaxButtonsHint)
  , ui(new Ui_DlgParameter)
{
    ui->setupUi(this);
    QStringList groupLabels; 
    groupLabels << tr( "Group" );
    paramGroup = new ParameterGroup(ui->splitter3);
    paramGroup->setHeaderLabels(groupLabels);
    paramGroup->setRootIsDecorated(false);

    QStringList valueLabels; 
    valueLabels << tr( "Name" ) << tr( "Type" ) << tr( "Value" );
    paramValue = new ParameterValue(ui->splitter3);
    paramValue->setHeaderLabels(valueLabels);
    paramValue->setRootIsDecorated(false);
#if QT_VERSION >= 0x050000
    paramValue->header()->setSectionResizeMode(0, QHeaderView::Stretch);
#else
    paramValue->header()->setResizeMode(0, QHeaderView::Stretch);
#endif

    QSizePolicy policy = paramValue->sizePolicy();
    policy.setHorizontalStretch(3);
    paramValue->setSizePolicy(policy);

#if 0 // This is needed for Qt's lupdate
    qApp->translate( "Gui::Dialog::DlgParameterImp", "System parameter" );
    qApp->translate( "Gui::Dialog::DlgParameterImp", "User parameter" );
#endif

    ParameterManager* sys = App::GetApplication().GetParameterSet("System parameter");
    const std::map<std::string,ParameterManager *>& rcList = App::GetApplication().GetParameterSetList();
    for (std::map<std::string,ParameterManager *>::const_iterator it= rcList.begin();it!=rcList.end();++it) {
        if (it->second != sys) // for now ignore system parameters because they are nowhere used
            ui->parameterSet->addItem(tr(it->first.c_str()), QVariant(QByteArray(it->first.c_str())));
    }

    QByteArray cStr("User parameter");
    ui->parameterSet->setCurrentIndex(ui->parameterSet->findData(cStr));
    onChangeParameterSet(ui->parameterSet->currentIndex());
    if (ui->parameterSet->count() < 2)
        ui->parameterSet->hide();

    connect(ui->parameterSet, SIGNAL(activated(int)), 
            this, SLOT(onChangeParameterSet(int)));
    connect(paramGroup, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), 
            this, SLOT(onGroupSelected(QTreeWidgetItem*)));
    onGroupSelected(paramGroup->currentItem());
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgParameterImp::~DlgParameterImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

/**
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void DlgParameterImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        paramGroup->headerItem()->setText( 0, tr( "Group" ) );
        paramValue->headerItem()->setText( 0, tr( "Name" ) );
        paramValue->headerItem()->setText( 1, tr( "Type" ) );
        paramValue->headerItem()->setText( 2, tr( "Value" ) );
    } else {
        QDialog::changeEvent(e);
    }
}

void DlgParameterImp::on_closeButton_clicked()
{
    close();
}

void DlgParameterImp::accept()
{
    close();
}

void DlgParameterImp::reject()
{
    close();
}

void DlgParameterImp::showEvent(QShowEvent* )
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences");
    hGrp = hGrp->GetGroup("ParameterEditor");
    std::string buf = hGrp->GetASCII("Geometry", "");
    if (!buf.empty()) {
        int x1, y1, x2, y2;
        char sep;
        std::stringstream str(buf);
        str >> sep >> x1
            >> sep >> y1
            >> sep >> x2
            >> sep >> y2;
        QRect rect;
        rect.setCoords(x1, y1, x2, y2);
        this->setGeometry(rect);
    }
}

void DlgParameterImp::closeEvent(QCloseEvent* )
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences");
    hGrp = hGrp->GetGroup("ParameterEditor");
    QTreeWidgetItem* current = paramGroup->currentItem();
    if (current) {
        QStringList paths;
        paths << current->text(0);
        QTreeWidgetItem* parent = current->parent();
        while (parent) {
            paths.push_front(parent->text(0));
            parent = parent->parent();
        }

        QString path = paths.join(QLatin1String("."));
        hGrp->SetASCII("LastParameterGroup", (const char*)path.toUtf8());
        // save geometry of window
        const QRect& r = this->geometry();
        std::stringstream str;
        str << "(" << r.left() << "," << r.top() << ","
            << r.right() << "," << r.bottom() << ")";
        hGrp->SetASCII("Geometry", str.str().c_str());
    }
}

void DlgParameterImp::onGroupSelected( QTreeWidgetItem * item )
{
    if ( item && item->type() == QTreeWidgetItem::UserType + 1 )
    {
        paramValue->clear();
        Base::Reference<ParameterGrp> _hcGrp = static_cast<ParameterGroupItem*>(item)->_hcGrp;
        static_cast<ParameterValue*>(paramValue)->setCurrentGroup( _hcGrp );

        // filling up Text nodes
        std::vector<std::pair<std::string,std::string> > mcTextMap = _hcGrp->GetASCIIMap();
        for(std::vector<std::pair<std::string,std::string> >::iterator It2=mcTextMap.begin();It2!=mcTextMap.end();++It2)
        {
            (void)new ParameterText(paramValue,QString::fromUtf8(It2->first.c_str()),
                It2->second.c_str(), _hcGrp);
        }

        // filling up Int nodes
        std::vector<std::pair<std::string,long> > mcIntMap = _hcGrp->GetIntMap();
        for(std::vector<std::pair<std::string,long> >::iterator It3=mcIntMap.begin();It3!=mcIntMap.end();++It3)
        {
            (void)new ParameterInt(paramValue,QString::fromUtf8(It3->first.c_str()),It3->second, _hcGrp);
        }

        // filling up Float nodes
        std::vector<std::pair<std::string,double> > mcFloatMap = _hcGrp->GetFloatMap();
        for(std::vector<std::pair<std::string,double> >::iterator It4=mcFloatMap.begin();It4!=mcFloatMap.end();++It4)
        {
            (void)new ParameterFloat(paramValue,QString::fromUtf8(It4->first.c_str()),It4->second, _hcGrp);
        }

        // filling up bool nodes
        std::vector<std::pair<std::string,bool> > mcBoolMap = _hcGrp->GetBoolMap();
        for(std::vector<std::pair<std::string,bool> >::iterator It5=mcBoolMap.begin();It5!=mcBoolMap.end();++It5)
        {
            (void)new ParameterBool(paramValue,QString::fromUtf8(It5->first.c_str()),It5->second, _hcGrp);
        }

        // filling up UInt nodes
        std::vector<std::pair<std::string,unsigned long> > mcUIntMap = _hcGrp->GetUnsignedMap();
        for(std::vector<std::pair<std::string,unsigned long> >::iterator It6=mcUIntMap.begin();It6!=mcUIntMap.end();++It6)
        {
            (void)new ParameterUInt(paramValue,QString::fromUtf8(It6->first.c_str()),It6->second, _hcGrp);
        }
    }
}

/** Switches the type of parameters with name @a config. */
void DlgParameterImp::activateParameterSet(const char* config)
{
    int index = ui->parameterSet->findData(QByteArray(config));
    if (index != -1) {
        ui->parameterSet->setCurrentIndex(index);
        onChangeParameterSet(index);
    }
}

/** Switches the type of parameters either to user or system parameters. */
void DlgParameterImp::onChangeParameterSet(int index)
{
    ParameterManager* rcParMngr = App::GetApplication().GetParameterSet(ui->parameterSet->itemData(index).toByteArray());
    if (!rcParMngr)
        return;

    ui->buttonSaveToDisk->setEnabled(rcParMngr->HasSerializer());

    // remove all labels
    paramGroup->clear();
    paramValue->clear();

    // root labels
    std::vector<Base::Reference<ParameterGrp> > grps = rcParMngr->GetGroups();
    for ( std::vector<Base::Reference<ParameterGrp> >::iterator it = grps.begin(); it != grps.end(); ++it ) {
        QTreeWidgetItem* item = new ParameterGroupItem(paramGroup, *it);
        paramGroup->expandItem(item);
        item->setIcon(0, QApplication::style()->standardPixmap(QStyle::SP_ComputerIcon));
    }

    // get the path of the last selected group in the editor
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences");
    hGrp = hGrp->GetGroup("ParameterEditor");
    QString path = QString::fromUtf8(hGrp->GetASCII("LastParameterGroup").c_str());
    QStringList paths = path.split(QLatin1String("."), QString::SkipEmptyParts);

    QTreeWidgetItem* parent = 0;
    for (int index=0; index < paramGroup->topLevelItemCount() && !paths.empty(); index++) {
        QTreeWidgetItem* child = paramGroup->topLevelItem(index);
        if (child->text(0) == paths.front()) {
            paths.pop_front();
            parent = child;
        }
    }

    while (parent && !paths.empty()) {
        paramGroup->setItemExpanded(parent, true);
        QTreeWidgetItem* item = parent;
        parent = 0;
        for (int index=0; index < item->childCount(); index++) {
            QTreeWidgetItem* child = item->child(index);
            if (child->text(0) == paths.front()) {
                paths.pop_front();
                parent = child;
                break;
            }
        }
    }

    if (parent)
        paramGroup->setCurrentItem(parent);
    else if (paramGroup->topLevelItemCount() > 0)
        paramGroup->setCurrentItem(paramGroup->topLevelItem(0));
}

void DlgParameterImp::on_buttonSaveToDisk_clicked()
{
    int index = ui->parameterSet->currentIndex();
    ParameterManager* parmgr = App::GetApplication().GetParameterSet(ui->parameterSet->itemData(index).toByteArray());
    if (!parmgr)
        return;

    parmgr->SaveDocument();
}

namespace Gui {
bool validateInput(QWidget* parent, const QString& input)
{
    if (input.isEmpty())
        return false;
    for (int i=0; i<input.size(); i++) {
        const char c = input.at(i).toLatin1();
        if ((c < '0' || c > '9') &&  // Numbers
            (c < 'A' || c > 'Z') &&  // Uppercase letters
            (c < 'a' || c > 'z') &&  // Lowercase letters
            (c != ' ')) {            // Space
            QMessageBox::warning(parent, DlgParameterImp::tr("Invalid input"), 
                                         DlgParameterImp::tr("Invalid key name '%1'").arg(input));
            return false;
        }
    }
    return true;
}
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::ParameterGroup */

ParameterGroup::ParameterGroup( QWidget * parent )
  : QTreeWidget(parent)
{
    menuEdit = new QMenu(this);
    expandAct = menuEdit->addAction(tr("Expand"), this, SLOT(onToggleSelectedItem()));
    menuEdit->addSeparator();
    subGrpAct = menuEdit->addAction(tr("Add sub-group"), this, SLOT(onCreateSubgroup()));
    removeAct = menuEdit->addAction(tr("Remove group"), this, SLOT(onDeleteSelectedItem()));
    renameAct = menuEdit->addAction(tr("Rename group"), this, SLOT(onRenameSelectedItem()));
    menuEdit->addSeparator();
    exportAct = menuEdit->addAction(tr("Export parameter"), this, SLOT(onExportToFile()));
    importAct = menuEdit->addAction(tr("Import parameter"), this, SLOT(onImportFromFile()));
    menuEdit->setDefaultAction(expandAct);
}

ParameterGroup::~ParameterGroup()
{
}

void ParameterGroup::contextMenuEvent ( QContextMenuEvent* event )
{
    QTreeWidgetItem* item = currentItem();
    if (isItemSelected(item))
    {
        expandAct->setEnabled(item->childCount() > 0);
        // do not allow to import parameters from a non-empty parameter group
        importAct->setEnabled(item->childCount() == 0);

        if ( isItemExpanded(item) )
            expandAct->setText( tr("Collapse") );
        else
        expandAct->setText( tr("Expand") );
        menuEdit->popup(event->globalPos());
    }
}

void ParameterGroup::keyPressEvent (QKeyEvent* event)
{
    switch ( tolower(event->key()) ) 
    {
    case Qt::Key_Delete:
        {
            onDeleteSelectedItem();
        }   break;
    default:
            QTreeWidget::keyPressEvent(event);
  }
}

void ParameterGroup::onDeleteSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (isItemSelected(sel) && sel->parent())
    {
        if ( QMessageBox::question(this, tr("Remove group"), tr("Do really want to remove this parameter group?"),
                               QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape) == 
                               QMessageBox::Yes )
        {
            QTreeWidgetItem* parent = sel->parent();
            int index = parent->indexOfChild(sel);
            parent->takeChild(index);
            ParameterGroupItem* para = static_cast<ParameterGroupItem*>(parent);
            para->_hcGrp->RemoveGrp(sel->text(0).toLatin1());
            delete sel;
        }
    }
}

void ParameterGroup::onToggleSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (isItemSelected(sel))
    {
        if ( isItemExpanded(sel) )
            setItemExpanded(sel, false);
        else if ( sel->childCount() > 0 )
            setItemExpanded(sel, true);
    }
}

void ParameterGroup::onCreateSubgroup()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New sub-group"), QObject::tr("Enter the name:"),
                                         QLineEdit::Normal, QString::null, &ok );

    if (ok && Gui::validateInput(this, name))
    {
        QTreeWidgetItem* item = currentItem();
        if (isItemSelected(item))
        {
            ParameterGroupItem* para = static_cast<ParameterGroupItem*>(item);
            Base::Reference<ParameterGrp> hGrp = para->_hcGrp;

            if ( hGrp->HasGroup( name.toLatin1() ) )
            {
                QMessageBox::critical( this, tr("Existing sub-group"),
                    tr("The sub-group '%1' already exists.").arg( name ) );
                return;
            }

            hGrp = hGrp->GetGroup( name.toLatin1() );
            (void)new ParameterGroupItem(para,hGrp);
            expandItem(para);
        }
    }
}

void ParameterGroup::onExportToFile()
{
    QString file = FileDialog::getSaveFileName( this, tr("Export parameter to file"),
        QString::null, QString::fromLatin1("XML (*.FCParam)"));
    if ( !file.isEmpty() )
    {
        QTreeWidgetItem* item = currentItem();
        if (isItemSelected(item))
        {
            ParameterGroupItem* para = static_cast<ParameterGroupItem*>(item);
            Base::Reference<ParameterGrp> hGrp = para->_hcGrp;
            hGrp->exportTo( file.toUtf8() );
        }
    }
}

void ParameterGroup::onImportFromFile()
{
    QString file = FileDialog::getOpenFileName( this, tr("Import parameter from file"),
        QString::null, QString::fromLatin1("XML (*.FCParam)"));
    if ( !file.isEmpty() )
    {
        QTreeWidgetItem* item = currentItem();
        if (isItemSelected(item))
        {
            ParameterGroupItem* para = static_cast<ParameterGroupItem*>(item);
            Base::Reference<ParameterGrp> hGrp = para->_hcGrp;

            // remove the items and internal parameter values
            QList<QTreeWidgetItem*> childs = para->takeChildren();
            for (QList<QTreeWidgetItem*>::iterator it = childs.begin(); it != childs.end(); ++it)
            {
                delete *it;
            }

            try
            {
                hGrp->importFrom( file.toUtf8() );
                std::vector<Base::Reference<ParameterGrp> > cSubGrps = hGrp->GetGroups();
                for ( std::vector<Base::Reference<ParameterGrp> >::iterator it = cSubGrps.begin(); it != cSubGrps.end(); ++it )
                {
                    new ParameterGroupItem(para,*it);
                }

                setItemExpanded(para, para->childCount());
            }
            catch( const Base::Exception& )
            {
                QMessageBox::critical(this, tr("Import Error"),tr("Reading from '%1' failed.").arg( file ));
            }
        }
    }
}

void ParameterGroup::onRenameSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (isItemSelected(sel))
    {
        editItem(sel, 0);
    }
}

void ParameterGroup::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        expandAct->setText(tr("Expand"));
        subGrpAct->setText(tr("Add sub-group"));
        removeAct->setText(tr("Remove group"));
        renameAct->setText(tr("Rename group"));
        exportAct->setText(tr("Export parameter"));
        importAct->setText(tr("Import parameter"));
    } else {
        QTreeWidget::changeEvent(e);
    }
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::ParameterValue */

ParameterValue::ParameterValue( QWidget * parent )
  : QTreeWidget(parent)
{
    menuEdit = new QMenu(this);
    changeAct = menuEdit->addAction(tr("Change value"), this, SLOT(onChangeSelectedItem()));
    menuEdit->addSeparator();
    removeAct = menuEdit->addAction(tr("Remove key"), this, SLOT(onDeleteSelectedItem()));
    renameAct = menuEdit->addAction(tr("Rename key"), this, SLOT(onRenameSelectedItem()));
    menuEdit->setDefaultAction(changeAct);

    menuEdit->addSeparator();
    menuNew = menuEdit->addMenu(tr("New"));
    newStrAct = menuNew->addAction(tr("New string item"), this, SLOT(onCreateTextItem()));
    newFltAct = menuNew->addAction(tr("New float item"), this, SLOT(onCreateFloatItem()));
    newIntAct = menuNew->addAction(tr("New integer item"), this, SLOT(onCreateIntItem()));
    newUlgAct = menuNew->addAction(tr("New unsigned item"), this, SLOT(onCreateUIntItem()));
    newBlnAct = menuNew->addAction(tr("New Boolean item"), this, SLOT(onCreateBoolItem()));

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), 
            this, SLOT(onChangeSelectedItem(QTreeWidgetItem*, int)));
}

ParameterValue::~ParameterValue()
{
}

void ParameterValue::setCurrentGroup( const Base::Reference<ParameterGrp>& hGrp )
{
    _hcGrp = hGrp;
}

bool ParameterValue::edit ( const QModelIndex & index, EditTrigger trigger, QEvent * event )
{
    if (index.column() > 0)
        return false;
    return QTreeWidget::edit(index, trigger, event);
}

void ParameterValue::contextMenuEvent ( QContextMenuEvent* event )
{
    QTreeWidgetItem* item = currentItem();
    if (isItemSelected(item))
        menuEdit->popup(event->globalPos());
    else
        menuNew->popup(event->globalPos());
}

void ParameterValue::keyPressEvent (QKeyEvent* event)
{
    switch ( tolower(event->key()) ) 
    {
    case Qt::Key_Delete:
        {
            onDeleteSelectedItem();
        }   break;
    default:
            QTreeWidget::keyPressEvent(event);
  }
}

void ParameterValue::onChangeSelectedItem(QTreeWidgetItem* item, int col)
{
    if (isItemSelected(item) && col > 0)
    {
        static_cast<ParameterValueItem*>(item)->changeValue();
    }
}

void ParameterValue::onChangeSelectedItem()
{
    onChangeSelectedItem(currentItem(), 1);
}

void ParameterValue::onDeleteSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (isItemSelected(sel))
    {
        takeTopLevelItem(indexOfTopLevelItem(sel));
        static_cast<ParameterValueItem*>(sel)->removeFromGroup();
        delete sel;
    }
}

void ParameterValue::onRenameSelectedItem()
{
    QTreeWidgetItem* sel = currentItem();
    if (isItemSelected(sel))
    {
        editItem(sel, 0);
    }
}

void ParameterValue::onCreateTextItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New text item"), QObject::tr("Enter the name:"), 
                                         QLineEdit::Normal, QString::null, &ok);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,std::string> > smap = _hcGrp->GetASCIIMap();
    for (std::vector<std::pair<std::string,std::string> >::iterator it = smap.begin(); it != smap.end(); ++it) {
        if (name == QLatin1String(it->first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    QString val = QInputDialog::getText(this, QObject::tr("New text item"), QObject::tr("Enter your text:"), 
                                        QLineEdit::Normal, QString::null, &ok);
    if ( ok && !val.isEmpty() )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterText(this, name, val.toUtf8(), _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateIntItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New integer item"), QObject::tr("Enter the name:"), 
                                         QLineEdit::Normal, QString::null, &ok);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,long> > lmap = _hcGrp->GetIntMap();
    for (std::vector<std::pair<std::string,long> >::iterator it = lmap.begin(); it != lmap.end(); ++it) {
        if (name == QLatin1String(it->first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    int val = QInputDialog::getInt(this, QObject::tr("New integer item"), QObject::tr("Enter your number:"),
                                   0, -2147483647, 2147483647, 1, &ok);

    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterInt(this,name,(long)val, _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateUIntItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New unsigned item"), QObject::tr("Enter the name:"), 
                                         QLineEdit::Normal, QString::null, &ok);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,unsigned long> > lmap = _hcGrp->GetUnsignedMap();
    for (std::vector<std::pair<std::string,unsigned long> >::iterator it = lmap.begin(); it != lmap.end(); ++it) {
        if (name == QLatin1String(it->first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    DlgInputDialogImp dlg(QObject::tr("Enter your number:"),this, true, DlgInputDialogImp::UIntBox);
    dlg.setWindowTitle(QObject::tr("New unsigned item"));
    UIntSpinBox* edit = dlg.getUIntBox();
    edit->setRange(0,UINT_MAX);
    if (dlg.exec() == QDialog::Accepted ) {
        QString value = edit->text();
        unsigned long val = value.toULong(&ok);

        if ( ok )
        {
            ParameterValueItem *pcItem;
            pcItem = new ParameterUInt(this,name, val, _hcGrp);
            pcItem->appendToGroup();
        }
    }
}

void ParameterValue::onCreateFloatItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New float item"), QObject::tr("Enter the name:"), 
                                         QLineEdit::Normal, QString::null, &ok);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,double> > fmap = _hcGrp->GetFloatMap();
    for (std::vector<std::pair<std::string,double> >::iterator it = fmap.begin(); it != fmap.end(); ++it) {
        if (name == QLatin1String(it->first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }
  
    double val = QInputDialog::getDouble(this, QObject::tr("New float item"), QObject::tr("Enter your number:"), 
                                         0, -2147483647, 2147483647, 12, &ok);
    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterFloat(this,name,val, _hcGrp);
        pcItem->appendToGroup();
    }
}

void ParameterValue::onCreateBoolItem()
{
    bool ok;
    QString name = QInputDialog::getText(this, QObject::tr("New Boolean item"), QObject::tr("Enter the name:"), 
                                         QLineEdit::Normal, QString::null, &ok);

    if (!ok || !Gui::validateInput(this, name))
        return;

    std::vector<std::pair<std::string,bool> > bmap = _hcGrp->GetBoolMap();
    for (std::vector<std::pair<std::string,bool> >::iterator it = bmap.begin(); it != bmap.end(); ++it) {
        if (name == QLatin1String(it->first.c_str()))
        {
            QMessageBox::critical( this, tr("Existing item"),
                tr("The item '%1' already exists.").arg( name ) );
            return;
        }
    }

    QStringList list; list << QString::fromLatin1("true")
                           << QString::fromLatin1("false");
    QString val = QInputDialog::getItem (this, QObject::tr("New boolean item"), QObject::tr("Choose an item:"),
                                         list, 0, false, &ok);
    if ( ok )
    {
        ParameterValueItem *pcItem;
        pcItem = new ParameterBool(this,name,(val == list[0] ? true : false), _hcGrp);
        pcItem->appendToGroup();
    }
}

// ---------------------------------------------------------------------------

ParameterGroupItem::ParameterGroupItem( ParameterGroupItem * parent, const Base::Reference<ParameterGrp> &hcGrp )
    : QTreeWidgetItem( parent, QTreeWidgetItem::UserType+1 ), _hcGrp(hcGrp)
{
    setFlags(flags() | Qt::ItemIsEditable);
    fillUp();
}

ParameterGroupItem::ParameterGroupItem( QTreeWidget* parent, const Base::Reference<ParameterGrp> &hcGrp)
    : QTreeWidgetItem( parent, QTreeWidgetItem::UserType+1 ), _hcGrp(hcGrp)
{
    setFlags(flags() | Qt::ItemIsEditable);
    fillUp();
}

ParameterGroupItem::~ParameterGroupItem()
{
  // if the group has already been removed from the parameters then clear the observer list
  // as we cannot notify the attached observers here
  if (_hcGrp.getRefCount() == 1)
    _hcGrp->ClearObserver();
}

void ParameterGroupItem::fillUp(void)
{
    // filing up groups
    std::vector<Base::Reference<ParameterGrp> > vhcParamGrp = _hcGrp->GetGroups();

    setText(0,QString::fromUtf8(_hcGrp->GetGroupName()));
    for(std::vector<Base::Reference<ParameterGrp> >::iterator It=vhcParamGrp.begin();It!=vhcParamGrp.end();++It)
        (void)new ParameterGroupItem(this,*It);
}

void ParameterGroupItem::setData ( int column, int role, const QVariant & value )
{
    if (role == Qt::EditRole) {
        QString oldName = text(0);
        QString newName = value.toString();
        if (newName.isEmpty() || oldName == newName)
            return;

        if (!Gui::validateInput(treeWidget(), newName))
            return;

        // first check if there is already a group with name "newName"
        ParameterGroupItem* item = static_cast<ParameterGroupItem*>(parent());
        if ( !item )
        {
            QMessageBox::critical( treeWidget(), QObject::tr("Rename group"),
                QObject::tr("The group '%1' cannot be renamed.").arg( oldName ) );
            return;
        }
        if ( item->_hcGrp->HasGroup( newName.toLatin1() ) )
        {
            QMessageBox::critical( treeWidget(), QObject::tr("Existing group"),
                QObject::tr("The group '%1' already exists.").arg( newName ) );
            return;
        } 
        else 
        {
            // rename the group by adding a new group, copy the content and remove the old group
            Base::Reference<ParameterGrp> hOldGrp = item->_hcGrp->GetGroup( oldName.toLatin1() );
            Base::Reference<ParameterGrp> hNewGrp = item->_hcGrp->GetGroup( newName.toLatin1() );
            hOldGrp->copyTo( hNewGrp );
            item->_hcGrp->RemoveGrp( oldName.toLatin1() );
        }
    }

    QTreeWidgetItem::setData(column, role, value);
}

QVariant ParameterGroupItem::data ( int column, int role ) const
{
    if (role == Qt::DecorationRole) {
        // The root item should keep its special pixmap
        if (parent()) {
            return treeWidget()->isItemExpanded(this) ?
                QApplication::style()->standardPixmap(QStyle::SP_DirOpenIcon):
                QApplication::style()->standardPixmap(QStyle::SP_DirClosedIcon);
        }
    }

    return QTreeWidgetItem::data(column, role);
}

// --------------------------------------------------------------------

ParameterValueItem::ParameterValueItem ( QTreeWidget* parent, const Base::Reference<ParameterGrp> &hcGrp)
  : QTreeWidgetItem( parent ), _hcGrp(hcGrp)
{
    setFlags(flags() | Qt::ItemIsEditable);
}

ParameterValueItem::~ParameterValueItem()
{
}

void ParameterValueItem::setData ( int column, int role, const QVariant & value )
{
    if (role == Qt::EditRole) {
        QString oldName = text(0);
        QString newName = value.toString();
        if (newName.isEmpty() || oldName == newName)
            return;

        if (!Gui::validateInput(treeWidget(), newName))
            return;

        replace( oldName, newName );
    }

    QTreeWidgetItem::setData(column, role, value);
}

// --------------------------------------------------------------------

ParameterText::ParameterText ( QTreeWidget * parent, QString label, const char* value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, hcGrp)
{
    setIcon(0,BitmapFactory().pixmap("Param_Text") );
    setText(0, label);
    setText(1, QString::fromLatin1("Text"));
    setText(2, QString::fromUtf8(value));
}

ParameterText::~ParameterText()
{
}

void ParameterText::changeValue()
{
    bool ok;
    QString txt = QInputDialog::getText(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your text:"), 
                                        QLineEdit::Normal, text(2), &ok);
    if ( ok )
    {
        setText( 2, txt );
        _hcGrp->SetASCII(text(0).toLatin1(), txt.toUtf8());
    }
}

void ParameterText::removeFromGroup ()
{
    _hcGrp->RemoveASCII(text(0).toLatin1());
}

void ParameterText::replace( const QString& oldName, const QString& newName )
{
    std::string val = _hcGrp->GetASCII(oldName.toLatin1());
    _hcGrp->RemoveASCII(oldName.toLatin1());
    _hcGrp->SetASCII(newName.toLatin1(), val.c_str());
}

void ParameterText::appendToGroup()
{
    _hcGrp->SetASCII(text(0).toLatin1(), text(2).toUtf8());
}

// --------------------------------------------------------------------

ParameterInt::ParameterInt ( QTreeWidget * parent, QString label, long value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, hcGrp)
{
    setIcon(0,BitmapFactory().pixmap("Param_Int") );
    setText(0, label);
    setText(1, QString::fromLatin1("Integer"));
    setText(2, QString::fromLatin1("%1").arg(value));
}

ParameterInt::~ParameterInt()
{
}

void ParameterInt::changeValue()
{
    bool ok;
    int num = QInputDialog::getInt(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your number:"),
                                   text(2).toInt(), -2147483647, 2147483647, 1, &ok);
    if ( ok )
    {
        setText(2, QString::fromLatin1("%1").arg(num));
        _hcGrp->SetInt(text(0).toLatin1(), (long)num);
    }
}

void ParameterInt::removeFromGroup ()
{
    _hcGrp->RemoveInt(text(0).toLatin1());
}

void ParameterInt::replace( const QString& oldName, const QString& newName )
{
    long val = _hcGrp->GetInt(oldName.toLatin1());
    _hcGrp->RemoveInt(oldName.toLatin1());
    _hcGrp->SetInt(newName.toLatin1(), val);
}

void ParameterInt::appendToGroup()
{
    _hcGrp->SetInt(text(0).toLatin1(), text(2).toLong());
}

// --------------------------------------------------------------------

ParameterUInt::ParameterUInt ( QTreeWidget * parent, QString label, unsigned long value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, hcGrp)
{
    setIcon(0,BitmapFactory().pixmap("Param_UInt") );
    setText(0, label);
    setText(1, QString::fromLatin1("Unsigned"));
    setText(2, QString::fromLatin1("%1").arg(value));
}

ParameterUInt::~ParameterUInt()
{
}

void ParameterUInt::changeValue()
{
    bool ok;
    DlgInputDialogImp dlg(QObject::tr("Enter your number:"),treeWidget(), true, DlgInputDialogImp::UIntBox);
    dlg.setWindowTitle(QObject::tr("Change value"));
    UIntSpinBox* edit = dlg.getUIntBox();
    edit->setRange(0,UINT_MAX);
    edit->setValue(text(2).toULong());
    if (dlg.exec() == QDialog::Accepted)
    {
        QString value = edit->text();
        unsigned long num = value.toULong(&ok);

        if ( ok )
        {
            setText(2, QString::fromLatin1("%1").arg(num));
            _hcGrp->SetUnsigned(text(0).toLatin1(), (unsigned long)num);
        }
    }
}

void ParameterUInt::removeFromGroup ()
{
    _hcGrp->RemoveUnsigned(text(0).toLatin1());
}

void ParameterUInt::replace( const QString& oldName, const QString& newName )
{
    unsigned long val = _hcGrp->GetUnsigned(oldName.toLatin1());
    _hcGrp->RemoveUnsigned(oldName.toLatin1());
    _hcGrp->SetUnsigned(newName.toLatin1(), val);
}

void ParameterUInt::appendToGroup()
{
    _hcGrp->SetUnsigned(text(0).toLatin1(), text(2).toULong());
}

// --------------------------------------------------------------------

ParameterFloat::ParameterFloat ( QTreeWidget * parent, QString label, double value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, hcGrp)
{
    setIcon(0,BitmapFactory().pixmap("Param_Float") );
    setText(0, label);
    setText(1, QString::fromLatin1("Float"));
    setText(2, QString::fromLatin1("%1").arg(value));
}

ParameterFloat::~ParameterFloat()
{
}

void ParameterFloat::changeValue()
{
    bool ok;
    double num = QInputDialog::getDouble(treeWidget(), QObject::tr("Change value"), QObject::tr("Enter your number:"), 
                                         text(2).toDouble(), -2147483647, 2147483647, 12, &ok);
    if ( ok )
    {
        setText(2, QString::fromLatin1("%1").arg(num));
        _hcGrp->SetFloat(text(0).toLatin1(), num);
    }
}

void ParameterFloat::removeFromGroup ()
{
    _hcGrp->RemoveFloat(text(0).toLatin1());
}

void ParameterFloat::replace( const QString& oldName, const QString& newName )
{
    double val = _hcGrp->GetFloat(oldName.toLatin1());
    _hcGrp->RemoveFloat(oldName.toLatin1());
    _hcGrp->SetFloat(newName.toLatin1(), val);
}

void ParameterFloat::appendToGroup()
{
    _hcGrp->SetFloat(text(0).toLatin1(), text(2).toDouble());
}

// --------------------------------------------------------------------

ParameterBool::ParameterBool ( QTreeWidget * parent, QString label, bool value, const Base::Reference<ParameterGrp> &hcGrp)
  :ParameterValueItem( parent, hcGrp)
{
    setIcon(0,BitmapFactory().pixmap("Param_Bool") );
    setText(0, label);
    setText(1, QString::fromLatin1("Boolean"));
    setText(2, QString::fromLatin1((value ? "true" : "false")));
}

ParameterBool::~ParameterBool()
{
}

void ParameterBool::changeValue()
{
    bool ok;
    QStringList list; list << QString::fromLatin1("true") 
                           << QString::fromLatin1("false");
    int pos = (text(2) == list[0] ? 0 : 1);

    QString txt = QInputDialog::getItem (treeWidget(), QObject::tr("Change value"), QObject::tr("Choose an item:"),
                                         list, pos, false, &ok);
    if ( ok )
    {
        setText( 2, txt );
        _hcGrp->SetBool(text(0).toLatin1(), (txt == list[0] ? true : false) );
    }
}

void ParameterBool::removeFromGroup ()
{
    _hcGrp->RemoveBool(text(0).toLatin1());
}

void ParameterBool::replace( const QString& oldName, const QString& newName )
{
    bool val = _hcGrp->GetBool(oldName.toLatin1());
    _hcGrp->RemoveBool(oldName.toLatin1());
    _hcGrp->SetBool(newName.toLatin1(), val);
}

void ParameterBool::appendToGroup()
{
    bool val = (text(2) == QLatin1String("true") ? true : false);
    _hcGrp->SetBool(text(0).toLatin1(), val);
}

#include "moc_DlgParameterImp.cpp"
