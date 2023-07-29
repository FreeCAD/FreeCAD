

#include <QFontComboBox.h>
#include <Base/Console.h>

class QFontComboBoxFC : QFontComboBox {
public:
    QFontComboBoxFC(QWidget* parent)
    {
        QFontComboBox(parent);
        Base::Console().Message("QFontComboBoxFC\n");
    }
}