"""Function plotting utilities for 3D visualization.

This module provides functions to plot mathematical functions as 3D surfaces
using Coin3D meshes.
"""

from pivy import coin
from .mesh import simple_quad_mesh
import numpy as np

def plot(foo, x, y):
    """Plot a 2D function as a 3D surface mesh.

    Evaluates a function f(x, y) over a grid and creates a 3D surface mesh
    visualization. The z-axis is automatically scaled to match the x-axis range.

    Args:
        foo: A callable function that takes two arguments (x, y) and returns z.
            The function should work with numpy arrays for vectorized evaluation.
        x: Tuple of three values (xmin, xmax, xnum) defining the x-axis range and
            number of sample points. xnum must be an integer.
        y: Tuple of three values (ymin, ymax, ynum) defining the y-axis range and
            number of sample points. ynum must be an integer.

    Returns:
        list: A list containing a scale node and a quad mesh separator node.
            Can be added directly to a Coin3D scene graph.

    Example:
        >>> def func(x, y):
        ...     return x**2 + y**2
        >>> surface = plot(func, (-5, 5, 20), (-5, 5, 20))
    """
    np_foo = np.vectorize(foo)
    x_space = np.linspace(*x)
    y_space = np.linspace(*y)
    xx, yy = np.meshgrid(x_space, y_space)
    xx = xx.flatten()
    yy = yy.flatten()
    zz = np_foo(xx, yy)
    num_x = x[-1]
    num_y = y[-1]
    points = np.array([xx, yy, zz]).T
    scale = coin.SoScale()
    scale.scaleFactor.setValue(1, 1, abs(x[1] - x[0]) / abs(max(zz) - min(zz)))
    return [scale, simple_quad_mesh(points, num_x, num_y)]
