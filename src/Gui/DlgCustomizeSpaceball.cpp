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
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#endif

#include "Base/Console.h"

#include "Command.h"
#include "DlgCustomizeSpaceball.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "GuiApplicationNativeEventAware.h"
#include "SpaceballEvent.h"


using GroupVector = std::vector<Base::Reference<ParameterGrp> >;

using namespace Gui::Dialog;

ButtonView::ButtonView(QWidget *parent) : QListView(parent)
{

}

void ButtonView::selectButton(int number)
{
    this->selectionModel()->select(this->model()->index(number, 0), QItemSelectionModel::ClearAndSelect);
    this->scrollTo(this->model()->index(number, 0), QAbstractItemView::EnsureVisible);
}

void ButtonView::goSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);
    if (selected.indexes().isEmpty())
        return;
    QModelIndex select(selected.indexes().at(0));
    Q_EMIT changeCommandSelection(this->model()->data(select, Qt::UserRole).toString());
}

void ButtonView::goChangedCommand(const QString& commandName)
{
    QModelIndex index(this->currentIndex());
    auto model = dynamic_cast<ButtonModel*>(this->model());
    if (model && index.isValid())
        model->setCommand(index.row(), commandName);
}

///////////////////////////////////////////////////////////////////////////////////////

ButtonModel::ButtonModel(QObject *parent) : QAbstractListModel(parent)
{
   //load3DConnexionButtons("SpacePilot Pro");
}

// Process the given Mapping tree to load in the Button mappings.
void ButtonModel::load3DConnexionButtonMapping(boost::property_tree::ptree ButtonMapTree)
{
   spaceballButtonGroup()->Clear();

   BOOST_FOREACH(const boost::property_tree::ptree::value_type &Map, ButtonMapTree.get_child("Mapping"))
   {
      if ("Map" == Map.first)
      {
         std::string ButtonDescription;
         std::string ButtonCode;
         std::string ButtonCommand;
         std::string ButtonDownTime;

         // Inspect Map attributes
         BOOST_FOREACH(const boost::property_tree::ptree::value_type &kv, Map.second.get_child("<xmlattr>"))
         {
            std::string Attribute;
            std::string Value;

            Attribute = kv.first.data();
            Value = kv.second.data();

            if (0 == Attribute.compare("Description"))
            {
               ButtonDescription = Value;
            }
            if (0 == Attribute.compare("KeyCode"))
            {
               ButtonCode = Value;
            }
            if (0 == Attribute.compare("DownTime"))
            {
               ButtonDownTime = Value;
            }
            if (0 == Attribute.compare("Command"))
            {
               ButtonCommand = Value;
            }
         }

         // ButtonCode is mandatory, the remaining attributes optional.
         if (!ButtonCode.empty())
         {
            Base::Reference<ParameterGrp> newGroup;

            newGroup = spaceballButtonGroup()->GetGroup(ButtonCode.c_str());
            newGroup->SetASCII("Command", ButtonCommand.c_str());
            newGroup->SetASCII("Description", ButtonDescription.c_str());
         }
      }
   }
}

// Optionally preload Button model with 3DConnexion configuration to match Solidworks
// For now the Button mapping file (3DConnexion.xml) is held the same folder as the FreeCAD executable.
void ButtonModel::load3DConnexionButtons(const char *RequiredDeviceName)
{
   try
   {
      boost::property_tree::ptree tree;
      boost::property_tree::ptree DeviceTree;

      // exception thrown if no file found
      std::string path = App::Application::getResourceDir();
      path += "3Dconnexion/3DConnexion.xml";
      read_xml(path.c_str(), tree);

      BOOST_FOREACH(const boost::property_tree::ptree::value_type &ButtonMap, tree.get_child(""))
      {
         if ("ButtonMap" == ButtonMap.first)
         {
            // Inspect ButtonMap attributes for DeviceName
            BOOST_FOREACH(const boost::property_tree::ptree::value_type &kv, ButtonMap.second.get_child("<xmlattr>"))
            {
               std::string Attribute;
               std::string Value;

               Attribute = kv.first.data();
               Value = kv.second.data();

               if (0 == Attribute.compare("DeviceName"))
               {
                  if (0 == Value.compare(RequiredDeviceName))
                  {
                     // We found the ButtonMap we want to load up
                     DeviceTree = ButtonMap.second;
                  }
               }
            }
         }
      }
      // If we found the required devices ButtonMap
      if (!DeviceTree.empty())
      {
         load3DConnexionButtonMapping(DeviceTree);
      }
   }
   catch (const std::exception& e)
   {
      // We don't mind not finding the file to be opened
      Base::Console().Warning("%s\n", e.what());
   }
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
        return {};
    }
    if (role == Qt::DisplayRole)
        return {getLabel(index.row())};
    if (role == Qt::DecorationRole)
    {
        static QPixmap icon(BitmapFactory().pixmap("spaceball_button").scaled
                            (32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        return {icon};
    }
    if (role == Qt::UserRole)
        return {QString::fromStdString(groupVector.at(index.row())->GetASCII("Command"))};
    if (role == Qt::SizeHintRole)
        return {QSize(32, 32)};
    return {};
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
        newGroup->SetASCII("Description", "");
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
    for (auto & it : groupVector)
        if (std::string(macroName.data()) == it->GetASCII("Command"))
            it->SetASCII("Command", "");
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
    if (number > -1 && number < 32) {
        QString numberString;
        numberString.setNum(number);
        QString desc = QString::fromStdString(spaceballButtonGroup()->
                                              GetGroup(numberString.toLatin1())->
                                              GetASCII("Description",""));
        if (desc.length())
            desc = QString::fromUtf8(" \"") + desc + QString::fromUtf8("\"");
        return tr("Button %1").arg(number + 1) + desc;
    } else
        return tr("Out Of Range");
}

void ButtonModel::loadConfig(const char *RequiredDeviceName)
{
    goClear();
    if (!RequiredDeviceName) {
        return;
    }
    load3DConnexionButtons(RequiredDeviceName);
}

//////////////////////////////////////////////////////////////////////////////////////////

CommandView::CommandView(QWidget *parent) : QTreeView(parent)
{
    this->setEnabled(false);
    connect(this, &QTreeView::clicked, this, &CommandView::goClicked);
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
    if (index.empty())
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
        Q_EMIT changedCommand(commandName);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////

CommandNode::CommandNode(NodeType typeIn)
{
    //NOLINTBEGIN
    nodeType = typeIn;
    parent = nullptr;
    children.clear();
    aCommand = nullptr;
    //NOLINTEND
}

CommandNode::~CommandNode()
{
    qDeleteAll(children);
}

/////////////////////////////////////////////////////////////////////////////////////////

CommandModel::CommandModel(QObject *parent) : QAbstractItemModel(parent)
{
    //NOLINTBEGIN
    rootNode = nullptr;
    initialize();
    //NOLINTEND
}

CommandModel::~CommandModel()
{
    delete rootNode;
    rootNode = nullptr;
}

QModelIndex CommandModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!rootNode)
        return {};
    if (!parent.isValid())
        return createIndex(row, column, rootNode->children.at(row));

    CommandNode *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return {};
    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex CommandModel::parent(const QModelIndex &index) const
{
    CommandNode *base = nodeFromIndex(index);
    if (!base)
        return {};
    CommandNode *parentNode = base->parent;
    if (!parentNode)
        return {};
    CommandNode *grandParentNode = parentNode->parent;
    if (!grandParentNode)
        return {};

    int row = grandParentNode->children.indexOf(parentNode);
    if (row == -1)
        return {};
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
        return {};
    if (role == Qt::DisplayRole)
    {
        if (node->nodeType == CommandNode::CommandType)
            return {qApp->translate(node->aCommand->className(), node->aCommand->getMenuText())};
        if (node->nodeType == CommandNode::GroupType)
        {
            if (node->children.empty())
                return {};
            CommandNode *childNode = node->children.at(0);
            return {qApp->translate(childNode->aCommand->className(), childNode->aCommand->getGroupName())};
        }
        return {};
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
    if (role == Qt::SizeHintRole) {
        if (node->nodeType == CommandNode::CommandType)
            return {QSize(32, 32)};
    }
    if (role == Qt::UserRole)
    {
        if (node->nodeType == CommandNode::CommandType)
            return {QString::fromLatin1(node->aCommand->getName())};
        if (node->nodeType == CommandNode::GroupType)
        {
            if (node->children.empty())
                return {};
            CommandNode *childNode = node->children.at(0);
            return {QString::fromLatin1(childNode->aCommand->getGroupName())};
        }
        return {};
    }
    if (role == Qt::ToolTipRole) {
        if (node->nodeType == CommandNode::CommandType)
            return {QString::fromLatin1(node->aCommand->getToolTipText())};
    }
    return {};
}

QVariant CommandModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal && section == 0)
        return {tr("Commands")};
    return {};
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
    if (indexList.empty())
    {
        //this is the first macro and we have to add the Macros item.
        //figure out where to insert it. Should be in the command groups now.
        QStringList groups = orderedGroups();
        int location(groups.indexOf(QString::fromLatin1("Macros")));
        if (location == -1)
            location = groups.size();
        //add row
        this->beginInsertRows(QModelIndex(), location, location);
        auto macroNode = new CommandNode(CommandNode::GroupType);
        macroNode->parent = rootNode;
        rootNode->children.insert(location, macroNode);
        this->endInsertRows();
        macrosIndex = this->index(location, 0);
    }
    else
        macrosIndex = indexList.at(0);

    Command *command = nullptr;
    command = Application::Instance->commandManager().getCommandByName(macroName);
    if (!command)
        return;

    CommandNode *parentNode = nodeFromIndex(macrosIndex);
    if (!parentNode)
        return;

    this->beginInsertRows(macrosIndex, parentNode->children.size(), parentNode->children.size());
    auto childNode = new CommandNode(CommandNode::CommandType);
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
    for (const auto & group : groups)
        groupCommands(group);
}

void CommandModel::groupCommands(const QString& groupName)
{
    auto parentNode = new CommandNode(CommandNode::GroupType);
    parentNode->parent = rootNode;
    rootNode->children.push_back(parentNode);
    std::vector <Command*> commands = Application::Instance->commandManager().getGroupCommands(groupName.toLatin1());
    for (const auto & command : commands)
    {
        auto childNode = new CommandNode(CommandNode::CommandType);
        childNode->parent = parentNode;
        parentNode->children.push_back(childNode);
        childNode->aCommand = command;
    }
}

QStringList CommandModel::orderedGroups()
{
    QStringList groups;
    std::vector <Command*> commands = Application::Instance->commandManager().getAllCommands();
    for (const auto & command : commands)
    {
        QString groupName(QString::fromLatin1(command->getGroupName()));
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
    //NOLINTBEGIN
    buttonModel = buttonModelIn;
    commandModel = commandModelIn;
    //NOLINTEND
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
            return {};

        QModelIndexList indexList(commandModel->match(commandModel->index(0,0), Qt::UserRole, QVariant(commandName), 1,
                                                   Qt::MatchWrap | Qt::MatchRecursive));
        if (indexList.isEmpty())
            return {};

        return commandModel->data(indexList.at(0), role);
    }
    return {};
}

QVariant PrintModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    if (section == 0)
        return {tr("Button")};
    if (section == 1)
        return {tr("Command")};
    else
        return {};
}

///////////////////////////////////////////////////////////////////////////////////////

DlgCustomizeSpaceball::DlgCustomizeSpaceball(QWidget *parent)
  : CustomizeActionPage(parent)
  , buttonView(nullptr)
  , buttonModel(nullptr)
  , commandView(nullptr)
  , commandModel(nullptr)
  , clearButton(nullptr)
  , printReference(nullptr)
  , devModel(nullptr)
{
    this->setWindowTitle(tr("Spaceball Buttons"));
    auto app = qobject_cast<GUIApplicationNativeEventAware *>(QApplication::instance());
    if (!app)
        return;
    if (!app->isSpaceballPresent())
    {
        this->setMessage(tr("No Spaceball Present"));
        return;
    }

    setupButtonModelView();
    setupCommandModelView();
    connect(buttonView, &ButtonView::changeCommandSelection,
            commandView, &CommandView::goChangeCommandSelection);
    connect(commandView, &CommandView::changedCommand,
            buttonView, &ButtonView::goChangedCommand);
    setupLayout();
    connect(clearButton, &QPushButton::clicked, this, &DlgCustomizeSpaceball::goClear);
    connect(printReference, &QPushButton::clicked, this, &DlgCustomizeSpaceball::goPrint);
}

DlgCustomizeSpaceball::~DlgCustomizeSpaceball() = default;

void DlgCustomizeSpaceball::setMessage(const QString& message)
{
    auto messageLabel = new QLabel(message,this);
    auto layout = new QVBoxLayout();
    auto layout2 = new QHBoxLayout();
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
    connect(buttonView->selectionModel(), &QItemSelectionModel::selectionChanged,
            buttonView, &ButtonView::goSelectionChanged);
}

void DlgCustomizeSpaceball::setupCommandModelView()
{
    commandModel = new CommandModel(this);
    commandView = new CommandView(this);
    commandView->setModel(commandModel);
}

void DlgCustomizeSpaceball::setupLayout()
{
    auto buttonLabel = new QLabel(tr("Buttons"), this);
    clearButton = new QPushButton(tr("Reset"), this);
    devModel = new QComboBox(this);

    // Load the devModel(s) from the config xml file
    devModel->addItems(getModels());

    // Select the current preference or the first entry
    QString model = QString::fromStdString(App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
            GetGroup("Spaceball")->GetASCII("Model",""));
    if (model.length() > 0) {
        devModel->setCurrentIndex(devModel->findText(model));
    } else {
        devModel->setCurrentIndex(0);
    }

    auto buttonGroup = new QVBoxLayout();
    buttonGroup->addWidget(buttonLabel);
    buttonGroup->addWidget(buttonView);
    auto clearLayout = new QHBoxLayout();
    clearLayout->addWidget(devModel);
    clearLayout->addWidget(clearButton);
    clearLayout->addStretch();
    buttonGroup->addLayout(clearLayout);

    auto splitter = new QSplitter(this);
    auto leftPane = new QWidget(this);
    leftPane->setLayout(buttonGroup);
    splitter->addWidget(leftPane);
    splitter->addWidget(commandView);

    printReference = new QPushButton(tr("Print Reference"), this);
    auto printLayout = new QHBoxLayout();
    printLayout->addStretch();
    printLayout->addWidget(printReference);

    auto layout = new QVBoxLayout();
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
    //buttonModel->goClear();

    QByteArray currentDevice = devModel->currentText().toLocal8Bit();
    App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
            GetGroup("Spaceball")->SetASCII("Model", currentDevice.data());
    buttonModel->loadConfig(currentDevice.data());
}

void DlgCustomizeSpaceball::goPrint()
{
    auto view = new QTableView(this);
    auto model = new PrintModel(this, buttonModel, commandModel);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
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
    auto buttonEvent = dynamic_cast<Spaceball::ButtonEvent *>(event);
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

QStringList DlgCustomizeSpaceball::getModels()
{
    QStringList modelList;
    try
    {
       boost::property_tree::ptree tree;
       boost::property_tree::ptree DeviceTree;

       // exception thrown if no file found
       std::string path = App::Application::getResourceDir();
       path += "3Dconnexion/3DConnexion.xml";
       read_xml(path.c_str(), tree);

       BOOST_FOREACH(const boost::property_tree::ptree::value_type &ButtonMap, tree.get_child(""))
       {
          if ("ButtonMap" == ButtonMap.first)
          {
             // Inspect ButtonMap attributes for DeviceName
             BOOST_FOREACH(const boost::property_tree::ptree::value_type &kv, ButtonMap.second.get_child("<xmlattr>"))
             {
                std::string Attribute;
                std::string Value;

                Attribute = kv.first.data();
                Value = kv.second.data();

                if (0 == Attribute.compare("DeviceName"))
                {
                    modelList << QString::fromStdString(Value);
                }
             }
          }
       }
    }
    catch (const std::exception& e)
    {
       // We don't mind not finding the file to be opened
       Base::Console().Warning("%s\n", e.what());
    }

    return modelList;
}

#include "moc_DlgCustomizeSpaceball.cpp"
