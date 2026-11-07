// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 Pierre-Louis Boyer

#ifndef GUI_TASKVIEW_TASKHATCHFACE_H
#define GUI_TASKVIEW_TASKHATCHFACE_H

#include <memory>
#include <string>
#include <vector>
#include <QWidget>

#include <App/DocumentObserver.h>
#include <Gui/TaskView/TaskDialog.h>

#include "HatchPatterns.h"

namespace TechDraw
{
class DrawViewPart;
}

namespace TechDrawGui
{
class Ui_TaskHatchFace;

class TaskHatchFace: public QWidget
{
    Q_OBJECT

public:
    TaskHatchFace(
        TechDraw::DrawViewPart* view,
        const std::vector<std::string>& subNames,
        QWidget* parent = nullptr
    );
    explicit TaskHatchFace(App::DocumentObject* hatch, QWidget* parent = nullptr);
    ~TaskHatchFace() override;

    bool accept();
    bool reject();
    void documentClosed();

protected:
    void changeEvent(QEvent* event) override;

private Q_SLOTS:
    void onPatternChanged(int index);
    void onAddCustomPattern();
    void updateHatchObjectPreview();

private:
    void init();
    void loadDefaults();
    void updateScaleRange(App::DocumentObject* obj);
    void loadFromObject(App::DocumentObject* obj);
    void removeExistingHatches();
    void applyToDocumentObject(App::DocumentObject* obj, const PatternEntry& entry);
    void refreshView();
    bool hasTransaction() const;

    std::unique_ptr<Ui_TaskHatchFace> ui;
    App::DocumentT m_document;
    App::DocumentObjectT m_view;
    App::DocumentObjectT m_target;
    App::DocumentObjectT m_svgHatch;
    App::DocumentObjectT m_patHatch;
    std::vector<std::string> m_subNames;
    std::string m_originalLabel;
    bool m_isEditMode = false;
    bool m_isLoading = true;
    int m_transactionId = 0;
};

class TaskDlgHatchFace: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskDlgHatchFace(TechDraw::DrawViewPart* view, const std::vector<std::string>& subNames);
    explicit TaskDlgHatchFace(App::DocumentObject* hatch);

    bool accept() override;
    bool reject() override;
    void autoClosedOnDeletedDocument() override;

private:
    void init(App::DocumentObject* object);
    TaskHatchFace* widget;
};

}  // namespace TechDrawGui

#endif
