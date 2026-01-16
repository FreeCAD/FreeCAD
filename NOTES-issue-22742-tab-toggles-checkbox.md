# Issue #22742: Tab Key Toggles Boolean Fields

**Issue**: https://github.com/FreeCAD/FreeCAD/issues/22742
**Status**: Ready for Testing
**Date**: 2026-01-16

## Problem Summary

When navigating the Data tab properties panel with the Tab key, boolean (Yes/No) fields get toggled just by receiving focus. They should only toggle on explicit user action (mouse click or Return key).

## Reproduction Steps

1. Select an object (e.g., an Extrude)
2. In the Data tab, click on a numeric field (e.g., "Length Rev")
3. Press Tab to move to the next field
4. If the next field is a boolean (e.g., "Solid"), it gets toggled automatically

**Expected**: Next field receives focus only; value unchanged
**Actual**: Boolean field is toggled to the opposite value

## Root Cause

The bug was in `PropertyItemDelegate::eventFilter()` - when a checkbox received a `FocusIn` event, it unconditionally called `toggle()`. This happened regardless of whether focus came from Tab navigation or mouse click.

The original code:
```cpp
if (parentEditor && parentEditor->activeEditor == checkBox) {
    checkBox->toggle();  // BUG: toggles on ANY FocusIn
    QTimer::singleShot(0, this, [this]() { valueChanged(); });
}
```

## Current Progress

### Working Behaviors ✅

1. **Click on checkbox field**: Toggles the value and persists correctly
2. **Return key on selected boolean row**: Toggles the value correctly
3. **Tab into checkbox field**: Does NOT auto-toggle (bug fixed!)

### Remaining Issues 🔧

1. **Tab skips checkbox fields entirely**: When tabbing through fields, checkbox fields are skipped. This is because the editor is closed immediately on non-mouse activation.
2. **Tab from checkbox exits property editor**: If a checkbox field somehow has focus, pressing Tab takes focus elsewhere in the UI instead of the next property field.

## The Fix (Current State)

### Changes to PropertyItemDelegate.h

Added new member:
```cpp
mutable bool activatedByMouse;  // Track if current editor was activated by mouse click
```

### Changes to PropertyItemDelegate.cpp

1. **In `editorEvent()`**: Only set `pressed = true` on MouseButtonPress:
```cpp
if (event->type() == QEvent::MouseButtonPress) {
    this->pressed = true;
}
```

2. **In `createEditor()`**: Save `pressed` state to `activatedByMouse` BEFORE calling `setFocus()`:
```cpp
this->activatedByMouse = this->pressed;
this->pressed = false;
// ... then setFocus() ...
```

3. **In `eventFilter()`**: Handle checkbox FocusIn based on activation method:
```cpp
auto* checkBox = qobject_cast<QCheckBox*>(o);
if (checkBox) {
    auto parentEditor = qobject_cast<PropertyEditor*>(this->parent());
    if (parentEditor && parentEditor->activeEditor == checkBox) {
        if (this->activatedByMouse) {
            // Mouse click activated the editor - toggle the checkbox
            checkBox->toggle();
            this->activatedByMouse = false;
        }
        else {
            // Tab/keyboard navigation - close editor immediately without toggling
            parentEditor->closeEditor();
        }
    }
}
```

4. **In `valueChanged()`**: Close editor after commit for checkboxes:
```cpp
if (qobject_cast<QComboBox*>(propertyEditor) || qobject_cast<QCheckBox*>(propertyEditor)) {
    Q_EMIT closeEditor(propertyEditor);
    return;
}
```

### Changes to PropertyEditor.cpp

Added Return key handling for boolean properties to toggle directly without opening editor:
```cpp
else if (isEditKey) {
    event->accept();
    auto index = model() ? model()->buddy(currentIndex()) : QModelIndex();
    if (index.isValid()) {
        // For boolean properties, toggle directly instead of opening editor
        auto* item = static_cast<PropertyItem*>(index.internalPointer());
        if (qobject_cast<PropertyBoolItem*>(item)) {
            QVariant currentValue = index.data(Qt::EditRole);
            bool newValue = !currentValue.toBool();
            propertyModel->setData(index, QVariant(newValue), Qt::EditRole);
            return;
        }
        openEditor(index);
    }
    return;
}
```

## Key Insights

1. **Empty lambda callback was a problem**: Originally, PropertyItemDelegate::createEditor() passed an empty lambda `[]() noexcept {}` for boolean items, which prevented valueChanged() from being called when the checkbox toggled.

2. **FocusIn fires synchronously from setFocus()**: The `activatedByMouse` flag must be set BEFORE calling `editor->setFocus()` because the FocusIn event fires immediately.

3. **Closing editor on Tab causes skip behavior**: The current approach of closing the editor immediately when Tab activates it causes the entire row to be skipped during Tab navigation.

## Latest Changes (Tab Navigation Fix)

Added Tab navigation support so Tab can navigate TO and FROM boolean fields:

### In PropertyEditor.cpp `closeEditor(QWidget*, EndEditHint)`

When Tab/Backtab closes an editor and navigates to a boolean field, **don't open editor** - just break out of the loop with the boolean row selected:

```cpp
// For boolean properties, don't open editor - just keep painted Yes/No visible.
// User can click or press Return to toggle. See GitHub issue #22742.
if (qobject_cast<PropertyBoolItem*>(item)) {
    break;
}
```

### In PropertyEditor.cpp `keyPressEvent()`

Added Tab/Backtab handling when a boolean row is selected (no editor open):

```cpp
else if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
    // Handle Tab/Backtab when not editing (e.g., boolean field selected without editor).
    auto index = model() ? model()->buddy(currentIndex()) : QModelIndex();
    if (index.isValid()) {
        auto* item = static_cast<PropertyItem*>(index.internalPointer());
        if (qobject_cast<PropertyBoolItem*>(item)) {
            event->accept();
            QAbstractItemDelegate::EndEditHint hint = (key == Qt::Key_Tab)
                ? QAbstractItemDelegate::EditNextItem
                : QAbstractItemDelegate::EditPreviousItem;
            closeEditor(nullptr, hint);
            return;
        }
    }
}
```

## Expected Behavior After Fix

- ✅ **Tab to boolean field**: Selects the row, keeps painted Yes/No visible, does NOT toggle
- ✅ **Tab from boolean field**: Navigates to next editable field
- ✅ **Click on boolean field**: Toggles the value
- ✅ **Double-click / rapid clicking**: Each click toggles (no more "every other click lost")
- ✅ **Return/Enter on boolean field** (Mac): Toggles the value
- ✅ **F2 on boolean field** (Windows/Linux): Toggles the value
- ✅ **Shift+Tab**: Works in reverse direction

## Additional Fix: Tab Navigation from Boolean Fields

Added interception in `PropertyEditor::event()` to prevent Qt's default focus navigation when Tab is pressed on a boolean field (which has no editor open):

```cpp
if (event->type() == QEvent::KeyPress) {
    auto kevent = static_cast<QKeyEvent*>(event);
    if (kevent->key() == Qt::Key_Tab || kevent->key() == Qt::Key_Backtab) {
        auto index = model() ? model()->buddy(currentIndex()) : QModelIndex();
        if (index.isValid()) {
            auto* item = static_cast<PropertyItem*>(index.internalPointer());
            if (qobject_cast<PropertyBoolItem*>(item)) {
                keyPressEvent(kevent);
                return true;
            }
        }
    }
}
```

## Additional Fix: Double-Click on Boolean Fields

Modified `editorEvent()` to allow double-clicks on boolean items to trigger toggle (prevents "every other click lost" with rapid clicking):

```cpp
if (dynamic_cast<PropertyBoolItem*>(property)) {
    // Treat double-click as a press to trigger toggle
    this->pressed = true;
    return QItemDelegate::editorEvent(event, model, option, index);
}
```

## Platform Differences

The "edit key" that toggles boolean values differs by platform:
- **macOS**: Return/Enter key
- **Windows/Linux**: F2 key

This is existing FreeCAD behavior (see `keyPressEvent()` lines 250-257).

## Files Modified

```
src/Gui/propertyeditor/PropertyItemDelegate.h   - Added activatedByMouse member
src/Gui/propertyeditor/PropertyItemDelegate.cpp - Modified editorEvent(), createEditor(), eventFilter(), valueChanged()
src/Gui/propertyeditor/PropertyEditor.cpp       - Added event(), keyPressEvent() handling for boolean properties
```

## Testing Checklist

### Basic Functionality
- [ ] Click on boolean field toggles value
- [ ] Rapid/double-clicking toggles on each click
- [ ] Tab into boolean field does NOT toggle
- [ ] Tab from boolean field goes to next property (not outside widget)
- [ ] Shift+Tab works in reverse
- [ ] Return (Mac) / F2 (Win/Linux) toggles value
- [ ] Value persists after toggle (check by clicking elsewhere and back)

### Edge Cases
- [ ] Tab through multiple consecutive boolean fields
- [ ] Boolean field at end of property list (Tab should wrap or stop gracefully)
- [ ] Boolean field at start of property list (Shift+Tab behavior)
- [ ] Read-only boolean fields are skipped during Tab navigation
- [ ] Combo boxes still work correctly (popup on focus)
- [ ] Other property types still work correctly

### Visual
- [ ] Yes/No text always visible (never replaced by checkbox widget during Tab navigation)
- [ ] Selection highlight visible on boolean fields
- [ ] No visual glitches or flickering

## TODO: Investigate Combo Box Behavior
There's a combo box behavior that should be looked into separately (noted by user during testing).

## Finding Edge Case Test Objects

### Boolean at start/end of property list
Need to find an object type where a boolean property is first or last in the Data tab.

### Read-only boolean fields
Need to find an object with a read-only boolean property to test Tab skip behavior.
