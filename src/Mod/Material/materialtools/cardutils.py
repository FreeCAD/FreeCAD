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
__url__ = "http://www.freecad.org"

import os
from os.path import join
from pathlib import Path

import FreeCAD
import Material


unicode = str


# TODO:
# implement method check_material_keys from FEM material task panel for material editor
# may be move out of the FEM material task panel to here
# make the method more generic to be compatible with all known params
# the material template knows the units


# ***** card handling data models ****************************************************************
'''
data model:
materials = { card_path: mat_dict, ... }
cards = { card_path: card_name, ... }
icons = { card_path: icon_path, ... }

- duplicates are allowed
- the whole card_path is saved into the combo box widget, this makes card unique
- sorting happens on adding to the combo box widgets

a data model which uses a class and attributes as well as methods to access the attributes
would makes sense, like some material library class
this has been done already by eivind see
https://forum.freecad.org/viewtopic.php?f=38&t=16714
'''

def get_material_preferred_directory(category=None):
    """
        Return the preferred material directory. In priority order they are:
        1. user specified
        2. user modules folder
        3. system folder
    """
    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
    use_built_in_materials = mat_prefs.GetBool("UseBuiltInMaterials", True)
    use_mat_from_config_dir = mat_prefs.GetBool("UseMaterialsFromConfigDir", True)
    use_mat_from_custom_dir = mat_prefs.GetBool("UseMaterialsFromCustomDir", True)

    preferred = None

    if use_built_in_materials:
        if category == 'Fluid':
            preferred = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "Resources", "Materials", "FluidMaterial"
            )

        elif category == 'Solid':
            preferred = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "Resources", "Materials", "StandardMaterial"
            )

        else:
            preferred = join(
                FreeCAD.getResourceDir(), "Mod", "Material"
            )

    if use_mat_from_config_dir:
        user = join(
            FreeCAD.ConfigGet("UserAppData"), "Material"
        )
        if os.path.isdir(user):
            preferred = user

    if use_mat_from_custom_dir:
        custom = mat_prefs.GetString("CustomMaterialsDir", "")
        if len(custom.strip()) > 0:
            preferred = custom

    return preferred

def get_material_preferred_save_directory():
    """
        Return the preferred directory for saving materials. In priority order they are:
        1. user specified
        2. user modules folder
    """
    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
    use_mat_from_config_dir = mat_prefs.GetBool("UseMaterialsFromConfigDir", True)
    use_mat_from_custom_dir = mat_prefs.GetBool("UseMaterialsFromCustomDir", True)

    if use_mat_from_custom_dir:
        custom = mat_prefs.GetString("CustomMaterialsDir", "")
        if len(custom.strip()) > 0:
            # Create the directory if it doesn't exist
            try:
                if not os.path.isdir(custom):
                    os.makedirs(custom)
                return custom
            except Exception as ex:
                print(ex)
                pass

    if use_mat_from_config_dir:
        user = join(
            FreeCAD.ConfigGet("UserAppData"), "Material"
        )
        try:
            if not os.path.isdir(user):
                os.makedirs(user)
            return user
        except Exception as ex:
            print(ex)
            pass


    return ""


# ***** get resources for cards ******************************************************************
def get_material_resources(category='Solid'):

    resources = {}  # { resource_path: icon_path, ... }

    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
    use_built_in_materials = mat_prefs.GetBool("UseBuiltInMaterials", True)
    use_mat_from_modules = mat_prefs.GetBool("UseMaterialsFromWorkbenches", True)
    use_mat_from_config_dir = mat_prefs.GetBool("UseMaterialsFromConfigDir", True)
    use_mat_from_custom_dir = mat_prefs.GetBool("UseMaterialsFromCustomDir", True)

    if use_built_in_materials:
        if category == 'Fluid':
            builtin_mat_dir = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "Resources", "Materials", "FluidMaterial"
            )

        else:
            builtin_mat_dir = join(
                FreeCAD.getResourceDir(), "Mod", "Material", "Resources", "Materials", "StandardMaterial"
            )
        resources[builtin_mat_dir] = ":/icons/freecad.svg"

    if use_mat_from_modules:
        module_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules")
        module_groups = module_prefs.GetGroups()
        for group in module_groups:
            module = module_prefs.GetGroup(group)
            module_mat_dir = module.GetString("ModuleDir", "")
            module_icon_dir = module.GetString("ModuleIcon", "")
            if len(module_mat_dir) > 0:
                resources[module_mat_dir] = module_icon_dir

    if use_mat_from_config_dir:
        config_mat_dir = join(
            FreeCAD.ConfigGet("UserAppData"), "Material"
        )
        if os.path.exists(config_mat_dir):
            resources[config_mat_dir] = ":/icons/preferences-general.svg"

    if use_mat_from_custom_dir:
        custom_mat_dir = mat_prefs.GetString("CustomMaterialsDir", "")
        if os.path.exists(custom_mat_dir):
            resources[custom_mat_dir] = ":/icons/user.svg"
        # fail silently
        # else:
        #     FreeCAD.Console.PrintError(
        #         'Custom material directory set by user: {} does not exist.\n'
        #         .format(custom_mat_dir)
        #     )

    return resources

def get_material_libraries():

    resources = {}  # { resource_path: icon_path, ... }

    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources")
    use_built_in_materials = mat_prefs.GetBool("UseBuiltInMaterials", True)
    use_mat_from_modules = mat_prefs.GetBool("UseMaterialsFromWorkbenches", True)
    use_mat_from_config_dir = mat_prefs.GetBool("UseMaterialsFromConfigDir", True)
    use_mat_from_custom_dir = mat_prefs.GetBool("UseMaterialsFromCustomDir", True)

    if use_built_in_materials:
        builtin_mat_dir = join(
            FreeCAD.getResourceDir(), "Mod", "Material", "Resources", "Materials"
        )
        resources["System"] = (builtin_mat_dir, ":/icons/freecad.svg")

    if use_mat_from_modules:
        module_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Resources/Modules")
        module_groups = module_prefs.GetGroups()
        for group in module_groups:
            print("\tGroup - {0}".format(group))
            module = module_prefs.GetGroup(group)
            module_mat_dir = module.GetString("ModuleDir", "")
            module_icon = module.GetString("ModuleIcon", "")
            if len(module_mat_dir) > 0:
                resources[group] = (module_mat_dir, module_icon)

    if use_mat_from_config_dir:
        config_mat_dir = join(
            FreeCAD.ConfigGet("UserAppData"), "Material"
        )
        if os.path.exists(config_mat_dir):
            resources["User"] = (config_mat_dir, ":/icons/preferences-general.svg")

    if use_mat_from_custom_dir:
        custom_mat_dir = mat_prefs.GetString("CustomMaterialsDir", "")
        if os.path.exists(custom_mat_dir):
            resources["Custom"] = (custom_mat_dir, ":/icons/user.svg")

    return resources


def list_cards(mat_dir, icon):
    import glob
    a_path = mat_dir + '/**/*.FCMat'
    print("path = '{0}'".format(a_path))
    dir_path_list = glob.glob(a_path, recursive=True)
    # Need to handle duplicates

    cards = []
    for a_path in dir_path_list:
        p = Path(a_path)
        relative = p.relative_to(mat_dir)
        cards.append(relative)

    return cards

def output_resources(resources):
    FreeCAD.Console.PrintMessage('Directories in which we will look for material cards:\n')
    for path in resources.keys():
        FreeCAD.Console.PrintMessage('  {}\n'.format(path))


# ***** card handling ****************************************************************************
# used in material editor and FEM material task panels

def import_materials(category='Solid', template=False):
    materialManager = Material.MaterialManager()
    mats = materialManager.Materials
    materials = {}
    cards = {}
    icons = {}
    for matUUID in mats:
        mat = materialManager.getMaterial(matUUID)
        physicalModels = mat.PhysicalModels
        fluid = ('1ae66d8c-1ba1-4211-ad12-b9917573b202' in physicalModels)
        if not fluid:
            path = mat.LibraryRoot + "/" + mat.Directory
            print(path)
            materials[path] = mat.Properties
            cards[path] = mat.Name
            icons[path] = mat.LibraryIcon

            print(path)
            print(mat.Properties)

    return (materials, cards, icons)

def add_cards_from_a_dir(materials, cards, icons, mat_dir, icon, template=False):
    # fill materials and icons
    import glob
    from importFCMat import read
    dir_path_list = glob.glob(mat_dir + '/*' + ".FCMat")
    mat_prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/Material/Cards")
    delete_duplicates = mat_prefs.GetBool("DeleteDuplicates", True)
    # duplicates are indicated on equality of mat dict
    # TODO if the unit is different two cards would be different too
    for a_path in dir_path_list:
        try:
            mat_dict = read(a_path)
        except Exception:
            FreeCAD.Console.PrintError(
                'Error on reading card data. The card data will be empty for card:\n{}\n'
                .format(a_path)
            )
            mat_dict = {}
        card_name = os.path.splitext(os.path.basename(a_path))[0]
        if (card_name == 'TEMPLATE') and (template is False):
            continue
        if delete_duplicates is False:
            materials[a_path] = mat_dict
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
    # https://www.freecad.org/wiki/Material_data_model
    # https://www.freecad.org/wiki/Material

    print("Call to get_material_template() successful")

    import yaml
    template_data = yaml.safe_load(
        open(join(FreeCAD.ConfigGet('AppHomePath'), 'Mod/Material/Templatematerial.yml'))
    )
    if withSpaces:
        # on attributes, add a space before a capital letter
        # will be used for better display in the ui
        import re
        new_template = []
        for group in template_data:
            new_group = {}
            gg = list(group)[0]  # group dict has only one key
            # iterating over a dict and changing it is not allowed
            # thus it is iterated over a list of the keys
            new_group[gg] = {}
            for proper in list(group[gg]):
                new_proper = re.sub(r"(\w)([A-Z]+)", r"\1 \2", proper)
                # strip underscores of vectorial properties
                new_proper = new_proper.replace("_", " ")
                new_group[gg][new_proper] = group[gg][proper]
            new_template.append(new_group)
        template_data = new_template
    return template_data


def create_mat_tools_header():
    headers = join(get_source_path(), 'src/Mod/Material/StandardMaterial/Tools/headers')
    # print(headers)
    if not os.path.isfile(headers):
        FreeCAD.Console.PrintError(
            'file not found: {}'.format(headers)
        )
        return
    template_data = get_material_template()
    f = open(headers, "w")
    for group in template_data:
        gg = list(group)[0]  # group dict has only one key
        # do not write group UserDefined
        if gg != 'UserDefined':
            for prop_name in group[gg]:
                if prop_name != 'None':
                    f.write(prop_name + '\n')
    f.close


def create_mat_template_card(write_group_section=True):
    template_card = join(get_source_path(), 'src/Mod/Material/StandardMaterial/TEMPLATE.FCMat')
    if not os.path.isfile(template_card):
        FreeCAD.Console.PrintError(
            'file not found: {}'.format(template_card)
        )
        return
    rev = "{}.{}.{}".format(
        FreeCAD.ConfigGet("BuildVersionMajor"),
        FreeCAD.ConfigGet("BuildVersionMinor"),
        FreeCAD.ConfigGet("BuildRevision")
    )
    template_data = get_material_template()
    f = open(template_card, "w")
    f.write('; TEMPLATE\n')
    f.write('; (c) 2013-2015 Juergen Riegel (CC-BY 3.0)\n')
    f.write('; information about the content of such cards can be found on the wiki:\n')
    f.write('; https://www.freecad.org/wiki/Material\n')
    f.write(': this template card was created by FreeCAD ' + rev + '\n\n')
    f.write('; localized Name, Description and KindOfMaterial uses 2 letter codes\n')
    f.write('; defined in ISO-639-1, see https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes\n')
    f.write('; find unit information in src/App/FreeCADInit.py')
    # write sections
    # write standard FCMat section if write group section parameter is set to False
    if write_group_section is False:
        f.write("\n[FCMat]\n")
    for group in template_data:
        gg = list(group)[0]  # group dict has only one key
        # do not write groups Meta and UserDefined
        if (gg != 'Meta') and (gg != 'UserDefined'):
            # only write group section if write group section parameter is set to True
            if write_group_section is True:
                f.write("\n\n[" + gg + "]")
            for prop_name in group[gg]:
                f.write('\n')
                description = group[gg][prop_name]['Description']
                if not description.strip():
                    f.write('; Description to be updated\n')
                else:
                    f.write('; ' + description + '\n')
                url = group[gg][prop_name]['URL']
                if url.strip():
                    f.write('; ' + url + '\n')
                f.write(prop_name + ' =\n')
    f.close


# card tools will not be copied into build ... thus they are not there ...
# thus the source dir is needed, this might not work on windows
def get_source_path():
    # in the file 'Makefile' in build directory the cmake variable CMAKE_SOURCE_DIR has the dir
    source_dir = ''
    make_file = join(FreeCAD.ConfigGet('AppHomePath'), 'Makefile')
    f = open(make_file, 'r')
    lines = f.readlines()
    f.close()
    for line in lines:
        if line.startswith('CMAKE_SOURCE_DIR'):
            source_dir = line.lstrip('CMAKE_SOURCE_DIR = ')
            source_dir = source_dir.rstrip()  # get rid on new line and white spaces etc.
            break
    # print(source_dir)
    return source_dir


def get_known_material_quantity_parameter():
    # get the material quantity parameter from material card template
    template_data = get_material_template()
    known_quantities = []
    for group in template_data:
        gname = list(group)[0]  # group dict has only one key
        for prop_name in group[gname]:
            prop_type = group[gname][prop_name]['Type']
            if prop_type == 'Quantity':
                # print('{} --> {}'.format(prop_name, prop_type))
                known_quantities.append(prop_name)
    return known_quantities


# ***** debug known and unknown material parameter *********************************************
def get_and_output_all_carddata(cards):
    print('\n\n\nSTART--get_and_output_all_carddata\n--------------------')
    # get all registered material property keys
    registed_cardkeys = []
    template_data = get_material_template()
    # print(template_data)
    for group in template_data:
        gg = list(group)[0]  # group dict has only one key
        for key in group[gg]:
            registed_cardkeys.append(key)
    registed_cardkeys = sorted(registed_cardkeys)
    # print(registed_cardkeys)

    # get all data from all known cards
    all_cards_and_data = {}  # {cardfilename: ['path', materialdict]}
    for card in cards:
        from importFCMat import read
        d = read(cards[card])
        all_cards_and_data[card] = [cards[card], d]
    '''
    for card in all_cards_and_data:
        print(card)
        print(all_cards_and_data[card][0])
        print(all_cards_and_data[card][1])
        print('\n')
    '''

    # find not registered and registered keys in the used data
    used_and_registered_cardkeys = []
    used_and_not_registered_cardkeys = []
    registered_and_not_used_cardkeys = []
    for card in all_cards_and_data:
        for k in all_cards_and_data[card][1]:
            if k in registed_cardkeys:
                used_and_registered_cardkeys.append(k)
            else:
                used_and_not_registered_cardkeys.append(k)
    for k in registed_cardkeys:
        if (k not in used_and_registered_cardkeys) and (k not in used_and_not_registered_cardkeys):
            registered_and_not_used_cardkeys.append(k)

    used_and_registered_cardkeys = sorted(list(set(used_and_registered_cardkeys)))
    used_and_not_registered_cardkeys = sorted(list(set(used_and_not_registered_cardkeys)))
    registered_and_not_used_cardkeys = sorted(list(set(registered_and_not_used_cardkeys)))
    FreeCAD.Console.PrintMessage(
        '\nused_and_registered_cardkeys:\n{}\n'
        .format(used_and_registered_cardkeys)
    )
    FreeCAD.Console.PrintMessage(
        '\nused_and_not_registered_cardkeys:\n{}\n'
        .format(used_and_not_registered_cardkeys)
    )
    FreeCAD.Console.PrintMessage(
        '\nregistered_and_not_used_cardkeys:\n{}\n'
        .format(registered_and_not_used_cardkeys)
    )

    # still there might be lots of properties in the template
    # which are not used in other materials
    # but the tmplate is handled here like a material
    print('--------------------\nget_and_output_all_carddata--END\n\n\n')


# ***** process multiple material cards **********************************************************
def read_cards_from_path(cards_path):
    from os import listdir
    from os.path import isfile, join, basename, splitext
    from importFCMat import read
    only_files = [f for f in listdir(cards_path) if isfile(join(cards_path, f))]
    # to make sure all file lower and upper and mixed endings are found, use upper and .FCMAT
    mat_files = [f for f in only_files if basename(splitext(f)[1]).upper() == '.FCMAT']
    # print(mat_files)
    mat_cards = []
    for f in sorted(mat_files):
        mat_cards.append(read(join(cards_path, f)))
    return mat_cards


def write_cards_to_path(cards_path, cards_data, write_group_section=True, write_template=False):
    from importFCMat import write
    from os.path import join
    for card_data in cards_data:
        if (card_data['CardName'] == 'TEMPLATE') and (write_template is False):
            continue
        else:
            card_path = join(cards_path, (card_data['CardName'] + '.FCMat'))
            # print(card_path)
            if write_group_section is True:
                write(card_path, card_data, True)
            else:
                write(card_path, card_data, False)


# ***** material parameter units *********************************************
def check_parm_unit(param):
    # check if this parameter is known to FreeCAD unit system
    # for properties with underscores (vectorial values), we must
    # strip the part after the first underscore to obtain the bound unit
    from FreeCAD import Units
    if param.find("_") != -1:
        param = param.split("_")[0]
    if hasattr(Units, param):
        return True
    else:
        return False


def check_value_unit(param, value):
    # check unit
    from FreeCAD import Units
    # FreeCAD.Console.PrintMessage('{} --> {}\n'.format(param, value))
    if hasattr(Units, param):
        # get unit and other information known by FreeCAD for this parameter
        unit = getattr(Units, param)
        quantity = Units.Quantity(1, unit)
        user_prefered_unit = quantity.getUserPreferred()[2]
        # test unit from mat dict value
        some_text = "Parameter: {} --> value: {} -->".format(param, value)
        try:
            param_value = Units.Quantity(value)
            try:
                user_unit = param_value.getValueAs(user_prefered_unit)
                if user_unit:
                    return True
                elif user_unit == 0:
                    FreeCAD.Console.PrintMessage(
                        '{} Value {} = 0 for {}\n'
                        .format(some_text, value, param)
                    )
                    return True
                else:
                    FreeCAD.Console.PrintError(
                        '{} Unknown problem in unit conversion.\n'
                        .format(some_text)
                    )
            except ValueError:
                unitproblem = value.split()[-1]
                FreeCAD.Console.PrintError(
                    '{} Unit {} is known by FreeCAD, but wrong for parameter {}.\n'
                    .format(some_text, unitproblem, param)
                )
            except Exception:
                FreeCAD.Console.PrintError(
                    '{} Unknown problem.\n'
                    .format(some_text)
                )
        except ValueError:
            unitproblem = value.split()[-1]
            FreeCAD.Console.PrintError(
                '{} Unit {} is unknown to FreeCAD.\n'
                .format(some_text, unitproblem)
            )
        except Exception:
            FreeCAD.Console.PrintError(
                '{} Unknown problem.\n'
                .format(some_text)
            )
    else:
        FreeCAD.Console.PrintError(
            'Parameter {} is unknown to the FreeCAD unit system.\n'
            .format(param)
        )
    return False


def output_parm_unit_info(param):
    # check unit
    from FreeCAD import Units
    FreeCAD.Console.PrintMessage('{}\n'.format(param))
    if hasattr(Units, param):
        FreeCAD.Console.PrintMessage(
            '\nParameter {} is known to FreeCAD unit system.'
            .format(param)
        )

        # get unit and other information known by FreeCAD for this parameter
        unit = getattr(Units, param)
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(unit)
        )

        quantity = Units.Quantity(1, unit)
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(quantity)
        )

        user_prefered_unit = quantity.getUserPreferred()[2]
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(user_prefered_unit)
        )

    else:
        FreeCAD.Console.PrintMessage(
            'Parameter {} is Unknown to the FreeCAD unit system.'
            .format(param)
        )


def output_value_unit_info(param, value):
    # check unit
    from FreeCAD import Units
    some_text = "Parameter: {} --> value: {} -->".format(param, value)
    FreeCAD.Console.PrintMessage('{} unit information:'.format(some_text))
    if hasattr(Units, param):

        # get unit and other information known by FreeCAD for this parameter
        unit = getattr(Units, param)
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(unit)
        )

        quantity = Units.Quantity(1, unit)
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(quantity)
        )

        user_prefered_unit = quantity.getUserPreferred()[2]
        FreeCAD.Console.PrintMessage(
            '{}\n'
            .format(user_prefered_unit)
        )

        # test unit from mat dict value
        try:
            param_value = Units.Quantity(value)
            try:
                user_unit = param_value.getValueAs(user_prefered_unit)
                FreeCAD.Console.PrintMessage(
                    '{} Value in preferred unit: {}\n'
                    .format(some_text, user_unit)
                )
            except ValueError:
                unitproblem = value.split()[-1]
                FreeCAD.Console.PrintError(
                    '{} Unit {} is known by FreeCAD, but wrong for parameter {}.\n'
                    .format(some_text, unitproblem, param)
                )
            except Exception:
                FreeCAD.Console.PrintError(
                    '{} Unknown problem.\n'
                    .format(some_text)
                )

        except ValueError:
            unitproblem = value.split()[-1]
            FreeCAD.Console.PrintError(
                '{} Unit {} is unknown by FreeCAD.\n'
                .format(some_text, unitproblem)
            )

        except Exception:
            FreeCAD.Console.PrintError(
                '{} Unknown problem.\n'
                .format(some_text)
            )

    else:
        FreeCAD.Console.PrintMessage(
            'Parameter {} is unknown to the FreeCAD unit system.'
            .format(param)
        )


def check_mat_units(mat):
    known_quantities = get_known_material_quantity_parameter()
    # check if the param is a Quantity according to the card template
    # then check unit
    # print(known_quantities)
    units_ok = True
    for param, value in mat.items():
        if param in known_quantities:
            if check_value_unit(param, value) is False:
                if units_ok is True:
                    units_ok = False
        else:
            pass
            # print('{} is not a known FreeCAD quantity.'.format(param))
    # print('')
    return units_ok


# ***** some code examples ***********************************************************************
'''
# cards, params, icons and resources **********
from materialtools.cardutils import get_material_resources as getres
from materialtools.cardutils import output_resources as outres
outres(getres())


from materialtools.cardutils import import_materials as getmats
from materialtools.cardutils import output_materials as outmats
from materialtools.cardutils import output_trio as outtrio

outmats(getmats()[0])

outtrio(getmats())

a,b,c = getmats()
materials, cards, icons = getmats()


# param template, header, template card **********
from materialtools.cardutils import get_material_template as gettemplate
gettemplate()

gettemplate()[1]['General']['Description']
gettemplate()[2]['Mechanical']['FractureToughness']


from materialtools.cardutils import get_material_template as gettemplate
template_data=gettemplate()
for group in template_data:
    gname = list(group)[0]  # group dict has only one key
    for prop_name in group[gname]:
        #prop_dict = group[gname][prop_name]
        #print(prop_dict)
        #print(prop_dict['Description'])
        print(group[gname][prop_name]['Description'])


from materialtools.cardutils import create_mat_tools_header as createheader
createheader()


from materialtools.cardutils import create_mat_template_card as createtemplate
createtemplate()
createtemplate(False)


from materialtools.cardutils import get_source_path as getsrc
getsrc()


# generate all cards **********
# run tools in source dir
./make_ods.sh
./make_FCMats.sh

# read cards
from materialtools.cardutils import read_cards_from_path as readcards
from materialtools.cardutils import get_source_path as getsrc
cards_data = readcards(getsrc() + '/src/Mod/Material/StandardMaterial/')

# print cards
for c in cards_data:
    print(c)

# write cards
from materialtools.cardutils import write_cards_to_path as writecards
from materialtools.cardutils import get_source_path as getsrc

# True writes sections ( method write_group_section is used =)
writecards(getsrc() + '/src/Mod/Material/StandardMaterial/', cards_data, True)

writecards(getsrc() + '/src/Mod/Material/StandardMaterial/', cards_data, False)

# last True writes the TEMPLATE card which has no mat params because they have no values
writecards(getsrc() + '/src/Mod/Material/StandardMaterial/', cards_data, True, True)


# material quantity parameter unit checks **********
from materialtools.cardutils import output_parm_unit_info as unitinfo
unitinfo('YoungsModulus')
unitinfo('FractureToughness')
unitinfo('PoissonRatio')

from materialtools.cardutils import check_parm_unit as checkparamunit
checkparamunit('YoungsModulus')
checkparamunit('FractureToughness')

from materialtools.cardutils import output_value_unit_info as valueunitinfo
valueunitinfo('YoungsModulus', '1 MPa')
valueunitinfo('FractureToughness', '25')
valueunitinfo('Density', '1 kg/m^3')
valueunitinfo('Density', '0 kg/m^3')

from materialtools.cardutils import check_value_unit as checkvalueunit
checkvalueunit('YoungsModulus', '1 MPa')
checkvalueunit('FractureToughness', '25')
checkvalueunit('Density', '1 kg/m^3')
checkvalueunit('Density', '0 kg/m^3')

# syntax error in unit system, only the try: except in the checkvalueunit let it work
checkvalueunit('ThermalConductivity', '0.02583 W/m/K')
checkvalueunit('ThermalConductivity', '123abc456 W/m/K')
checkvalueunit('ThermalConductivity', '25.83e−3 W/m/K')
checkvalueunit('ThermalConductivity', '25.83e-3 W/m/K')
from FreeCAD import Units
Units.Quantity('25.83e−3 W/m/K')


# test mat unit properties
mat = {
    'Name': 'Concrete',
    'AngleOfFriction' : '1 deg',
    'CompressiveStrength': '1 MPa',
    'Density': '1 kg/m^3',
    'ShearModulus' : '1 MPa',
    'UltimateTensileStrength' : '1 MPa',
    'YieldStrength' : '1 MPa',
    'YoungsModulus': '1 MPa',
    'SpecificHeat' : '1 J/kg/K',
    'ThermalConductivity' : '1 W/m/K',
    'ThermalExpansionCoefficient' : '1 mm/mm/K'
}
from materialtools.cardutils import check_mat_units as checkunits
checkunits(mat)

# unknown quantities, returns True too
mat = {
    'Name': 'Concrete',
    'FractureToughness' : '1',
    'PoissonRatio': '0.17'  # no unit but important too, proof somehow too
}
from materialtools.cardutils import check_mat_units as checkunits
checkunits(mat)

# wrong units
mat = {
    'Name': 'Concrete',
    'CompressiveStrength': '12356 MBa',  # type on unit, means unit not knwn
    'YoungsModulus': '654321 m',  # unit known, but wrong unit for this property
}
from materialtools.cardutils import check_mat_units as checkunits
checkunits(mat)

# missing unit, returns False
mat = {
    'Name': 'Concrete',
    'YoungsModulus' : '1'
}
from materialtools.cardutils import check_mat_units as checkunits
checkunits(mat)

# empty dict, returns True
from materialtools.cardutils import check_mat_units as checkunits
checkunits({})


# some unit code **********
from FreeCAD import Units
getattr(Units, 'Pressure')
Units.Pressure
Units.Quantity('25 MPa')
Units.Quantity('25 MPa').getValueAs('Pa')
Units.Quantity('25 MPa').getUserPreferred()[2]
Units.Quantity(25000, Units.Pressure)
Units.Quantity(25000, Units.Pressure).getValueAs('MPa')
Units.Unit('25 MPa')
Units.Unit(-1,1,-2,0,0,0,0,0)

# base units
from FreeCAD import Units
Units.Length
Units.Mass
Units.TimeSpan
Units.ElectricCurrent
Units.Temperature
Units.AmountOfSubstance
Units.LuminousIntensity
Units.Angle


'''
