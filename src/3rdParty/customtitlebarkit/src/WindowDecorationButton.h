// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef WINDOWDECORATIONBUTTON_H
#define WINDOWDECORATIONBUTTON_H

#include <QAbstractButton>

class QStyleOptionButton;

/// A borderless window control button with text-based icons.
/// Themeable via QSS — object names: "minimizeButton", "maximizeButton", "closeButton".
/// Supports :hover and :pressed pseudo-states for both background and color.
class WindowDecorationButton : public QAbstractButton {
    Q_OBJECT

public:
    enum Role { Minimize, Maximize, Restore, Close };

    explicit WindowDecorationButton(Role role, QWidget *parent = nullptr);

    void setRole(Role role);
    Role role() const { return m_role; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void initStyleOption(QStyleOptionButton *option) const;

private:
    void updateObjectName();
    void updateText();

    Role m_role;
};

#endif // WINDOWDECORATIONBUTTON_H
