# ***************************************************************************
# *   Copyright (c) 2019 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "material cards utilities"
__author__ = "Bernd Hahnebach"
__url__ = "http://www.freecadweb.org"

import os
import sys
from os.path import join

import FreeCAD


if sys.version_info.major >= 3:
    unicode = str


# ***** get resources for cards ******************************************************************
def get_material_resources(category='Solid'):

    resources = {}  # { resource_path: icon_path, ... }

    # TODO: move GUI preferences from FEM to a new side tab Material
    # https://forum.freecadweb.org/viewtopic.php?f=10&t=35515
    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
    use_built_in_materials = mat_prefs.GetBool("UseBuiltInMaterials", True)
    use_mat_from_config_dir = mat_prefs.GetBool("UseMaterialsFromConfigDir", True)
    use_mat_from_custom_dir = mat_prefs.GetBool("UseMaterialsFromCustomDir", True)

    if use_built_in_materials:
        if category == 'Fluid':
            builtin_mat_dir = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "FluidMaterial"
            )

        else:
            builtin_mat_dir = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "StandardMaterial"
            )
        resources[builtin_mat_dir] = ":/icons/freecad.svg"

    if use_mat_from_config_dir:
        config_mat_dir = join(
            FreeCAD.ConfigGet("UserAppData"), "Material"
        )
        resources[config_mat_dir] = ":/icons/preferences-general.svg"

    if use_mat_from_custom_dir:
        custom_mat_dir = mat_prefs.GetString("CustomMaterialsDir", "")
        if os.path.exists(custom_mat_dir):
            resources[custom_mat_dir] = ":/icons/user.svg"
        else:
            FreeCAD.Console.PrintError(
                'Custom material directory set by user: {} does not exist.\n'
                .format(custom_mat_dir)
            )

    return resources


def output_resources(resources):
    FreeCAD.Console.PrintMessage('Directories we gone look for material cards:\n')
    for path in resources.keys():
        FreeCAD.Console.PrintMessage('  {}\n'.format(path))


# ***** card handling ****************************************************************************
# used in material editor and FEM material task panels

def import_materials(category='Solid'):

    resources = get_material_resources(category)

    materials = {}
    cards = {}
    icons = {}
    for path in resources.keys():
        materials, cards, icons = add_cards_from_a_dir(
            materials,
            cards,
            icons,
            path,
            resources[path]
        )

    return (materials, cards, icons)


def add_cards_from_a_dir(materials, cards, icons, mat_dir, icon):
    # fill materials and icons
    import glob
    from importFCMat import read
    dir_path_list = glob.glob(mat_dir + '/*' + ".FCMat")
    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
    delete_duplicates = mat_prefs.GetBool("DeleteDuplicates", True)
    # duplicates are indicated on equality of mat dict
    # TODO if the unit is different two cards would be different too
    for a_path in dir_path_list:
        mat_dict = read(a_path)
        card_name = os.path.splitext(os.path.basename(a_path))[0]
        if delete_duplicates is False:
            materials[a_path] = mat_dict
            i = 1
            while card_name in cards.values():
                if i == 1:
                    card_name += ('_' + str(i))
                else:
                    card_name = card_name[:-1] + str(i)
                i += 1
                # print(card_name)
            cards[a_path] = card_name
            icons[a_path] = icon
        else:
            if mat_dict not in materials.values():
                materials[a_path] = mat_dict
                cards[a_path] = card_name
                icons[a_path] = icon

    return (materials, cards, icons)


def output_trio(trio):
    materials, cards, icons = trio
    FreeCAD.Console.PrintMessage('\n\n')
    for mat_card in materials:
        FreeCAD.Console.PrintMessage(
            '{} --> {} -->{}\n'
            .format(cards[mat_card], mat_card, icons[mat_card])
        )
    FreeCAD.Console.PrintMessage('\n\n')


def output_cards(cards):
    FreeCAD.Console.PrintMessage('\n\n')
    for mat_card in cards:
        FreeCAD.Console.PrintMessage('{} --> {}\n'.format(mat_card, cards[mat_card]))
    FreeCAD.Console.PrintMessage('\n\n')


def output_icons(icons):
    FreeCAD.Console.PrintMessage('\n\n')
    for mat_card in icons:
        FreeCAD.Console.PrintMessage('{} --> {}\n'.format(mat_card, icons[mat_card]))
    FreeCAD.Console.PrintMessage('\n\n')


def output_materials(materials):
    FreeCAD.Console.PrintMessage('\n\n')
    for mat_card in materials:
        FreeCAD.Console.PrintMessage('{}\n'.format(mat_card))
        output_material_param(materials[mat_card])
    FreeCAD.Console.PrintMessage('\n\n')


def output_material_param(mat_dict):
    # thus we check for None
    if not mat_dict:
        FreeCAD.Console.PrintMessage('  empty matdict\n')
    else:
        for p in mat_dict:
            FreeCAD.Console.PrintMessage('   {} --> {}\n'.format(p, mat_dict[p]))
    FreeCAD.Console.PrintMessage('\n')


# ***** material card template *******************************************************************
def get_material_template(withSpaces=False):
    # material properties
    # see the following resources in the FreeCAD wiki for more
    # information about the material specific properties:
    # https://www.freecadweb.org/wiki/Material_data_model
    # https://www.freecadweb.org/wiki/Material

    import yaml
    template_data = yaml.safe_load(
        open(join(FreeCAD.ConfigGet('AppHomePath'), 'Mod/Material/Templatematerial.yml'))
    )
    if withSpaces:
        # on attributes, add a space before a capital letter
        # will be used for better display in the ui
        import re
        for group in template_data:
            gg = list(group.keys())[0]  # group dict has only one key
            # iterating over a dict and changing it is not allowed
            # thus it is iterated over a list of the keys
            for proper in list(group[gg].keys()):
                new_proper = re.sub(r"(\w)([A-Z]+)", r"\1 \2", proper)
                group[gg][new_proper] = group[gg][proper]
                del group[gg][proper]
    return template_data
