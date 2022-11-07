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

#include <QDialog>
#include <QSplashScreen>
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
    explicit SplashScreen(  const QPixmap & pixmap = QPixmap ( ), Qt::WindowFlags f = Qt::WindowFlags() );
    ~SplashScreen() override;

    void showMessage(const QString &message);
protected:
    void drawContents ( QPainter * painter ) override;

private:
    // these three functions can, and likely should, be put someplace else
    // this is not stuff a splash screen should care about
    static int alignStrToInt(std::string str);
    static std::string fromConfig(const std::map<std::string, std::string>& config, std::string
                                                                                      value);
    static QColor colorFromString(const std::string str);

    SplashObserver* splashObserver;
    int alignment {Qt::AlignBottom | Qt::AlignLeft};
    QColor textColor {Qt::black};
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
    static AboutDialogFactory* factory {};
};

class GuiExport LicenseView : public Gui::MDIView
{
    Q_OBJECT

public:
    explicit LicenseView(QWidget* parent=nullptr);
    ~LicenseView() override;

    void setSource(const QUrl & url);
    const char *getName() const override {
        return "LicenseView";
    }

private:
    QTextBrowser* browser;
};

class SummaryReport{
public:
    SummaryReport();
    ~SummaryReport();
    void addItem(const std::string item);
    std::string asStdString();
    std::string codeWrap(std::string wrappee);
private:
    std::vector<std::string> items {};
};

/** This widget provides the "About dialog" of an application.
 * This shows the current version, the build number and date.
 * \author Werner Mayer
 */
class GuiExport AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(bool showLic, QWidget* parent = nullptr);
    ~AboutDialog() override;

protected:
    void setupLabels();
    void showCredits();
    void showLicenseInformation();
    QString getAdditionalLicenseInformation() const;
    void showLibraryInformation();
    void showCollectionInformation();
    void showOrHideImage(const QRect& rect);

protected Q_SLOTS:
    virtual void on_copyButton_clicked();
    void linkActivated(const QUrl& link);

private:
    Ui_AboutApplication* ui;
    QString libraryInfoAsHtml();
    QTextBrowser *showCommon(const char *name,
                             const char *tabName,
                             const bool openExternalLinks = false,
                             const bool openLinks = true);
    std::string makeSummaryReport();

    SummaryReport summaryReport {};
};

} // namespace Dialog
} // namespace Gui


#endif // GUI_SPLASHSCREEN_H
