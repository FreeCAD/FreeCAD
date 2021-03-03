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


#ifndef GUI_SPLASHSCREEN_H
#define GUI_SPLASHSCREEN_H

#include <QSplashScreen>
#include <QDialog>
#include <QTextBrowser>
#include <Gui/MDIView.h>

namespace Gui {

class SplashObserver;

/** This widget provides a splash screen that can be shown during application startup.
 *
 * \author Werner Mayer
 */
class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    SplashScreen(  const QPixmap & pixmap = QPixmap ( ), Qt::WindowFlags f = Qt::WindowFlags() );
    ~SplashScreen();

protected:
    void drawContents ( QPainter * painter );

private:
    SplashObserver* messages;
};

namespace Dialog {
class Ui_AboutApplication;

class GuiExport AboutDialogFactory
{
public:
    AboutDialogFactory() {}
    virtual ~AboutDialogFactory();

    virtual QDialog *create(QWidget *parent) const;

    static const AboutDialogFactory *defaultFactory();
    static void setDefaultFactory(AboutDialogFactory *factory);

private:
    static AboutDialogFactory* factory;
};

class GuiExport LicenseView : public Gui::MDIView
{
    Q_OBJECT

public:
    LicenseView(QWidget* parent=0);
    virtual ~LicenseView();

    void setSource(const QUrl & url);
    const char *getName(void) const {
        return "LicenseView";
    }

private:
    QTextBrowser* browser;
};

/** This widget provides the "About dialog" of an application.
 * This shows the current version, the build number and date.
 * \author Werner Mayer
 */
class GuiExport AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget* parent = 0);
    ~AboutDialog() = default;

protected:
    void setupLabels();
    void showCredits();
    void showLicenseInformation();
    void showLibraryInformation();
    void showCollectionInformation();
    
    QString getAdditionalLicenseInformation();

protected Q_SLOTS:
    virtual void on_copyButton_clicked();
    void linkActivated(const QUrl& link);

private:
    std::unique_ptr<Ui_AboutApplication> ui;
    class LibraryInfo;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_SPLASHSCREEN_H
