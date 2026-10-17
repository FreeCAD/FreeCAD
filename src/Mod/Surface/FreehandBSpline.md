# Freehand B-spline

Choose **Freehand B-Spline** to enter drawing mode. Each click places an
interpolation point; the moving cursor previews the next point and resulting
B-spline without adding it to the document. New points lie in a plane parallel
to the view through the preceding point (the object origin for the first point).
Drawing clicks that snap to a vertex or edge also lock the new point to that
reference. Disable snapping to place points without automatic locks. Moving the
cursor previews the snap without creating a lock.
Choose **End B-spline**, right-click, or press Esc to enter editing mode.
Then select and drag points or control-polygon segments, and accept the task
when finished. Double-click the object in the tree to reopen editing mode.

- Ctrl-click adds/removes points or segments from the selection. Ctrl+A selects
  all points. Dragging a segment moves both endpoints.
- X, Y, Z toggle a document-axis constraint during a drag; Shift is not required.
- Shift reduces movement to one tenth and temporarily suppresses snapping.
- Double-click the curve to insert a point on it; Delete removes selected points
  and both endpoints of selected segments.
- Esc cancels an active drag. Task Cancel restores the entire edit transaction.
- Click an X, Y or Z coordinate label in the view to edit it in a unit-aware
  spinbox. Enter or leaving the field applies the value; Escape cancels. Existing
  reference locks still apply.
- The table edits local point coordinates. Align selection projects the selected
  points onto the line through the first and last selected points in curve order.
- Dragging snaps to nearby vertices and edges within ten pixels, preferring
  vertices. This is temporary positioning, separate from a persistent lock.
- To lock a point, select the spline point and a reference vertex, edge or face,
  then choose **Lock to**. Vertex locks fix the point; edge and
  face locks allow sliding on the reference. The spline updates when its source
  geometry changes. Edge locks retain their fractional parameter; face locks
  retain UV coordinates, clamped to the trimmed face boundary.
- The **Locked to** column shows `ObjectName.Vertex1`, `ObjectName.Edge1` or
  `ObjectName.Face1`. Edit the reference or clear the cell to unlock. References
  are saved with the document; self-references and dependency cycles are rejected.
  Deleted objects or missing subelements unlock the affected points at their last
  positions and clear their tangencies. Unrelated valid locks remain attached.
- **Tangent to** offers **None** and edges meeting at a locked vertex, the locked
  edge, or the locked face. Edge tangency follows the edge direction; face tangency
  projects the neighbouring spline direction into the face tangent plane. If that
  direction is normal to the face, its surface U direction is used. Unlocking or
  replacing a point reference clears its tangency. Tangencies survive save/reopen
  and are remapped when points are inserted or deleted. Conflicting adjacent linear
  segments are rejected.
- **Interpolation spacing** controls the curve between its points: Uniform uses
  equal parameter intervals, Centripetal uses square roots of point distances,
  and Chord length uses the distances themselves. Centripetal is the default.
  All three methods interpolate the same points; the combo box explains them
  in its tooltip.
- Linear interpolation makes the selected spans exactly straight. Smooth
  interpolation restores their interpolated shape. Adjacent straight spans may
  form sharp corners. Closed curves need at least three distinct points.

Editing hides the normal object display and uses one unpickable curve preview,
with Sketcher colors and line styles, a bold tree entry, small coordinate labels,
and separate point/control-polygon handles.

Run `TestSurfaceFreehandBSpline` for geometry and GUI regressions.
