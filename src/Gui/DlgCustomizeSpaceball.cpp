/***************************************************************************
 *   Copyright (c) 2011 Thomas Anderson <ta@nextgenengineering>            *
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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>
#include <QHeaderView>
#include <QPrintDialog>
#include <QPrinter>
#include <QPainter>
#include <QTableView>
#endif

#include "Base/Console.h"
#include "Application.h"
#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"
#include "Command.h"
#include "BitmapFactory.h"
#include "DlgCustomizeSpaceball.h"

typedef std::vector<Base::Reference<ParameterGrp> > GroupVector;

using namespace Gui::Dialog;

ButtonView::ButtonView(QWidget *parent) : QListView(parent)
{

}

void ButtonView::selectButton(int number)
{
    this->selectionModel()->select(this->model()->index(number, 0), QItemSelectionModel::ClearAndSelect);
}

void ButtonView::goSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected); 
    if (selected.indexes().isEmpty())
        return;
    QModelIndex select(selected.indexes().at(0));
    changeCommandSelection(this->model()->data(select, Qt::UserRole).toString());
}

void ButtonView::goChangedCommand(const QString& commandName)
{
    QModelIndex index(this->currentIndex());
    ButtonModel *model = dynamic_cast<ButtonModel*>(this->model());
    if (model && index.isValid())
        model->setCommand(index.row(), commandName);
}

///////////////////////////////////////////////////////////////////////////////////////

ButtonModel::ButtonModel(QObject *parent) : QAbstractListModel(parent)
{

}

int ButtonModel::rowCount (const QModelIndex &parent) const
{
    Q_UNUSED(parent); 
    return spaceballButtonGroup()->GetGroups().size();
}

QVariant ButtonModel::data (const QModelIndex &index, int role) const
{
    GroupVector groupVector = spaceballButtonGroup()->GetGroups();
    if (index.row() >= (int)groupVector.size())
    {
        Base::Console().Log("index error in ButtonModel::data\n");
        return QVariant();
    }
    if (role == Qt::DisplayRole)
        return QVariant(getLabel(index.row()));
    if (role == Qt::DecorationRole)
    {
        static QPixmap icon(BitmapFactory().pixmap("spaceball_button").scaled
                            (32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        return QVariant(icon);
    }
    if (role == Qt::UserRole)
        return QVariant(QString::fromStdString(groupVector.at(index.row())->GetASCII("Command")));
    if (role == Qt::SizeHintRole)
        return QVariant(QSize(32, 32));
    return QVariant();
}

void ButtonModel::insertButtonRows(int number)
{
    int buttonCount = spaceballButtonGroup()->GetGroups().size();
    beginInsertRows(QModelIndex(), buttonCount, number-buttonCount+1);
    for (int index = buttonCount; index < number + 1; ++index)
    {
        QString groupName;
        groupName.setNum(index);
        Base::Reference<ParameterGrp> newGroup = spaceballButtonGroup()->GetGroup(groupName.toLatin1());//builds the group.
        newGroup->SetASCII("Command", "");
    }
    endInsertRows();
    return;
}

void ButtonModel::setCommand(int row, QString command)
{
    GroupVector groupVector = spaceballButtonGroup()->GetGroups();
    groupVector.at(row)->SetASCII("Command", command.toLatin1());
}

void ButtonModel::goButtonPress(int number)
{
    QString numberString;
    numberString.setNum(number);
    if (!spaceballButtonGroup()->HasGroup(numberString.toLatin1()))
        insertButtonRows(number);
}

void ButtonModel::goMacroRemoved(const QByteArray& macroName)
{
    GroupVector groupVector = spaceballButtonGroup()->GetGroups();
    for (GroupVector::iterator it = groupVector.begin(); it != groupVector.end(); ++it)
        if (std::string(macroName.data()) == (*it)->GetASCII("Command"))
            (*it)->SetASCII("Command", "");
}

void ButtonModel::goClear()
{
    if (this->rowCount() < 1)
        return;
    this->beginRemoveRows(QModelIndex(), 0, this->rowCount()-1);
    spaceballButtonGroup()->Clear();
    this->endRemoveRows();
}

ParameterGrp::handle ButtonModel::spaceballButtonGroup() const
{
    static ParameterGrp::handle group = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
            GetGroup("Spaceball")->GetGroup("Buttons");
    return group;
}

QString ButtonModel::getLabel(const int &number) const
{
    if (number > -1 && number < 20)
        return tr("Button %1").arg(number+1);
    else
        return tr("Out Of Range");
}

//////////////////////////////////////////////////////////////////////////////////////////

CommandView::CommandView(QWidget *parent) : QTreeView(parent)
{
    this->setEnabled(false);
    connect(this, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(goClicked(const QModelIndex&)));
}

void CommandView::goChangeCommandSelection(const QString& commandName)
{
    if (!this->isEnabled())
        this->setEnabled(true);
    this->selectionModel()->clear();
    this->collapseAll();
    if (commandName.isEmpty())
        return;
    QModelIndexList index(this->model()->match(this->model()->index(0,0), Qt::UserRole, QVariant(commandName), 1,
                                               Qt::MatchWrap | Qt::MatchRecursive));
    if (index.size() < 1)
        return;
    this->expand(index.at(0));
    this->setCurrentIndex(index.at(0));
}

void CommandView::goClicked(const QModelIndex &index)
{
    if (index.flags() & Qt::ItemIsSelectable)
    {
        QString commandName = this->model()->data(index, Qt::UserRole).toString();
        if (commandName.isEmpty())
            return;
        changedCommand(commandName);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

CommandNode::CommandNode(NodeType typeIn)
{
    nodeType = typeIn;
    parent = 0;
    children.clear();
    aCommand = 0;
}

CommandNode::~CommandNode()
{
    qDeleteAll(children);
}

/////////////////////////////////////////////////////////////////////////////////////////

CommandModel::CommandModel(QObject *parent) : QAbstractItemModel(parent)
{
    rootNode = 0;
    initialize();
}

CommandModel::~CommandModel()
{
    delete rootNode;
    rootNode = 0;
}

QModelIndex CommandModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!rootNode)
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, rootNode->children.at(row));

    CommandNode *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return QModelIndex();
    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex CommandModel::parent(const QModelIndex &index) const
{
    CommandNode *base = nodeFromIndex(index);
    if (!base)
        return QModelIndex();
    CommandNode *parentNode = base->parent;
    if (!parentNode)
        return QModelIndex();
    CommandNode *grandParentNode = parentNode->parent;
    if (!grandParentNode)
        return QModelIndex();

    int row = grandParentNode->children.indexOf(parentNode);
    if (row == -1)
        return QModelIndex();
    return createIndex(row, index.column(), parentNode);
}

int CommandModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return rootNode->children.size();

    CommandNode *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return 0;
    return parentNode->children.count();
}

int CommandModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); 
    return 1;
}

QVariant CommandModel::data(const QModelIndex &index, int role) const
{
    CommandNode *node = nodeFromIndex(index);
    if (!node)
        return QVariant();
    if (role == Qt::DisplayRole)
    {
        if (node->nodeType == CommandNode::CommandType)
            return QVariant(qApp->translate(node->aCommand->className(), node->aCommand->getMenuText()));
        if (node->nodeType == CommandNode::GroupType)
        {
            if (node->children.size() < 1)
                return QVariant();
            CommandNode *childNode = node->children.at(0);
            return QVariant(qApp->translate(childNode->aCommand->className(), childNode->aCommand->getGroupName()));
        }
        return QVariant();
    }
    if (role == Qt::DecorationRole)
    {
        if (node->nodeType == CommandNode::CommandType)
        {
            if (node->aCommand->getPixmap())
                return QVariant(BitmapFactory().pixmap(node->aCommand->getPixmap()).scaled
                                (32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
    }
    if (role == Qt::SizeHintRole)
        if (node->nodeType == CommandNode::CommandType)
            return QVariant(QSize(32, 32));
    if (role == Qt::UserRole)
    {
        if (node->nodeType == CommandNode::CommandType)
            return QVariant(QString::fromLatin1(node->aCommand->getName()));
        if (node->nodeType == CommandNode::GroupType)
        {
            if (node->children.size() < 1)
                return QVariant();
            CommandNode *childNode = node->children.at(0);
            return QVariant(QString::fromLatin1(childNode->aCommand->getGroupName()));
        }
        return QVariant();
    }
    if (role == Qt::ToolTipRole)
        if (node->nodeType == CommandNode::CommandType)
            return QVariant(QString::fromLatin1(node->aCommand->getToolTipText()));
    return QVariant();
}

QVariant CommandModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section == 0)
        return QVariant(tr("Commands"));
    return QVariant();
}

Qt::ItemFlags CommandModel::flags (const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    CommandNode *node = nodeFromIndex(index);
    if (!node)
        return Qt::NoItemFlags;
    if (node->nodeType == CommandNode::CommandType)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::NoItemFlags;
}

CommandNode* CommandModel::nodeFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<CommandNode *>(index.internalPointer());
    return rootNode;
}

void CommandModel::goAddMacro(const QByteArray &macroName)
{
    QModelIndexList indexList(this->match(this->index(0,0), Qt::UserRole, QVariant(QString::fromLatin1("Macros")),
                                          1, Qt::MatchWrap | Qt::MatchRecursive));
    QModelIndex macrosIndex;
    if (indexList.size() < 1)
    {
        //this is the first macro and we have to add the Macros item.
        //figure out where to insert it. Should be in the command groups now.
        QStringList groups = orderedGroups();
        int location(groups.indexOf(QString::fromLatin1("Macros")));
        if (location == -1)
            location = groups.size();
        //add row
        this->beginInsertRows(QModelIndex(), location, location);
        CommandNode *macroNode = new CommandNode(CommandNode::GroupType);
        macroNode->parent = rootNode;
        rootNode->children.insert(location, macroNode);
        this->endInsertRows();
        macrosIndex = this->index(location, 0);
    }
    else
        macrosIndex = indexList.at(0);

    Command *command = 0;
    command = Application::Instance->commandManager().getCommandByName(macroName);
    if (!command)
        return;

    CommandNode *parentNode = nodeFromIndex(macrosIndex);
    if (!parentNode)
        return;

    this->beginInsertRows(macrosIndex, parentNode->children.size(), parentNode->children.size());
    CommandNode *childNode = new CommandNode(CommandNode::CommandType);
    childNode->parent = parentNode;
    parentNode->children.push_back(childNode);
    childNode->aCommand = command;
    this->endInsertRows();
}

void CommandModel::goRemoveMacro(const QByteArray &macroName)
{
    QModelIndexList macroList(this->match(this->index(0,0), Qt::UserRole, QVariant(QString::fromLatin1(macroName.data())),
                                          1, Qt::MatchWrap | Qt::MatchRecursive));
    if (macroList.isEmpty())
        return;

    QModelIndex childIndex(macroList.at(0));
    QModelIndex parentIndex(this->parent(childIndex));
    if (!childIndex.isValid() || !parentIndex.isValid())
        return;

    CommandNode *parentNode = nodeFromIndex(parentIndex);
    if (!parentNode)
        return;

    this->beginRemoveRows(parentIndex, childIndex.row(), childIndex.row());
    delete parentNode->children.takeAt(childIndex.row());
    this->endRemoveRows();
    if (parentNode->children.isEmpty())
    {
        QModelIndex grandParentIndex(this->parent(parentIndex));//this should be root.
        CommandNode *grandParentNode = nodeFromIndex(grandParentIndex);
        this->beginRemoveRows(grandParentIndex, parentIndex.row(), parentIndex.row());
        delete grandParentNode->children.takeAt(parentIndex.row());
        this->endRemoveRows();
    }
}

void CommandModel::initialize()
{
    rootNode = new CommandNode(CommandNode::RootType);
    QStringList groups(orderedGroups());
    for (QStringList::iterator it = groups.begin(); it != groups.end(); ++it)
        groupCommands(*it);
}

void CommandModel::groupCommands(const QString& groupName)
{
    CommandNode *parentNode = new CommandNode(CommandNode::GroupType);
    parentNode->parent = rootNode;
    rootNode->children.push_back(parentNode);
    std::vector <Command*> commands = Application::Instance->commandManager().getGroupCommands(groupName.toLatin1());
    for (std::vector <Command*>::iterator it = commands.begin(); it != commands.end(); ++it)
    {
        CommandNode *childNode = new CommandNode(CommandNode::CommandType);
        childNode->parent = parentNode;
        parentNode->children.push_back(childNode);
        childNode->aCommand = *it;
    }
}

QStringList CommandModel::orderedGroups()
{
    QStringList groups;
    std::vector <Command*> commands = Application::Instance->commandManager().getAllCommands();
    for (std::vector <Command*>::iterator it = commands.begin(); it != commands.end(); ++it)
    {
        QString groupName(QString::fromLatin1((*it)->getGroupName()));
        if (!groups.contains(groupName))
            groups << groupName;
    }
    //how to sort?
    groups.sort();
    return groups;
}

///////////////////////////////////////////////////////////////////////////////////////

PrintModel::PrintModel(QObject *parent, ButtonModel *buttonModelIn, CommandModel *commandModelIn) : QAbstractTableModel(parent)
{
    buttonModel = buttonModelIn;
    commandModel = commandModelIn;
}

int PrintModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); 
    return buttonModel->rowCount();
}

int PrintModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent); 
    return 2;
}

QVariant PrintModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0)
    {
        //button column;
        return buttonModel->data(buttonModel->index(index.row(), 0), role);
    }

    if (index.column() == 1)
    {
        //command column;
        QString commandName(buttonModel->data(buttonModel->index(index.row(), 0), Qt::UserRole).toString());
        if (commandName.isEmpty())
            return (QVariant());

        QModelIndexList indexList(commandModel->match(commandModel->index(0,0), Qt::UserRole, QVariant(commandName), 1,
                                                   Qt::MatchWrap | Qt::MatchRecursive));
        if (indexList.isEmpty())
            return QVariant();

        return commandModel->data(indexList.at(0), role);
    }
    return QVariant();
}

QVariant PrintModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();
    if (section == 0)
        return QVariant(tr("Button"));
    if (section == 1)
        return QVariant(tr("Command"));
    else
        return QVariant();
}

///////////////////////////////////////////////////////////////////////////////////////

DlgCustomizeSpaceball::DlgCustomizeSpaceball(QWidget *parent)
  : CustomizeActionPage(parent), buttonView(0), buttonModel(0),
    commandView(0), commandModel(0), clearButton(0), printReference(0)
{
    this->setWindowTitle(tr("Spaceball Buttons"));
    GUIApplicationNativeEventAware *app = qobject_cast<GUIApplicationNativeEventAware *>(QApplication::instance());
    if (!app)
        return;
    if (!app->isSpaceballPresent())
    {
        this->setMessage(tr("No Spaceball Present"));
        return;
    }

    setupButtonModelView();
    setupCommandModelView();
    connect(buttonView, SIGNAL(changeCommandSelection(const QString&)),
            commandView, SLOT(goChangeCommandSelection(const QString&)));
    connect(commandView, SIGNAL(changedCommand(const QString&)),
            buttonView, SLOT(goChangedCommand(const QString&)));
    setupLayout();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(goClear()));
    connect(printReference, SIGNAL(clicked()), this, SLOT(goPrint()));
}

DlgCustomizeSpaceball::~DlgCustomizeSpaceball()
{

}

void DlgCustomizeSpaceball::setMessage(const QString& message)
{
    QLabel *messageLabel = new QLabel(message,this);
    QVBoxLayout *layout = new QVBoxLayout();
    QHBoxLayout *layout2 = new QHBoxLayout();
    layout2->addStretch();
    layout2->addWidget(messageLabel);
    layout2->addStretch();
    layout->addItem(layout2);
    this->setLayout(layout);
}

void DlgCustomizeSpaceball::setupButtonModelView()
{
    buttonModel = new ButtonModel(this);
    buttonView = new ButtonView(this);
    buttonView->setModel(buttonModel);

    //had to do this here as the views default selection model is not created until after construction.
    connect(buttonView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            buttonView, SLOT(goSelectionChanged(const QItemSelection&, const QItemSelection&)));
}

void DlgCustomizeSpaceball::setupCommandModelView()
{
    commandModel = new CommandModel(this);
    commandView = new CommandView(this);
    commandView->setModel(commandModel);
}

void DlgCustomizeSpaceball::setupLayout()
{
    QLabel *buttonLabel = new QLabel(tr("Buttons"), this);
    clearButton = new QPushButton(tr("Clear"), this);
    QVBoxLayout *buttonGroup = new QVBoxLayout();
    buttonGroup->addWidget(buttonLabel);
    buttonGroup->addWidget(buttonView);
    QHBoxLayout *clearLayout = new QHBoxLayout();
    clearLayout->addWidget(clearButton);
    clearLayout->addStretch();
    buttonGroup->addLayout(clearLayout);

    QSplitter *splitter = new QSplitter(this);
    QWidget *leftPane = new QWidget(this);
    leftPane->setLayout(buttonGroup);
    splitter->addWidget(leftPane);
    splitter->addWidget(commandView);

    printReference = new QPushButton(tr("Print Reference"), this);
    QHBoxLayout *printLayout = new QHBoxLayout();
    printLayout->addStretch();
    printLayout->addWidget(printReference);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(splitter);
    layout->addLayout(printLayout);

    this->setLayout(layout);

    QList<int> sizes;
    sizes << this->size().width()*0.40;
    sizes << this->size().width()-sizes.at(0);
    splitter->setSizes(sizes);
}

void DlgCustomizeSpaceball::goClear()
{
    commandView->clearSelection();
    commandView->collapseAll();
    commandView->setDisabled(true);
    buttonModel->goClear();
}

void DlgCustomizeSpaceball::goPrint()
{
    QTableView *view = new QTableView(this);
    PrintModel *model = new PrintModel(this, buttonModel, commandModel);
#if QT_VERSION >= 0x050000
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
#else
    view->horizontalHeader()->setResizeMode(QHeaderView::Fixed);
#endif
    view->setModel(model);
    view->horizontalHeader()->resizeSection(0, 150);
    view->horizontalHeader()->resizeSection(1, 300);
    view->resize(600, 600);

    QPrinter printer;
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted)
    {
        QPainter p(&printer);
        view->render(&p);
    }
}

bool DlgCustomizeSpaceball::event(QEvent *event)
{
    if (event->type() != Spaceball::ButtonEvent::ButtonEventType)
        return CustomizeActionPage::event(event);
    Spaceball::ButtonEvent *buttonEvent = dynamic_cast<Spaceball::ButtonEvent *>(event);
    if (!buttonEvent)
        return true;
    buttonEvent->setHandled(true);
    if (buttonEvent->buttonStatus() == Spaceball::BUTTON_PRESSED)
        buttonModel->goButtonPress(buttonEvent->buttonNumber());
    buttonView->selectButton(buttonEvent->buttonNumber());

    return true;
}

void DlgCustomizeSpaceball::hideEvent(QHideEvent *event)
{
    //having a crash with the last item of the macro command list
    //being selected and that macro is removed from the macro tab. Hopefully
    //clearing the selection will cure the problem.
    if (buttonView)
        buttonView->selectionModel()->clear();
    if (commandView) {
        commandView->selectionModel()->clear();
        commandView->collapseAll();
        commandView->setEnabled(false);
    }

    CustomizeActionPage::hideEvent(event);
}

void DlgCustomizeSpaceball::showEvent (QShowEvent *event)
{
    if (buttonView)
        buttonView->setFocus();

    CustomizeActionPage::showEvent(event);
}

void DlgCustomizeSpaceball::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        //I don't think I need do anything here. Qt should take care of it?
//        this->setWindowTitle(tr("Spaceball"));
    }
    QWidget::changeEvent(e);
}

void DlgCustomizeSpaceball::onAddMacroAction(const QByteArray &macroName)
{
    //need to get the new macro to model.
    if (commandModel)
        commandModel->goAddMacro(macroName);
}

void DlgCustomizeSpaceball::onRemoveMacroAction(const QByteArray &macroName)
{
    //need to remove macro from model.
    if (commandModel)
        commandModel->goRemoveMacro(macroName);
    //need to change any button mapped to macro to an empty string.
    if (buttonModel)
        buttonModel->goMacroRemoved(macroName);
}

void DlgCustomizeSpaceball::onModifyMacroAction(const QByteArray &macroName)
{
    //don't think I need to do anything here.
    Q_UNUSED(macroName); 
}

#include "moc_DlgCustomizeSpaceball.cpp"
