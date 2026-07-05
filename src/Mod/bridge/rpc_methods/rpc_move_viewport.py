from __future__ import annotations

from rpc import *


@rpc_method
def move_viewport(
    self,
    action: str,
    params: dict[str, Any] | None = None,
    take_screenshot: bool = True,
    width: int | None = None,
    height: int | None = None,
) -> dict[str, Any]:
    if params is None:
        params = {}

    def gui_task():
        try:
            view = FreeCADGui.ActiveDocument.ActiveView
            if not hasattr(view, "viewIsometric"):
                return "No 3D view available"

            if action == "fit_all":
                view.fitAll()

            elif action == "zoom_to_object":
                obj_name = params.get("object_name")
                doc = FreeCAD.ActiveDocument
                obj = doc.getObject(obj_name) if doc and obj_name else None
                if obj:
                    FreeCADGui.Selection.clearSelection()
                    FreeCADGui.Selection.addSelection(obj)
                    FreeCADGui.SendMsgToActiveView("ViewSelection")
                else:
                    view.fitAll()

            elif action in ("zoom_in", "zoom_out"):
                try:
                    from pivy import coin

                    factor = float(params.get("factor", 1.25))
                    if action == "zoom_out":
                        factor = 1.0 / factor
                    camera = view.getCameraNode()
                    if camera.getTypeId() == coin.SoOrthographicCamera.getClassTypeId():
                        camera.height.setValue(camera.height.getValue() / factor)
                    else:
                        pos = camera.position.getValue()
                        direction = camera.orientation.getValue().multVec(
                            coin.SbVec3f(0, 0, -1)
                        )
                        move = coin.SbVec3f(
                            direction[0]
                            * camera.focalDistance.getValue()
                            * (1.0 - 1.0 / factor),
                            direction[1]
                            * camera.focalDistance.getValue()
                            * (1.0 - 1.0 / factor),
                            direction[2]
                            * camera.focalDistance.getValue()
                            * (1.0 - 1.0 / factor),
                        )
                        camera.position.setValue(pos + move)
                        camera.focalDistance.setValue(
                            camera.focalDistance.getValue() / factor
                        )
                except ImportError:
                    view.fitAll()

            elif action == "navicube":
                preset = params.get("preset", "Isometric")
                fn = _VIEW_PRESETS.get(preset, lambda v: v.viewIsometric())
                fn(view)
                view.fitAll()

            elif action == "pan":
                try:
                    from pivy import coin

                    dx = float(params.get("dx", 0))
                    dy = float(params.get("dy", 0))
                    camera = view.getCameraNode()
                    orient = camera.orientation.getValue()
                    right = orient.multVec(coin.SbVec3f(1, 0, 0))
                    up = orient.multVec(coin.SbVec3f(0, 1, 0))
                    scale = camera.focalDistance.getValue() * 0.001
                    pos = camera.position.getValue()
                    camera.position.setValue(
                        pos[0] - right[0] * dx * scale - up[0] * dy * scale,
                        pos[1] - right[1] * dx * scale - up[1] * dy * scale,
                        pos[2] - right[2] * dx * scale - up[2] * dy * scale,
                    )
                except ImportError:
                    pass

            elif action == "rotate":
                axis = params.get("axis", [0, 0, 1])
                angle_deg = float(params.get("angle_deg", 45))
                rot = FreeCAD.Rotation(FreeCAD.Vector(*axis), angle_deg)
                current = view.getCameraOrientation()
                combined = rot.multiply(current)
                view.setCameraOrientation(combined)

            elif action == "set_camera":
                if "orientation" in params:
                    o = params["orientation"]
                    rot = FreeCAD.Rotation(
                        FreeCAD.Vector(
                            o.get("axis_x", 0),
                            o.get("axis_y", 0),
                            o.get("axis_z", 1),
                        ),
                        o.get("angle_deg", 0),
                    )
                    view.setCameraOrientation(rot)
                if "type" in params:
                    view.setCameraType(params["type"])
                if "fit" in params and params["fit"]:
                    view.fitAll()

            FreeCADGui.updateGui()
            return True
        except Exception as e:
            FreeCAD.Console.PrintError(f"move_viewport error: {e}\n")
            return str(e)

    rpc_request_queue.put(lambda: _engage_viewport_lock() or True)
    rpc_response_queue.get(timeout=self.TIMEOUT)

    rpc_request_queue.put(gui_task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)

    if res is not True:
        rpc_request_queue.put(lambda: _release_viewport_lock() or True)
        try:
            rpc_response_queue.get(timeout=self.TIMEOUT)
        except Exception:
            pass
        return {"success": False, "error": res, "screenshot": None}

    if not take_screenshot:
        rpc_request_queue.put(lambda: _release_viewport_lock() or True)
        try:
            rpc_response_queue.get(timeout=self.TIMEOUT)
        except Exception:
            pass
        return {"success": True, "error": None, "screenshot": None}

    fd, tmp_path = tempfile.mkstemp(suffix=".png")
    os.close(fd)
    rpc_request_queue.put(
        lambda: self._capture_current_view_gui(tmp_path, width, height)
    )
    cap_res = rpc_response_queue.get(timeout=self.TIMEOUT)

    rpc_request_queue.put(lambda: _release_viewport_lock() or True)
    try:
        rpc_response_queue.get(timeout=self.TIMEOUT)
    except Exception:
        pass

    if cap_res is True:
        try:
            encoded = _read_valid_screenshot(tmp_path)
        finally:
            if os.path.exists(tmp_path):
                os.remove(tmp_path)
        return {"success": True, "error": None, "screenshot": encoded}

    if os.path.exists(tmp_path):
        os.remove(tmp_path)
    return {"success": True, "error": None, "screenshot": None}
