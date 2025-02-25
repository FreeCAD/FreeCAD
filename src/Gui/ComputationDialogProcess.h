#ifndef GUI_COMPUTATIONDIALOGPROCESS_H
#define GUI_COMPUTATIONDIALOGPROCESS_H

#include <atomic>
#include <QProgressDialog>

namespace Gui {

class ComputationDialogProcess : public QProgressDialog {
    Q_OBJECT
public:
    explicit ComputationDialogProcess(QWidget* parent = nullptr);
    void run();
    bool UserBreak();
    void Show(float position, bool isForce);

protected:
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    void abort();

private:
    std::atomic<bool> aborted;
};

} // namespace Gui

#endif // GUI_COMPUTATIONDIALOGPROCESS_H