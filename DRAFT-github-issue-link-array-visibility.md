# Investigation Notes: Link Array Child Visibility Toggle Issue

**Status**: Could not reproduce reliably - keeping notes for future reference
**Date**: 2026-01-15

## Summary
Observed an issue where expanding a Draft Link Array in the tree view and clicking the eye icon or pressing Space on the base object (the linked object shown as a child) did not toggle its visibility.

However, after restarting FreeCAD and creating fresh documents, the issue could not be reproduced. This may be:
1. An intermittent bug that requires specific conditions
2. A glitched UI state from a previous session
3. Related to a recently reverted commit (see Git History section below)

## Observed Behavior (When Issue Occurred)
1. Create any object (e.g., a Cube)
2. Create a Draft Polar Array (with "Use Link" enabled) from the object
3. In the Properties panel, set "Expand Array" to `true` on the Array object
4. Expand the Array in the tree view to see the child element
5. Click on the child (the base object, e.g., "Cube") in the tree
6. Press Space or click the eye icon to toggle visibility

**Expected:** The visibility of the array element toggles
**Actual:** Nothing happens

## Root Cause Analysis

In `src/Gui/Tree.cpp` lines 1964-1983, the eye icon click handler:

```cpp
if (iconRect.contains(mousePos)) {
    auto obj = objitem->object()->getObject();
    char const* objname = obj->getNameInDocument();  // e.g., "Cube"

    App::DocumentObject* parent = nullptr;
    std::ostringstream subName;
    objitem->getSubName(subName, parent);  // parent = Array, subName = ""

    int visible = -1;
    if (parent) {
        visible = parent->isElementVisible(objname);  // BUG: passes "Cube", should pass "0"
    }
    if (parent && visible >= 0) {
        parent->setElementVisible(objname, !visible);
    }
    // ...
}
```

The problem is that `objname` is the base object's document name (e.g., "Cube"), but `LinkBaseExtension::extensionIsElementVisible()` expects an **array index** like "0", "1", "2" (when `ShowElement=false`).

In `src/App/Link.cpp` lines 1147-1165:
```cpp
int LinkBaseExtension::extensionIsElementVisible(const char* element)
{
    int index = _getShowElementValue() ? getElementIndex(element) : getArrayIndex(element);
    if (index >= 0) {
        // ... returns visibility for that index
    }
    // index is -1 for non-numeric names like "Cube", falls through
    DocumentObject* linked = getTrueLinkedObject(false);
    if (linked) {
        return linked->isElementVisible(element);  // Delegates to base object
    }
    return -1;
}
```

When `element` is "Cube", `getArrayIndex()` returns -1 (it expects a numeric string like "0"), so the function falls through and delegates to the linked object, which doesn't handle it correctly.

## Comparison with Working Code

`Selection::setVisible()` in `src/Gui/Selection/Selection.cpp` (lines 1680-1730) works correctly because it:
1. Uses `obj->resolve(sel.SubName.c_str(), &parent, &elementName)` to get the proper `elementName`
2. Passes `elementName` (not `objname`) to `isElementVisible`/`setElementVisible`

## Proposed Fix

In `src/Gui/Tree.cpp`, the eye icon handler should build the proper element identifier:

```cpp
if (iconRect.contains(mousePos)) {
    auto obj = objitem->object()->getObject();
    char const* objname = obj->getNameInDocument();

    App::DocumentObject* parent = nullptr;
    std::ostringstream subName;
    objitem->getSubName(subName, parent);

    int visible = -1;
    std::string elementName;

    if (parent) {
        // Build the full subname including the child object name
        std::string fullSubName = subName.str() + std::string(objname) + ".";

        // Use resolve() to get the proper element identifier
        App::DocumentObject* resolvedParent = nullptr;
        parent->resolve(fullSubName.c_str(), &resolvedParent, &elementName);

        if (resolvedParent) {
            visible = resolvedParent->isElementVisible(elementName.c_str());
            if (visible >= 0) {
                resolvedParent->setElementVisible(elementName.c_str(), !visible);
            }
        }
    }

    if (visible < 0) {
        // Fallback to object's own Visibility property
        visible = obj->Visibility.getValue();
        obj->Visibility.setValue(!visible);
    }

    event->accept();
    return;
}
```

Alternatively, a simpler approach might be to append the child object name to the subname and pass that directly, as the `setElementVisible` infrastructure already handles path resolution in some cases.

## Git History - Potentially Related Commits

Recent commits to `src/Gui/Tree.cpp` that may be relevant:

```
69058376e6 Base: Remove Boost-based signals and switch to `FastSignals`.
60bd277d7a UI: Fix select all instances of an object in the tree
1b886ef961 Revert "GUI: fix "select all instances" (#25503)"   <-- Nov 28, 2025
6b60867368 GUI: fix "select all instances" (#25503)            <-- Nov 26, 2025
```

The commit `6b60867368` modified `getSubName()` to add a `claimChildren()` check, which was then **reverted** by `1b886ef961`. This change affected how the tree determines parent-child relationships for visibility toggling.

The reverted code added:
```cpp
auto children = parent->object()->claimChildren();
bool parentClaimsThis = false;
for (auto child : children) {
    if (child == object()->getObject()) {
        parentClaimsThis = true;
        break;
    }
}
if (!parentClaimsThis) {
    topParent = nullptr;
    str.str("");
    return NotGroup;
}
```

If the issue resurfaces, check if it's related to this `claimChildren()` logic.

## Environment
- OS: macOS
- FreeCAD Version: main branch (commit 4cc0a1b718)
- Workbench: Draft

## Related Resources
- Forum thread: [App::LinkGroup breaks Visibility property handling](https://forum.freecad.org/viewtopic.php?p=366938)
- Forum thread: [Array hide one Element - can not bring it back to visible](https://forum.freecad.org/viewtopic.php?p=529216)
- GitHub Issue #25503: "fix select all instances" (reverted)

## If Issue Resurfaces

1. Note exact steps to reproduce, including any prior actions in the session
2. Check if `getSubName()` is returning the correct `parent` and `subName`
3. Verify that the element identifier passed to `isElementVisible()` matches what `LinkBaseExtension` expects (array index like "0", not object name like "Cube")
4. Check for any new commits to Tree.cpp that modify `getSubName()` or visibility handling
