# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2025 Bruno  <@blinhares on github>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************


import FreeCAD
import FreeCADGui

QT_TRANSLATE_NOOP = FreeCAD.Qt.QT_TRANSLATE_NOOP
translate = FreeCAD.Qt.translate

PARAMS = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/BIM")


class Bim_Extend:
    """The command definition for the Arch workbench's gui tool, BIM Extend.

    A tool for extend BIM objects.

    Extends BIM objects by selecting two edges. The first edge selected refers
    to the direction in which the object will be extended, and the second edge
    is the alignment segment.

    Find documentation on the end user usage of Arch Wall here:
    #missing documentation
    """

    def __init__(self):
        self.obj_01 = None
        self.obj_01_alin_points = None
        self.obj_02_alin_points = None
        self.intersection_point = None

    def GetResources(self):
        """Returns a dictionary with the visual aspects of the Bim Extend tool."""

        return {
            # TODO: Change This
            "Pixmap": "Arch_Wall",
            "MenuText": QT_TRANSLATE_NOOP("Arch_Wall", "Wall"),
            "Accel": "O, E",
            "ToolTip": QT_TRANSLATE_NOOP(
                "Arch_Wall",
                "Extend Bim Objects",
            ),
        }

    def IsActive(self):

        v = hasattr(FreeCADGui.getMainWindow().getActiveWindow(), "getSceneGraph")
        return v

    def Activated(self):
        """Executed when Arch Wall is called.

        Creates a wall from the object selected by the user. If no objects are
        selected, enters an interactive mode to create a wall using selected
        points to create a base.
        """
        if not self.is_able():
            return

        # TODO:Adicionar faces laterais irregulares

        doc = FreeCAD.ActiveDocument
        doc.openTransaction(self.__class__.__name__)

        prop = self.get_prop()
        if prop is None:
            FreeCAD.Console.PrintWarning(
                translate("Arch", "Could not determine which property to extend on the wall.\n")
            )
            return

        # TODO: apos aplicar o comando duas vezes o movimento passa a nao funcionar corretamente

        obj_prop_attribute = getattr(self.obj_01, prop)

        # move wall
        obj_01_start_to_inter = self.intersection_point - self.obj_01_alin_points[0]
        obj_01_end_to_inter = self.intersection_point - self.obj_01_alin_points[-1]
        move_vector = FreeCAD.Vector(0, 0, 0)

        # se os pontos de inicio e fim se deslocam em direcoes opostas
        if obj_01_start_to_inter.dot(obj_01_end_to_inter) < 0:

            if obj_01_start_to_inter.Length < obj_01_end_to_inter.Length:
                delta_length = -obj_01_start_to_inter.Length
                obj_prop_attribute += FreeCAD.Units.Quantity(delta_length, FreeCAD.Units.Length)
                setattr(self.obj_01, prop, obj_prop_attribute)
                self.update_walls_alin_axis()
                move_vector = self.intersection_point - self.obj_01_alin_points[0]
            else:
                delta_length = -obj_01_end_to_inter.Length
                obj_prop_attribute += FreeCAD.Units.Quantity(delta_length, FreeCAD.Units.Length)
                setattr(self.obj_01, prop, obj_prop_attribute)
                self.update_walls_alin_axis()
                move_vector = self.intersection_point - self.obj_01_alin_points[-1]

        else:

            # Pontos de inicio e fim se deslocam em mesma direcao
            delta_length = min(obj_01_end_to_inter.Length, obj_01_start_to_inter.Length)
            obj_prop_attribute += FreeCAD.Units.Quantity(delta_length, FreeCAD.Units.Length)
            setattr(self.obj_01, prop, obj_prop_attribute)

            self.update_walls_alin_axis()

            if obj_01_start_to_inter.Length > obj_01_end_to_inter.Length:
                move_vector = self.intersection_point - self.obj_01_alin_points[-1]
            else:
                move_vector = self.intersection_point - self.obj_01_alin_points[0]

        self.obj_01.Placement.move(move_vector)

        doc.commitTransaction()
        doc.recompute()

    def is_able(self) -> bool:
        """Verifica se é possível extender os objetos selecionadas."""
        sel = FreeCADGui.Selection.getCompleteSelection()

        # Verifica se há Pelo menos dois objetos selecionadas
        if len(sel) < 2:
            FreeCAD.Console.PrintWarning(translate("Arch", "Select two walls to extend"))
            return False

        # coletando objeto 01
        self.obj_01 = sel[0].Object

        self.update_walls_alin_axis()

        # verifica se há interseção entre as areastas selecionadas
        have_intersection, self.intersection_point = self.get_intersection_point(
            self.obj_01_alin_points, self.obj_02_alin_points, make_vertex=True
        )

        if not have_intersection:
            FreeCAD.Console.PrintWarning(
                translate("Arch", "The selected walls do not intersect, cannot extend.")
            )

        return have_intersection

    def update_walls_alin_axis(self):
        """Atualiza os pontos de alinhamento das paredes."""
        doc = FreeCAD.ActiveDocument
        doc.recompute()

        # get alin axis points
        self.obj_01_alin_points = self.get_alin_axis(0)
        self.obj_02_alin_points = self.get_alin_axis(1)

    def get_alin_axis(self, obj_num: int) -> tuple[FreeCAD.Vector, FreeCAD.Vector]:
        """Retorna os pontos do alinhamento do objeto.
        obj_num: índice do objeto selecionado.
        Ex: 0 Para o primeiro objeto, 1 para o segundo"""

        p1 = FreeCADGui.Selection.getSelectionEx()[obj_num].SubObjects[0].Vertexes[0].Point
        p2 = FreeCADGui.Selection.getSelectionEx()[obj_num].SubObjects[0].Vertexes[-1].Point

        return (p1, p2)

    def get_prop(self) -> str | None:
        """Retorna a propriedade do objeto01 que está alinhada com a aresta selecionada do
        mesmo objeto. Verifica as arestas dos objeto 01 e compara com as dimensões Height, Width e Length.
        """

        # Tolerância numérica
        tol = 1e-3

        prop_list = ["Height", "Width", "Length"]

        for prop in prop_list:
            if hasattr(self.obj_01, prop):
                i_size = getattr(self.obj_01, prop)
                size = i_size
                size += FreeCAD.Units.Quantity(0.1, FreeCAD.Units.Length)
                setattr(self.obj_01, prop, size)
                self.update_walls_alin_axis()
                obj1_edge_lenght = (self.obj_01_alin_points[-1] - self.obj_01_alin_points[0]).Length

                if abs(size.Value - obj1_edge_lenght) < tol:
                    # reset size
                    setattr(self.obj_01, prop, i_size)
                    self.update_walls_alin_axis()
                    return prop

        return None

    # def get_alin_axis(self, obj)-> tuple[FreeCAD.Vector, FreeCAD.Vector]:
    #     """Retorna os pontos do alinhamento da parede na direção do comprimento."""
    #     # Tolerância numérica
    #     tol = 1e-5

    #     doc = FreeCAD.ActiveDocument
    #     obj.Length += FreeCAD.Units.Quantity(0.1, FreeCAD.Units.Length)
    #     doc.recompute()
    #     obj_length = FreeCAD.Units.Quantity(obj.Length, FreeCAD.Units.Length).Value
    #     p1s = []
    #     p2s = []
    #     for i, e in enumerate(obj.Shape.Edges):
    #         edges_lenght = FreeCAD.Units.Quantity(e.Length, FreeCAD.Units.Length).Value
    #         if abs((obj_length-edges_lenght)) < tol:
    #             p1 = e.Vertexes[0].Point
    #             p2 = e.Vertexes[-1].Point
    #             p1s.append(p1)
    #             p2s.append(p2)

    #     obj.Length -= FreeCAD.Units.Quantity(0.1, FreeCAD.Units.Length)
    #     doc.recompute()

    #     p1 = self.media_pontos(p1s)
    #     p2 = self.media_pontos(p2s)

    #     # import Part
    #     # doc = FreeCAD.ActiveDocument
    #     # line = Part.makeLine(p1,p2)
    #     # obj = doc.addObject("Part::Feature", "Linha")
    #     # obj.Shape = line
    #     # doc.recompute()
    #     if len(p1s) == 0 or len(p2s) == 0:
    #         FreeCAD.Console.PrintWarning( translate("Arch", "The objects are not compatible. please select `wall` type objects."))
    #         raise Exception("Não foi possível determinar o eixo de alinhamento da parede.")

    #     return(p1,p2)

    def media_pontos(lesf, lista):
        soma = FreeCAD.Vector(0, 0, 0)
        for p in lista:
            soma = soma.add(p)
        return soma.multiply(1.0 / len(lista))

    # def get_nearest_face(self,obj,point):
    #     """Retorna as faces laterais da parede."""
    #     #TODO: refer essa tolerancia por as paredes podem estar em angos diferentesde 90 graus
    #     import Part
    #     vertex = Part.Vertex(point)

    #     sh = obj.Shape
    #     dist = sh.Faces[0].distToShape(vertex)[0]
    #     face = sh.Faces[0]

    #     for i,f in enumerate(sh.Faces,start=1):
    #         if f.distToShape(vertex)[0] < dist:
    #             dist = f.distToShape(vertex)[0]
    #             face = f
    #             FreeCAD.Console.PrintWarning(translate("Arch", f"Menor distancia {dist}\n"))

    #     return face

    def get_intersection_point(
        self, obj01_points, obj02_points, make_vertex=False
    ) -> tuple[bool, FreeCAD.Vector]:

        # Tolerância numérica
        tol = 1e-6

        objw01_start_point, obj01_end_point = obj01_points
        obj01_vect = obj01_end_point - objw01_start_point

        obj02_start_point, obj02_end_point = obj02_points
        obj02_vect = obj02_end_point - obj02_start_point

        r_cross_s = obj01_vect.cross(obj02_vect)
        r_cross_s_len2 = r_cross_s.Length**2

        # Verifica paralelismo com tolerância
        if r_cross_s_len2 < (tol**2):
            return (False, None)

        # parâmetros t e u (resolução da interseção paramétrica)
        v = obj02_start_point - objw01_start_point
        a = v.cross(obj02_vect).dot(r_cross_s) / r_cross_s_len2
        b = v.cross(obj01_vect).dot(r_cross_s) / r_cross_s_len2

        # Ponto de interseção na reta 1
        intersection_point = objw01_start_point + obj01_vect.multiply(a)

        # # Verifica se a interseção cai dentro dos segmentos (com pequena folga)
        # if (-tol <= a <= 1.0 + tol) and (-tol <= b <= 1.0 + tol):
        #     FreeCAD.Console.PrintWarning(f"Ponto de interseção (no segmento): {intersection_point}")

        # else:
        #     FreeCAD.Console.PrintWarning("Os segmentos não se cruzam dentro de seus comprimentos (t={}, u={}).\n".format(a, b))

        if make_vertex:
            self.create_vertex(intersection_point)

        return (True, intersection_point)

    def create_vertex(self, point: FreeCAD.Vector):
        import Part

        doc = FreeCAD.ActiveDocument
        if doc is not None:
            v = Part.Vertex(point)
            obj = doc.addObject("Part::Feature", "Intersecao")
            obj.Shape = v
            doc.recompute()

    def make_line(
        self, name: str, point1: FreeCAD.Vector, point2: FreeCAD.Vector, color=(1.0, 0.0, 0.0)
    ):
        import Part

        doc = FreeCAD.ActiveDocument
        if doc is not None and point1 != point2:
            line = Part.makeLine(point1, point2)
            obj = doc.addObject("Part::Feature", name)
            obj.Shape = line
            # Define a cor vermelha (R, G, B) no ViewObject
            obj.ViewObject.LineColor = color  #
            doc.recompute()


# This file not work for some reason
# FreeCADGui.addCommand("BIM_Extend", Bim_Extend())
