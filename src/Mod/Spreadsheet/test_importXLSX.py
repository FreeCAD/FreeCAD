import unittest
from unittest.mock import patch, MagicMock

from importXLSX import FormulaTranslator, getText, handleStrings, open


class TestFormulaTranslator(unittest.TestCase):
    def test_translate_expressions(self):
        # With
        formulas_and_expressions = [
            ("1=2", "1==2"),
            ("a<b", "a<b"),
            ("a>b", "a>b"),
            ("1<>2", "1!=2"),
            ("a>=b", "a>=b"),
            ("a<=b", "a<=b"),
            ("a!b", "a.b"),
            ("a+b", "a+b"),
            ("a-b", "a-b"),
            ("a*b", "a*b"),
            ("a/b", "a/b"),
            ("a^b", "a^b"),
            ("c*(a+b)", "c*(a+b)"),
            ("IF(a=b, c, d)", "(a==b? c: d)"),
            ("ABS(a)", "abs(a)"),
            ("ACOS(a)", "pi/180deg*acos(a)"),
            ("ASIN(a)", "pi/180deg*asin(a)"),
            ("ATAN(a)", "pi/180deg*atan(a)"),
            ("ATAN2(a)", "pi/180deg*atan2(a)"),
            ("COS(a)", "cos(1rad*(a))"),
            ("COSH(a)", "cosh(1rad*(a))"),
            ("EXP(a)", "exp(a)"),
            ("LOG(n)", "log(n)"),
            ("LOG10(n)", "log10(n)"),
            ("MOD(n,d)", "mod(n,d)"),
            ("POWER(n,p)", "pow(n,p)"),
            ("SIN(a)", "sin(1rad*(a))"),
            ("SINH(a)", "sinh(1rad*(a))"),
            ("SQRT(a)", "sqrt(a)"),
            ("TAN(a)", "tan(1rad*(a))"),
            ("TANH(a)", "tanh(1rad*(a))"),
            ("AVERAGE(a,b,c)", "average(a,b,c)"),
            ("COUNT(a,b,c)", "count(a,b,c)"),
            ("MAX(a,b,c)", "max(a,b,c)"),
            ("MIN(a,b,c)", "min(a,b,c)"),
            ("STDEVA(a,b,c)", "stddev(a,b,c)"),
            ("SUM(a,b,c)", "sum(a,b,c)"),
            ("PI", "pi"),
            ("_xlfn.CEILING.MATH(a)", "ceil(a)"),
            ("_xlfn.FLOOR.MATH(a)", "floor(a)"),
        ]

        # When
        result = []
        for formula, _ in formulas_and_expressions:
            translator = FormulaTranslator()
            result.append(translator.translateForm(formula))

        # Then
        expected = [f"={expression}" for _, expression in formulas_and_expressions]
        self.assertListEqual(expected, result)

    def test_translate_multi_character_branching_operators(self):
        # With
        formulas_and_expressions = [
            ("1<>2", "1!=2"),
            ("a>=b", "a>=b"),
            ("a<=b", "a<=b"),
        ]

        # When
        result = []
        for formula, _ in formulas_and_expressions:
            translator = FormulaTranslator()
            result.append(translator.translateForm(formula))

        # Then
        expected = [f"={expression}" for _, expression in formulas_and_expressions]
        self.assertListEqual(expected, result)

    def test_translate_nested_expression(self):
        # With
        formulas_and_expressions = [
            (
                "IF(a<b+c,0.1+SIN(0.5),0.3+MAX(COS(0.2),SIN(0.1)))",
                "(a<b+c?0.1+sin(1rad*(0.5)):0.3+max(cos(1rad*(0.2)),sin(1rad*(0.1))))",
            ),
        ]

        # When
        result = []
        for formula, _ in formulas_and_expressions:
            translator = FormulaTranslator()
            result.append(translator.translateForm(formula))

        # Then
        expected = [f"={expression}" for _, expression in formulas_and_expressions]
        self.assertListEqual(expected, result)
