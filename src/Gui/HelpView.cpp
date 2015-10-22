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
# include <QApplication>
# include <QContextMenuEvent>
# include <QDropEvent>
# include <QGroupBox>
# include <QHBoxLayout>
# include <QHttp>
# include <QLabel>
# include <QMenu>
# include <QMessageBox>
# include <QMimeData>
# include <QProcess>
# include <QTimerEvent>
# include <QToolButton>
# include <QToolTip>
# include <QWhatsThis>
#endif

#include "HelpView.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "WhatsThis.h"
#include "Action.h"
#include "Command.h"


using namespace Gui;
using namespace Gui::DockWnd;

namespace Gui {
namespace DockWnd {

struct TextBrowserResources
{
  QUrl url;
  int type;
};

class TextBrowserPrivate
{
public:
  bool bw, fw;
  int toolTipId;
  QString toolTip;
  QHttp* http;
  QUrl source;
  QList<TextBrowserResources> resources;

  TextBrowserPrivate() : bw(false), fw(false), toolTipId(0)
  {
    http = new QHttp;
  }
  
  ~TextBrowserPrivate()
  {
    delete http;
  }
};

} // namespace DockWnd
} // namespace Gui

/* TRANSLATOR Gui::DockWnd::TextBrowser */

TextBrowser::TextBrowser(QWidget * parent)
  : QTextBrowser(parent)
{
  d = new TextBrowserPrivate;

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setAcceptDrops( TRUE );
  viewport()->setAcceptDrops( TRUE );

  connect( d->http, SIGNAL(done(bool)), this, SLOT(done(bool)));
  connect( d->http, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
  connect( d->http, SIGNAL(responseHeaderReceived(const QHttpResponseHeader &)), this, SLOT(onResponseHeaderReceived(const QHttpResponseHeader &)));
  connect( this, SIGNAL(highlighted(const QString&)), this, SLOT(onHighlighted(const QString&)));
  connect( this, SIGNAL(backwardAvailable(bool)), this, SLOT(setBackwardAvailable(bool)));
  connect( this, SIGNAL(forwardAvailable (bool)), this, SLOT(setForwardAvailable (bool)));
}

TextBrowser::~TextBrowser()
{
  delete d;
}

QString TextBrowser::findUrl(const QUrl &name) const
{
  QString fileName = name.toLocalFile();
  QFileInfo fi(fileName);
  if (fi.isAbsolute())
    return fileName;

  QString slash(QLatin1String("/"));
  QStringList spaths = searchPaths();
  for (QStringList::ConstIterator it = spaths.begin(); it != spaths.end(); ++it) {
    QString path = *it;
    if (!path.endsWith(slash))
      path.append(slash);
    path.append(fileName);
    fi.setFile(path);
    if (fi.isReadable())
      return path;
  }

  QUrl src = source();
  if (src.isEmpty())
    return fileName;

  QFileInfo path(QFileInfo(src.toLocalFile()).absolutePath(), fileName);
  return path.absoluteFilePath();
}

/**
 * Checks whether the resource data must be downloaded via http or
 * it is a file resource on the disk.
 */
QVariant TextBrowser::loadResource ( int type, const QUrl& url )
{
  QString name = url.toString();
  if (url.scheme() == QLatin1String("http") ||
      d->source.scheme() == QLatin1String("http")) {
    return loadHttpResource(type, url);
  } else { // file scheme
    return loadFileResource(type, url);
  }
}

/**
 * Supports only rendering of local files and resources that can be downloaded over
 * the http protocol. In the latter case the download gets started.
 */
void TextBrowser::setSource (const QUrl& url)
{
  bool relativeUrl = url.isRelative();
  if (!relativeUrl)
    d->source = url; // last set absolute url
  QString name = url.toString();
  if (url.scheme() == QLatin1String("http")) {
    // start the download but do not call setSource() of the base
    // class because we must wait until the data are available.
    // The slot done() is invoked automatically then. 
    d->http->setHost(url.host());
    d->http->get(url.path(), 0);
  } else if (d->source.scheme() == QLatin1String("http")) {
    // relative hyperlink in previously downloaded a HTML page 
    d->source = d->source.resolved(url);
    d->http->get(url.path(), 0);
  } else {
    QUrl resolved = url;
#if defined (Q_OS_WIN)
    if (url.scheme() == QLatin1String("file") && !url.isRelative()) {
      QString auth = url.authority();
      QString path = url.path();
      //If we click on a hyperlink with a reference to an absolute file name
      //then we get a string that cannot be used to open the file. So we try 
      //to reproduce the original url.
      if (!auth.isEmpty() && !path.isEmpty()) {
        QString fileName = auth + QLatin1Char(':') + path;
        resolved = QUrl::fromLocalFile(fileName);
      }
    }
#endif
    QTextBrowser::setSource(resolved);
  }
}

/**
 * The URL \a name must be a local file and its contents is returned to the caller. If the file doesn't exist
 * an error message in HTML format or an empty image -- depending on \a type -- is returned to the caller. 
 */
QVariant TextBrowser::loadFileResource(int type, const QUrl& name)
{
  QVariant data;
  QUrl resolved = name;
  if (!QFileInfo(name.toLocalFile()).isAbsolute() && QFileInfo(d->source.toLocalFile()).isAbsolute())
    resolved = d->source.resolved(name);
  QString fileName = findUrl(resolved);
  QFile file(fileName);
  if (file.open(QFile::ReadOnly)) {
    data = file.readAll();
    file.close();
  } else if (type == QTextDocument::HtmlResource) {
      data = QString::fromAscii(
      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
      "<html>"
      "<head>"
      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-16\">"
      "<title>Error</title>"
      "</head>"
      "<body>"
      "<h1>%1</h1>"
      "<div><p><strong>%2</strong></p>"
      "</div></body>"
      "</html>").arg(tr("Could not open file.")).arg(tr("You tried to access the address %1 which is currently unavailable. "
      "Please make sure that the URL exists and try reloading the page.").arg(name.toString()));
  } else if (type == QTextDocument::ImageResource) {
    static const char * empty_xpm[] = {
          "24 24 1 1",
          ". c #C0C0C0",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................"};
    QPixmap px(empty_xpm);
    data.setValue<QPixmap>(px);
  }

  return data;
}

/**
 * Returns the downloaded resource to the caller. In case the resource contains
 * references to further resources (e.g. referenced images in HTML text) we store
 * the URLs in a list and start the download afterwards.
 */
QVariant TextBrowser::loadHttpResource(int type, const QUrl& name)
{
  QVariant data;
  if (type == QTextDocument::ImageResource) {
    TextBrowserResources res;
    res.url = name;
    res.type = type;
    d->resources.push_back(res);
    
    static const char * empty_xpm[] = {
          "24 24 1 1",
          ". c #C0C0C0",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................",
          "........................"};
    QPixmap px(empty_xpm);
    data.setValue<QPixmap>(px);
    return data;
  }

  if (d->http->error() == QHttp::NoError) {
    return d->http->readAll();
  } else {
    if (type == QTextDocument::HtmlResource) {
        data = QString::fromAscii(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
        "<html>"
        "<head>"
        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-16\">"
        "<title>Error</title>"
        "</head>"
        "<body>"
        "<h1>%1</h1>"
        "<div><p><strong>%2</strong></p>"
        "</div></body>"
        "</html>").arg(d->http->errorString()).arg(tr("You tried to access the address %1 which is currently unavailable. "
        "Please make sure that the URL exists and try reloading the page.").arg(name.toString()));
    }
  }

  return data;
}

/**
 * The download has finished. We add the resource to the document now.
 */
void TextBrowser::done( bool /*err*/ )
{
  if (d->resources.isEmpty()/* && d->requestId == d->http->currentId()*/) {
    // set the HTML text
    QTextBrowser::setSource(d->source);
  } else {
    // add the referenced resource to the document
    TextBrowserResources res = d->resources.front();

    QVariant data(d->http->readAll());
    document()->addResource(res.type, res.url, data);
    viewport()->repaint();
    d->resources.pop_front();
  }

  // if we need to download further resources start a http request
  if (!d->resources.isEmpty()) {
    TextBrowserResources res = d->resources.front();
    d->http->get(res.url.toString());
  } else {
    stateChanged(d->source.toString());
  }
}

/**
 * Prints some status information.
 */
void TextBrowser::onStateChanged ( int state )
{
  switch (state) {
    case QHttp::Connecting:
      stateChanged(tr("Connecting to %1").arg(d->source.host()));
      break;
    case QHttp::Sending:
      stateChanged(tr("Sending to %1").arg(d->source.host()));
      break;
    case QHttp::Reading:
      stateChanged(tr("Reading from %1").arg(d->source.host()));
      break;
    case QHttp::Closing:
    case QHttp::Unconnected:
      if (d->http->error() == QHttp::NoError)
        stateChanged(d->source.toString());
      else
        stateChanged(d->http->errorString());
      break;
    default:
      break;
  }
}

/**
 * Checks for the status code and aborts the request if needed.
 */
void TextBrowser::onResponseHeaderReceived(const QHttpResponseHeader &responseHeader)
{
  if (responseHeader.statusCode() != 200) {
    stateChanged(tr("Download failed: %1.").arg(responseHeader.reasonPhrase()));
    d->http->abort();
  }
}

void TextBrowser::onHighlighted(const QString& url)
{
  if (url.isEmpty() && d->toolTipId != 0) {
    killTimer(d->toolTipId);
    d->toolTipId = 0;
  } else if (!url.isEmpty()) {
    d->toolTip = url;
    d->toolTipId = startTimer(1000);
  } else {
    QToolTip::showText(QCursor::pos(), url, this);
  }
}

void TextBrowser::backward()
{
  QTextBrowser::backward();
  reload();
}

void TextBrowser::forward()
{
  QTextBrowser::forward();
  reload();
}

void TextBrowser::setBackwardAvailable( bool b )
{
  d->bw = b;
}

void TextBrowser::setForwardAvailable( bool b )
{
  d->fw = b;
}

void TextBrowser::timerEvent ( QTimerEvent * e )
{
  if (d->toolTipId == e->timerId()) {
    QToolTip::showText(QCursor::pos(), d->toolTip, this);
    killTimer(d->toolTipId);
    d->toolTipId = 0;
  }
}

void TextBrowser::contextMenuEvent ( QContextMenuEvent * e )
{
  QMenu* menu = new QMenu(this);

  QAction* prev = menu->addAction(Gui::BitmapFactory().pixmap("back_pixmap"), tr("Previous"), this, SLOT(backward()));
  prev->setEnabled(d->bw);

  QAction* next = menu->addAction(Gui::BitmapFactory().pixmap("forward_pixmap"), tr("Forward"), this, SLOT(forward()));
  next->setEnabled(d->fw);

  menu->addSeparator();
  menu->addAction(Gui::BitmapFactory().pixmap("home_pixmap"), tr("Home"), this, SLOT(home()));
  menu->addAction(tr("Refresh"), this, SLOT(reload()));
  menu->addSeparator();
  menu->addAction(tr("Copy"), this, SLOT(copy()));
  menu->addAction(tr("Select all"), this, SLOT(selectAll()));

  menu->exec(e->globalPos());
  delete menu;
}

void TextBrowser::dropEvent(QDropEvent  * e)
{
  const QMimeData* mimeData = e->mimeData();
  if ( mimeData->hasFormat(QLatin1String("text/x-action-items")) ) {
    QByteArray itemData = mimeData->data(QLatin1String("text/x-action-items"));
    QDataStream dataStream(&itemData, QIODevice::ReadOnly);

    int ctActions; dataStream >> ctActions;

    // handle the first item only
    QString action;
    dataStream >> action;

    CommandManager& rclMan = Application::Instance->commandManager();
    Command* pCmd = rclMan.getCommandByName(action.toAscii());
    if ( pCmd ) {
      QString info = pCmd->getAction()->whatsThis();
      if ( !info.isEmpty() ) {
        // cannot show help to this command
        info = QString::fromAscii(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">"
        "<html>"
        "<body bgcolor=white text=black alink=red link=darkblue vlink=darkmagenta>"
        "%1"
        "</body>"
        "</html>" ).arg( info );
      } else {
        info = QString::fromAscii(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">"
        "<html>"
        "<body bgcolor=white text=black alink=red link=darkblue vlink=darkmagenta>"
        "<h2>"
        "  %1 '%2'"
        "</h2>"
        "<hr>"
        "</body>"
        "</html>" ).arg(tr("No description for")).arg(action);
      }

      setHtml( info );
    }

    e->setDropAction(Qt::CopyAction);
    e->accept();
  } else if ( mimeData->hasUrls() ) {
    QList<QUrl> uri = mimeData->urls();
    QUrl url = uri.front();
    setSource(url);

    e->setDropAction(Qt::CopyAction);
    e->accept();
  } else {
    e->ignore();
  }
}

void TextBrowser::dragEnterEvent  (QDragEnterEvent * e)
{
  const QMimeData* mimeData = e->mimeData();
  if ( mimeData->hasFormat(QLatin1String("text/x-action-items")) )
    e->accept();
  else if (mimeData->hasUrls())
    e->accept();
  else
    e->ignore();
}

void TextBrowser::dragMoveEvent( QDragMoveEvent *e )
{
  const QMimeData* mimeData = e->mimeData();
  if ( mimeData->hasFormat(QLatin1String("text/x-action-items")) )
    e->accept();
  else if (mimeData->hasUrls())
    e->accept();
  else
    e->ignore();
}


// --------------------------------------------------------------------

/* TRANSLATOR Gui::DockWnd::HelpView */

/**
 *  Constructs a HelpView which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f'. \a home is the start page to show.
 */
HelpView::HelpView( const QString& start,  QWidget* parent )
  : QWidget(parent)
{
  TextBrowser* browser = new TextBrowser(this);
  browser->setFrameStyle( QFrame::Panel | QFrame::Sunken );
  // set the start page now
  if (!start.isEmpty())
    browser->setSource(QUrl::fromLocalFile(start));
  
  QHBoxLayout* layout = new QHBoxLayout();
  layout->setAlignment(Qt::AlignTop);
  layout->setSpacing(1);
  layout->setMargin (1);

  // create the group box for the layout
  QGroupBox* groupBox = new QGroupBox(this);

  // the 'Backward' button
  QToolButton* back = new QToolButton( groupBox );
  back->setIcon( Gui::BitmapFactory().pixmap("back_pixmap") );
  back->setAutoRaise(true);
  back->setToolTip(tr("Previous"));

  // the 'Forward' button
  QToolButton* forward = new QToolButton( groupBox );
  forward->setIcon( Gui::BitmapFactory().pixmap("forward_pixmap") );
  forward->setAutoRaise(true);
  forward->setToolTip(tr("Next"));

  // the 'Home' button
  QToolButton* home = new QToolButton( groupBox );
  home->setIcon( Gui::BitmapFactory().pixmap("home_pixmap") );
  home->setAutoRaise(true);
  home->setToolTip(tr("Home"));

  // the 'Open' button
  QToolButton* open = new QToolButton( groupBox );
  open->setIcon( Gui::BitmapFactory().pixmap("helpopen") );
  open->setAutoRaise(true);
  open->setToolTip(tr("Open"));

  QGridLayout* formLayout = new QGridLayout( this );
  formLayout->setSpacing( 1 );
  formLayout->setMargin ( 1 );

  // add the buttons and the space
  layout->addWidget( back );
  layout->addWidget( forward );
  layout->addWidget( home );
  layout->addWidget( open );
  QSpacerItem* spacer = new QSpacerItem( 0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
  layout->addItem( spacer );
  groupBox->setLayout(layout);

  label = new QLabel(this);
  label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
  label->setText(start);

  // add the button group with its elements and the browser to the layout
  formLayout->addWidget( groupBox, 0, 0 );
  formLayout->addWidget( browser, 1, 0 );
  formLayout->addWidget( label, 2, 0 );

  connect( this, SIGNAL(setSource( const QUrl& )), browser, SLOT(setSource( const QUrl& )));
  connect( browser, SIGNAL(stateChanged(const QString&)), this, SLOT(onStateChanged(const QString&)));
  connect( browser, SIGNAL(backwardAvailable(bool)), back, SLOT(setEnabled(bool)));
  connect( browser, SIGNAL(forwardAvailable (bool)), forward, SLOT(setEnabled(bool)));
  connect( browser, SIGNAL(startExternalBrowser(const QString&)), 
           this,        SLOT  (startExternalBrowser(const QString&)));
  connect( back,    SIGNAL(clicked()), browser, SLOT(backward()));
  connect( browser, SIGNAL(backwardAvailable(bool) ), back, SLOT(setEnabled(bool) ) );
  connect( forward, SIGNAL(clicked()), browser, SLOT(forward()));
  connect( browser, SIGNAL(forwardAvailable(bool) ), forward, SLOT(setEnabled(bool) ) );
  connect( home,    SIGNAL(clicked()), browser, SLOT(home()));
  connect( open,    SIGNAL(clicked()), this, SLOT(openHelpFile()));
  forward->setEnabled( false );
  back->setEnabled( false );
  qApp->installEventFilter(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
HelpView::~HelpView()
{
  // no need to delete child widgets, Qt does it all for us
  qApp->removeEventFilter(this);
}

/**
 * Sets the file source \a src to the help view's text browser.
 */
void HelpView::setFileSource( const QString& src )
{
  setSource( src );
}

/**
 * Opens a file dialog to specify a help page to open.
 */
void HelpView::openHelpFile()
{
  QString fn = QFileDialog::getOpenFileName(this, tr("Open file"), QString(), tr("All HTML files (*.html *.htm)"));
  if ( !fn.isEmpty() )
    setSource( QUrl::fromLocalFile(fn) );
}

/** 
 * Looks up into preferences to start an external browser to render sites which this class cannot do
 * right now. If now browser has been specified so far or the start fails an error message appears.
 */
void HelpView::startExternalBrowser( const QString& url )
{
  ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
      ("User parameter:BaseApp/Preferences/OnlineHelp");
  QString browser = QString::fromUtf8(hGrp->GetASCII( "ExternalBrowser", "" ).c_str());

  if (browser.isEmpty())
  {
    QMessageBox::critical( this, tr("External browser"), tr("No external browser found. Specify in preferences, please") );
    return;
  }

  // create the command to execute
  QStringList arguments;
  arguments << url;
  
  if (!QProcess::startDetached(browser, arguments))
  {
    QMessageBox::critical( this, tr("External browser"), tr("Starting of %1 failed").arg( browser ) );
  }
}

void HelpView::onStateChanged(const QString& state)
{
  label->setText(state);
}

bool HelpView::eventFilter ( QObject* o, QEvent* e )
{
  // Handles What's This click events
  if (e->type() == QEvent::WhatsThisClicked) {
    QString url = static_cast<QWhatsThisClickedEvent*>(e)->href();
    setSource(QUrl::fromLocalFile(url));
    QWhatsThis::hideText();
    return true;
  }

  return QWidget::eventFilter( o, e );
}

#include "moc_HelpView.cpp"

