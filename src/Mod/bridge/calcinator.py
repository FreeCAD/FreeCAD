import cmath
import math
from typing import Any

import numpy as np
import scipy
import sympy as sp
from scipy import constants as _scipy_constants
from scipy import integrate as _scipy_integrate
from scipy import linalg as _scipy_linalg
from scipy import optimize as _scipy_optimize
from scipy import special as _scipy_special
from scipy import stats as _scipy_stats
from sympy.parsing.sympy_parser import (
    convert_xor,
    implicit_multiplication_application,
    parse_expr,
    standard_transformations,
)

_PARSE_TRANSFORMS = standard_transformations + (
    implicit_multiplication_application,
    convert_xor,
)

_SAFE_BUILTINS = {
    "abs": abs,
    "all": all,
    "any": any,
    "bool": bool,
    "complex": complex,
    "dict": dict,
    "divmod": divmod,
    "enumerate": enumerate,
    "filter": filter,
    "float": float,
    "frozenset": frozenset,
    "int": int,
    "len": len,
    "list": list,
    "map": map,
    "max": max,
    "min": min,
    "pow": pow,
    "range": range,
    "reversed": reversed,
    "round": round,
    "set": set,
    "slice": slice,
    "sorted": sorted,
    "str": str,
    "sum": sum,
    "tuple": tuple,
    "zip": zip,
    "True": True,
    "False": False,
    "None": None,
}


class CalcinatorError(Exception):
    pass


def _build_namespace() -> dict[str, Any]:
    namespace: dict[str, Any] = {}

    math_names = [
        "sin",
        "cos",
        "tan",
        "asin",
        "acos",
        "atan",
        "atan2",
        "sinh",
        "cosh",
        "tanh",
        "asinh",
        "acosh",
        "atanh",
        "exp",
        "expm1",
        "log",
        "log2",
        "log10",
        "log1p",
        "sqrt",
        "cbrt",
        "pow",
        "hypot",
        "ceil",
        "floor",
        "trunc",
        "fabs",
        "factorial",
        "gamma",
        "lgamma",
        "erf",
        "erfc",
        "degrees",
        "radians",
        "isfinite",
        "isinf",
        "isnan",
        "isqrt",
        "gcd",
        "lcm",
        "comb",
        "perm",
        "copysign",
        "fmod",
        "remainder",
        "modf",
        "frexp",
        "ldexp",
        "dist",
        "prod",
    ]
    for name in math_names:
        if hasattr(math, name):
            namespace[name] = getattr(math, name)

    namespace["pi"] = math.pi
    namespace["e"] = math.e
    namespace["tau"] = math.tau
    namespace["inf"] = math.inf
    namespace["nan"] = math.nan
    namespace["cot"] = lambda x: 1.0 / math.tan(x)
    namespace["sec"] = lambda x: 1.0 / math.cos(x)
    namespace["csc"] = lambda x: 1.0 / math.sin(x)
    namespace["sign"] = lambda x: (x > 0) - (x < 0)

    namespace["math"] = math
    namespace["cmath"] = cmath
    namespace["np"] = np
    namespace["numpy"] = np

    np_names = [
        "array",
        "asarray",
        "arange",
        "linspace",
        "logspace",
        "zeros",
        "ones",
        "eye",
        "identity",
        "full",
        "diag",
        "reshape",
        "ravel",
        "concatenate",
        "stack",
        "vstack",
        "hstack",
        "transpose",
        "dot",
        "inner",
        "outer",
        "cross",
        "matmul",
        "where",
        "clip",
        "unique",
        "sort",
        "argsort",
        "argmax",
        "argmin",
        "cumsum",
        "cumprod",
        "diff",
        "gradient",
        "mean",
        "median",
        "average",
        "std",
        "var",
        "percentile",
        "quantile",
        "min",
        "max",
        "sum",
        "prod",
        "abs",
        "round",
        "floor",
        "ceil",
        "exp",
        "log",
        "log2",
        "log10",
        "sqrt",
        "power",
        "sin",
        "cos",
        "tan",
        "arcsin",
        "arccos",
        "arctan",
        "arctan2",
        "deg2rad",
        "rad2deg",
        "real",
        "imag",
        "conj",
        "angle",
        "isclose",
        "allclose",
        "nan_to_num",
        "histogram",
        "bincount",
        "corrcoef",
        "cov",
        "interp",
        "polyval",
        "polyfit",
        "roots",
        "convolve",
        "trapezoid",
        "kron",
    ]
    for name in np_names:
        if hasattr(np, name):
            namespace["np_" + name] = getattr(np, name)

    namespace["stats"] = _scipy_stats
    namespace["optimize"] = _scipy_optimize
    namespace["integrate_scipy"] = _scipy_integrate
    namespace["linalg"] = _scipy_linalg
    namespace["special"] = _scipy_special
    namespace["constants"] = _scipy_constants

    namespace["sp"] = sp
    namespace["sympy"] = sp

    return namespace


_NAMESPACE = _build_namespace()


def _coerce_variables(variables: dict[str, Any] | None) -> dict[str, Any]:
    resolved: dict[str, Any] = {}
    if not variables:
        return resolved
    for key, value in variables.items():
        if isinstance(value, list):
            resolved[key] = np.array(value)
        else:
            resolved[key] = value
    return resolved


def _to_jsonable(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, float, str)):
        return value
    if isinstance(value, complex):
        return {"real": value.real, "imag": value.imag}
    if isinstance(value, np.bool_):
        return bool(value)
    if isinstance(value, np.integer):
        return int(value)
    if isinstance(value, np.floating):
        return float(value)
    if isinstance(value, np.complexfloating):
        c = complex(value)
        return {"real": c.real, "imag": c.imag}
    if isinstance(value, np.ndarray):
        return [_to_jsonable(item) for item in value.tolist()]
    if isinstance(value, (list, tuple)):
        return [_to_jsonable(item) for item in value]
    if isinstance(value, dict):
        return {str(k): _to_jsonable(v) for k, v in value.items()}
    if isinstance(value, range):
        return list(value)
    if isinstance(value, sp.Matrix):
        return [
            [_to_jsonable(value[r, c]) for c in range(value.cols)]
            for r in range(value.rows)
        ]
    if isinstance(value, sp.Basic):
        return _sympy_to_jsonable(value)
    return str(value)


def _sympy_to_jsonable(expr: sp.Basic) -> Any:
    if isinstance(expr, sp.logic.boolalg.BooleanAtom):
        return bool(expr)
    if not expr.free_symbols:
        try:
            numeric = complex(expr)
            if abs(numeric.imag) < 1e-12:
                real = numeric.real
                if real == int(real) and abs(real) < 1e15:
                    return int(real)
                return real
            return {"real": numeric.real, "imag": numeric.imag}
        except (TypeError, ValueError):
            return str(expr)
    return str(expr)


def _parse(expression: str) -> sp.Expr:
    if not isinstance(expression, str) or not expression.strip():
        raise CalcinatorError("Expression must be a non-empty string")
    try:
        return parse_expr(expression, transformations=_PARSE_TRANSFORMS, evaluate=True)
    except (sp.SympifyError, SyntaxError, TypeError) as exc:
        raise CalcinatorError(f"Could not parse expression: {exc}") from exc


def _resolve_symbol(name: str) -> sp.Symbol:
    if not isinstance(name, str) or not name.strip():
        raise CalcinatorError("Variable name must be a non-empty string")
    return sp.Symbol(name.strip())


def _matrix_is_numeric(data: list[list[Any]]) -> bool:
    for row in data:
        for entry in row:
            if not isinstance(entry, (int, float)) or isinstance(entry, bool):
                return False
    return True


def _as_matrix(data: Any, label: str) -> np.ndarray | sp.Matrix:
    if (
        not isinstance(data, list)
        or not data
        or not all(isinstance(row, list) for row in data)
    ):
        raise CalcinatorError(f"{label} must be a non-empty list of rows")
    width = len(data[0])
    if width == 0 or any(len(row) != width for row in data):
        raise CalcinatorError(f"{label} rows must all share the same non-zero length")
    if _matrix_is_numeric(data):
        return np.array(data, dtype=float)
    return sp.Matrix([[_parse(str(entry)) for entry in row] for row in data])


def evaluate(
    expression: str, variables: dict[str, Any] | None = None
) -> dict[str, Any]:
    """Numerically evaluate an arithmetic or scientific expression."""
    try:
        if not isinstance(expression, str) or not expression.strip():
            raise CalcinatorError("Expression must be a non-empty string")
        local_namespace = dict(_NAMESPACE)
        local_namespace.update(_coerce_variables(variables))
        compiled = compile(expression.strip(), "<calcinator>", "eval")
        for name in compiled.co_names:
            if name.startswith("__"):
                raise CalcinatorError(f"Disallowed name in expression: {name}")
        result = eval(compiled, {"__builtins__": _SAFE_BUILTINS}, local_namespace)
        return {"result": _to_jsonable(result), "result_type": type(result).__name__}
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def solve_equation(equations: Any, unknowns: Any = None) -> dict[str, Any]:
    """Solve an equation or system of equations symbolically."""
    try:
        if isinstance(equations, str):
            equation_list = [equations]
        elif isinstance(equations, list) and equations:
            equation_list = [str(item) for item in equations]
        else:
            raise CalcinatorError(
                "equations must be a string or a non-empty list of strings"
            )

        parsed: list[sp.Expr] = []
        for raw in equation_list:
            if "=" in raw and "==" not in raw and "<=" not in raw and ">=" not in raw:
                left, right = raw.split("=", 1)
                parsed.append(sp.Eq(_parse(left), _parse(right)))
            else:
                parsed.append(_parse(raw))

        if unknowns is None:
            symbol_set: set[sp.Symbol] = set()
            for expr in parsed:
                symbol_set |= expr.free_symbols
            symbols = sorted(symbol_set, key=lambda s: s.name)
            if not symbols:
                raise CalcinatorError("No free symbols found to solve for")
        elif isinstance(unknowns, str):
            symbols = [_resolve_symbol(unknowns)]
        elif isinstance(unknowns, list) and unknowns:
            symbols = [_resolve_symbol(name) for name in unknowns]
        else:
            raise CalcinatorError("unknowns must be a string or list of strings")

        solutions = sp.solve(parsed, symbols, dict=True)
        if not isinstance(solutions, list):
            solutions = [solutions]
        formatted = []
        for solution in solutions:
            if isinstance(solution, dict):
                formatted.append(
                    {str(key): _to_jsonable(val) for key, val in solution.items()}
                )
            else:
                formatted.append(_to_jsonable(solution))
        return {
            "solutions": formatted,
            "count": len(formatted),
            "solved_for": [s.name for s in symbols],
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def differentiate(
    expression: str, variable: str = "x", order: int = 1
) -> dict[str, Any]:
    """Compute the symbolic derivative of an expression."""
    try:
        if not isinstance(order, int) or order < 1:
            raise CalcinatorError("order must be a positive integer")
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        derivative = sp.diff(expr, symbol, order)
        simplified = sp.simplify(derivative)
        return {
            "result": str(simplified),
            "result_latex": sp.latex(simplified),
            "variable": symbol.name,
            "order": order,
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def integrate_expression(
    expression: str,
    variable: str = "x",
    lower: Any = None,
    upper: Any = None,
) -> dict[str, Any]:
    """Compute a definite or indefinite integral symbolically."""
    try:
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        if lower is None and upper is None:
            antiderivative = sp.integrate(expr, symbol)
            return {
                "result": str(antiderivative),
                "result_latex": sp.latex(antiderivative),
                "definite": False,
                "variable": symbol.name,
            }
        if lower is None or upper is None:
            raise CalcinatorError(
                "Definite integral requires both lower and upper bounds"
            )
        low = _parse(str(lower))
        high = _parse(str(upper))
        definite = sp.integrate(expr, (symbol, low, high))
        payload: dict[str, Any] = {
            "result": str(definite),
            "result_latex": sp.latex(definite),
            "definite": True,
            "variable": symbol.name,
            "lower": str(low),
            "upper": str(high),
        }
        if not definite.free_symbols:
            try:
                payload["numeric"] = float(definite.evalf())
            except (TypeError, ValueError):
                payload["numeric"] = None
        return payload
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def limit_expression(
    expression: str,
    variable: str = "x",
    point: Any = "0",
    direction: str = "+",
) -> dict[str, Any]:
    """Compute the limit of an expression as a variable approaches a point."""
    try:
        if direction not in ("+", "-", "+-"):
            raise CalcinatorError("direction must be '+', '-', or '+-'")
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        target = (
            sp.oo
            if str(point) in ("oo", "inf", "+oo")
            else (-sp.oo if str(point) in ("-oo", "-inf") else _parse(str(point)))
        )
        value = sp.limit(expr, symbol, target, dir=direction)
        return {
            "result": str(value),
            "result_latex": sp.latex(value),
            "variable": symbol.name,
            "point": str(point),
            "direction": direction,
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def series_expansion(
    expression: str,
    variable: str = "x",
    point: Any = 0,
    order: int = 6,
) -> dict[str, Any]:
    """Compute the Taylor or Laurent series expansion of an expression."""
    try:
        if not isinstance(order, int) or order < 1:
            raise CalcinatorError("order must be a positive integer")
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        center = _parse(str(point))
        expansion = expr.series(symbol, center, order)
        return {
            "result": str(expansion),
            "result_latex": sp.latex(expansion),
            "without_order_term": str(expansion.removeO()),
            "variable": symbol.name,
            "point": str(point),
            "order": order,
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


_TRANSFORMS = {
    "simplify": sp.simplify,
    "expand": sp.expand,
    "factor": sp.factor,
    "cancel": sp.cancel,
    "apart": sp.apart,
    "together": sp.together,
    "trigsimp": sp.trigsimp,
    "expand_trig": sp.expand_trig,
    "radsimp": sp.radsimp,
    "ratsimp": sp.ratsimp,
    "powsimp": sp.powsimp,
    "logcombine": sp.logcombine,
    "expand_log": lambda e: sp.expand_log(e, force=True),
    "nsimplify": sp.nsimplify,
}


def transform_expression(
    expression: str, operation: str = "simplify"
) -> dict[str, Any]:
    """Apply a symbolic transformation such as simplify, expand, or factor."""
    try:
        if operation not in _TRANSFORMS:
            raise CalcinatorError(
                f"Unknown operation '{operation}'. "
                f"Available: {', '.join(sorted(_TRANSFORMS))}"
            )
        expr = _parse(expression)
        result = _TRANSFORMS[operation](expr)
        return {
            "result": str(result),
            "result_latex": sp.latex(result),
            "operation": operation,
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def summation(
    expression: str,
    variable: str = "x",
    lower: Any = 0,
    upper: Any = 10,
) -> dict[str, Any]:
    """Evaluate the summation of an expression over an index range."""
    try:
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        low = _parse(str(lower))
        high = _parse(str(upper))
        total = sp.summation(expr, (symbol, low, high))
        payload: dict[str, Any] = {
            "result": str(total),
            "result_latex": sp.latex(total),
            "variable": symbol.name,
            "lower": str(low),
            "upper": str(high),
        }
        if not total.free_symbols:
            try:
                payload["numeric"] = float(total.evalf())
            except (TypeError, ValueError):
                payload["numeric"] = None
        return payload
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def product_sequence(
    expression: str,
    variable: str = "x",
    lower: Any = 1,
    upper: Any = 10,
) -> dict[str, Any]:
    """Evaluate the product of an expression over an index range."""
    try:
        expr = _parse(expression)
        symbol = _resolve_symbol(variable)
        low = _parse(str(lower))
        high = _parse(str(upper))
        total = sp.product(expr, (symbol, low, high))
        payload: dict[str, Any] = {
            "result": str(total),
            "result_latex": sp.latex(total),
            "variable": symbol.name,
            "lower": str(low),
            "upper": str(high),
        }
        if not total.free_symbols:
            try:
                payload["numeric"] = float(total.evalf())
            except (TypeError, ValueError):
                payload["numeric"] = None
        return payload
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


_MATRIX_UNARY = (
    "determinant",
    "inverse",
    "transpose",
    "rank",
    "trace",
    "eigenvalues",
    "eigenvectors",
    "nullspace",
    "rref",
    "norm",
)
_MATRIX_BINARY = ("add", "subtract", "multiply", "solve")


def matrix_operation(
    operation: str,
    matrix: Any = None,
    matrix_b: Any = None,
    vector: Any = None,
) -> dict[str, Any]:
    """Perform linear algebra on matrices, including decompositions and solves."""
    try:
        if operation not in _MATRIX_UNARY and operation not in _MATRIX_BINARY:
            raise CalcinatorError(
                f"Unknown operation '{operation}'. Available: "
                f"{', '.join(sorted(set(_MATRIX_UNARY) | set(_MATRIX_BINARY)))}"
            )
        if matrix is None:
            raise CalcinatorError("matrix is required")
        a = _as_matrix(matrix, "matrix")
        numeric = isinstance(a, np.ndarray)

        if operation in _MATRIX_BINARY:
            if operation == "solve":
                if vector is None:
                    raise CalcinatorError(
                        "solve requires 'vector' as the right-hand side"
                    )
                if numeric:
                    b = np.array(vector, dtype=float)
                    solution = np.linalg.solve(a, b)
                else:
                    b = sp.Matrix(vector)
                    solution = a.LUsolve(b)
                return {"operation": operation, "result": _to_jsonable(solution)}
            if matrix_b is None:
                raise CalcinatorError(f"{operation} requires 'matrix_b'")
            b = _as_matrix(matrix_b, "matrix_b")
            if numeric and isinstance(b, np.ndarray):
                if operation == "add":
                    result = a + b
                elif operation == "subtract":
                    result = a - b
                else:
                    result = a @ b
            else:
                a_s = a if not numeric else sp.Matrix(a.tolist())
                b_s = b if isinstance(b, sp.Matrix) else sp.Matrix(b.tolist())
                if operation == "add":
                    result = a_s + b_s
                elif operation == "subtract":
                    result = a_s - b_s
                else:
                    result = a_s * b_s
            return {"operation": operation, "result": _to_jsonable(result)}

        if operation == "determinant":
            result = float(np.linalg.det(a)) if numeric else a.det()
        elif operation == "inverse":
            result = np.linalg.inv(a) if numeric else a.inv()
        elif operation == "transpose":
            result = a.T
        elif operation == "rank":
            result = int(np.linalg.matrix_rank(a)) if numeric else a.rank()
        elif operation == "trace":
            result = float(np.trace(a)) if numeric else a.trace()
        elif operation == "norm":
            result = float(np.linalg.norm(a)) if numeric else a.norm()
        elif operation == "eigenvalues":
            if numeric:
                result = np.linalg.eigvals(a)
            else:
                result = list(a.eigenvals().keys())
        elif operation == "eigenvectors":
            if numeric:
                values, vectors = np.linalg.eig(a)
                return {
                    "operation": operation,
                    "eigenvalues": _to_jsonable(values),
                    "eigenvectors": _to_jsonable(vectors.T),
                }
            triples = a.eigenvects()
            return {
                "operation": operation,
                "result": [
                    {
                        "eigenvalue": _to_jsonable(val),
                        "multiplicity": int(mult),
                        "eigenvectors": [_to_jsonable(vec) for vec in vecs],
                    }
                    for val, mult, vecs in triples
                ],
            }
        elif operation == "nullspace":
            basis = (sp.Matrix(a.tolist()) if numeric else a).nullspace()
            return {"operation": operation, "result": [_to_jsonable(v) for v in basis]}
        else:
            reduced, pivots = (sp.Matrix(a.tolist()) if numeric else a).rref()
            return {
                "operation": operation,
                "result": _to_jsonable(reduced),
                "pivots": list(pivots),
            }
        return {"operation": operation, "result": _to_jsonable(result)}
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except np.linalg.LinAlgError as exc:
        return {"error": f"LinAlgError: {exc}"}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def vector_operation(
    operation: str, vector_a: Any, vector_b: Any = None
) -> dict[str, Any]:
    """Perform vector algebra such as dot, cross, norm, and angle."""
    try:
        valid = (
            "dot",
            "cross",
            "add",
            "subtract",
            "scale",
            "norm",
            "normalize",
            "angle",
            "projection",
        )
        if operation not in valid:
            raise CalcinatorError(
                f"Unknown operation '{operation}'. Available: {', '.join(valid)}"
            )
        a = np.array(vector_a, dtype=float)
        if a.ndim != 1 or a.size == 0:
            raise CalcinatorError("vector_a must be a non-empty 1-D list")

        if operation == "norm":
            return {"operation": operation, "result": float(np.linalg.norm(a))}
        if operation == "normalize":
            magnitude = float(np.linalg.norm(a))
            if magnitude == 0.0:
                raise CalcinatorError("Cannot normalize the zero vector")
            return {"operation": operation, "result": _to_jsonable(a / magnitude)}
        if operation == "scale":
            if not isinstance(vector_b, (int, float)):
                raise CalcinatorError("scale requires vector_b to be a numeric scalar")
            return {"operation": operation, "result": _to_jsonable(a * float(vector_b))}

        b = np.array(vector_b, dtype=float)
        if b.ndim != 1 or b.size == 0:
            raise CalcinatorError("vector_b must be a non-empty 1-D list")

        if operation == "dot":
            return {"operation": operation, "result": float(np.dot(a, b))}
        if operation == "cross":
            if a.size != 3 or b.size != 3:
                raise CalcinatorError("cross product requires two 3-D vectors")
            return {"operation": operation, "result": _to_jsonable(np.cross(a, b))}
        if a.size != b.size:
            raise CalcinatorError("vectors must share the same length")
        if operation == "add":
            return {"operation": operation, "result": _to_jsonable(a + b)}
        if operation == "subtract":
            return {"operation": operation, "result": _to_jsonable(a - b)}
        if operation == "projection":
            denominator = float(np.dot(b, b))
            if denominator == 0.0:
                raise CalcinatorError("Cannot project onto the zero vector")
            return {
                "operation": operation,
                "result": _to_jsonable(b * (float(np.dot(a, b)) / denominator)),
            }
        na = float(np.linalg.norm(a))
        nb = float(np.linalg.norm(b))
        if na == 0.0 or nb == 0.0:
            raise CalcinatorError("Cannot compute angle with a zero vector")
        cosine = max(-1.0, min(1.0, float(np.dot(a, b)) / (na * nb)))
        radians = math.acos(cosine)
        return {
            "operation": operation,
            "radians": radians,
            "degrees": math.degrees(radians),
        }
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def _require_data(data: Any, label: str = "data") -> np.ndarray:
    if not isinstance(data, list) or not data:
        raise CalcinatorError(f"{label} must be a non-empty list of numbers")
    arr = np.array(data, dtype=float)
    if arr.ndim != 1:
        raise CalcinatorError(f"{label} must be one-dimensional")
    return arr


def statistics(
    operation: str,
    data: Any = None,
    data_y: Any = None,
    confidence: float = 0.95,
    popmean: float = 0.0,
) -> dict[str, Any]:
    """Compute descriptive statistics, correlation, regression, and hypothesis tests."""
    try:
        if operation == "describe":
            arr = _require_data(data)
            described = _scipy_stats.describe(arr)
            quartiles = np.percentile(arr, [25, 50, 75])
            mode_result = _scipy_stats.mode(arr, keepdims=False)
            return {
                "operation": operation,
                "count": int(described.nobs),
                "mean": float(described.mean),
                "median": float(quartiles[1]),
                "mode": float(np.atleast_1d(mode_result.mode)[0]),
                "variance": float(described.variance),
                "std": float(np.std(arr, ddof=1)) if arr.size > 1 else 0.0,
                "min": float(described.minmax[0]),
                "max": float(described.minmax[1]),
                "range": float(described.minmax[1] - described.minmax[0]),
                "q1": float(quartiles[0]),
                "q3": float(quartiles[2]),
                "iqr": float(quartiles[2] - quartiles[0]),
                "skewness": float(described.skewness),
                "kurtosis": float(described.kurtosis),
                "sum": float(np.sum(arr)),
            }
        if operation in ("mean", "median", "std", "variance", "sum", "min", "max"):
            arr = _require_data(data)
            mapping = {
                "mean": float(np.mean(arr)),
                "median": float(np.median(arr)),
                "std": float(np.std(arr, ddof=1)) if arr.size > 1 else 0.0,
                "variance": float(np.var(arr, ddof=1)) if arr.size > 1 else 0.0,
                "sum": float(np.sum(arr)),
                "min": float(np.min(arr)),
                "max": float(np.max(arr)),
            }
            return {"operation": operation, "result": mapping[operation]}
        if operation == "correlation":
            x = _require_data(data, "data")
            y = _require_data(data_y, "data_y")
            if x.size != y.size:
                raise CalcinatorError("data and data_y must share the same length")
            pearson = _scipy_stats.pearsonr(x, y)
            spearman = _scipy_stats.spearmanr(x, y)
            return {
                "operation": operation,
                "pearson_r": float(pearson.statistic),
                "pearson_p_value": float(pearson.pvalue),
                "spearman_r": float(spearman.statistic),
                "spearman_p_value": float(spearman.pvalue),
            }
        if operation == "linear_regression":
            x = _require_data(data, "data")
            y = _require_data(data_y, "data_y")
            if x.size != y.size:
                raise CalcinatorError("data and data_y must share the same length")
            fit = _scipy_stats.linregress(x, y)
            return {
                "operation": operation,
                "slope": float(fit.slope),
                "intercept": float(fit.intercept),
                "r_value": float(fit.rvalue),
                "r_squared": float(fit.rvalue**2),
                "p_value": float(fit.pvalue),
                "std_error": float(fit.stderr),
            }
        if operation == "confidence_interval":
            arr = _require_data(data)
            if arr.size < 2:
                raise CalcinatorError("confidence_interval needs at least two values")
            if not 0.0 < confidence < 1.0:
                raise CalcinatorError("confidence must be between 0 and 1")
            mean = float(np.mean(arr))
            sem = float(_scipy_stats.sem(arr))
            margin = sem * float(
                _scipy_stats.t.ppf((1 + confidence) / 2.0, arr.size - 1)
            )
            return {
                "operation": operation,
                "mean": mean,
                "confidence": confidence,
                "lower": mean - margin,
                "upper": mean + margin,
                "margin_of_error": margin,
            }
        if operation == "t_test_1samp":
            arr = _require_data(data)
            test = _scipy_stats.ttest_1samp(arr, popmean)
            return {
                "operation": operation,
                "popmean": float(popmean),
                "statistic": float(test.statistic),
                "p_value": float(test.pvalue),
            }
        if operation == "t_test_ind":
            x = _require_data(data, "data")
            y = _require_data(data_y, "data_y")
            test = _scipy_stats.ttest_ind(x, y)
            return {
                "operation": operation,
                "statistic": float(test.statistic),
                "p_value": float(test.pvalue),
            }
        raise CalcinatorError(
            "Unknown operation. Available: describe, mean, median, std, variance, "
            "sum, min, max, correlation, linear_regression, confidence_interval, "
            "t_test_1samp, t_test_ind"
        )
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def distribution(
    name: str,
    operation: str,
    x: Any = None,
    params: list[float] | None = None,
    q: float | None = None,
) -> dict[str, Any]:
    """Evaluate pdf, cdf, ppf, or summary statistics for a SciPy distribution."""
    try:
        dist = getattr(_scipy_stats, name, None)
        if dist is None or not hasattr(dist, "rvs"):
            raise CalcinatorError(f"Unknown distribution '{name}'")
        args = [float(p) for p in (params or [])]
        if operation == "pdf":
            if x is None:
                raise CalcinatorError("pdf requires x")
            method = getattr(dist, "pdf", None) or dist.pmf
            return {"operation": operation, "result": _to_jsonable(method(x, *args))}
        if operation == "cdf":
            if x is None:
                raise CalcinatorError("cdf requires x")
            return {"operation": operation, "result": _to_jsonable(dist.cdf(x, *args))}
        if operation == "ppf":
            if q is None:
                raise CalcinatorError("ppf requires q in (0, 1)")
            return {"operation": operation, "result": _to_jsonable(dist.ppf(q, *args))}
        if operation == "stats":
            mean, var, skew, kurt = dist.stats(*args, moments="mvsk")
            return {
                "operation": operation,
                "mean": _to_jsonable(mean),
                "variance": _to_jsonable(var),
                "skewness": _to_jsonable(skew),
                "kurtosis": _to_jsonable(kurt),
            }
        raise CalcinatorError("operation must be one of: pdf, cdf, ppf, stats")
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def constant(name: str) -> dict[str, Any]:
    """Look up a physical or mathematical constant by name."""
    try:
        if not isinstance(name, str) or not name.strip():
            raise CalcinatorError("name must be a non-empty string")
        key = name.strip()
        named = {
            "pi": (math.pi, "dimensionless"),
            "e": (math.e, "dimensionless"),
            "tau": (math.tau, "dimensionless"),
            "golden": ((1 + 5**0.5) / 2, "dimensionless"),
        }
        if key in named:
            value, unit = named[key]
            return {"name": key, "value": value, "unit": unit}
        if hasattr(_scipy_constants, key):
            attr = getattr(_scipy_constants, key)
            if isinstance(attr, (int, float)):
                return {"name": key, "value": float(attr), "unit": "SI"}
        if key in _scipy_constants.physical_constants:
            value, unit, uncertainty = _scipy_constants.physical_constants[key]
            return {
                "name": key,
                "value": float(value),
                "unit": unit,
                "uncertainty": float(uncertainty),
            }
        matches = [
            label
            for label in _scipy_constants.physical_constants
            if key.lower() in label.lower()
        ]
        if matches:
            raise CalcinatorError(
                f"No exact match for '{key}'. Did you mean: {', '.join(matches[:8])}"
            )
        raise CalcinatorError(f"Unknown constant '{key}'")
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}


def capabilities() -> dict[str, Any]:
    """List the operations, callable symbols, and constants the engine exposes."""
    callables = sorted(
        name
        for name, value in _NAMESPACE.items()
        if callable(value) and not name.startswith("_")
    )
    symbol_constants = sorted(
        name
        for name, value in _NAMESPACE.items()
        if isinstance(value, (int, float)) and not name.startswith("_")
    )
    return {
        "engine": "calcinator",
        "version": "1.0.0",
        "libraries": {
            "numpy": np.__version__,
            "scipy": scipy.__version__,
            "sympy": sp.__version__,
        },
        "tools": {
            "evaluate": "Numeric evaluation with the scientific namespace",
            "solve_equation": "Solve single equations or systems symbolically",
            "differentiate": "Symbolic derivatives of any order",
            "integrate_expression": "Definite and indefinite integration",
            "limit_expression": "Limits including one-sided and at infinity",
            "series_expansion": "Taylor and Laurent series",
            "transform_expression": list(sorted(_TRANSFORMS)),
            "summation": "Symbolic and numeric summation",
            "product_sequence": "Symbolic and numeric products",
            "matrix_operation": sorted(set(_MATRIX_UNARY) | set(_MATRIX_BINARY)),
            "vector_operation": [
                "dot",
                "cross",
                "add",
                "subtract",
                "scale",
                "norm",
                "normalize",
                "angle",
                "projection",
            ],
            "statistics": [
                "describe",
                "mean",
                "median",
                "std",
                "variance",
                "sum",
                "min",
                "max",
                "correlation",
                "linear_regression",
                "confidence_interval",
                "t_test_1samp",
                "t_test_ind",
            ],
            "distribution": ["pdf", "cdf", "ppf", "stats"],
            "constant": "Look up physical and mathematical constants",
        },
        "evaluate_functions": callables,
        "evaluate_constants": symbol_constants,
        "evaluate_modules": [
            "math",
            "cmath",
            "np",
            "stats",
            "optimize",
            "linalg",
            "special",
            "constants",
            "sp",
        ],
    }


def describe(name: str) -> dict[str, Any]:
    """Return the signature and documentation for a symbol in the evaluation namespace."""
    try:
        if not isinstance(name, str) or not name.strip():
            raise CalcinatorError("name must be a non-empty string")
        key = name.strip()
        if key not in _NAMESPACE:
            matches = sorted(n for n in _NAMESPACE if key.lower() in n.lower())
            if matches:
                raise CalcinatorError(
                    f"'{key}' not found. Similar: {', '.join(matches[:10])}"
                )
            raise CalcinatorError(f"'{key}' is not available in the namespace")
        target = _NAMESPACE[key]
        payload: dict[str, Any] = {"name": key, "kind": type(target).__name__}
        if isinstance(target, (int, float)):
            payload["value"] = target
            return payload
        if callable(target):
            import inspect

            try:
                payload["signature"] = f"{key}{inspect.signature(target)}"
            except (TypeError, ValueError):
                payload["signature"] = key
            doc = inspect.getdoc(target)
            if doc:
                payload["doc"] = doc.split("\n\n")[0].strip()
        return payload
    except CalcinatorError as exc:
        return {"error": str(exc)}
    except Exception as exc:
        return {"error": f"{type(exc).__name__}: {exc}"}
