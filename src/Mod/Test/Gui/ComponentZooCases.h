// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <vector>

class QWidget;

namespace App
{
class DocumentObject;
}

namespace Zoo
{
struct WidgetCase
{
    const char* variant;
    std::function<QWidget*()> make;
};

std::vector<WidgetCase> buttons();
std::vector<WidgetCase> text(App::DocumentObject* object);
std::vector<WidgetCase> numbers(App::DocumentObject* object);
std::vector<WidgetCase> spinboxes(App::DocumentObject* object);
std::vector<WidgetCase> labels();
std::vector<WidgetCase> views(App::DocumentObject* object);
std::vector<WidgetCase> containers();
}  // namespace Zoo
