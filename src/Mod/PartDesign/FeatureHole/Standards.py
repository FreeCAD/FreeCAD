# -*- coding: iso-8859-15 -*-
#/******************************************************************************
# * Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>   *
# *                                                                            *
# * This file is part of the FreeCAD CAx development system.                   *
# *                                                                            *
# * This library is free software; you can redistribute it and/or              *
# * modify it under the terms of the GNU Library General Public                *
# * License as published by the Free Software Foundation; either               *
# * version 2 of the License, or (at your option) any later version.           *
# *                                                                            *
# * This library is distributed in the hope that it will be useful,            *
# * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
# * GNU Library General Public License for more details.                       *
# *                                                                            *
# * You should have received a copy of the GNU Library General Public          *
# * License along with this library; see the file COPYING.LIB. If not,         *
# * write to the Free Software Foundation, Inc., 59 Temple Place,              *
# * Suite 330, Boston, MA 02111-1307, USA                                      *
# *                                                                            *
# ******************************************************************************/

import FreeCAD

"Standards for bore hole feature"
sources = {
    "inet_arcor"    : "http://home.arcor.de/maschinenelemente2-din/DIN%20EN%2020273_Durchgangsl%F6cher%20fuer%20Schrauben.PDF",
    "inet_duckma"  : "http://www.duckma.de/mb14/SiteDocs/DIN%20Grundlagen%20Maschinenbau.pdf",
    "klein_14" : "Klein: Einführung in die DIN-Normen, 14. Auflage. Stuttgart, Teubner 2008"
}

StandardYear = 0
StandardTitle = 1
StandardSource = 2
StandardType = 3

standards = {
# "Standard name" : ("Year", "Title", "Source", "Type")
    "DIN 13-1"   : ("1999", "Metrisches ISO-Gewinde allgemeiner Anwendung (Auszug); Nennmaße für Regelgewinde",  "klein_14",  "thread"),
    "DIN 74-A"   : ("2003",  "Senkungen fur Senkschrauben, ausgenommen Senkschrauben mit Kopfen nach DIN EN 27721; Form A",  "klein_14",  "countersink"),
    "DIN 74-E"   : ("2003",  "Senkungen fur Senkschrauben, ausgenommen Senkschrauben mit Kopfen nach DIN EN 27721; Form E",  "klein_14",  "countersink"),
    "DIN 74-F"   : ("2003",  "Senkungen fur Senkschrauben, ausgenommen Senkschrauben mit Kopfen nach DIN EN 27721; Form F",  "klein_14",  "countersink"),
    "DIN 76-2"  : ("1984",  "Gewindeausläufe und Gewindefreistiche (Auszug); für Metrisches ISO-Gewinde nach DIN 13; Innengewinde (Gewindegrundlöcher)", "klein_14",  "threaded"),
    "DIN 974-1" : ("1991", "Senkdurchmesser für Schrauben mit Zylinderkopf; Konstruktionsmaße (Auszug)", "klein_14", "counterbore"),
    "DIN 974-2" : ("1991", "Senkdurchmesser fur Sechskantschrauben und Sechskantmuttern; Konstruktionsmaße(Auszug)", "klein_14", "counterbore"),
    "ISO 273"     : ("1979", "Fasteners; Clearance holes for bolts and screws", "inet_arcor", "through"),
    "ISO 15065" : ("2005",  "Senkungen fur Senkschrauben mit Kopfform nach ISO 7721",  "klein_14", "countersink")
}

aliases = {
    "ISO 273" : ("ISO 273:1979", "EN 20273:1991", "DIN EN 20273:1992", "DIN ISO 273", "DIN ISO 273/09.79"),
    "ISO 15065" : ("ISO 15065:2005", "EN ISO 15065", "EN ISO 15065:2005",  "DIN EN ISO 15065")
}

standards_tolerance = ("fine", "medium", "coarse")

standards_through = {
# "Standard name" : (Thread_dia : Hole_dia(Fine, Medium, Coarse))
    "ISO 273" : {
        1.0 : (1.1, 1.2, 1.3),
        1.2 : (1.3, 1.4, 1.5),
        1.4 : (1.5, 1.6, 1.8),
        1.6 : (1.7, 1.8, 2.0),
        1.8 : (2.0, 2.1, 2.2),
        2.0 : (2.2, 2.4, 2.6),
        2.5 : (2.7, 2.9, 3.1),
        3.0 : (3.2, 3.4, 3.6),
        3.5 : (3.7, 3.9, 4.2),
        4.0 : (4.3, 4.5, 4.8),
        4.5 : (4.8, 5.0, 5.3),
        5.0 : (5.3, 5.5, 5.8),
        6.0 : (6.4, 6.6, 7.0),
        7.0 : (7.4, 7.6, 8.0),
        8.0 : (8.4, 9.0, 10),
        10.0: (10.5, 11, 12),
        12.0: (13, 13.5, 14.5),
        14.0: (15, 15.5, 16.5),
        16.0: (17, 17.5, 18.5),
        18.0: (19, 20, 21),
        20.0: (21, 22, 24),
        22.0: (23, 24, 26),
        24.0: (25, 26, 28),
        27.0: (28, 30, 32),
        30.0: (31, 33, 35),
        33.0: (34, 36, 38),
        36.0: (37, 39, 42),
        39.0: (40, 42, 45),
        42.0: (43, 45, 48),
        45.0: (46, 48, 52),
        48.0: (50, 52, 56),
        52.0: (54, 56, 62),
        56.0: (58, 62, 66),
        60.0: (62, 66, 70),
        64.0: (66, 70, 74),
        68.0: (70, 74, 80),
        72.0: (74, 78, 82),
        76.0: (78, 82, 86),
        80.0: (82, 86, 91),
        85.0: (87, 91, 96),
        90.0: (93, 96, 101),
        95.0: (98, 101, 107),
        100.0: (104, 107, 112),
        105.0: (109, 112, 117),
        110.0: (114, 117, 121),
        115.0: (119, 122, 127),
        120.0: (124, 127, 132),
        125.0: (129, 132, 137),
        130.0: (134, 137, 144),
        140.0: (144, 147, 155),
        150.0: (155, 158, 165)
    }
}

standards_counterbore = {
# "Standard name" : {Thread_dia : counterboredia(row1, row2, row3, row4, row5, row6)}
    "DIN 974-1" : {
        1.0 : (2.2, None,  None,  None,  None,  None),
        1.2 : (2.5, None,  None,  None,  None,  None),
        1.4 : (3.0, None,  None,  None,  None,  None),
        1.6 : (3.5,  3.5,  None,  None,  None, None),
        1.8 : (3.8,  None,  None,  None,  None,  None),
        2.0 : (4.4,  5.0,  None,  5.5,  6,  6),
        2.5 : (5.5,  6,  None,  6,  7,  7),
        3.0 : (6.5,  7,  6.5,  7,  9,  8),
        3.5 : (6.5,  8,  6.5,  8,  9,  9),
        4.0 : (8,  9,  8,  9,  10,  10),
        5.0 : (10,  11,  10,  11,  13,  13),
        6.0 : (11,  13,  11,  13,  15,  15),
        8.0 : (15,  18,  15,  16,  18,  20),
        10.0 : (18,  24,  18,  20,  24,  24),
        12.0 : (20,  None,  20,  24,  26,  33),
        14.0 : (24,  None,  24,  26,  30,  40),
        16.0 : (26,  None,  26,  30,  33,  43),
        18.0 : (30,  None,  30,  33,  36,  46),
        20.0 : (33,  None,  33,  36,  40,  48),
        22.0 : (36,  None,  36,  40,  43,  54),
        24.0 : (40,  None,  40,  43,  48,  58),
        27.0 : (46,  None,  46,  46,  54,  63),
        30.0 : (50,  None,  50,  54,  61,  73),
        33.0 : (54,  None,  54,  None,  63,  None),
        36.0 : (58,  None,  58,  63,  69,  None),
        42.0 : (69,  None,  69,  73,  82,  None),
        48.0 : (78,  None,  78,  82,  98,  None),
        56.0 : (93,  None,  93,  93,  112,  None),
        64.0 : (107,  None,  107,  107,  125,  None),
        72.0 : (118,  None,  118,  118,  132,  None),
        80.0 : (132,  None,  132,  132,  150,  None),
        90.0 : (145,  None,  145,  145,  170,  None),
        100.0:(160,  None,  160,  160,  182,  None)
    },
    "DIN 974-2" : {
        3.0 : (11, 11,  9),
        4.0 : (13,  15,  10),
        5.0 : (15,  18,  11),
        6.0 : (18,  20,  13),
        8.0 : (24,  26,  18),
        10.0:(28,  33,  22),
        12.0:(33,  36,  26),
        14.0:(36,  43,  30),
        16.0:(40,  46,  33),
        18.0:(43,  50,  36),
        20.0:(46,  54,  40),
        22.0:(54,  61,  46),
        24.0:(58,  73,  48),
        27.0:(61,  76,  54),
        30.0:(73,  82,  61),
        33.0:(76,  89,  69),
        36.0:(82,  93,  73),
        39.0:(89,  98,  76),
        42.0:(98,  107,  82),
        45.0:(107,  112,  89)
    }
}

standards_counterbore_through = {
# Standard name : Through hole standard name
    "DIN 74-A"   : "ISO 273",
    "DIN 74-E"   : "ISO 273", # Note that the standards seems to allow tolerance class "fine" only
    "DIN 74-F"   : "ISO 273",
    "DIN 974-1" : "ISO 273",
    "DIN 974-2" : "ISO 273",
    "ISO 15065" : "ISO 273"
}

standards_counterbore_rows = {
# Row index : ( extra standards e.g. bolt used or washer used )
# Note that DIN 7980 has been cancelled, therefore row three should not be used any more
    "DIN 974-1" : {
        1 : ("ISO 1207", "ISO 4762",  "DIN 6912",  "DIN 7984"),
        2 : ("ISO 1580", "ISO 7045"),
        3 : ("DIN 7980",  ""),  # single value gives wrong iteration when collecting the standards
        4 : ("ISO 10673 type C",  "DIN 6798", "DIN 6907"),
        5 : ("ISO 7089", "ISO 7090",  "ISO 10673 type A"),
        6 : ("DIN 6796", "DIN 6908")
    },
    "DIN 974-2" : {
        1 : ("DIN 659",  "DIN 896",  "DIN 3112",  "DIN 3124"),
        2 : ("DIN 838", "DIN 897",  "DIN 3129"),
        3 : ("tight",  "")
    }
}

standards_counterbore_extradepth = {
# max Thread diameter : extra depth
    1.4 : 0.2,
    6.0 : 0.4,
    20.0 : 0.6,
    27.0 : 0.8,
    100.0 : 1.0
}

standards_countersink_dia = 0
standards_countersink_angle = 1

standards_countersink = {
# "Standard name" : {Thread_dia : (countersinkdia, head angle)}
    "DIN 74-A" : {
        1.6 : (3.7, 90.0),
        2.0 : (4.6, 90.0),
        2.5 : (5.7, 90.0),
        3.0 : (6.5, 90.0),
        3.5 : (7.6, 90.0),
        4.0 : (8.6, 90.0),
        4.5 : (9.5, 90.0),
        5.0 : (10.4, 90.0),
        5.5 : (11.4, 90.0),
        6.0 : (12.4, 90.0),
        7.0 : (14.4, 90.0),
        8.0 : (16.4,  90.0)
    },
    "DIN 74-E" : {
        10.0 : (19.0, 75.0),
        12.0 : (24.0, 75.0),
        16.0 : (31.0,  75.0),
        20.0 : (34.0,  60.0),
        22.0 : (37.0,  60.0),
        24.0 : (40.0,  60.0)
    },
    "DIN 74-F" : {
          3.0 : (6.94,  90.0),
          4.0 : (9.18,  90.0),
          5.0 : (11.47,  90.0),
          6.0 : (13.71,  90.0),
          8.0 : (18.25,  90.0),
        10.0 : (22.73, 90.0),
        12.0 : (27.21, 90.0),
        14.0 : (31.19,  90.0),
        16.0 : (33.39,  90.0),
        20.0 : (40.71,  90.0)
    },
    "ISO 15065" : {
        2.0 : (4.4, 90.0),
        3.0 : (6.3, 90.0),
        4.0 : (9.4, 90.0),
        5.0 : (0.4, 90.0),
        6.0 : (12.6, 90.0),
        8.0 : (17.3, 90.0),
        10.0 : (20.0,  90.0)
    }
}

standards_threaded_types = ("normal", "short",  "long")

standards_threaded = {
# Standard name : { Tread pitch : threadFinish(normal, short, long) }
    "DIN 76-2" : {
        0.20 : (1.3,  0.8,  2.0),
        0.25 : (1.5,  1.0,  2.4),
        0.30 : (1.8,  1.2,  2.9),
        0.35 : (2.1,  1.3,  3.3),
        0.40 : (2.3,  1.5,  3.7),
        0.45 : (2.6,  1.6,  4.1),
        0.50 : (2.8,  1.8,  4.5),
        0.60 : (3.4,  2.1,  5.4),
        0.70 : (3.8,  2.4,  6.1),
        0.75 : (4.0 , 2.5,  6.4),
        0.80 : (4.2,  2.7,  6.8),
        1.00 : (5.1,  3.2,  8.2),
        1.25 : (6.2,  3.9,  10),
        1.5   : (7.3,  4.6,  11.6),
        1.75 : (8.3,  5.2,  13.3),
        2.0   : (9.3,  5.8,  14.8),
        2.5   : (11.2,  7.0,  17.9),
        3.0   : (13.1,  8.2,  21.0),
        3.5   : (15.2,  9.5,  24.3),
        4.0   : (16.8,  10.5,  26.9),
        4.5   : (18.4,  11.5,  29.4),
        5.0   : (20.8,  13.0,  33.3),
        5.5   : (22.4,  14.0,  35.8),
        6.0   : (24.0,  15,  38.4)
    }
}

standards_threaded_thread = {
# Standard name for thread attribute : standard name for thread }
    "DIN 76-2" : "DIN 13-1"
}

standards_thread_pitch = 0
standards_thread_flankdia = 1
standards_thread_outercoredia = 2
standards_thread_innercoredia = 3
standards_thread_outerdepth = 4
standards_thread_innerdepth = 5
standards_thread_round = 6

standards_thread = {
# Standard name : { Thread diameter : (pitch, flank diameter, core diameter, thread depth outer, thread depth inner, round) }
# Note: This table only has the most common thread diameters
    "DIN 13-1" : {
        1.0  :  (0.25, 0.838,  0.693,  0.729,  0.153,  0.135,  0.036),
        1.1  :  (0.25, 0.938,  0.793,  0.829,  0.153,  0.135,  0.036),
        1.2  :  (0.25, 1.038,  0.893,  0.929,  0.153,  0.135,  0.036),
        2.0  :  (0.4,  1.740,  1.509,  1.567,  0.245,  0.217,  0.058),
        3.0  :  (0.5,  2.675,  2.387,  2.459,  0.307,  0.271,  0.072),
        4.0  :  (0.7,  3.545,  3.141,  3.242,  0.429, 0.379,  0.101 ),
        5.0  :  (0.8,  4.480,  4.019,  4.134,  0.491, 0.433,  0.115),
        6.0   : (1.0,  5.350,  4.773,  4.917,  0.613, 0.541,  0.144),
        7.0   : (1.0,  6.350,  5.773,  5.917,  0.613, 0.541,  0.144),
        8.0   : (1.25,  7.188,  6.466,  6.647,  0.767,  0.677,  0.180),
        10.0 : (1.5,    9.026,  8.160,  8.376,  0.920,  0.812,  0.217),
        12.0 : (1.75,  10.863,  9.853,  10.106,  1.074,  0.947,  0.253),
        14.0 : (2.0,  12.701,  11.546,  11.835,  1.227,  1.083,  0.289),
        16.0 : (2.0,  14.701,  13.546,  13.835,  1.227,  1.083,  0.289),
        18.0 : (2.5,  16.376,  14.933,  15.294,  1.534,  1.353,  0.361),
        20.0 : (2.5,  18.376,  16.933,  17.294,  1.534,  1.353,  0.361),
        22.0 : (2.5,  20.376,  18.933,  19.294,  1.534,  1.353,  0.361),
        24.0 : (3.0,  22.051,  20.319,  20.752,  1.840,  1.624,  0.433),
        27.0 : (3.0,  25.051,  23.319,  23.752,  1.840,  1.624,  0.433),
        30.0 : (3.5,  27.727,  25.706,  26.211,  2.147,  1.894,  0.505),
        33.0 : (3.5,  30.727,  28.706,  29.211,  2.147,  1.894,  0.505),
        36.0 : (4.0,  33.402,  31.093,  31.670,  2.454,  2.165,  0.577),
        39.0 : (4.0,  36.402,  34.093,  34.670,  2.454,  2.165,  0.577),
        42.0 : (4.5 , 39.077,  36.479,  37.129,  2.760,  2.436,  0.650),
        45.0 : (4.5,  42.077,  39.479,  40.129,  2.760,  2.436,  0.650)
    }
}

def getStandards(holetype):
    "Return the names of all available standards for the given hole type"
    result = []
    for key, value in standards.items():
        if value[StandardType] == holetype:
            result.append(key)

    #FreeCAD.Console.PrintMessage("Number of matching standards: " + str(len(result)) + "\n")
    return sorted(result)

def getBaseDiameters(standard):
    "Return the base diameters of all holes defined in the given norm"
    if not standard in standards:
        return []
    #FreeCAD.Console.PrintMessage("Getting diameters for " + standard + "\n")
    if standards[standard][StandardType] == "through":
        return standards_through[standard].keys()
    elif standards[standard][StandardType] == "counterbore":
        return standards_counterbore[standard].keys()
    elif standards[standard][StandardType] == "countersink":
        return standards_countersink[standard].keys()
    elif standards[standard][StandardType] == "thread":
        return standards_thread[standard].keys()
    return []

def getThroughHoleDia(standard, threadDia, tolerance = "medium"):
    if not standard in standards_through:
        raise Exception("No such standard exists")
    values = standards_through[standard]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][standards_tolerance.index(tolerance)]
    return values[threadDia][standards_tolerance.index(tolerance)]

def getThroughHoleStandard(standard):
    if not standard in standards_counterbore_through:
        raise Exception("No such standard exists")
    return standards_counterbore_through[standard]

def getCounterboreDia(standard,  threadDia,  extraStandard = ""):
    if not standard in standards_counterbore:
        raise Exception("No such standard exists")
    values = standards_counterbore[standard]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][0]
    row = 1 # Use row 1 by default
    for r in standards_counterbore_rows[standard].keys():
        if extraStandard in standards_counterbore_rows[standard][r]:
            row = r
            break
    return values[threadDia][row-1]

def calcCounterboreDepth(standard,  threadDia,  standardBolt,  standardsWashers = []):
    headHeight = getBoltHead(standardBolt)
    washerHeight = 0.0
    for standard in standardsWashers:
        washerHeight = washerHeight + getWasherHeight(standard)
    for maxThread in reverse(standards_counterbore_extradepth.keys()):
        if threadDia <= maxThread:
            extraDepth = standards_counterbore_extradepth[maxThread]
    return headHeight + washerHeight + extraDepth

def getRowStandards(standard):
    if not standard in standards_counterbore_rows:
        raise Exception("No such standard exists")
    result = []
    rowdict = standards_counterbore_rows[standard]
    for stds in rowdict.values():
        for std in stds:
            if std != "":
                result.append(std)
    return result

def getCountersinkDia(standard,  threadDia):
    if not standard in standards_countersink:
        raise Exception("No such standard exists")
    values = standards_countersink[standard]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][standards_countersink_dia]
    return values[threadDia][standards_countersink_dia]

def getCountersinkAngle(standard,  threadDia):
    if not standard in standards_countersink:
        raise Exception("No such standard exists")
    values = standards_countersink[standard]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][standards_countersink_angle]
    return values[threadDia][standards_countersink_angle]

def getThreadCoreDiameter(standard,  threadDia):
    if not standard in standards_thread:
        raise Exception("No such standard exists")
    values = standards_thread[standard]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][standards_thread_innercoredia]
    return values[threadDia][standards_thread_innercoredia]

def getThreadFinishLength(standard,  threadDia,  length = "normal"):
    if not standard in standards_threaded:
        raise Exception("No such standard exists")
    stdThread = standards_threaded_thread[standard]
    values = standards_thread[stdThread]
    if not threadDia in values:
        FreeCAD.Console.PrintMessage("Warning: Diameter %f is not in %s"  % (threadDia,  standard))
        return values[values.keys()[0]][standards_thread_pitch]
    pitch = values[threadDia][standards_thread_pitch]
    return standards_threaded[standard][pitch][standards_threaded_types.index(length)]
