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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QApplication>
# include <QButtonGroup>
# include <QCompleter>
# include <QComboBox>
# include <QDesktopServices>
# include <QDialogButtonBox>
# include <QDir>
# include <QGridLayout>
# include <QGroupBox>
# include <QLineEdit>
# include <QPushButton>
# include <QRadioButton>
# include <QStyle>
# include <QUrl>
# include <QResizeEvent>
#endif

#include <Base/Parameter.h>
#include <App/Application.h>

#include "FileDialog.h"
#include "MainWindow.h"
#include "BitmapFactory.h"
#include "Tools.h"

using namespace Gui;

namespace {
bool dontUseNativeDialog()
{
#if defined(USE_QT_FILEDIALOG)
    bool notNativeDialog = true;
#else
    bool notNativeDialog = false;
#endif

    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Dialog");
    notNativeDialog = group->GetBool("DontUseNativeDialog", notNativeDialog);
    return notNativeDialog;
}
}

/* TRANSLATOR Gui::FileDialog */

FileDialog::FileDialog(QWidget * parent)
  : QFileDialog(parent)
{
    connect(this, SIGNAL(filterSelected(const QString&)),
            this, SLOT(onSelectedFilter(const QString&)));
}

FileDialog::~FileDialog()
{
}

void FileDialog::onSelectedFilter(const QString& /*filter*/)
{
    QRegExp rx(QLatin1String("\\(\\*.(\\w+)"));
    QString suf = selectedNameFilter();
    if (rx.indexIn(suf) >= 0) {
        suf = rx.cap(1);
        setDefaultSuffix(suf);
    }
}

bool FileDialog::hasSuffix(const QString& ext) const
{
    QRegExp rx(QString::fromLatin1("\\*.(%1)\\W").arg(ext));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    QStringList filters = nameFilters();
    for (QStringList::iterator it = filters.begin(); it != filters.end(); ++it) {
        QString str = *it;
        if (rx.indexIn(str) != -1) {
            return true;
        }
    }

    return false;
}

void FileDialog::accept()
{
    // When saving to a file make sure that the entered filename ends with the selected
    // file filter
    if (acceptMode() == QFileDialog::AcceptSave) {
        QStringList files = selectedFiles();
        if (!files.isEmpty()) {
            QString ext = this->defaultSuffix();
            QString file = files.front();
            QString suffix = QFileInfo(file).suffix();
            // #0001928: do not add a suffix if a file with suffix is entered
            // #0002209: make sure that the entered suffix is part of one of the filters
            if (!ext.isEmpty() && (suffix.isEmpty() || !hasSuffix(suffix))) {
                file = QString::fromLatin1("%1.%2").arg(file, ext);
                // That's the built-in line edit
                QLineEdit* fileNameEdit = this->findChild<QLineEdit*>(QString::fromLatin1("fileNameEdit"));
                if (fileNameEdit)
                    fileNameEdit->setText(file);
            }
        }
    }
    QFileDialog::accept();
}

/**
 * This is a convenience static function that will return a file name selected by the user. The file does not have to exist.
 */
QString FileDialog::getSaveFileName (QWidget * parent, const QString & caption, const QString & dir,
                                     const QString & filter, QString * selectedFilter, Options options)
{
    QString dirName = dir;
    bool hasFilename = false;
    if (dirName.isEmpty()) {
        dirName = getWorkingDirectory();
    } else {
        QFileInfo fi(dir);
        if (fi.isRelative()) {
            dirName = getWorkingDirectory();
            dirName += QLatin1String("/");
            dirName += fi.fileName();
        }
        if (!fi.fileName().isEmpty()) {
            hasFilename = true;
        }

        // get the suffix for the filter: use the selected filter if there is one,
        // otherwise find the first valid suffix in the complete list of filters
        const QString *filterToSearch;
        if (selectedFilter != nullptr) {
            filterToSearch = selectedFilter;
        }
        else {
            filterToSearch = &filter;
        }
        QRegExp rx;
        rx.setPattern(QLatin1String("\\s(\\(\\*\\.\\w{1,})\\W"));
        int index = rx.indexIn(*filterToSearch);
        if (index != -1) {
            // get the suffix with the leading dot
            QString suffix = filterToSearch->mid(index+3, rx.matchedLength()-4);
            if (fi.suffix().isEmpty())
                dirName += suffix;
        }
    }

    QString windowTitle = caption;
    if (windowTitle.isEmpty())
        windowTitle = FileDialog::tr("Save as");

    // NOTE: We must not change the specified file name afterwards as we may return the name of an already
    // existing file. Hence we must extract the first matching suffix from the filter list and append it
    // before showing the file dialog.
    QString file;
    if (dontUseNativeDialog()) {
        QList<QUrl> urls;

        options |= QFileDialog::DontUseNativeDialog;
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
        urls << QUrl::fromLocalFile(getWorkingDirectory());
        urls << QUrl::fromLocalFile(restoreLocation());
        urls << QUrl::fromLocalFile(QDir::currentPath());

        FileDialog dlg(parent);
        dlg.setWindowTitle(windowTitle);
        dlg.setSidebarUrls(urls);
        auto iconprov = std::make_unique<FileIconProvider>();
        dlg.setIconProvider(iconprov.get());
        dlg.setFileMode(QFileDialog::AnyFile);
        dlg.setAcceptMode(QFileDialog::AcceptSave);
        dlg.setDirectory(dirName);
        if (hasFilename)
            dlg.selectFile(dirName);
        dlg.setOptions(options);
        dlg.setNameFilters(filter.split(QLatin1String(";;")));
        if (selectedFilter && !selectedFilter->isEmpty())
            dlg.selectNameFilter(*selectedFilter);
        dlg.onSelectedFilter(dlg.selectedNameFilter());
        dlg.setOption(QFileDialog::HideNameFilterDetails, false);
        dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
        if (dlg.exec() == QDialog::Accepted) {
            if (selectedFilter)
                *selectedFilter = dlg.selectedNameFilter();
            file = dlg.selectedFiles().front();
        }
    }
    else {
        file = QFileDialog::getSaveFileName(parent, windowTitle, dirName, filter, selectedFilter, options);
        file = QDir::fromNativeSeparators(file);
    }

    if (!file.isEmpty()) {
        setWorkingDirectory(file);
        return file;
    } else {
        return QString();
    }
}

/**
 * This is a convenience static function that will return an existing directory selected by the user.
 */
QString FileDialog::getExistingDirectory( QWidget * parent, const QString & caption, const QString & dir, Options options )
{
    QString path = QFileDialog::getExistingDirectory(parent, caption, dir, options);
    // valid path was selected
    if ( !path.isEmpty() ) {
        QDir d(path);
        path = d.path(); // get path in Qt manner
    }

    return path;
}

/**
 * This is a convenience static function that returns an existing file selected by the user.
 * If the user pressed Cancel, it returns a null string.
 */
QString FileDialog::getOpenFileName(QWidget * parent, const QString & caption, const QString & dir,
                                    const QString & filter, QString * selectedFilter, Options options)
{
    QString dirName = dir;
    if (dirName.isEmpty()) {
        dirName = getWorkingDirectory();
    }

    QString windowTitle = caption;
    if (windowTitle.isEmpty())
        windowTitle = FileDialog::tr("Open");

    QString file;
    if (dontUseNativeDialog()) {
        QList<QUrl> urls;

        options |= QFileDialog::DontUseNativeDialog;
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
        urls << QUrl::fromLocalFile(getWorkingDirectory());
        urls << QUrl::fromLocalFile(restoreLocation());
        urls << QUrl::fromLocalFile(QDir::currentPath());

        FileDialog dlg(parent);
        dlg.setWindowTitle(windowTitle);
        dlg.setSidebarUrls(urls);
        dlg.setIconProvider(new FileIconProvider());
        dlg.setFileMode(QFileDialog::ExistingFile);
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        dlg.setDirectory(dirName);
        dlg.setOptions(options);
        dlg.setNameFilters(filter.split(QLatin1String(";;")));
        dlg.setOption(QFileDialog::HideNameFilterDetails, false);
        if (selectedFilter && !selectedFilter->isEmpty())
            dlg.selectNameFilter(*selectedFilter);
        if (dlg.exec() == QDialog::Accepted) {
            if (selectedFilter)
                *selectedFilter = dlg.selectedNameFilter();
            file = dlg.selectedFiles().front();
        }
    }
    else {
        file = QFileDialog::getOpenFileName(parent, windowTitle, dirName, filter, selectedFilter, options);
        file = QDir::fromNativeSeparators(file);
    }

    if (!file.isEmpty()) {
        setWorkingDirectory(file);
        return file;
    } else {
        return QString();
    }
}

/**
 * This is a convenience static function that will return one or more existing files selected by the user.
 */
QStringList FileDialog::getOpenFileNames (QWidget * parent, const QString & caption, const QString & dir,
                                          const QString & filter, QString * selectedFilter, Options options)
{
    QString dirName = dir;
    if (dirName.isEmpty()) {
        dirName = getWorkingDirectory();
    }

    QString windowTitle = caption;
    if (windowTitle.isEmpty())
        windowTitle = FileDialog::tr("Open");

    QStringList files;
    if (dontUseNativeDialog()) {
        QList<QUrl> urls;

        options |= QFileDialog::DontUseNativeDialog;
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        urls << QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation));
        urls << QUrl::fromLocalFile(getWorkingDirectory());
        urls << QUrl::fromLocalFile(restoreLocation());
        urls << QUrl::fromLocalFile(QDir::currentPath());

        FileDialog dlg(parent);
        dlg.setWindowTitle(windowTitle);
        dlg.setSidebarUrls(urls);
        dlg.setIconProvider(new FileIconProvider());
        dlg.setFileMode(QFileDialog::ExistingFiles);
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        dlg.setDirectory(dirName);
        dlg.setOptions(options);
        dlg.setNameFilters(filter.split(QLatin1String(";;")));
        dlg.setOption(QFileDialog::HideNameFilterDetails, false);
        if (selectedFilter && !selectedFilter->isEmpty())
            dlg.selectNameFilter(*selectedFilter);
        if (dlg.exec() == QDialog::Accepted) {
            if (selectedFilter)
                *selectedFilter = dlg.selectedNameFilter();
            files = dlg.selectedFiles();
        }
    }
    else {
        files = QFileDialog::getOpenFileNames(parent, windowTitle, dirName, filter, selectedFilter, options);
        for (QStringList::iterator it = files.begin(); it != files.end(); ++it) {
            *it = QDir::fromNativeSeparators(*it);
        }
    }

    if (!files.isEmpty()) {
        setWorkingDirectory(files.front());
    }

    return files;
}

QString FileDialog::workingDirectory;

/**
 * Returns the working directory for the file dialog. This path can be used in
 * combination with getSaveFileName(), getOpenFileName(), getOpenFileNames() or
 * getExistingDirectory() to open the dialog in this path.
 */
QString FileDialog::getWorkingDirectory()
{
    return workingDirectory;
}

/**
 * Sets the working directory to \a dir for the file dialog.
 * If \a dir is a file then the path only is taken.
 * getWorkingDirectory() returns the working directory.
 */
void FileDialog::setWorkingDirectory(const QString& dir)
{
    QString dirName = dir;
    if (!dir.isEmpty()) {
        QFileInfo info(dir);
        if (!info.exists() || info.isFile())
            dirName = info.absolutePath();
        else
            dirName = info.absoluteFilePath();
    }

    workingDirectory = dirName;
    saveLocation(dirName);
}

/*!
 * \brief Return the last location where a file save or load dialog was used.
 * \return QString
 */
QString FileDialog::restoreLocation()
{
    std::string path = App::GetApplication().Config()["UserHomePath"];
    Base::Reference<ParameterGrp> hPath = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("Preferences")->GetGroup("General");
    std::string dir = hPath->GetASCII("FileOpenSavePath", path.c_str());
    QFileInfo fi(QString::fromUtf8(dir.c_str()));
    if (!fi.exists())
        dir = path;
    return QString::fromUtf8(dir.c_str());
}

/*!
 * \brief Save the last location where a file save or load dialog was used.
 * \param dirName
 */
void FileDialog::saveLocation(const QString& dirName)
{
    Base::Reference<ParameterGrp> hPath = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("Preferences")->GetGroup("General");
    hPath->SetASCII("FileOpenSavePath", dirName.toUtf8());
}

// ======================================================================

/* TRANSLATOR Gui::FileOptionsDialog */

FileOptionsDialog::FileOptionsDialog( QWidget* parent, Qt::WindowFlags fl )
  : QFileDialog( parent, fl )
  , extensionPos(ExtensionRight)
{
    extensionButton = new QPushButton( this );
    extensionButton->setText( tr( "Extended" ) );

    setOption(QFileDialog::DontUseNativeDialog);

    // This is an alternative to add the button to the grid layout
    //QDialogButtonBox* box = this->findChild<QDialogButtonBox*>();
    //box->addButton(extensionButton, QDialogButtonBox::ActionRole);

    //search for the grid layout and add the new button
    QGridLayout* grid = this->findChild<QGridLayout*>();
    grid->addWidget(extensionButton, 4, 2, Qt::AlignLeft);

    connect(extensionButton, SIGNAL(clicked()), this, SLOT(toggleExtension()));
}

FileOptionsDialog::~FileOptionsDialog()
{
}

void FileOptionsDialog::accept()
{
    // Fixes a bug of the default implementation when entering an asterisk
    QLineEdit* filename = this->findChild<QLineEdit*>();
    QString fn = filename->text();
    if (fn.startsWith(QLatin1String("*"))) {
        QFileInfo fi(fn);
        QString ext = fi.suffix();
        ext.prepend(QLatin1String("*."));
        QStringList filters = this->nameFilters();
        bool ok=false;
        // Compare the given suffix with the suffixes of all filters
        QString filter;
        for (QStringList::ConstIterator it = filters.begin(); it != filters.end(); ++it) {
            if ((*it).contains(ext)) {
                filter = *it;
                ok = true;
                break;
            }
        }

        // if no appropriate filter was found the add the 'All files' filter
        if (!ok) {
            filter = tr("All files (*.*)");
            filters << filter;
            setNameFilters(filters);
        }

        // empty the line edit
        filename->blockSignals(true);
        filename->clear();
        filename->blockSignals(false);
        selectNameFilter(filter);

        return;
    }
    else if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        QString ext = fi.completeSuffix();
        QRegExp rx(QLatin1String("\\(\\*.(\\w+)"));
        QString suf = selectedNameFilter();
        if (rx.indexIn(suf) >= 0)
            suf = rx.cap(1);
        if (ext.isEmpty())
            setDefaultSuffix(suf);
        else if (ext.toLower() != suf.toLower()) {
            fn = QString::fromLatin1("%1.%2").arg(fn, suf);
            selectFile(fn);
            // That's the built-in line edit (fixes Debian bug #811200)
            QLineEdit* fileNameEdit = this->findChild<QLineEdit*>(QString::fromLatin1("fileNameEdit"));
            if (fileNameEdit)
                fileNameEdit->setText(fn);
        }
    }

    QFileDialog::accept();
}

void FileOptionsDialog::toggleExtension()
{
    if (extensionWidget) {
        bool showIt = !extensionWidget->isVisible();
        if (showIt) {
            oldSize = size();
            QSize s(extensionWidget->sizeHint()
                   .expandedTo(extensionWidget->minimumSize())
                   .boundedTo(extensionWidget->maximumSize()));
            if (extensionPos == ExtensionRight) {
                setFixedSize(width() + s.width(), height());
            }
            else {
                setFixedSize(width(), height() + s.height());
            }

            extensionWidget->show();
        }
        else {
            extensionWidget->hide();
            setFixedSize(oldSize);
        }
    }
}

void FileOptionsDialog::setOptionsWidget(FileOptionsDialog::ExtensionPosition pos, QWidget* w, bool show)
{
    extensionPos = pos;
    extensionWidget = w;
    if (extensionWidget->parentWidget() != this)
        extensionWidget->setParent(this);

    QGridLayout* grid = this->findChild<QGridLayout*>();

    if (extensionPos == ExtensionRight) {
        int cols = grid->columnCount();
        grid->addWidget(extensionWidget, 0, cols, -1, -1);
        setMinimumHeight(extensionWidget->height());
    }
    else if (extensionPos == ExtensionBottom) {
        int rows = grid->rowCount();
        grid->addWidget(extensionWidget, rows, 0, -1, -1);
        setMinimumWidth(extensionWidget->width());
    }

    // Instead of resizing the dialog we can fix the layout size.
    // This however, doesn't work nicely when the extension widget
    // is higher/wider than the dialog.
    //grid->setSizeConstraint(QLayout::SetFixedSize);

    oldSize = size();
    w->hide();
    if (show)
        toggleExtension();
}

QWidget* FileOptionsDialog::getOptionsWidget() const
{
    return extensionWidget;
}

// ======================================================================

/**
 * Constructs an empty file icon provider called \a name, with the parent \a parent.
 */
FileIconProvider::FileIconProvider()
{
}

FileIconProvider::~FileIconProvider()
{
}

QIcon FileIconProvider::icon(IconType type) const
{
    return QFileIconProvider::icon(type);
}

QIcon FileIconProvider::icon(const QFileInfo & info) const
{
    if (info.suffix().toLower() == QLatin1String("fcstd")) {
        // return QApplication::windowIcon();
        return QIcon(QString::fromLatin1(":/icons/freecad-doc.png"));
    }
    else if (info.suffix().toLower().startsWith(QLatin1String("fcstd"))) {
        QIcon icon(QString::fromLatin1(":/icons/freecad-doc.png"));
        QIcon darkIcon;
        int w = QApplication::style()->pixelMetric(QStyle::PM_ListViewIconSize);
        darkIcon.addPixmap(icon.pixmap(w, w, QIcon::Disabled, QIcon::Off), QIcon::Normal, QIcon::Off);
        darkIcon.addPixmap(icon.pixmap(w, w, QIcon::Disabled, QIcon::On ), QIcon::Normal, QIcon::On );
        return darkIcon;
    }
    return QFileIconProvider::icon(info);
}

QString FileIconProvider::type(const QFileInfo & info) const
{
    return QFileIconProvider::type(info);
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::FileChooser */

/**
 * Constructs a file chooser called \a name with the parent \a parent.
 */
FileChooser::FileChooser ( QWidget * parent )
  : QWidget(parent)
  , md( File )
  , accMode( AcceptOpen )
  , _filter( QString() )
{
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setMargin( 0 );
    layout->setSpacing( 2 );

    lineEdit = new QLineEdit ( this );
    completer = new QCompleter ( this );
    completer->setMaxVisibleItems( 12 );
    fs_model = new QFileSystemModel( completer );
    fs_model->setRootPath(QString::fromUtf8(""));
    completer->setModel( fs_model );
    lineEdit->setCompleter( completer );

    layout->addWidget( lineEdit );

    connect(lineEdit, SIGNAL(textChanged(const QString &)),
            this, SIGNAL(fileNameChanged(const QString &)));

    connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinished()));

    button = new QPushButton(QLatin1String("..."), this);

#if defined (Q_OS_MAC)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif

    layout->addWidget(button);

    connect( button, SIGNAL(clicked()), this, SLOT(chooseFile()));

    setFocusProxy(lineEdit);
}

FileChooser::~FileChooser()
{
}

void FileChooser::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

/**
 * \property FileChooser::fileName
 *
 * This property holds the file name.
 * Set this property's value with setFileName() and get this property's value with fileName().
 *
 * \sa fileName(), setFileName().
 */
QString FileChooser::fileName() const
{
    return lineEdit->text();
}

void FileChooser::editingFinished()
{
    QString le_converted = QDir::fromNativeSeparators(lineEdit->text());
    lineEdit->setText(le_converted);
    FileDialog::setWorkingDirectory(le_converted);
    fileNameSelected(le_converted);
}

/**
 * Sets the file name \a s.
 */
void FileChooser::setFileName( const QString& s )
{
    lineEdit->setText( s );
}

/**
 * Opens a FileDialog to choose either a file or a directory in dependency of the
 * value of the Mode property.
 */
void FileChooser::chooseFile()
{
    QString prechosenDirectory = lineEdit->text();
    if (prechosenDirectory.isEmpty()) {
        prechosenDirectory = FileDialog::getWorkingDirectory();
    }

    QFileDialog::Options dlgOpt;
    if (dontUseNativeDialog()) {
        dlgOpt = QFileDialog::DontUseNativeDialog;
    }

    QString fn;
    if ( mode() == File ) {
        if (acceptMode() == AcceptOpen)
            fn = QFileDialog::getOpenFileName(this, tr( "Select a file" ), prechosenDirectory, _filter, 0, dlgOpt);
        else
            fn = QFileDialog::getSaveFileName(this, tr( "Select a file" ), prechosenDirectory, _filter, 0, dlgOpt);
    } else {
        QFileDialog::Options option = QFileDialog::ShowDirsOnly | dlgOpt;
        fn = QFileDialog::getExistingDirectory( this, tr( "Select a directory" ), prechosenDirectory,option );
    }

    if (!fn.isEmpty()) {
        fn = QDir::fromNativeSeparators(fn);
        lineEdit->setText(fn);
        FileDialog::setWorkingDirectory(fn);
        fileNameSelected(fn);
    }
}

/**
 * \property FileChooser::mode
 *
 * This property holds whether the widgets selects either a file or a directory.
 * The default value of chooseFile is set to File.
 *
 * \sa chooseFile(), mode(), setMode().
 */
FileChooser::Mode FileChooser::mode() const
{
    return md;
}

/**
 * If \a m is File the widget is set to choose a file, otherwise it is set to
 * choose a directory.
 */
void FileChooser::setMode( FileChooser::Mode m )
{
    md = m;
}

/**
 * \property FileChooser::filter
 *
 * This property holds the set filter to choose a file. This property is used only if
 * FileChooser::Mode is set to File.
 *
 * \sa chooseFile(), filter(), setFilter().
 */
QString FileChooser::filter() const
{
    return _filter;
}

/**
 * Sets the filter for choosing a file.
 */
void FileChooser::setFilter ( const QString& filter )
{
    _filter = filter;
}

/**
 * Sets the browse button's text to \a txt.
 */
void FileChooser::setButtonText( const QString& txt )
{
    button->setText( txt );
    int w1 = 2 * QtTools::horizontalAdvance(button->fontMetrics(), txt);
    int w2 = 2 * QtTools::horizontalAdvance(button->fontMetrics(), QLatin1String(" ... "));
    button->setFixedWidth( (w1 > w2 ? w1 : w2) );
}

/**
 * Returns the browse button's text.
 */
QString FileChooser::buttonText() const
{
    return button->text();
}


// ----------------------------------------------------------------------

/* TRANSLATOR Gui::SelectModule */

SelectModule::SelectModule (const QString& type, const SelectModule::Dict& types, QWidget * parent)
  : QDialog(parent, Qt::WindowTitleHint)
{
    setWindowTitle(tr("Select module"));
    groupBox = new QGroupBox(this);
    groupBox->setTitle(tr("Open %1 as").arg(type));

    group = new QButtonGroup(this);
    gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(9);

    gridLayout1 = new QGridLayout(groupBox);
    gridLayout1->setSpacing(6);
    gridLayout1->setMargin(9);

    int index = 0;
    for (SelectModule::Dict::const_iterator it = types.begin(); it != types.end(); ++it) {
        QRadioButton* button = new QRadioButton(groupBox);

        QRegExp rx;
        QString filter = it.key();
        QString module = it.value();

        // ignore file types in (...)
        rx.setPattern(QLatin1String("\\s+\\([\\w\\*\\s\\.]+\\)$"));
        int pos = rx.indexIn(filter);
        if (pos != -1) {
            filter = filter.left(pos);
        }

        // ignore Gui suffix in module name
        rx.setPattern(QLatin1String("Gui$"));
        pos = rx.indexIn(module);
        if (pos != -1) {
            module = module.left(pos);
        }

        button->setText(QString::fromLatin1("%1 (%2)").arg(filter, module));
        button->setObjectName(it.value());
        gridLayout1->addWidget(button, index, 0, 1, 1);
        group->addButton(button, index);
        index++;
    }

    gridLayout->addWidget(groupBox, 0, 0, 1, 1);
    spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem, 1, 0, 1, 1);

    hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);
    spacerItem1 = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem1);

    okButton = new QPushButton(this);
    okButton->setObjectName(QString::fromUtf8("okButton"));
    okButton->setText(tr("Select"));
    okButton->setEnabled(false);

    hboxLayout->addWidget(okButton);
    gridLayout->addLayout(hboxLayout, 2, 0, 1, 1);

    // connections
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(group, SIGNAL(buttonClicked(int)), this, SLOT(onButtonClicked()));
}

SelectModule::~SelectModule()
{
}

void SelectModule::accept()
{
    if (group->checkedButton())
        QDialog::accept();
}

void SelectModule::reject()
{
    if (group->checkedButton())
        QDialog::reject();
}

void SelectModule::onButtonClicked()
{
    if (group->checkedButton())
        okButton->setEnabled(true);
    else
        okButton->setEnabled(false);
}

QString SelectModule::getModule() const
{
    QAbstractButton* button = group->checkedButton();
    return (button ? button->objectName() : QString());
}

SelectModule::Dict SelectModule::exportHandler(const QString& fileName, const QString& filter)
{
    return exportHandler(QStringList() << fileName, filter);
}

SelectModule::Dict SelectModule::exportHandler(const QStringList& fileNames, const QString& filter)
{
    // first check if there is a certain filter selected
    SelectModule::Dict dict;
    if (!filter.isEmpty()) {
        // If an export filter is specified search directly for the module
        std::map<std::string, std::string> filterList = App::GetApplication().getExportFilters();
        std::map<std::string, std::string>::const_iterator it;
        it = filterList.find((const char*)filter.toUtf8());
        if (it != filterList.end()) {
            QString module = QString::fromLatin1(it->second.c_str());
            for (QStringList::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
                dict[*it] = module;
            }
            return dict;
        }
    }

    // the global filter (or no filter) was selected. We now try to sort filetypes that are
    // handled by more than one module and ask to the user to select one.
    QMap<QString, SelectModule::Dict> filetypeHandler;
    QMap<QString, QStringList > fileExtension;
    for (QStringList::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        QFileInfo fi(*it);
        QString ext = fi.completeSuffix().toLower();
        std::map<std::string, std::string> filters = App::GetApplication().getExportFilters(ext.toLatin1());

        if (filters.empty()) {
            ext = fi.suffix().toLower();
            filters = App::GetApplication().getExportFilters(ext.toLatin1());
        }

        fileExtension[ext].push_back(*it);
        for (std::map<std::string, std::string>::iterator jt = filters.begin(); jt != filters.end(); ++jt)
            filetypeHandler[ext][QString::fromUtf8(jt->first.c_str())] = QString::fromLatin1(jt->second.c_str());
        // set the default module handler
        if (!filters.empty())
            dict[*it] = QString::fromLatin1(filters.begin()->second.c_str());
    }

    for (QMap<QString, SelectModule::Dict>::const_iterator it = filetypeHandler.begin();
        it != filetypeHandler.end(); ++it) {
        if (it.value().size() > 1) {
            SelectModule dlg(it.key(),it.value(), getMainWindow());
            QApplication::beep();
            if (dlg.exec()) {
                QString mod = dlg.getModule();
                const QStringList& files = fileExtension[it.key()];
                for (QStringList::const_iterator jt = files.begin(); jt != files.end(); ++jt)
                    dict[*jt] = mod;
            }
        }
    }

    return dict;
}

SelectModule::Dict SelectModule::importHandler(const QString& fileName, const QString& filter)
{
    return importHandler(QStringList() << fileName, filter);
}

SelectModule::Dict SelectModule::importHandler(const QStringList& fileNames, const QString& filter)
{
    // first check if there is a certain filter selected
    SelectModule::Dict dict;
    if (!filter.isEmpty()) {
        // If an import filter is specified search directly for the module
        std::map<std::string, std::string> filterList = App::GetApplication().getImportFilters();
        std::map<std::string, std::string>::const_iterator it;
        it = filterList.find((const char*)filter.toUtf8());
        if (it != filterList.end()) {
            QString module = QString::fromLatin1(it->second.c_str());
            for (QStringList::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
                dict[*it] = module;
            }
            return dict;
        }
    }

    // the global filter (or no filter) was selected. We now try to sort filetypes that are
    // handled by more than one module and ask to the user to select one.
    QMap<QString, SelectModule::Dict> filetypeHandler;
    QMap<QString, QStringList > fileExtension;
    for (QStringList::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
        QFileInfo fi(*it);
        QString ext = fi.completeSuffix().toLower();
        std::map<std::string, std::string> filters = App::GetApplication().getImportFilters(ext.toLatin1());

        if (filters.empty()) {
            ext = fi.suffix().toLower();
            filters = App::GetApplication().getImportFilters(ext.toLatin1());
        }

        fileExtension[ext].push_back(*it);
        for (std::map<std::string, std::string>::iterator jt = filters.begin(); jt != filters.end(); ++jt)
            filetypeHandler[ext][QString::fromUtf8(jt->first.c_str())] = QString::fromLatin1(jt->second.c_str());
        // set the default module handler
        if (!filters.empty())
            dict[*it] = QString::fromLatin1(filters.begin()->second.c_str());
    }

    for (QMap<QString, SelectModule::Dict>::const_iterator it = filetypeHandler.begin();
        it != filetypeHandler.end(); ++it) {
        if (it.value().size() > 1) {
            SelectModule dlg(it.key(),it.value(), getMainWindow());
            QApplication::beep();
            if (dlg.exec()) {
                QString mod = dlg.getModule();
                const QStringList& files = fileExtension[it.key()];
                for (QStringList::const_iterator jt = files.begin(); jt != files.end(); ++jt)
                    dict[*jt] = mod;
            }
        }
    }

    return dict;
}


#include "moc_FileDialog.cpp"

