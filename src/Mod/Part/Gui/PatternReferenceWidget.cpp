// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PatternReferenceWidget.h"

#include <QComboBox>

#include <App/DocumentObject.h>
#include <Base/Tools.h>

using namespace PartGui;

PatternReferenceWidget::PatternReferenceWidget(QWidget* parent)
    : QWidget(parent)
{}

void PatternReferenceWidget::setupReferenceCombo(QComboBox* combo)
{
    dirLinks.setCombo(combo);
    connect(combo,
            qOverload<int>(&QComboBox::activated),
            this,
            &PatternReferenceWidget::onDirectionChanged);
}

void PatternReferenceWidget::bindReference(App::PropertyLinkSub* reference)
{
    referenceProperty = reference;
    updateReferenceUI();
}

void PatternReferenceWidget::addDirection(App::DocumentObject* object,
                                          const std::string& subname,
                                          const QString& text,
                                          int userData)
{
    dirLinks.addLink(object, subname, text, userData);
}

void PatternReferenceWidget::updateReferenceUI()
{
    if (blockReferenceUpdate || !referenceProperty) {
        return;
    }

    Base::StateLocker locker(blockReferenceUpdate, true);
    if (dirLinks.setCurrentLink(*referenceProperty) != -1 || !referenceProperty->getValue()) {
        return;
    }

    const auto& subnames = referenceProperty->getSubValues();
    const QString reference = QStringLiteral("%1:%2")
                                  .arg(QString::fromLatin1(
                                           referenceProperty->getValue()->getNameInDocument()),
                                       subnames.empty()
                                           ? QString()
                                           : QString::fromStdString(subnames.front()));
    dirLinks.addLink(*referenceProperty, reference);
    dirLinks.setCurrentLink(*referenceProperty);
}

const App::PropertyLinkSub& PatternReferenceWidget::getCurrentDirectionLink() const
{
    return dirLinks.getCurrentLink();
}

bool PatternReferenceWidget::isSelectReferenceMode() const
{
    const int userData = dirLinks.getCurrentUserData();
    if (userData == SelectReferenceUserData) {
        return true;
    }
    if (userData == DefaultDirectionUserData || userData == ObjectDirectionUserData) {
        return false;
    }

    return !dirLinks.getCurrentLink().getValue();
}

void PatternReferenceWidget::getAxis(App::DocumentObject*& object,
                                     std::vector<std::string>& subnames) const
{
    const App::PropertyLinkSub& link = dirLinks.getCurrentLink();
    object = link.getValue();
    subnames = link.getSubValues();
}

void PatternReferenceWidget::onDirectionChanged(int)
{
    if (blockReferenceUpdate || !referenceProperty) {
        return;
    }

    if (isSelectReferenceMode()) {
        Q_EMIT requestReferenceSelection();
        return;
    }

    referenceProperty->Paste(dirLinks.getCurrentLink());
    Q_EMIT parametersChanged();
}
