/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QFontMetrics>
#include <QMessageBox>
#endif

#include <Base/Interpreter.h>
#include <Gui/MainWindow.h>

#include "UnitTestImp.h"
#include "ui_UnitTest.h"


using namespace TestGui;

/* TRANSLATOR TestGui::UnitTestDialog */

UnitTestDialog* UnitTestDialog::_instance = nullptr;

/**
 * Creates and returns the one and only instance of this dialog.
 */
UnitTestDialog* UnitTestDialog::instance()
{
    // not initialized?
    if (!_instance) {
        _instance = new UnitTestDialog(Gui::getMainWindow());
    }
    return _instance;
}

/**
 * Destructs the instance of this dialog.
 */
void UnitTestDialog::destruct()
{
    if (_instance) {
        UnitTestDialog* pTmp = _instance;
        _instance = nullptr;
        delete pTmp;
    }
}

/**
 * Returns \a true if an instance of this dialog exists, \a false otherwise.
 */
bool UnitTestDialog::hasInstance()
{
    return _instance != nullptr;
}

/**
 *  Constructs a TestGui::UnitTestDialog as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
UnitTestDialog::UnitTestDialog(QWidget* parent, Qt::WindowFlags f)
    : QDialog(parent, f)
    , ui(new Ui_UnitTest)
{
    ui->setupUi(this);
    setupConnections();

    setProgressColor(QColor(40, 210, 43));  // a darker green
    ui->progressBar->setAlignment(Qt::AlignCenter);

    // red items
    QPalette palette;
    palette.setColor(ui->treeViewFailure->foregroundRole(), Qt::red);
    ui->treeViewFailure->setPalette(palette);
}

/**
 *  Destroys the object and frees any allocated resources
 */
UnitTestDialog::~UnitTestDialog() = default;

void UnitTestDialog::setupConnections()
{
    connect(ui->treeViewFailure,
            &QTreeWidget::itemDoubleClicked,
            this,
            &UnitTestDialog::onTreeViewFailureItemDoubleClicked);
    connect(ui->helpButton, &QPushButton::clicked, this, &UnitTestDialog::onHelpButtonClicked);
    connect(ui->aboutButton, &QPushButton::clicked, this, &UnitTestDialog::onAboutButtonClicked);
    connect(ui->startButton, &QPushButton::clicked, this, &UnitTestDialog::onStartButtonClicked);
}

/**
 * Sets the color to the progressbar to \a col.
 */
void UnitTestDialog::setProgressColor(const QColor& col)
{
    QString qss = QString::fromLatin1("QProgressBar {\n"
                                      "    border: 2px solid grey;\n"
                                      "    border-radius: 5px;\n"
                                      "}\n"
                                      "\n"
                                      "QProgressBar::chunk {\n"
                                      "    background-color: %1;\n"
                                      "}")
                      .arg(col.name());
    ui->progressBar->setStyleSheet(qss);
}

/**
 * Opens a dialog to display a detailed description about the error.
 */
void UnitTestDialog::onTreeViewFailureItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    QString text = item->data(0, Qt::UserRole).toString();

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle(item->text(0));
    msgBox.setDetailedText(text);

    // truncate the visible text when it's too long
    if (text.count(QLatin1Char('\n')) > 20) {
        QStringList lines = text.split(QLatin1Char('\n'));
        lines.erase(lines.begin() + 20, lines.end());
        text = lines.join(QLatin1String("\n"));
    }
    if (text.size() > 1000) {
        text = text.left(1000);
    }

    msgBox.setText(text);
    msgBox.exec();
}

/**
 * Shows the help dialog.
 */
void UnitTestDialog::onHelpButtonClicked()
{
    QMessageBox::information(
        this,
        tr("Help"),
        tr("Enter the name of a callable object which, when called, will return a TestCase.\n"
           "Click 'start', and the test thus produced will be run.\n\n"
           "Double click on an error in the tree view to see more information about it, "
           "including the stack trace."));
}

/**
 * Shows the about dialog.
 */
void UnitTestDialog::onAboutButtonClicked()
{
    QMessageBox::information(
        this,
        tr("About FreeCAD UnitTest"),
        tr("Copyright (c) Werner Mayer\n\n"
           "FreeCAD UnitTest is part of FreeCAD and supports writing Unit Tests for "
           "ones own modules."));
}

/**
 * Runs the unit tests.
 */
void UnitTestDialog::onStartButtonClicked()
{
    reset();
    setProgressColor(QColor(40, 210, 43));  // a darker green
    ui->startButton->setDisabled(true);
    try {
        Base::Interpreter().runString("import qtunittest, gc\n"
                                      "__qt_test__=qtunittest.QtTestRunner(0,\"\")\n"
                                      "__qt_test__.runClicked()\n"
                                      "del __qt_test__\n"
                                      "gc.collect()\n");
    }
    catch (const Base::PyException& e) {
        std::string msg = e.what();
        msg += "\n\n";
        msg += e.getStackTrace();
        showErrorDialog("Exception", msg.c_str());
    }
    catch (const Base::Exception& e) {
        showErrorDialog("Exception", e.what());
    }
    catch (const std::exception& e) {
        showErrorDialog("C++ standard exception", e.what());
    }
    catch (...) {
        showErrorDialog("Unknown exception", "Unknown exception raised");
    }
    ui->startButton->setEnabled(true);
}

/**
 * Shows an error dialog with \a title and \a message.
 */
void UnitTestDialog::showErrorDialog(const char* title, const char* message)
{
    QMessageBox::critical(this, QString::fromLatin1(title), QString::fromLatin1(message));
}

/**
 * Closes and resets this dialog.
 */
void UnitTestDialog::reject()
{
    reset();
    QDialog::reject();
}

/**
 * Resets this dialog.
 */
void UnitTestDialog::reset()
{
    ui->progressBar->reset();
    ui->treeViewFailure->clear();
    ui->textLabelRunCt->setText(QString::fromLatin1("<font color=\"#0000ff\">0</font>"));
    ui->textLabelFailCt->setText(QString::fromLatin1("<font color=\"#0000ff\">0</font>"));
    ui->textLabelErrCt->setText(QString::fromLatin1("<font color=\"#0000ff\">0</font>"));
    ui->textLabelRemCt->setText(QString::fromLatin1("<font color=\"#0000ff\">0</font>"));
}

/**
 * Adds a unit test. If a test with name \a unit already is added then nothing happens.
 */
void UnitTestDialog::addUnitTest(const QString& unit)
{
    int ct = ui->comboTests->count();
    for (int i = 0; i < ct; i++) {
        if (ui->comboTests->itemText(i) == unit) {
            return;
        }
    }

    ui->comboTests->addItem(unit);
}

/**
 * Sets the unit test. If the item is not there yet it gets added.
 */
void UnitTestDialog::setUnitTest(const QString& unit)
{
    addUnitTest(unit);
    for (int i = 0; i < ui->comboTests->count(); i++) {
        if (ui->comboTests->itemText(i) == unit) {
            ui->comboTests->setCurrentIndex(i);
            break;
        }
    }
}

/**
 * Clears the unit tests.
 */
void UnitTestDialog::clearUnitTests()
{
    ui->comboTests->clear();
}

/**
 * Returns the unit test.
 */
QString UnitTestDialog::getUnitTest() const
{
    return ui->comboTests->currentText();
}

/**
 * Runs the currently selected test and closes the dialog afterwards.
 * It returns true if all tests have passed and false otherwise.
 */
bool UnitTestDialog::runCurrentTest()
{
    clearErrorList();
    onStartButtonClicked();
    int count = ui->treeViewFailure->topLevelItemCount();
    reject();
    return (count == 0);
}

/**
 * Sets the text in the status bar.
 */
void UnitTestDialog::setStatusText(const QString& text)
{
    QFontMetrics fm(font());
    int labelWidth = ui->textLabelStatus->width() - 10;
    ui->textLabelStatus->setText(fm.elidedText(text, Qt::ElideMiddle, labelWidth));
}

/**
 * Sets the progress of the progress bar whereas fraction is in between 0.0 and 1.0.
 * It also sets the color of the progress bar to red if a failure or error in the unit
 * test occurred.
 */
void UnitTestDialog::setProgressFraction(float fraction, const QString& color)
{
    if (fraction == 0.0f) {
        ui->progressBar->setRange(0, 100);
    }
    else {
        if (color == QLatin1String("red")) {
            setProgressColor(Qt::red);
        }

        ui->progressBar->setValue((int)(100 * fraction));
    }
}

/**
 * Empties the error listview.
 */
void UnitTestDialog::clearErrorList()
{
    ui->treeViewFailure->clear();
}

/**
 * Inserts an item with text \a failure to the listview. \a details will be shown
 * when double-clicking on the matching listitem.
 */
void UnitTestDialog::insertError(const QString& failure, const QString& details)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeViewFailure);
    item->setText(0, failure);
    item->setForeground(0, Qt::red);
    item->setData(0, Qt::UserRole, QVariant(details));
}

/**
 * Sets the number of performed tests to \a ct.
 */
void UnitTestDialog::setRunCount(int ct)
{
    ui->textLabelRunCt->setText(QString::fromLatin1("<font color=\"#0000ff\">%1</font>").arg(ct));
}

/**
 * Sets the number of failed tests to \a ct.
 */
void UnitTestDialog::setFailCount(int ct)
{
    ui->textLabelFailCt->setText(QString::fromLatin1("<font color=\"#0000ff\">%1</font>").arg(ct));
}

/**
 * Sets the number of faulty tests to \a ct.
 */
void UnitTestDialog::setErrorCount(int ct)
{
    ui->textLabelErrCt->setText(QString::fromLatin1("<font color=\"#0000ff\">%1</font>").arg(ct));
}

/**
 * Sets the number of remaining tests to \a ct.
 */
void UnitTestDialog::setRemainCount(int ct)
{
    ui->textLabelRemCt->setText(QString::fromLatin1("<font color=\"#0000ff\">%1</font>").arg(ct));
}

#include "moc_UnitTestImp.cpp"
