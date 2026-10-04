// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <fastsignals/signal.h>
#include <Gui/TaskView/TaskView.h>

namespace Gui
{
class LayerWidget;
}

namespace SketcherGui
{
class ViewProviderSketch;

/// The sketch's layers, shown with the generic layer widget.
class TaskSketcherLayers: public Gui::TaskView::TaskBox
{
    Q_OBJECT
public:
    explicit TaskSketcherLayers(ViewProviderSketch* view);
    ~TaskSketcherLayers() override;

private:
    void updateVisibility();
    ViewProviderSketch* view;
    Gui::LayerWidget* widget;
    fastsignals::scoped_connection connection;
};
}  // namespace SketcherGui
