/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DOWNLOADMANAGER_H
#define GUI_DIALOG_DOWNLOADMANAGER_H

#include <QAbstractListModel>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <FCGlobal.h>


class AutoSaver;
class QFileIconProvider;

namespace Gui {
namespace Dialog {
class DownloadItem;
class DownloadModel;
class Ui_DownloadManager;

class GuiExport DownloadManager : public QDialog
{
    Q_OBJECT

public:
    enum RemovePolicy {
        Never,
        Exit,
        SuccessFullDownload
    };

    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy) // clazy:exclude=qproperty-without-notify
    Q_ENUM(RemovePolicy)

public:
    static DownloadManager* getInstance();

private:
    explicit DownloadManager(QWidget *parent = nullptr);
    ~DownloadManager() override;

public:
    int activeDownloads() const;
    QNetworkAccessManager * networkAccessManager()
    { return m_manager; }

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);
    void closeEvent(QCloseEvent* e) override;
    QUrl redirectUrl(const QUrl&) const;

public Q_SLOTS:
    void download(const QNetworkRequest &request, bool requestFileName = false);
    inline void download(const QUrl &url, bool requestFileName = false)
        { download(QNetworkRequest(url), requestFileName); }
    void handleUnsupportedContent(QNetworkReply *reply, bool requestFileName = false);
    void cleanup();

private Q_SLOTS:
    void save() const;
    void updateRow();

private:
    void addItem(DownloadItem *item);
    void updateItemCount();
    void load();

    AutoSaver *m_autoSaver;
    DownloadModel *m_model;
    QNetworkAccessManager *m_manager;
    QFileIconProvider *m_iconProvider;
    QList<DownloadItem*> m_downloads;
    RemovePolicy m_removePolicy;
    friend class DownloadModel;

private:
    Ui_DownloadManager* ui;
    static DownloadManager* self;
};

class DownloadModel : public QAbstractListModel
{
    friend class DownloadManager;
    Q_OBJECT

public:
    explicit DownloadModel(DownloadManager *downloadManager, QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    DownloadManager *m_downloadManager;

};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DOWNLOADMANAGER_H
