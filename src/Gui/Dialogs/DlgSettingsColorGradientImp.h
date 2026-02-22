// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/ColorModel.h>
#include <QDialog>
#include <memory>

class QDoubleValidator;

namespace Gui
{
namespace Dialog
{
class Ui_DlgSettingsColorGradient;

/**
 * The DlgSettingsColorGradientImp class implements a preference page to change settings
 * for the color gradient bar.
 * @author Werner Mayer
 */
class DlgSettingsColorGradientImp: public QDialog
{
    Q_OBJECT

public:
    explicit DlgSettingsColorGradientImp(
        const App::ColorGradient& cg,
        QWidget* parent = nullptr,
        Qt::WindowFlags fl = Qt::WindowFlags()
    );
    ~DlgSettingsColorGradientImp() override;

    void accept() override;

    /** @name Color profile */
    //@{
    App::ColorGradientProfile getProfile() const;
    void setProfile(const App::ColorGradientProfile& pro);
    //@}
    /** @name Parameter range and scale */
    //@{
    void setNumberOfDecimals(int, float fMin, float fMax);
    int numberOfDecimals() const;
    //@}

private:
    void setupConnections();

    /** @name Color model */
    //@{
    void setColorModelNames(const std::vector<std::string>&);
    void setColorModel(std::size_t tModel);
    std::size_t colorModel() const;
    //@}
    /** @name Color style */
    //@{
    void setColorStyle(App::ColorBarStyle tStyle);
    App::ColorBarStyle colorStyle() const;
    //@}
    /** @name Display mode */
    //@{
    void setOutGrayed(bool grayed);
    bool isOutGrayed() const;
    void setOutInvisible(bool invisible);
    bool isOutInvisible() const;
    //@}
    /** @name Parameter range and scale */
    //@{
    void setRange(float fMin, float fMax);
    void getRange(float& fMin, float& fMax) const;
    void setNumberOfLabels(int);
    int numberOfLabels() const;
    //@}

Q_SIGNALS:
    void colorModelChanged();

private:
    QDoubleValidator* validator;
    std::unique_ptr<Ui_DlgSettingsColorGradient> ui;
};

}  // namespace Dialog
}  // namespace Gui
