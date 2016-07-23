/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_TRANSACTIONOBJECT_H
#define GUI_TRANSACTIONOBJECT_H

#include <App/Transactions.h>

namespace Gui
{
class TransactionViewProvider : public App::TransactionObject
{
    TYPESYSTEM_HEADER();

public:
    TransactionViewProvider();
    virtual ~TransactionViewProvider();

    void applyNew(App::Document& Doc, App::TransactionalObject* pcObj);
    void applyDel(App::Document& Doc, App::TransactionalObject* pcObj);
};

} //namespace Gui


#endif // GUI_TRANSACTIONOBJECT_H
