# SPDX-License-Identifier: LGPL-2.1-or-later
"""Small reusable numerical kernels, independent of FreeCAD and the GUI."""


class DenseLU:
    """Partial-pivoted LU factorization reusable for multiple right hand sides."""

    def __init__(self, matrix):
        self.lu = [list(map(float, row)) for row in matrix]
        size = len(self.lu)
        if not size or any(len(row) != size for row in self.lu):
            raise ValueError("A square non-empty matrix is required")
        self.pivots = list(range(size))
        scale = max(abs(value) for row in self.lu for value in row)
        for column in range(size):
            pivot = max(range(column, size), key=lambda row: abs(self.lu[row][column]))
            if abs(self.lu[pivot][column]) <= max(scale * 1.0e-14, 1.0e-300):
                raise ValueError("The linear system is singular")
            self.lu[column], self.lu[pivot] = self.lu[pivot], self.lu[column]
            self.pivots[column], self.pivots[pivot] = self.pivots[pivot], self.pivots[column]
            for row in range(column + 1, size):
                factor = self.lu[row][column] / self.lu[column][column]
                self.lu[row][column] = factor
                for item in range(column + 1, size):
                    self.lu[row][item] -= factor * self.lu[column][item]

    def solve(self, values):
        size = len(self.lu)
        if len(values) != size:
            raise ValueError("The right hand side has the wrong size")
        result = [float(values[index]) for index in self.pivots]
        for row in range(size):
            result[row] -= sum(self.lu[row][col] * result[col] for col in range(row))
        for row in reversed(range(size)):
            result[row] = (result[row] - sum(
                self.lu[row][col] * result[col] for col in range(row + 1, size)
            )) / self.lu[row][row]
        return result

    def solve_points(self, points):
        return list(zip(*(self.solve([p[axis] for p in points]) for axis in range(3))))
