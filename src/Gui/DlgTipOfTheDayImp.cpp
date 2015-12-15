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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <qcheckbox.h>
# include <qfile.h>
# include <qlabel.h>
# include <qlayout.h>
# include <qtextstream.h>
# include <QHttp>
#endif

#include "DlgTipOfTheDayImp.h"
#include "Application.h"
#include "MainWindow.h"

#include <Base/Parameter.h>
#include <Base/Console.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgTipOfTheDayImp */

/**
 *  Constructs a DlgTipOfTheDayImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgTipOfTheDayImp::DlgTipOfTheDayImp( QWidget* parent, Qt::WindowFlags fl )
  : QDialog( parent, fl | Qt::WindowTitleHint | Qt::WindowSystemMenuHint ),
  WindowParameter("General")
{
    setupUi(this);
    _http = new QHttp;
    connect(_http, SIGNAL(done(bool)), this, SLOT(onDone(bool)));
    connect(_http, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
    connect(_http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), 
            this, SLOT(onResponseHeaderReceived(const QHttpResponseHeader &)));

    bool tips = getWindowParameter()->GetBool("Tipoftheday", true);
    checkShowTips->setChecked(tips);

    // Since the resize mode of DlgTipOfTheDayBase does not
    // work properly so set this by hand 
    setMinimumSize(QSize(320, 250));
    layout()->setSizeConstraint( QLayout::SetNoConstraint );

    reload();
    on_buttonNextTip_clicked();
}

/** Destroys the object and frees any allocated resources */
DlgTipOfTheDayImp::~DlgTipOfTheDayImp()
{
    delete _http;
    getWindowParameter()->SetBool("Tipoftheday", checkShowTips->isChecked());
}

/** Shows next tip taken from the Tip-of-the-day site. */
void DlgTipOfTheDayImp::on_buttonNextTip_clicked()
{
    _iCurrentTip = (_iCurrentTip + 1) % _lTips.size();
    textTip->setText(_lTips[_iCurrentTip]);
}

/** Reloads all tips from Tip-of-the-day site. */
void DlgTipOfTheDayImp::reload()
{
    // set the host and start the download
    _http->setHost(QLatin1String("www.freecadweb.org"));
    _http->get(QLatin1String("/wiki/index.php?title=Tip_of_the_day"), 0);

    _iCurrentTip = 0;
    _lTips << tr("If you want to learn more about FreeCAD you must go to %1 or press the Help item in the Help menu.")
        .arg(QLatin1String("<a href=\"http://www.freecadweb.org/wiki/\">"
                           "http://www.freecadweb.org/wiki/</a>"));
}

void DlgTipOfTheDayImp::onResponseHeaderReceived(const QHttpResponseHeader & responseHeader)
{
    if (responseHeader.statusCode() != 200) {
        QString msg = tr("Download failed: %1\n").arg(responseHeader.reasonPhrase());
        Base::Console().Log(msg.toLatin1());
        _http->abort();
    }
}

void DlgTipOfTheDayImp::onDone(bool err)
{
    if (err)
        return;

    // get the page and search for the tips section
    QString text = QString::fromLatin1(_http->readAll());
    QRegExp rx(QLatin1String("<p>You find the latest information.+<div class=\"printfooter\">"));
    if (rx.indexIn(text) > -1) {
        // the text of interest
        text = rx.cap();
        rx.setPattern(QLatin1String("<ul><li>.+</li></ul>\n"));
        rx.setMinimal(true);
        _lTips += text.split(rx, QString::SkipEmptyParts);
    }
}

/**
 * Prints some status information.
 */
void DlgTipOfTheDayImp::onStateChanged (int state)
{
    switch (state) {
        case QHttp::Connecting:
            Base::Console().Log("Connecting to host...\n");
            break;
        case QHttp::Sending:
            Base::Console().Log("Sending to host...\n");
            break;
        case QHttp::Reading:
            Base::Console().Log("Reading from host...\n");
            break;
        case QHttp::Closing:
        case QHttp::Unconnected:
            Base::Console().Log("%s\n",(const char*)_http->errorString().toLatin1());
            break;
        default:
            break;
  }
}

#include "moc_DlgTipOfTheDayImp.cpp"
