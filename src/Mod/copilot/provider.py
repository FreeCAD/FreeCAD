# SPDX-License-Identifier: LGPL-2.1-or-later
"""Intent provider interfaces for Copilot.

The default provider is local and deterministic. It converts common CAD prompts
into a small command language that executor.py applies to the active document.
"""

import json
import os
import re
import ast
import urllib.error
import urllib.request


class LocalIntentProvider:
    """Parse simple English prompts without any network dependency."""

    def plan(self, prompt, context=None):
        text = prompt.strip()
        if not text:
            raise ValueError("Enter a modeling request first.")
        if context and context.get("image"):
            raise ValueError("Image prompts require COPILOT_PROVIDER=openrouter with a vision-capable model.")

        lower = text.lower()
        if _has_any(lower, ["delete", "remove"]) and _has_any(lower, ["selected", "selection"]):
            return [{"action": "delete_selected"}]

        if _has_any(lower, ["fit", "zoom"]) and "view" in lower:
            return [{"action": "fit_view"}]

        if lower.startswith("save") or " save " in " {0} ".format(lower):
            path = _quoted(text) or _path_after_keyword(text, "as")
            return [{"action": "save", "path": path}]

        if lower.startswith("open "):
            return [{"action": "open", "path": _quoted(text) or text[5:].strip()}]

        color = _color(lower)
        if color and _has_any(lower, ["selected", "selection"]):
            return [{"action": "set_color", "target": "selected", "color": color}]

        transform = _transform(lower)
        if transform:
            return [transform]

        primitive = _primitive(lower)
        if primitive:
            return [primitive]

        raise ValueError(
            "I can create boxes, cylinders, spheres, and cones; move, rotate, scale, "
            "color, delete selected objects; fit the view; and save/open files."
        )


def get_provider():
    """Return the configured provider.

    COPILOT_PROVIDER=openai uses OpenAI's Responses API when OPENAI_API_KEY is
    present. The local provider keeps this module usable inside offline FreeCAD
    containers.
    """

    provider_name = os.environ.get("COPILOT_PROVIDER", "local").strip().lower()
    if provider_name == "local":
        return LocalIntentProvider()
    if provider_name == "openai":
        return OpenAIIntentProvider()
    if provider_name == "openrouter":
        return OpenRouterIntentProvider()
    raise ValueError("Unsupported COPILOT_PROVIDER: {0}".format(provider_name))


def describe_plan(plan):
    return json.dumps(plan, indent=2, sort_keys=True)


class OpenAIIntentProvider:
    """Use OpenAI to translate prompts into the same safe action plan."""

    endpoint = "https://api.openai.com/v1/responses"

    def __init__(self):
        self.api_key = os.environ.get("OPENAI_API_KEY", "").strip()
        self.model = os.environ.get("OPENAI_MODEL", "gpt-4.1-mini").strip()
        if not self.api_key:
            raise ValueError("OPENAI_API_KEY is required when COPILOT_PROVIDER=openai.")

    def plan(self, prompt, context=None):
        payload = {
            "model": self.model,
            "instructions": (
                "You are a FreeCAD CAD planning assistant. Convert the user request "
                "into JSON only. Use only supported actions. Prefer selected-object "
                "edit actions when the user says selected or selection. Do not invent "
                "unsupported operations."
            ),
            "input": [
                {
                    "role": "user",
                    "content": [
                        {
                            "type": "input_text",
                            "text": json.dumps(
                                {
                                    "request": prompt,
                                    "context": context or {},
                                    "supported_actions": _supported_actions(),
                                }
                            ),
                        }
                    ],
                }
            ],
            "text": {
                "format": {
                    "type": "json_schema",
                    "name": "freecad_copilot_plan",
                    "strict": False,
                    "schema": {
                        "type": "object",
                        "additionalProperties": False,
                        "required": ["steps"],
                        "properties": {
                            "steps": {
                                "type": "array",
                                "minItems": 1,
                                "items": {"type": "object"},
                            }
                        },
                    },
                }
            },
            "max_output_tokens": 800,
        }
        data = json.dumps(payload).encode("utf-8")
        request = urllib.request.Request(
            self.endpoint,
            data=data,
            headers={
                "Authorization": "Bearer {0}".format(self.api_key),
                "Content-Type": "application/json",
            },
            method="POST",
        )
        try:
            with urllib.request.urlopen(request, timeout=45) as response:
                body = response.read().decode("utf-8")
        except urllib.error.HTTPError as err:
            detail = err.read().decode("utf-8", errors="replace")
            raise ValueError("OpenAI request failed: {0} {1}".format(err.code, detail))
        parsed = json.loads(body)
        plan_text = _response_text(parsed)
        plan = json.loads(plan_text)
        return _validate_plan(plan.get("steps", []))


class OpenRouterIntentProvider:
    """Use OpenRouter's OpenAI-compatible Chat Completions API."""

    def __init__(self):
        self.api_key = os.environ.get("OPENROUTER_API_KEY", "").strip()
        self.model = os.environ.get("OPENROUTER_MODEL", "openai/gpt-4o-mini").strip()
        self.base_url = os.environ.get("OPENROUTER_BASE_URL", "https://openrouter.ai/api/v1").rstrip("/")
        self.site_url = os.environ.get("OPENROUTER_SITE_URL", "http://localhost:8010").strip()
        self.app_name = os.environ.get("OPENROUTER_APP_NAME", "MetaCad FreeCAD Copilot").strip()
        if not self.api_key:
            raise ValueError("OPENROUTER_API_KEY is required when COPILOT_PROVIDER=openrouter.")

    def plan(self, prompt, context=None):
        payload = {
            "model": self.model,
            "messages": [
                {
                    "role": "system",
                    "content": (
                        "You are a FreeCAD CAD planning assistant. Return JSON only, "
                        "with this shape: {\"steps\":[{\"action\":\"...\"}]}. "
                        "Use only the supported actions. Do not include markdown. "
                        "If an image is provided, approximate the visible object as a "
                        "parametric CAD model by decomposing it into safe primitives, "
                        "placements, colors, boolean fuses, and cuts. Create multiple "
                        "steps for complex objects. Use reasonable dimensions when the "
                        "image has no scale."
                    ),
                },
                _openrouter_user_message(prompt, context or {}),
            ],
            "temperature": 0.1,
            "max_tokens": 800,
            "response_format": {"type": "json_object"},
        }
        data = json.dumps(payload).encode("utf-8")
        headers = {
            "Authorization": "Bearer {0}".format(self.api_key),
            "Content-Type": "application/json",
        }
        if self.site_url:
            headers["HTTP-Referer"] = self.site_url
        if self.app_name:
            headers["X-Title"] = self.app_name

        request = urllib.request.Request(
            "{0}/chat/completions".format(self.base_url),
            data=data,
            headers=headers,
            method="POST",
        )
        try:
            with urllib.request.urlopen(request, timeout=45) as response:
                body = response.read().decode("utf-8")
        except urllib.error.HTTPError as err:
            detail = err.read().decode("utf-8", errors="replace")
            raise ValueError("OpenRouter request failed: {0} {1}".format(err.code, detail))
        parsed = json.loads(body)
        content = parsed["choices"][0]["message"]["content"]
        plan = json.loads(content)
        return _validate_plan(plan.get("steps", []))


def _response_text(response):
    chunks = []
    for item in response.get("output", []):
        for content in item.get("content", []):
            if content.get("type") == "output_text" and "text" in content:
                chunks.append(content["text"])
    if chunks:
        return "".join(chunks)
    if "output_text" in response:
        return response["output_text"]
    raise ValueError("OpenAI response did not include output text.")


def _supported_actions():
    return [
        "create_box(length,width,height,name,x,y,z)",
        "create_cylinder(radius,height,name,x,y,z)",
        "create_sphere(radius,name,x,y,z)",
        "create_cone(radius1,radius2,height,name,x,y,z)",
        "create_torus(radius1,radius2,name,x,y,z)",
        "move_selected(x,y,z)",
        "rotate_selected(axis,angle)",
        "scale_selected(factor)",
        "select_by_name(name)",
        "delete_selected()",
        "set_color(color,name)",
        "boolean_fuse(objects,name)",
        "boolean_cut(base,tool,name)",
        "fit_view()",
        "save(path)",
        "open(path)",
    ]


def _validate_plan(plan):
    allowed = {
        "create_box",
        "create_cylinder",
        "create_sphere",
        "create_cone",
        "create_torus",
        "move_selected",
        "rotate_selected",
        "scale_selected",
        "select_by_name",
        "delete_selected",
        "set_color",
        "boolean_fuse",
        "boolean_cut",
        "fit_view",
        "save",
        "open",
    }
    if not isinstance(plan, list) or not plan:
        raise ValueError("Planner returned no steps.")
    plan = [_normalize_step(step) for step in plan]
    for step in plan:
        if not isinstance(step, dict):
            raise ValueError("Planner returned an invalid step.")
        action = step.get("action")
        if action not in allowed:
            raise ValueError("Planner returned unsupported action: {0}".format(action))
    return plan


def _normalize_step(step):
    if isinstance(step, str):
        return _parse_function_style_step(step)
    if not isinstance(step, dict):
        return step

    action = step.get("action")
    if isinstance(action, str) and "(" in action and action.endswith(")"):
        parsed = _parse_function_style_step(action)
        for key, value in step.items():
            if key != "action":
                parsed[key] = value
        return parsed
    return step


def _parse_function_style_step(text):
    try:
        expression = ast.parse(text.strip(), mode="eval").body
    except SyntaxError:
        return {"action": text}

    if not isinstance(expression, ast.Call) or not isinstance(expression.func, ast.Name):
        return {"action": text}

    action = expression.func.id
    step = _step_from_function_args(action, [_literal_arg(arg) for arg in expression.args])
    for keyword in expression.keywords:
        if keyword.arg:
            value = _literal_arg(keyword.value)
            if value is not _PLACEHOLDER:
                step[keyword.arg] = value
    return step


def _step_from_function_args(action, args):
    step = {"action": action}
    if action == "create_box":
        _assign_args(step, args, ["length", "width", "height", "name", "x", "y", "z"])
    elif action == "create_cylinder":
        _assign_args(step, args, ["radius", "height", "name", "x", "y", "z"])
    elif action == "create_sphere":
        _assign_args(step, args, ["radius", "name", "x", "y", "z"])
    elif action == "create_cone":
        _assign_args(step, args, ["radius1", "radius2", "height", "name", "x", "y", "z"])
    elif action == "create_torus":
        _assign_args(step, args, ["radius1", "radius2", "name", "x", "y", "z"])
    elif action == "move_selected":
        _assign_args(step, args, ["x", "y", "z"])
    elif action == "rotate_selected":
        _assign_args(step, args, ["axis", "angle"])
    elif action == "scale_selected":
        _assign_args(step, args, ["factor"])
    elif action == "select_by_name":
        _assign_args(step, args, ["name"])
    elif action == "set_color":
        _assign_args(step, args, ["color", "name"])
    elif action == "boolean_fuse":
        _assign_args(step, args, ["objects", "name"])
    elif action == "boolean_cut":
        _assign_args(step, args, ["base", "tool", "name"])
    elif action in ("delete_selected", "fit_view"):
        pass
    elif action in ("save", "open"):
        _assign_args(step, args, ["path"])
    return step


def _assign_args(step, args, names):
    for index, name in enumerate(names):
        if index < len(args) and args[index] is not _PLACEHOLDER:
            step[name] = args[index]


_PLACEHOLDER = object()


def _literal_arg(node):
    if isinstance(node, ast.Constant):
        return node.value
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        value = _literal_arg(node.operand)
        if isinstance(value, (int, float)):
            return -value
    if isinstance(node, ast.List):
        values = [_literal_arg(item) for item in node.elts]
        return [value for value in values if value is not _PLACEHOLDER]
    if isinstance(node, ast.Tuple):
        values = [_literal_arg(item) for item in node.elts]
        return tuple(value for value in values if value is not _PLACEHOLDER)
    return _PLACEHOLDER


def _has_any(text, words):
    return any(word in text for word in words)


def _openrouter_user_message(prompt, context):
    image = context.get("image") or {}
    request_context = dict(context)
    if "image" in request_context:
        request_context["image"] = {"path": image.get("path"), "attached": True}

    text = json.dumps(
        {
            "request": prompt,
            "context": request_context,
            "supported_actions": _supported_actions(),
            "output_rules": [
                "Return only JSON.",
                "Use {\"steps\": [...]} as the top-level shape.",
                "Each step must use an exact supported action name, not prose.",
                "Prefer multiple simple primitives for complex image objects.",
                "Set x, y, z placement fields when parts should not overlap.",
            ],
        }
    )
    data_url = image.get("data_url")
    if not data_url:
        return {"role": "user", "content": text}
    return {
        "role": "user",
        "content": [
            {"type": "text", "text": text},
            {"type": "image_url", "image_url": {"url": data_url}},
        ],
    }


def _quoted(text):
    match = re.search(r"['\"]([^'\"]+)['\"]", text)
    return match.group(1) if match else None


def _path_after_keyword(text, keyword):
    match = re.search(r"\b{0}\b\s+(.+)$".format(re.escape(keyword)), text, re.IGNORECASE)
    return match.group(1).strip() if match else None


def _number_after(text, names, default):
    for name in names:
        match = re.search(r"\b{0}\b\s*[:=]?\s*(-?\d+(?:\.\d+)?)".format(re.escape(name)), text)
        if match:
            return float(match.group(1))
    return default


def _first_number(text, default):
    match = re.search(r"-?\d+(?:\.\d+)?", text)
    return float(match.group(0)) if match else default


def _primitive(text):
    if "box" in text or "cube" in text:
        size = _first_number(text, 10.0)
        return {
            "action": "create_box",
            "name": "CopilotBox",
            "length": _number_after(text, ["length", "x", "width"], size),
            "width": _number_after(text, ["width", "y", "depth"], size),
            "height": _number_after(text, ["height", "z"], size),
        }

    if "cylinder" in text:
        radius = _number_after(text, ["radius", "r"], 5.0)
        height = _number_after(text, ["height", "h"], 10.0)
        return {"action": "create_cylinder", "name": "CopilotCylinder", "radius": radius, "height": height}

    if "sphere" in text or "ball" in text:
        radius = _number_after(text, ["radius", "r"], _first_number(text, 5.0))
        return {"action": "create_sphere", "name": "CopilotSphere", "radius": radius}

    if "cone" in text:
        radius1 = _number_after(text, ["radius1", "bottom", "base"], 5.0)
        radius2 = _number_after(text, ["radius2", "top"], 0.0)
        height = _number_after(text, ["height", "h"], 10.0)
        return {
            "action": "create_cone",
            "name": "CopilotCone",
            "radius1": radius1,
            "radius2": radius2,
            "height": height,
        }

    return None


def _transform(text):
    if "move" in text or "translate" in text:
        return {
            "action": "move_selected",
            "x": _number_after(text, ["x", "dx"], 0.0),
            "y": _number_after(text, ["y", "dy"], 0.0),
            "z": _number_after(text, ["z", "dz"], 0.0),
        }

    if "rotate" in text:
        axis = "z"
        for candidate in ("x", "y", "z"):
            if " around {0}".format(candidate) in text or " {0} axis".format(candidate) in text:
                axis = candidate
        return {"action": "rotate_selected", "axis": axis, "angle": _first_number(text, 90.0)}

    if "scale" in text:
        return {"action": "scale_selected", "factor": _first_number(text, 1.0)}

    return None


def _color(text):
    colors = {
        "red": [1.0, 0.0, 0.0],
        "green": [0.0, 0.8, 0.0],
        "blue": [0.0, 0.2, 1.0],
        "yellow": [1.0, 0.9, 0.0],
        "orange": [1.0, 0.45, 0.0],
        "white": [1.0, 1.0, 1.0],
        "black": [0.0, 0.0, 0.0],
        "gray": [0.45, 0.45, 0.45],
        "grey": [0.45, 0.45, 0.45],
    }
    for name, rgb in colors.items():
        if name in text:
            return rgb
    return None
