/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer@users.sourceforge.net>        *
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


#include <QtGui>

#include "wizard.h"

Wizard::Wizard(QWidget* parent)
    : QDialog(parent)
{
    textLabel = new QLabel();

    topLine = new QFrame();
    topLine->setFrameShape(QFrame::HLine);
    topLine->setFrameShadow(QFrame::Sunken);
    bottomLine = new QFrame();
    bottomLine->setFrameShape(QFrame::HLine);
    bottomLine->setFrameShadow(QFrame::Sunken);

    _cancelButton = new QPushButton(tr("Cancel"));
    _backButton = new QPushButton(tr("< &Back"));
    _backButton->setDisabled(true);
    _nextButton = new QPushButton(tr("Next >"));
    _nextButton->setDisabled(true);
    _finishButton = new QPushButton(tr("&Finish"));
    _finishButton->setDisabled(true);

    connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(_backButton, SIGNAL(clicked()), this, SLOT(backButtonClicked()));
    connect(_nextButton, SIGNAL(clicked()), this, SLOT(nextButtonClicked()));
    connect(_finishButton, SIGNAL(clicked()), this, SLOT(accept()));

    buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(_cancelButton);
    buttonLayout->addWidget(_backButton);
    buttonLayout->addWidget(_nextButton);
    buttonLayout->addWidget(_finishButton);

    stackWidget = new QStackedWidget();

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget(textLabel);
    mainLayout->addWidget(topLine);
    mainLayout->addWidget(stackWidget);
    mainLayout->addWidget(bottomLine);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

QSize Wizard::sizeHint() const
{
    return QSize(200, 150);
}

void Wizard::addPage(QWidget* page)
{
    insertPage(count(), page);
}

void Wizard::removePage(int index)
{
    QWidget* widget = stackWidget->widget(index);
    stackWidget->removeWidget(widget);

    index = currentIndex();
    _backButton->setEnabled(index > 0);
    _nextButton->setEnabled(index < count() - 1);
}

int Wizard::count() const
{
    return stackWidget->count();
}

int Wizard::currentIndex() const
{
    return stackWidget->currentIndex();
}

void Wizard::insertPage(int index, QWidget* page)
{
    page->setParent(stackWidget);

    stackWidget->insertWidget(index, page);

    QString title = page->windowTitle();
    if (title.isEmpty()) {
        title = tr("Page %1").arg(stackWidget->count());
        page->setWindowTitle(title);
    }

    if (currentIndex() == index) {
        textLabel->setText(title);
    }

    int current = currentIndex();
    _backButton->setEnabled(current > 0);
    _nextButton->setEnabled(current < count() - 1);
}

void Wizard::backButtonClicked()
{
    int index = currentIndex();
    if (index > 0) {
        setCurrentIndex(index - 1);
    }
}

void Wizard::nextButtonClicked()
{
    int index = currentIndex();
    if (index < count() - 1) {
        setCurrentIndex(index + 1);
    }
}

QPushButton* Wizard::backButton() const
{
    return _backButton;
}

QPushButton* Wizard::nextButton() const
{
    return _nextButton;
}

void Wizard::setCurrentIndex(int index)
{
    if (index != currentIndex()) {
        stackWidget->setCurrentIndex(index);
        textLabel->setText(stackWidget->currentWidget()->windowTitle());
        _backButton->setEnabled(index > 0);
        _nextButton->setEnabled(index < count() - 1);
        Q_EMIT currentIndexChanged(index);
    }
}

QWidget* Wizard::widget(int index)
{
    return stackWidget->widget(index);
}

QString Wizard::pageTitle() const
{
    return stackWidget->currentWidget()->windowTitle();
}

void Wizard::setPageTitle(QString const& newTitle)
{
    stackWidget->currentWidget()->setWindowTitle(newTitle);
    textLabel->setText(newTitle);
    Q_EMIT pageTitleChanged(newTitle);
}

WizardExtension::WizardExtension(Wizard* widget, QObject* parent)
    : QObject(parent)
{
    myWidget = widget;
}

void WizardExtension::addWidget(QWidget* widget)
{
    myWidget->addPage(widget);
}

int WizardExtension::count() const
{
    return myWidget->count();
}

int WizardExtension::currentIndex() const
{
    return myWidget->currentIndex();
}

void WizardExtension::insertWidget(int index, QWidget* widget)
{
    myWidget->insertPage(index, widget);
}

void WizardExtension::remove(int index)
{
    myWidget->removePage(index);
}

void WizardExtension::setCurrentIndex(int index)
{
    myWidget->setCurrentIndex(index);
}

QWidget* WizardExtension::widget(int index) const
{
    return myWidget->widget(index);
}

WizardExtensionFactory::WizardExtensionFactory(QExtensionManager* parent)
    : QExtensionFactory(parent)
{}

QObject*
WizardExtensionFactory::createExtension(QObject* object, const QString& iid, QObject* parent) const
{
    Wizard* widget = qobject_cast<Wizard*>(object);

    if (widget && (iid == Q_TYPEID(QDesignerContainerExtension))) {
        return new WizardExtension(widget, parent);
    }
    else {
        return 0;
    }
}
