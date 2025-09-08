/***************************************************************************
 *   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com> *
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

#include "PDMigrationGate.h"
#include <unordered_set>
#include <mutex>
#include "Document.h"
#include "Application.h"

namespace App {
namespace {
std::mutex g_mt;
std::unordered_set<const Document*> g_docs;
}

void MarkDocNeedsPDMigration(const Document* d) {
    if (!d) return;
    std::lock_guard<std::mutex> lk(g_mt);
    g_docs.insert(d);
}

bool ConsumeDocNeedsPDMigration(const Document* d) {
    if (!d) return false;
    std::lock_guard<std::mutex> lk(g_mt);
    auto it = g_docs.find(d);
    if (it == g_docs.end()) return false;
    g_docs.erase(it);
    return true;
}

void ClearDocFromPDMigration(const Document* d) {
    if (!d) return;
    std::lock_guard<std::mutex> lk(g_mt);
    g_docs.erase(d);
}

} // namespace App

