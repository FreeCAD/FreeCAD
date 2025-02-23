#ifndef GUI_COMPUTATION_DIALOG_H
#define GUI_COMPUTATION_DIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>
#include "MainWindow.h"
#include "Base/ProgressIndicator.h"

namespace Gui {

class ComputationDialog : public QDialog, public Base::ProgressIndicator {
public:
    ComputationDialog(QWidget* parent = Gui::MainWindow::getInstance());

    // Message_ProgressIndicator interface implementation
    bool UserBreak() override;
    void Show(float position, bool isForce) override;
    void abort();
    void run(std::function<void()> func);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void onAbort();

    std::atomic<bool> aborted;
    QProgressBar* progressBar;
};

} // namespace Gui

#endif // GUI_COMPUTATION_DIALOG_H
