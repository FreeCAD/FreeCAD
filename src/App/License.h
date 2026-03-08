// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <array>
#include <cstring>
#include <string>
#include <Base/Tools.h>

namespace App
{

/**
 * Licenses data [identifier, fullName, url]
 * See also https://spdx.org/licenses/
 */
constexpr int colsInArray = 3;
using TLicenseArr = std::array<const char*, colsInArray>;
constexpr int posnOfIdentifier = 0;
constexpr int posnOfFullName = 1;
constexpr int posnOfUrl = 2;
constexpr int countOfLicenses {19};
// clang-format off
constexpr std::array<TLicenseArr, countOfLicenses> licenseItems {{
    { "AllRightsReserved", "All rights reserved",                                          "https://en.wikipedia.org/wiki/All_rights_reserved"  },
    { "CC_BY_40",          "Creative Commons Attribution 4.0",                             "https://creativecommons.org/licenses/by/4.0/"       },
    { "CC_BY_SA_40",       "Creative Commons Attribution-ShareAlike 4.0",                  "https://creativecommons.org/licenses/by-sa/4.0/"    },
    { "CC_BY_ND_40",       "Creative Commons Attribution-NoDerivatives 4.0",               "https://creativecommons.org/licenses/by-nd/4.0/"    },
    { "CC_BY_NC_40",       "Creative Commons Attribution-NonCommercial 4.0",               "https://creativecommons.org/licenses/by-nc/4.0/"    },
    { "CC_BY_NC_SA_40",    "Creative Commons Attribution-NonCommercial-ShareAlike 4.0",    "https://creativecommons.org/licenses/by-nc-sa/4.0/" },
    { "CC_BY_NC_ND_40",    "Creative Commons Attribution-NonCommercial-NoDerivatives 4.0", "https://creativecommons.org/licenses/by-nc-nd/4.0/" },
    { "CC_BY_30",          "Creative Commons Attribution 3.0",                             "https://creativecommons.org/licenses/by/3.0/"       },
    { "CC_BY_SA_30",       "Creative Commons Attribution-ShareAlike 3.0",                  "https://creativecommons.org/licenses/by-sa/3.0/"    },
    { "CC_BY_ND_30",       "Creative Commons Attribution-NoDerivatives 3.0",               "https://creativecommons.org/licenses/by-nd/3.0/"    },
    { "CC_BY_NC_30",       "Creative Commons Attribution-NonCommercial 3.0",               "https://creativecommons.org/licenses/by-nc/3.0/"    },
    { "CC_BY_NC_SA_30",    "Creative Commons Attribution-NonCommercial-ShareAlike 3.0",    "https://creativecommons.org/licenses/by-nc-sa/3.0/" },
    { "CC_BY_NC_ND_30",    "Creative Commons Attribution-NonCommercial-NoDerivatives 3.0", "https://creativecommons.org/licenses/by-nc-nd/3.0/" },
    { "PublicDomain",      "Public Domain",                                                "https://en.wikipedia.org/wiki/Public_domain"        },
    { "FreeArt",           "FreeArt",                                                      "https://artlibre.org/licence/lal"                   },
    { "CERN_OHS_S",        "CERN Open Hardware Licence strongly-reciprocal",               "https://cern-ohl.web.cern.ch/"                      },
    { "CERN_OHS_W",        "CERN Open Hardware Licence weakly-reciprocal",                 "https://cern-ohl.web.cern.ch/"                      },
    { "CERN_OHS_P",        "CERN Open Hardware Licence permissive",                        "https://cern-ohl.web.cern.ch/"                      },
    { "GPL-3.0-or-later",  "GNU General Public License 3.0 or later",                      "https://www.gnu.org/licenses/gpl-3.0.html"          },
}};
// clang-format on

int constexpr findLicense(const char* identifier)
{
    if (Base::Tools::isNullOrEmpty(identifier)) {
        return -1;
    }
    for (int i = 0; i < countOfLicenses; i++) {
        if (strcmp(licenseItems.at(i).at(posnOfIdentifier), identifier) == 0) {
            return i;
        }
    }
    return -1;
}
}  // namespace App
