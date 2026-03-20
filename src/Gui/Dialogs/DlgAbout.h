// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QDialog>
#include <QTextBrowser>
#include <Gui/MDIView.h>
#include <qlocale.h>

namespace Gui
{
namespace Dialog
{

class Ui_AboutApplication;

class GuiExport AboutDialogFactory
{
public:
    AboutDialogFactory() = default;
    virtual ~AboutDialogFactory();

    virtual QDialog* create(QWidget* parent) const;

    static const AboutDialogFactory* defaultFactory();
    static void setDefaultFactory(AboutDialogFactory* factory);

private:
    static AboutDialogFactory* factory;
};

class GuiExport LicenseView: public Gui::MDIView
{
    Q_OBJECT

public:
    explicit LicenseView(QWidget* parent = nullptr);
    ~LicenseView() override;

    void setSource(const QUrl& url);
    const char* getName() const override
    {
        return "LicenseView";
    }

private:
    QTextBrowser* browser;
};

/** This widget provides the "About dialog" of an application.
 * This shows the current version, the build number and date.
 * \author Werner Mayer
 */
class GuiExport AboutDialog: public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;

protected:
    void setupLabels();
    void showCredits();
    void showLicenseInformation();
    QString getAdditionalLicenseInformation() const;
    void showLibraryInformation();
    void showCollectionInformation();
    void showPrivacyPolicy();
    void showOrHideImage(const QRect& rect);
    void addModuleInfo(QTextStream& inout_str, const QString& modPath, bool& inout_first);

protected:
    QPixmap aboutImage() const;
    virtual void copyToClipboard();
    void linkActivated(const QUrl& link);

private:
    Ui_AboutApplication* ui;
};

}  // namespace Dialog
}  // namespace Gui
