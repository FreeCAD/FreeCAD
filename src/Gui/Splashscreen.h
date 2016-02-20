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

namespace Gui {

class SplashObserver;

/** This widget provides a splash screen that can be shown  during application startup.
 *
 * \author Werner Mayer
 */
class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    SplashScreen(  const QPixmap & pixmap = QPixmap ( ), Qt::WindowFlags f = 0 );
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

/** This widget provides the "About dialog" of an application. 
 * This shows the current version, the build number and date. 
 * \author Werner Mayer
 */
class GuiExport AboutDialog : public QDialog
{
    Q_OBJECT

public:
    AboutDialog(bool showLic, QWidget* parent = 0);
    ~AboutDialog();

protected:
    void setupLabels();

protected Q_SLOTS:
    virtual void on_licenseButton_clicked();
    virtual void on_copyButton_clicked();

private:
    Ui_AboutApplication* ui;
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_SPLASHSCREEN_H
