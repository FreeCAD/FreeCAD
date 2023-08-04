import glob
import unittest
import FreeCAD
from lxml.etree import XMLParser
from xml.etree.ElementTree import ElementTree


def QT_TRANSLATE_NOOP(ctxt, txt):
    return txt


class TemplatesTest(unittest.TestCase):
    # tests if all editable fields have a common name
    def test_EditableFieldsNaming(self):
        print("cunt")

        allowedNames = [
            QT_TRANSLATE_NOOP("TechDraw Template", "A__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Approval date"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Approved1"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Approved2"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Author"),
            QT_TRANSLATE_NOOP("TechDraw Template", "B__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "C__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Client"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Code"),
            QT_TRANSLATE_NOOP("TechDraw Template", "D__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Date created"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description2"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description3"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description4"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description5"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Description6"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Document type"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Drawing number"),
            QT_TRANSLATE_NOOP("TechDraw Template", "E__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "F__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "G__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "H__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "I__"),
            QT_TRANSLATE_NOOP("TechDraw Template", "myEditableField"),
            QT_TRANSLATE_NOOP("TechDraw Template", "myFieldName"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Organization address"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Organization phone number"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Organization"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Owner"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Paper size"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Part material"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Part number"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Project name"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Rights"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Scale"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Sheet"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Subtitle"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Subtitle2"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Subtitle3"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Supervisor"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Title"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Title2"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Title3"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Tolerance"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Version"),
            QT_TRANSLATE_NOOP("TechDraw Template", "Weight"),
        ]

        # Stuff that hasn't yet been converted to common names
        # idk some of these cause I don't read Chinese or Russian
        ignore = [
            "APELLIDO1",
            "APELLIDO2",
            "APELLIDO3",
            "APELLIDO4",
            "CHECKED_NAME",
            "CRAFTS_NAME",
            "DATE-1",
            "DATE-2",
            "EMPRESA",
            "FECHA1",
            "FECHA2",
            "FECHA3",
            "FORMATO",
            "MATERIALMARK",
            "NOMBRE_ARCHIVO",
            "NUM_PLANO_CLIENTE",
            "NUM_PLANO",
            "NUM_REPRESENTADO",
            "NUM" "OF_SHEET",
            "PARTNAME",
            "STAGE-A",
            "STAGE-B",
            "STAGE-C",
            "STAGE-S",
            "TOTAL_SHEETS",
            "Дата_проверки",
            "Дата_разработки",
            "Дата_утверждения",
            "Информация",
            "Лист",
            "Листов",
            "Масса",
            "Масштаб",
            "Масштаб",
            "Масштаб",
            "Масштаб",
            "Масштаб",
            "Масштаб",
            "Материал1",
            "Материал2",
            "Материал3",
            "Название",
            "Номер",
            "Организация1",
            "Организация2",
            "Организация3",
            "Проверил",
            "Разработал",
            "Утвердил",
        ]

        path = FreeCAD.getResourceDir() + "Mod/TechDraw/Templates/**/*.svg"

        # Recurse through all files in the templates folder that have file ending .svg
        for filename in glob.glob(path, recursive=True):
            parser = XMLParser(recover=True)
            tree = ElementTree()
            tree.parse(filename, parser=parser)
            # this is so dumb, doesn't work without a url (empty string not enough)
            namespace = {
                "freecad": "http://www.freecad.org/wiki/index.php?title=Svg_Namespace"
            }
            elementsWithEditable = tree.findall("//*[@freecad:editable]", namespace)
            # Loop over all values of attributes freecad:editable
            for elementWithEditable in elementsWithEditable:
                # dumb², can't use freecad:, has to be url
                editableName = elementWithEditable.attrib[
                    "{" + namespace["freecad"] + "}editable"
                ]
                self.assertTrue(
                    editableName in allowedNames or editableName in ignore,
                    msg=f"{editableName} is not a valid fieldname in template {filename}",
                )
