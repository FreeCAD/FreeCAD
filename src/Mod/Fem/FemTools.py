
""" This is an adapter, before FemTools is fully moved to CaeSolver
InspectionGui reference to this file
"""
import CaeSolver

def FemTools():
    return CaeSolver.makeCaeSolver('Calculix')