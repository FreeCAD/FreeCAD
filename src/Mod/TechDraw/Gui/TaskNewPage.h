/* SPDX - License - Identifier: LGPL - 2.1 - or -later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Pierre-Louis Boyer                                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef TECHDRAWGUI_TASKNEWPAGE_H
#define TECHDRAWGUI_TASKNEWPAGE_H

#include <Gui/TaskView/TaskDialog.h>
#include <QWidget>
#include <memory>

class Ui_TaskNewPage;
class QButtonGroup;

namespace TechDrawGui
{

class TaskNewPage: public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(TaskNewPage)
public:
    explicit TaskNewPage(QWidget* parent = nullptr);

    QString getSelectedTemplatePath() const;
    bool isTemplateValid() const;
    void updatePreviewAndPath();

    // For TaskDlgNewPage to delegate accept
    bool acceptPageCreation();

protected:
    void changeEvent(QEvent* e) override;

public Q_SLOTS:
    void onOpenTemplateFolderClicked();

private Q_SLOTS:
    void onStandardChanged(int index);
    void onSizeChanged(int index);
    void onOrientationChanged();

private:
    void populateStandards();
    void populateSizes();
    QString findTemplateFile(const QString& standard, const QString& size, bool landscape) const;

    std::unique_ptr<Ui_TaskNewPage> ui;
    QString m_baseTemplateDir;
    QString m_currentTemplateFile;
    QButtonGroup* m_orientationGroup;
};


class TaskDlgNewPage: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    explicit TaskDlgNewPage();

    void open() override;
    bool accept() override;
    bool reject() override;


    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    TaskNewPage* m_widget;
};

}  // namespace TechDrawGui

#endif  // TECHDRAWGUI_TASKNEWPAGE_H