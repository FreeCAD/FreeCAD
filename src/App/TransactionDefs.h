// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Théo Veilleux-Trinh
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef APP_TRANSACTIONDEFS_H_
#define APP_TRANSACTIONDEFS_H_

#include <string>

namespace App {

class Document;

constexpr int NullTransaction = 0;

struct TransactionName {
    // Name of the transaction as it will appear in the GUI
    std::string name;
    
    // If true, the transaction is allowed to be renamed
    bool temporary { false };
};

struct TransactionDescription {
    // Document on which the transaction was first created
    // useful for mass transaction (e.g. delete from an assembly)
    // where other documents may use the same transaction id
    Document* initiator { nullptr };
    TransactionName name;
};

enum class TransactionCloseMode {
    Commit = 0,
    Abort = 1,
};
}

#endif