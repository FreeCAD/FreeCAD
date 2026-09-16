// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <Gui/BitmapFactory.h>
#include <Gui/InputHint.h>

#include "DrawSketchDefaultHandler.h"

namespace SketcherGui
{
class DrawSketchHandlerCreateBlock;
using DrawSketchHandlerCreateBlockBase
    = DrawSketchDefaultHandler<DrawSketchHandlerCreateBlock, StateMachines::OneSeekEnd, 1>;

class DrawSketchHandlerCreateBlock: public DrawSketchHandlerCreateBlockBase
{
public:
    using SaveBlock = std::function<void(const Base::Vector3d&, bool)>;
    explicit DrawSketchHandlerCreateBlock(SaveBlock save)
        : saveBlock(std::move(save))
    {}

private:
    SaveBlock saveBlock;
    Base::Vector2d origin;

    void activated() override
    {
        DrawSketchHandlerCreateBlockBase::activated();
        continuousMode = false;
    }

    void updateDataAndDrawToPosition(Base::Vector2d position) override
    {
        if (state() == SelectMode::SeekFirst) {
            seekAndRenderAutoConstraint(sugConstraints[0], position, Base::Vector2d());
            Base::Vector2d snapPoint;
            origin = getLineExtensionAutoConstraintSnapPoint(snapPoint) ? snapPoint : position;
            drawPositionAtCursor(origin);
        }
    }

    void executeCommands() override
    {
        auto* fixedSize = toolwidget->findChild<QCheckBox*>(QStringLiteral("createBlockFixedSize"));
        saveBlock(Base::Vector3d(origin.x, origin.y, 0), fixedSize->isChecked());
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        return {
            {QObject::tr("%1 choose the origin of the selected block geometry"),
             {Gui::InputHint::UserInput::MouseLeft}}
        };
    }

    std::string getToolName() const override
    {
        return "DSH_CreateBlock";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Block.svg");
    }

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_InsertBlock");
    }

    QString getToolWidgetText() const override
    {
        return QObject::tr("Create Block");
    }

    bool isWidgetVisible() const override
    {
        return true;
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        auto widget = std::make_unique<QWidget>();
        widget->setObjectName(QStringLiteral("CreateBlockWidget"));
        auto* layout = new QVBoxLayout(widget.get());
        auto* hint = new QLabel(QObject::tr("Click in the sketch to choose the block origin."));
        hint->setWordWrap(true);
        layout->addWidget(hint);
        auto* fixedSize = new QCheckBox(QObject::tr("Fixed Size"));
        fixedSize->setObjectName(QStringLiteral("createBlockFixedSize"));
        fixedSize->setToolTip(QObject::tr("Use the source size by default when inserting this block"));
        layout->addWidget(fixedSize);
        return widget;
    }
};
}  // namespace SketcherGui
