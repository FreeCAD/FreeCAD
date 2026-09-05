/* SPDX - License - Identifier: LGPL - 2.1 - or -later
 ****************************************************************************
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

#ifndef GUI_TASKVIEW_TASKHATCHFACE_H
#define GUI_TASKVIEW_TASKHATCHFACE_H

#include <QWidget>
#include <memory>  // For std::unique_ptr

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

#include <Mod/TechDraw/TechDrawGlobal.h>

class QComboBox;

namespace App
{
class DocumentObject;
}

namespace Gui
{
class Document;
}

namespace TechDraw
{
class DrawViewPart;
class DrawHatch;
class DrawGeomHatch;
}  // namespace TechDraw

namespace TechDrawGui
{
class Ui_TaskHatchFace;
}  // namespace TechDrawGui


namespace TechDrawGui
{

enum class PatternUIType
{
    SVG,
    PAT
};

struct PatternEntry
{
    QString displayName;
    PatternUIType type;
    QString filePath;         // Full path to SVG or PAT file
    QString patNameInternal;  // Specific pattern name within a PAT file

    bool operator==(const PatternEntry& other) const
    {
        return type == other.type && filePath == other.filePath
            && (type == PatternUIType::SVG || patNameInternal == other.patNameInternal);
    }
};

class TaskHatchFace: public QWidget
{
    Q_OBJECT

public:
    explicit TaskHatchFace(TechDraw::DrawViewPart* dvp,
                           const std::vector<std::string>& subNames,
                           QWidget* parent = nullptr);
    explicit TaskHatchFace(App::DocumentObject* hatchObjectToEdit, QWidget* parent = nullptr);
    ~TaskHatchFace() override;  // Recommended for std::unique_ptr to incomplete type

    bool accept();
    bool reject();

    static void populatePatternsComboBox(QComboBox* box);

public Q_SLOTS:
    static void onOpenPatternsFolder();

private Q_SLOTS:
    void onPatternChanged(int index);
    void onAddCustomPattern();
    void onParamsChanged();

private:
    void init();
    void updateUIControlsForPatternType(PatternUIType type);
    void loadDefaults();                            // For new hatch UI setup
    void loadFromObject(App::DocumentObject* obj);  // For edit mode UI setup
    void saveOriginalState();  // Saves state of the initial object in edit mode

    bool applyToDocumentObject(App::DocumentObject* obj, const PatternEntry& entry);
    void updateHatchObjectPreview();  // New method for live updates


    std::unique_ptr<Ui_TaskHatchFace> ui;

    TechDraw::DrawViewPart* m_dvp;
    std::vector<std::string> m_subNames;
    Gui::Document* m_doc;

    App::DocumentObject* m_targetHatchObject;
    bool m_isEditMode;
    bool m_isLoading;

    // Original state for edit mode and for handling type switches within command
    PatternEntry m_originalPattern;
    double m_originalScale;
    Base::Color m_originalColor;
    double m_originalLineWidth;
    double m_originalRotation;
    Base::Vector3d m_originalOffset;
    PatternUIType m_originalType;
    std::string m_originalObjectName;
};

class TaskDlgHatchFace: public Gui::TaskView::TaskDialog  // Unchanged from your original
{
    Q_OBJECT

public:
    TaskDlgHatchFace(TechDraw::DrawViewPart* dvp, const std::vector<std::string>& subNames);
    explicit TaskDlgHatchFace(App::DocumentObject* hatchObjectToEdit);
    // ~TaskDlgHatchFace() override = default; // Keep if you add it to .h

    void open() override;
    void clicked(int) override;
    bool accept() override;
    bool reject() override;
    bool isAllowedAlterDocument() const override
    {
        return true;
    }

private:
    TaskHatchFace* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}  // namespace TechDrawGui

Q_DECLARE_METATYPE(TechDrawGui::PatternEntry)

#endif  // GUI_TASKVIEW_TASKHATCHFACE_H
