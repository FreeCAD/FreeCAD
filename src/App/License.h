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

#ifndef APP_LICENSE_H
#define APP_LICENSE_H

#include <array>
#include <string>

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
constexpr int countOfLicenses {18};
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
}};
// clang-format on

int constexpr findLicense(const char* identifier)
{
    for (int i = 0; i < countOfLicenses; i++) {
        if (licenseItems.at(i).at(posnOfIdentifier) == identifier) {
            return i;
        }
    }
    return -1;
}
}// namespace App

#endif// APP_LICENSE_H
