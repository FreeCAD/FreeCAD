/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#ifndef PARTDESIGNGUI_FeaturePickDialog_H
#define PARTDESIGNGUI_FeaturePickDialog_H

#include <QDialog>
#include <QString>
#include <App/DocumentObject.h>

namespace PartDesignGui {

class Ui_FeaturePickDialog;
class FeaturePickDialog : public QDialog
{
    Q_OBJECT

public:
    enum featureStatus {
        validFeature = 0,
        invalidShape,
        noWire,
        isUsed,
        otherBody
    };

    FeaturePickDialog(std::vector<App::DocumentObject*> &objects, const std::vector<featureStatus> &status);
    ~FeaturePickDialog();

    std::vector<App::DocumentObject*> getFeatures();

    void accept();

protected Q_SLOTS:
    void onCheckOtherFeature(bool);
    void onCheckOtherBody(bool);
    void onUpdate(bool);

private:
    Ui_FeaturePickDialog* ui;

    std::vector<QString> features;
    std::vector<featureStatus> statuses;

    void updateList();

    const QString getFeatureStatusString(const featureStatus st);
};

}

#endif // PARTDESIGNGUI_FeaturePickDialog_H
