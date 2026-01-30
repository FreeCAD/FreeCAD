// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026                                                   *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "Translation.h"

#include <atomic>

namespace Base::Translation
{

namespace
{
std::atomic<const Translator*> activeTranslator {nullptr};

class NullTranslator final: public Translator
{
public:
    std::string translate(
        std::string_view /*context*/,
        std::string_view sourceText,
        std::string_view /*disambiguation*/,
        int /*n*/
    ) const override
    {
        return std::string(sourceText);
    }

    bool installTranslator(std::string_view /*filename*/) const override
    {
        return false;
    }

    bool removeTranslators(const std::vector<std::string>& /*filenames*/) const override
    {
        return false;
    }
};

NullTranslator nullTranslator;

const Translator* translator()
{
    const Translator* translator = activeTranslator.load(std::memory_order_acquire);
    return translator ? translator : &nullTranslator;
}
}  // namespace

void setTranslator(const Translator* translator)
{
    activeTranslator.store(translator, std::memory_order_release);
}

const Translator* getTranslator()
{
    return translator();
}

std::string translate(
    std::string_view context,
    std::string_view sourceText,
    std::string_view disambiguation,
    int n
)
{
    return translator()->translate(context, sourceText, disambiguation, n);
}

bool installTranslator(std::string_view filename)
{
    return translator()->installTranslator(filename);
}

bool removeTranslators(const std::vector<std::string>& filenames)
{
    return translator()->removeTranslators(filenames);
}

}  // namespace Base::Translation
