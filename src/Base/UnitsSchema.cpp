/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)        *
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


#include "PreCompiled.h"
#ifdef __GNUC__
# include <unistd.h>
#endif

#include <QString>
#include <QLocale>

#include "Exception.h"
#include "UnitsApi.h"
#include "UnitsSchema.h"

using namespace Base;

QString UnitsSchema::toLocale(const Base::Quantity& quant, double factor, const QString& unitString) const
{
    //return QString::fromUtf8("%L1 %2").arg(quant.getValue() / factor).arg(unitString);
    QLocale Lc = QLocale::system();
    const QuantityFormat& format = quant.getFormat();
    if (format.option != QuantityFormat::None) {
        uint opt = static_cast<uint>(format.option);
        Lc.setNumberOptions(static_cast<QLocale::NumberOptions>(opt));
    }

    QString Ln = Lc.toString((quant.getValue() / factor), format.toFormat(), format.precision);
    return QString::fromUtf8("%1 %2").arg(Ln, unitString);
}
