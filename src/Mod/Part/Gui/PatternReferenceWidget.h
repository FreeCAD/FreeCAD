// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QWidget>

#include <App/PropertyLinks.h>
#include <Gui/ComboLinks.h>
#include <Mod/Part/PartGlobal.h>

class QComboBox;

namespace PartGui
{

class PartGuiExport PatternReferenceWidget: public QWidget
{
    Q_OBJECT

public:
    static constexpr int DefaultDirectionUserData = -2;
    static constexpr int SelectReferenceUserData = -3;
    static constexpr int ObjectDirectionUserData = -4;

    explicit PatternReferenceWidget(QWidget* parent = nullptr);

    void setupReferenceCombo(QComboBox* combo);
    void bindReference(App::PropertyLinkSub* reference);
    void addDirection(App::DocumentObject* object,
                      const std::string& subname,
                      const QString& text,
                      int userData = -1);
    void updateReferenceUI();

    const App::PropertyLinkSub& getCurrentDirectionLink() const;
    bool isSelectReferenceMode() const;
    void getAxis(App::DocumentObject*& object, std::vector<std::string>& subnames) const;

    Gui::ComboLinks dirLinks;

Q_SIGNALS:
    void requestReferenceSelection();
    void parametersChanged();

private:
    void onDirectionChanged(int index);

    App::PropertyLinkSub* referenceProperty = nullptr;
    bool blockReferenceUpdate = false;
};

}  // namespace PartGui
