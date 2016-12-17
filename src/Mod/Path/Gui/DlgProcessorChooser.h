/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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

#ifndef PATH_DlgProcessorChooser_H
#define PATH_DlgProcessorChooser_H

#include <QDialog>
#include <QString>
#include <App/DocumentObject.h>

class Ui_DlgProcessorChooser;

namespace PathGui {

class DlgProcessorChooser : public QDialog
{
    Q_OBJECT

public:
    DlgProcessorChooser(std::vector<std::string> &scriptnames, bool withArguments = false);
    ~DlgProcessorChooser();

    std::string getProcessor();
    std::string getArguments();

    void accept();

protected Q_SLOTS:

private:
    Ui_DlgProcessorChooser* ui;
    std::string processor, arguments;
};

}

#endif // PATH_DlgProcessorChooser_H
