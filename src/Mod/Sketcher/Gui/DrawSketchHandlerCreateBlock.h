// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <functional>
#include <Precision.hxx>
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
    = DrawSketchDefaultHandler<DrawSketchHandlerCreateBlock, StateMachines::TwoSeekEnd, 2>;

class DrawSketchHandlerCreateBlock: public DrawSketchHandlerCreateBlockBase
{
public:
    using SaveBlock = std::function<void(const Base::Vector3d&, const Base::Vector3d&, bool)>;
    explicit DrawSketchHandlerCreateBlock(SaveBlock save)
        : saveBlock(std::move(save))
    {}

private:
    SaveBlock saveBlock;
    Base::Vector2d origin, endpoint;

    QCheckBox* fixedSizeBox() const
    {
        return toolwidget->findChild<QCheckBox*>(QStringLiteral("createBlockFixedSize"));
    }

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
        else if (state() == SelectMode::SeekSecond) {
            seekAndRenderAutoConstraint(sugConstraints[1], position, position - origin);
            Base::Vector2d snapPoint;
            endpoint = getLineExtensionAutoConstraintSnapPoint(snapPoint) ? snapPoint : position;
            drawDirectionAtCursor(endpoint, origin);
            drawEdit(std::vector<Base::Vector2d> {origin, endpoint});
        }
    }

    void onButtonPressed(Base::Vector2d position) override
    {
        updateDataAndDrawToPosition(position);
        if (state() == SelectMode::SeekFirst && fixedSizeBox()->isChecked()) {
            endpoint = origin;
            setState(SelectMode::End);
        }
        else if (state() == SelectMode::SeekFirst) {
            fixedSizeBox()->setEnabled(false);
            moveToNextMode();
        }
        else if ((endpoint - origin).Length() > Precision::Confusion()) {
            moveToNextMode();
        }
    }

    void angleSnappingControl() override
    {
        setAngleSnapping(state() == SelectMode::SeekSecond, origin);
    }

    void executeCommands() override
    {
        saveBlock(toVector3d(origin), toVector3d(endpoint), fixedSizeBox()->isChecked());
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        if (state() == SelectMode::SeekSecond) {
            return {
                {QObject::tr("%1 choose the end of the block's line handle"),
                 {Gui::InputHint::UserInput::MouseLeft}}
            };
        }
        return {
            {QObject::tr("%1 choose the block origin and start of its handle"),
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
        return Gui::BitmapFactory().pixmap("Sketcher_BlockCreate");
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
        auto* hint = new QLabel(
            QObject::tr(
                "Choose the block origin, then the end of its line handle. "
                "Fixed-size blocks need only the origin."
            )
        );
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
