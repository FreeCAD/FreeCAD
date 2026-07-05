from __future__ import annotations

from rpc import *


@rpc_method
def boolean_op(
    self,
    doc_name: str,
    operation: str,
    operands: list[str],
    result_name: str | None = None,
    delete_operands: bool = False,
    recompute: bool = True,
    verify_growth: bool = True,
) -> dict[str, Any]:
    if not isinstance(operands, list) or len(operands) < 2:
        return {
            "success": False,
            "error": "'operands' must be a list of at least 2 object names",
        }

    op_norm = (operation or "").strip().lower()
    if op_norm not in ("union", "fuse", "cut", "difference", "intersect", "common"):
        return {
            "success": False,
            "error": f"Unknown boolean operation '{operation}'. Use union/cut/intersect.",
        }

    def task():
        try:
            doc = FreeCAD.getDocument(doc_name)
            if not doc:
                return f"Document '{doc_name}' not found"

            refs = []
            for ident in operands:
                ref, _ = _resolve_target(doc, ident)
                if ref is None:
                    return f"Operand '{ident}' not found"
                if not hasattr(ref, "Shape") or ref.Shape is None or ref.Shape.isNull():
                    return f"Operand '{ref.Name}' has no usable Shape"
                refs.append(ref)

            base_shape = refs[0].Shape.copy()
            others = [r.Shape.copy() for r in refs[1:]]

            def _volume_of(shape) -> float | None:
                try:
                    return float(shape.Volume)
                except Exception:
                    return None

            operand_volumes = []
            for r, shape in zip(refs, [base_shape, *others]):
                operand_volumes.append({"operand": r.Name, "volume": _volume_of(shape)})
            known_volumes = [
                entry["volume"]
                for entry in operand_volumes
                if entry["volume"] is not None
            ]
            max_operand_volume = max(known_volumes) if known_volumes else None
            min_operand_volume = min(known_volumes) if known_volumes else None
            sum_operand_volume = sum(known_volumes) if known_volumes else None

            if op_norm in ("union", "fuse"):
                out = base_shape.fuse(others)
            elif op_norm in ("cut", "difference"):
                out = base_shape
                for s in others:
                    out = out.cut(s)
            else:
                out = base_shape
                for s in others:
                    out = out.common(s)
            out = out.removeSplitter()

            result_volume = _volume_of(out)
            tolerance = 1e-6
            verification: dict[str, Any] = {
                "operation": op_norm,
                "checked": bool(verify_growth),
                "result_volume": result_volume,
                "operand_volumes": operand_volumes,
                "max_operand_volume": max_operand_volume,
                "min_operand_volume": min_operand_volume,
                "sum_operand_volume": sum_operand_volume,
                "passed": None,
                "violations": [],
            }

            if verify_growth and result_volume is not None:
                violations: list[str] = []
                if op_norm in ("union", "fuse"):
                    if (
                        max_operand_volume is not None
                        and result_volume < max_operand_volume - tolerance
                    ):
                        violations.append(
                            f"union result volume {result_volume:.6f} is smaller than largest "
                            f"operand volume {max_operand_volume:.6f}; an operand was dropped or the "
                            f"result is hollow"
                        )
                    if (
                        sum_operand_volume is not None
                        and result_volume > sum_operand_volume + tolerance
                    ):
                        violations.append(
                            f"union result volume {result_volume:.6f} exceeds the sum of operand "
                            f"volumes {sum_operand_volume:.6f}; geometry was added unexpectedly"
                        )
                elif op_norm in ("intersect", "common"):
                    if (
                        min_operand_volume is not None
                        and result_volume > min_operand_volume + tolerance
                    ):
                        violations.append(
                            f"intersection result volume {result_volume:.6f} exceeds the smallest "
                            f"operand volume {min_operand_volume:.6f}"
                        )
                else:
                    base_volume = (
                        operand_volumes[0]["volume"] if operand_volumes else None
                    )
                    if (
                        base_volume is not None
                        and result_volume > base_volume + tolerance
                    ):
                        violations.append(
                            f"cut result volume {result_volume:.6f} exceeds the base operand "
                            f"volume {base_volume:.6f}"
                        )
                if result_volume <= tolerance:
                    violations.append(
                        f"result volume {result_volume:.6f} collapsed to zero; the boolean produced "
                        f"empty or degenerate geometry"
                    )
                verification["violations"] = violations
                verification["passed"] = len(violations) == 0
                if violations:
                    return {
                        "verification_failed": True,
                        "operation": op_norm,
                        "verification": verification,
                        "error": "; ".join(violations),
                    }

            final_name = result_name or f"{op_norm.title()}_{refs[0].Name}"
            feature = doc.addObject("Part::Feature", final_name)
            feature.Shape = out

            deleted: list[str] = []
            if delete_operands:
                for r in refs:
                    try:
                        doc.removeObject(r.Name)
                        deleted.append(r.Name)
                    except Exception:
                        pass

            if recompute:
                doc.recompute()

            return {
                "operation": op_norm,
                "result": feature.Name,
                "operand_count": len(refs),
                "deleted_operands": deleted,
                "verification": verification,
            }
        except Exception as e:
            return f"boolean_op error: {e}"

    rpc_request_queue.put(task)
    res = rpc_response_queue.get(timeout=self.TIMEOUT)
    if isinstance(res, dict):
        if res.get("verification_failed"):
            return {"success": False, **res}
        return {"success": True, **res}
    return {"success": False, "error": str(res)}
