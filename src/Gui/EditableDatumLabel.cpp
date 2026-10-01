/// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2023 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <limits>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPath.h>

#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QString>
#include <QTimer>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/InlineExpression.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "EditableDatumLabel.h"
#include "Base/Console.h"

using namespace Gui;


struct NodeData
{
    EditableDatumLabel* label;
};

EditableDatumLabel::EditableDatumLabel(
    View3DInventorViewer* view,
    const Base::Placement& plc,
    bool autoDistance,
    bool avoidMouseCursor
)
    : isSet(false)
    , hasFinishedEditing(false)
    , autoDistance(autoDistance)
    , autoDistanceReverse(false)
    , avoidMouseCursor(avoidMouseCursor)
    , value(0.0)
    , viewer(view)
    , spinBox(nullptr)
    , lockIconLabel(nullptr)
    , cameraSensor(nullptr)
    , pickStyle(nullptr)
    , function(Function::Positioning)
    , editStartValue(0.0)
    , mAlive(std::make_shared<bool>(true))
{
    // NOLINTBEGIN
    initColors();

    root = new SoSwitch;
    root->ref();

    annotation = new SoAnnotation;
    annotation->ref();
    annotation->renderCaching = SoSeparator::OFF;
    root->addChild(annotation);

    transform = new SoTransform();
    transform->ref();
    annotation->addChild(transform);

    eventCallback = new SoEventCallback;
    eventCallback->ref();
    eventCallback->addEventCallback(SoMouseButtonEvent::getClassTypeId(), eventCallbackF, this);
    annotation->addChild(eventCallback);
    pickStyle = new SoPickStyle;
    pickStyle->ref();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    annotation->addChild(pickStyle);

    label = new SoDatumLabel();
    label->ref();
    label->string = " ";
    setDeactivatedColor();
    label->size.setValue(17);
    label->lineWidth = 2.0;
    label->useAntialiasing = false;
    label->datumtype = SoDatumLabel::DISTANCE;
    label->param1 = 0.;
    label->param2 = 0.;
    label->param3 = 0.;
    if (autoDistance) {
        setLabelRecommendedDistance();
    }
    annotation->addChild(label);

    setPlacement(plc);
    // NOLINTEND

    static_cast<SoSeparator*>(viewer->getSceneGraph())->addChild(root);  // NOLINT

    if (view) {
        connect(view, &View3DInventorViewer::cameraChanged, this, [this]() {
            if (this->label) {
                // 1. Re-attach the sensor to the NEW camera object
                if (this->cameraSensor && this->viewer && this->viewer->getCamera()) {
                    this->cameraSensor->detach();
                    this->cameraSensor->attach(this->viewer->getCamera());
                }

                if (this->autoDistance) {
                    this->setLabelRecommendedDistance();
                }
                this->label->touch();

                if (this->isInEdit()) {
                    this->positionSpinbox();
                }
            }
        });
    }
}

EditableDatumLabel::~EditableDatumLabel()
{
    // Mark the label as dead before tearing anything down, so that code re-entering
    // from a signal emitted during destruction can detect it via the liveness token.
    *mAlive = false;

    deactivate();
    transform->unref();
    annotation->unref();
    eventCallback->unref();
    pickStyle->unref();
    root->unref();
    label->unref();
}

void EditableDatumLabel::activate()
{
    if (!viewer || isActive()) {
        return;
    }

    root->whichChild = 0;

    // track camera movements to update spinbox position.
    auto info = new NodeData {this};
    cameraSensor = new SoNodeSensor(
        [](void* data, SoSensor* sensor) {
            Q_UNUSED(sensor);
            auto info = static_cast<NodeData*>(data);
            info->label->positionSpinbox();
            if (info->label->autoDistance) {
                info->label->setLabelRecommendedDistance();
            }
        },
        info
    );
    cameraSensor->attach(viewer->getCamera());
}

void EditableDatumLabel::deactivate()
{
    stopEdit();

    if (cameraSensor) {
        auto data = static_cast<NodeData*>(cameraSensor->getData());
        delete data;
        cameraSensor->detach();
        delete cameraSensor;
        cameraSensor = nullptr;
    }

    root->whichChild = SO_SWITCH_NONE;
}

void EditableDatumLabel::startEdit(double val, QObject* eventFilteringObj, bool visibleToMouse)
{
    if (isInEdit()) {
        return;
    }

    // Reset locked state when starting to edit
    this->resetLockedState();
    expression.clear();
    hasUserEditedText = false;
    isSet = false;
    value = val;
    editStartValue = val;

    QWidget* mdi = viewer->parentWidget();

    label->string = " ";

    spinBox = new QuantitySpinBox(mdi);
    spinBox->setUnit(Base::Unit::Length);
    spinBox->setMinimum(-std::numeric_limits<int>::max());
    spinBox->setMaximum(std::numeric_limits<int>::max());
    spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    spinBox->setFocusPolicy(Qt::ClickFocus);  // prevent passing focus with tab.
    spinBox->setAutoNormalize(false);
    spinBox->setKeyboardTracking(true);
    spinBox->setAutoAdjustWidth(true);
    spinBox->setMaxExpectedDigits(16);
    spinBox->installEventFilter(this);

    auto* lineEdit = spinBox->findChild<QLineEdit*>();
    if (lineEdit) {
        lineEdit->installEventFilter(this);
        connect(lineEdit, &QLineEdit::textEdited, this, [this](const QString&) {
            hasUserEditedText = true;
        });
        connect(lineEdit, &QLineEdit::textChanged, this, [this, lineEdit]() {
            this->updateGeometry(lineEdit);
            this->positionSpinbox();
        });
    }

    spinBox->setValue(Base::Quantity(val, Base::Unit::Length));

    lockIconLabel = new QLabel(spinBox);
    lockIconLabel->setObjectName(QStringLiteral("onViewParameterLockIcon"));
    lockIconLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // load icon and scale it to fit in spinbox
    QPixmap lockIcon = Gui::BitmapFactory().pixmap("Constraint_Lock");
    const QFontMetrics fm(spinBox->fontMetrics());
    int iconSize = fm.height();
    QPixmap scaledIcon
        = lockIcon.scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    lockIconLabel->setPixmap(scaledIcon);
    lockIconLabel->setVisible(false);

    if (eventFilteringObj) {
        spinBox->installEventFilter(eventFilteringObj);
        if (lineEdit) {
            lineEdit->installEventFilter(eventFilteringObj);
        }
    }

    if (!visibleToMouse) {
        setSpinboxVisibleToMouse(visibleToMouse);
    }

    spinBox->show();
    updateGeometry();
    positionSpinbox();
    setFocusToSpinbox();
    QTimer::singleShot(0, this, [this]() {
        if (!spinBox) {
            return;
        }
        positionSpinbox();
        setFocusToSpinbox();
    });

    connect(
        spinBox,
        qOverload<double>(&QuantitySpinBox::valueChanged),
        this,
        &EditableDatumLabel::handleSpinBoxValueChanged
    );
    connect(spinBox, &QuantitySpinBox::inputCleared, this, &EditableDatumLabel::handleSpinBoxInputCleared);
}

bool EditableDatumLabel::syncValueFromSpinBox(bool emitParameterUnset)
{
    if (!spinBox) {
        return false;
    }

    expression = spinBox->takeUnboundExpressionText();

    if (!spinBox->hasValidInput()) {
        expression.clear();
        if (emitParameterUnset) {
            resetLockedState();
            Q_EMIT parameterUnset();
        }
        return false;
    }

    value = spinBox->rawValue();
    isSet = true;

    if (hasFinishedEditing) {
        setLockedAppearance(true);
    }

    return true;
}

void EditableDatumLabel::handleSpinBoxValueChanged()
{
    if (syncValueFromSpinBox()) {
        Q_EMIT valueChanged(value);
    }
}

void EditableDatumLabel::handleSpinBoxInputCleared()
{
    if (!isSet && !hasFinishedEditing) {
        return;
    }

    // Clearing an OVP removes its provisional parameter; it does not create a new numeric value.
    // Keep the committed value intact so Escape and cancellation can still restore it.
    isSet = false;
    resetLockedState();
    Q_EMIT parameterUnset();
}

bool EditableDatumLabel::eventFilter(QObject* watched, QEvent* event)
{
    // handle key events relevant to expression input in OVA
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        const bool isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        const bool isBacktab = keyEvent->key() == Qt::Key_Backtab
            || (keyEvent->key() == Qt::Key_Tab && (keyEvent->modifiers() & Qt::ShiftModifier));
        const bool isTab = keyEvent->key() == Qt::Key_Tab && !isBacktab;
        const bool isEditorWidget = watched == spinBox || qobject_cast<QLineEdit*>(watched);

        if (keyEvent->key() == Qt::Key_Escape) {
            if (isEditorWidget) {
                this->value = this->editStartValue;
                this->isSet = false;
                this->hasFinishedEditing = false;
                this->expression.clear();
                this->hasUserEditedText = false;
                this->setLockedAppearance(false);
                this->setSpinboxValue(this->editStartValue);
                this->stopEdit();
                Q_EMIT this->editingCanceled(this->value);
                return true;
            }
        }

        if (isEnter || isTab || isBacktab) {
            if (isEditorWidget) {
                if (!this->spinBox) {
                    return QObject::eventFilter(watched, event);
                }

                // Control + Enter finalizes all visible OVPs in the current stage.
                if (isEnter && (keyEvent->modifiers() & Qt::ControlModifier)) {
                    Q_EMIT this->finishEditingOnAllOVPs();
                    return true;
                }

                // Backtab is reserved so it does not fight the controller's focus cycling.
                if (isBacktab) {
                    return true;
                }

                // Tab on untouched field should only cycle focus.
                if (isTab && !this->isSet && !hasUserEditedText) {
                    if (!this->spinBox->hasValidInput()) {
                        this->hasFinishedEditing = false;
                        return true;
                    }
                    return false;
                }

                // Enter or tab with edited input accepts the current value.
                this->hasFinishedEditing = true;
                // Committing the value re-enters the tool (via the valueChanged
                // signal) and may destroy this label; prevent invalid access.
                const auto alive = this->mAlive;
                const bool committed = this->commitPendingInlineExpression();
                if (!*alive) {
                    return true;
                }
                if (committed) {
                    Q_EMIT this->editingFinished(value);
                    return true;
                }

                auto* lineEdit = this->spinBox->findChild<QLineEdit*>();
                const QString normalizedInput = lineEdit
                    ? InlineExpression::normalizeInput(lineEdit->text())
                    : QString();
                if (normalizedInput.isEmpty()) {
                    syncValueFromSpinBox();
                    return true;
                }

                this->hasFinishedEditing = false;
                this->setLockedAppearance(false);
                this->setFocusToSpinbox();
                return true;
            }
        }
        // Any other key on a locked field unlocks visual state for editing.
        else if (this->hasFinishedEditing && keyEvent->key() != Qt::Key_Tab) {
            this->resetLockedState();
            return false;
        }
    }
    else if (event->type() == QEvent::FocusIn) {
        // Emit focusGained whenever the spinbox or its child QLineEdit receives focus
        Q_EMIT focusGained();
    }
    else if (event->type() == QEvent::FocusOut) {
        if (watched == spinBox) {
            Q_EMIT focusLost();
        }
    }

    return QObject::eventFilter(watched, event);
}

void EditableDatumLabel::stopEdit(bool writeChanges)
{
    if (spinBox) {
        if (writeChanges) {
            // write the spinbox value in the label.
            Base::Quantity quantity = spinBox->value();
            std::string valueStr = quantity.getUserString();
            label->string = SbString(valueStr.c_str());
        }
        else {
            Base::Quantity quantity(editStartValue, spinBox->unit());
            label->string = quantity.getUserString().c_str();
        }

        spinBox->deleteLater();
        spinBox = nullptr;

        // Lock icon will be automatically destroyed as it's a child of spinbox
        lockIconLabel = nullptr;
    }
}

bool EditableDatumLabel::isActive() const
{
    return cameraSensor != nullptr;
}

bool EditableDatumLabel::isInEdit() const
{
    return spinBox != nullptr;
}


double EditableDatumLabel::getValue() const
{
    // We use value rather than spinBox->rawValue() in case edit stopped.
    return value;
}

std::string EditableDatumLabel::constraintExpression() const
{
    return expression;
}

bool EditableDatumLabel::commitPendingInlineExpression()
{
    if (!spinBox) {
        return false;
    }
    auto* lineEdit = spinBox->findChild<QLineEdit*>();
    const QString input = lineEdit ? lineEdit->text() : QString();
    const QString normalized = InlineExpression::normalizeInput(input);

    // Untouched OVA fields should not become explicit constraints on click-out.
    // Keep this as a no-op commit so tool flow can continue.
    if (!hasUserEditedText && !InlineExpression::looksLikeExpressionInput(normalized)) {
        return true;
    }

    if (spinBox->commitInlineExpressionTextForUi()) {
        return true;
    }
    if (!spinBox->hasValidInput()) {
        return false;
    }

    if (InlineExpression::looksLikeExpressionInput(normalized)) {
        return false;
    }
    const Base::Quantity quant = spinBox->valueFromText(input);
    {
        QSignalBlocker blocker(spinBox);
        spinBox->setValue(quant);
    }
    Q_EMIT spinBox->valueChanged(spinBox->rawValue());
    return true;
}

void EditableDatumLabel::setSpinboxValue(double val, const Base::Unit& unit)
{
    value = val;

    if (!spinBox) {
        Base::Quantity quantity(val, unit);
        double factor {};
        std::string unitStr;
        std::string valueStr = quantity.getUserString(factor, unitStr);
        label->string = SbString(valueStr.c_str());
        return;
    }

    QSignalBlocker block(spinBox);
    spinBox->setValue(Base::Quantity(val, unit));
    positionSpinbox();

    if (spinBox->hasFocus()) {
        spinBox->selectNumber();
    }
}

void EditableDatumLabel::setFocusToSpinbox()
{
    if (!spinBox) {
        return;
    }
    QPointer<QuantitySpinBox> focused = spinBox;
    QWidget* focusWidget = QApplication::focusWidget();
    const bool focusWithinSpinbox = focused->hasFocus()
        || (focusWidget && (focusWidget == focused || focused->isAncestorOf(focusWidget)));
    if (!focusWithinSpinbox) {
        focused->setFocus();
        if (focused) {
            focused->selectNumber();
        }
    }
}

void EditableDatumLabel::clearSelection()
{
    if (!spinBox) {
        return;
    }

    if (auto* edit = spinBox->findChild<QLineEdit*>()) {
        edit->deselect();
    }
}

void EditableDatumLabel::positionSpinbox()
{
    if (!spinBox) {
        return;
    }

    if (spinBox->hasFocus()) {
        spinBox->raise();
    }

    QSize wSize = spinBox->size();
    QWidget* parent = spinBox->parentWidget();
    QSize vSize = parent ? parent->size() : viewer->size();
    QPoint pxCoord = viewer->toQPoint(viewer->getPointOnViewport(getTextCenterPoint()));
    if (parent && parent != viewer) {
        pxCoord = viewer->mapTo(parent, pxCoord);
    }

    int posX = std::min(std::max(pxCoord.x() - wSize.width() / 2, 0), vSize.width() - wSize.width());
    int posY = std::min(std::max(pxCoord.y() - wSize.height() / 2, 0), vSize.height() - wSize.height());

    if (avoidMouseCursor) {
        QPoint cursorPos = viewer->mapFromGlobal(QCursor::pos());
        int margin = static_cast<int>(wSize.height() * 0.7);  // NOLINT
        if ((cursorPos.x() > posX - margin && cursorPos.x() < posX + wSize.width() + margin)
            && (cursorPos.y() > posY - margin && cursorPos.y() < posY + wSize.height() + margin)) {
            posY = cursorPos.y()
                + ((cursorPos.y() > pxCoord.y()) ? -wSize.height() - margin : margin);
        }
    }

    pxCoord.setX(posX);
    pxCoord.setY(posY);
    spinBox->move(pxCoord);
}

SbVec3f EditableDatumLabel::getTextCenterPoint() const
{
    // Here we need the 3d point and not the 2d point as are the SoLabel points.
    //  First we get the 2D point (on the sketch/image plane) of the middle of the text label.
    SbVec3f point2D = label->getLabelTextCenter();
    // Get the translation and rotation values from the transform
    SbVec3f translation = transform->translation.getValue();
    SbRotation rotation = transform->rotation.getValue();

    // Calculate the inverse transformation
    SbVec3f invTranslation = -translation;
    SbRotation invRotation = rotation.inverse();

    // Transform the 2D coordinates to 3D
    // Plane form
    SbVec3f RX(1, 0, 0);
    SbVec3f RY(0, 1, 0);

    // move to position of Sketch
    invRotation.multVec(RX, RX);
    invRotation.multVec(RY, RY);
    invRotation.multVec(invTranslation, invTranslation);

    // we use invTranslation as the Base because in setPlacement we set transform->translation using
    // placement.getPosition() to fix the Zoffset. But this applies the X & Y translation too.
    Base::Vector3d pos(invTranslation[0], invTranslation[1], invTranslation[2]);
    Base::Vector3d RXb(RX[0], RX[1], RX[2]);
    Base::Vector3d RYb(RY[0], RY[1], RY[2]);
    Base::Vector3d P2D(point2D[0], point2D[1], point2D[2]);
    P2D.TransformToCoordinateSystem(pos, RXb, RYb);

    return {float(P2D.x), float(P2D.y), float(P2D.z)};
}

void EditableDatumLabel::setPlacement(const Base::Placement& plc)
{
    double x {}, y {}, z {}, w {};  // NOLINT
    plc.getRotation().getValue(x, y, z, w);
    transform->rotation.setValue(x, y, z, w);  // NOLINT

    Base::Vector3d pos = plc.getPosition();
    transform->translation.setValue(float(pos.x), float(pos.y), float(pos.z));

    Base::Vector3d RN(0, 0, 1);
    RN = plc.getRotation().multVec(RN);
    label->norm.setValue(SbVec3f(float(RN.x), float(RN.y), float(RN.z)));
}

void EditableDatumLabel::updateGeometry()
{
    if (!spinBox) {
        return;
    }
    updateGeometry(spinBox->findChild<QLineEdit*>());
}

void EditableDatumLabel::updateGeometry(QLineEdit* edit)
{
    if (!spinBox || !edit) {
        return;
    }
    // Workaround: adjustSize() causes the cursor to jump to the end and selections to clear
    // Save the state beforehand and restore it after the geometry update
    int pos = edit->cursorPosition();
    int selStart = edit->selectionStart();
    int selEnd = edit->selectionEnd();
    spinBox->adjustSize();
    edit->setCursorPosition(pos);
    if (selStart != -1 && selEnd != -1) {
        edit->setSelection(selStart, selEnd - selStart);
    }
}

// NOLINTNEXTLINE
void EditableDatumLabel::setColor(SbColor color)
{
    label->textColor = color;
}

void EditableDatumLabel::setActivatedColor()
{
    label->textColor = dimConstrColor;
}

void EditableDatumLabel::setDeactivatedColor()
{
    label->textColor = dimConstrDeactivatedColor;
}

void EditableDatumLabel::initColors()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    dimConstrColor = SbColor(1.0f, 0.149f, 0.0f);           // NOLINT
    dimConstrDeactivatedColor = SbColor(0.5f, 0.5f, 0.5f);  // NOLINT

    float transparency = 0.f;
    unsigned long color = (unsigned long)(dimConstrColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstrainedDimColor", color);
    dimConstrColor.setPackedValue((uint32_t)color, transparency);

    color = (unsigned long)(dimConstrDeactivatedColor.getPackedValue());
    color = hGrp->GetUnsigned("DeactivatedConstrDimColor", color);
    dimConstrDeactivatedColor.setPackedValue((uint32_t)color, transparency);
}

void EditableDatumLabel::eventCallbackF(void* userData, SoEventCallback* cb)
{
    auto* self = static_cast<EditableDatumLabel*>(userData);
    self->handleEvent(cb);
}

void EditableDatumLabel::handleEvent(SoEventCallback* cb)
{
    const auto* event = cb->getEvent();
    if (!event->isOfType(SoMouseButtonEvent::getClassTypeId())) {
        return;
    }

    const auto* mouseEvent = static_cast<const SoMouseButtonEvent*>(event);

    const SoPickedPoint* pickedPoint = cb->getPickedPoint();
    if (!pickedPoint || !pickedPoint->getPath()->containsNode(this->annotation)) {
        return;
    }

    if (mouseEvent->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mouseEvent->getState() == SoMouseButtonEvent::UP) {
            cb->setHandled();
            Q_EMIT clicked(this);
        }
    }
    else if (mouseEvent->getButton() == SoMouseButtonEvent::BUTTON2) {
        cb->setHandled();
        if (mouseEvent->getState() == SoMouseButtonEvent::UP) {
            Q_EMIT rightClicked(this, QCursor::pos());
        }
    }
}

void EditableDatumLabel::setPickable(bool val)
{
    pickStyle->style = val ? SoPickStyle::SHAPE_ON_TOP : SoPickStyle::UNPICKABLE;
}

void EditableDatumLabel::setFocus()
{
    if (spinBox) {
        spinBox->selectNumber();
    }
}

void EditableDatumLabel::setPoints(SbVec3f p1, SbVec3f p2)
{
    label->setPoints(p1, p2);
    // TODO: here the position of the spinbox is not going to be center of p1, p2 because the point
    // given by getTextCenterPoint
    //  is not updated yet. it will be only on redraw so it is actually positioning on previous position.

    positionSpinbox();
    if (autoDistance) {
        setLabelRecommendedDistance();
    }
}

void EditableDatumLabel::setPoints(Base::Vector3d p1, Base::Vector3d p2)
{
    setPoints(
        SbVec3f(float(p1.x), float(p1.y), float(p1.z)),
        SbVec3f(float(p2.x), float(p2.y), float(p2.z))
    );
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelType(SoDatumLabel::Type type, Function funct)
{
    label->datumtype = type;
    function = funct;
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelDistance(double val)
{
    label->param1 = float(val);
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelStartAngle(double val)
{
    label->param2 = float(val);
}

// NOLINTNEXTLINE
void EditableDatumLabel::setLabelRange(double val)
{
    label->param3 = float(val);
}

void EditableDatumLabel::setLabelRecommendedDistance()
{
    // Takes the 3d view size, and set the label distance to a % of that, such that the distance
    // does not depend on the zoom level.
    float width = -1.;
    float length = -1.;
    viewer->getDimensions(width, length);

    if (width == -1. || length == -1.) {
        return;
    }

    label->param1 = (autoDistanceReverse ? -1.0F : 1.0F) * (width + length) * 0.03F;  // NOLINT
}

void EditableDatumLabel::setLabelAutoDistanceReverse(bool val)
{
    autoDistanceReverse = val;
}

void EditableDatumLabel::setSpinboxVisibleToMouse(bool val)
{
    if (!spinBox) {
        return;
    }
    spinBox->setAttribute(Qt::WA_TransparentForMouseEvents, !val);
}

void EditableDatumLabel::setLockedAppearance(bool locked)
{
    if (!spinBox || !lockIconLabel) {
        return;
    }
    spinBox->addIconSpace(locked);
    lockIconLabel->setVisible(locked);
    if (auto* edit = spinBox->findChild<QLineEdit*>()) {
        updateGeometry(edit);
    }
    const QFontMetrics fm(spinBox->fontMetrics());
    int iconSize = fm.height();
    int padding = spinBox->getMargin();
    // position lock icon inside the spinbox
    QSize spinboxSize = spinBox->size();
    lockIconLabel->setGeometry(
        spinboxSize.width() - iconSize - padding,
        (spinboxSize.height() - iconSize) / 2,
        iconSize,
        iconSize
    );
}

void EditableDatumLabel::resetLockedState()
{
    hasFinishedEditing = false;
    setLockedAppearance(false);
}

EditableDatumLabel::Function EditableDatumLabel::getFunction()
{
    return function;
}

#include "moc_EditableDatumLabel.cpp"  // NOLINT
