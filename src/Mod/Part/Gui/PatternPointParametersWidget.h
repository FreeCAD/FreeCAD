// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>

#include <App/PropertyLinks.h>
#include <Mod/Part/PartGlobal.h>

#include "PatternReferenceWidget.h"

namespace PartGui
{

class Ui_PatternPointParametersWidget;

class PartGuiExport PatternPointParametersWidget: public PatternReferenceWidget
{
    Q_OBJECT

public:
    explicit PatternPointParametersWidget(QWidget* parent = nullptr);
    ~PatternPointParametersWidget() override;

    void bindProperty(App::PropertyLinkSub* pointObject);
    void updateUI();
    void getPointObject(App::DocumentObject*& object, std::vector<std::string>& subnames) const;

private:
    void updatePointObjectButton();

    std::unique_ptr<Ui_PatternPointParametersWidget> ui;
    App::PropertyLinkSub* pointObjectProperty = nullptr;
};

}  // namespace PartGui
