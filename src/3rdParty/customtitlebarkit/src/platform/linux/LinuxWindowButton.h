// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef LINUXWINDOWBUTTON_H
#define LINUXWINDOWBUTTON_H

#include <QAbstractButton>
#include <QObject>

class QHBoxLayout;

/// Tracks hover state for a group of Linux window buttons.
/// When the mouse enters the group widget, all buttons show their icons.
class LinuxButtonGroup : public QObject {
    Q_OBJECT
public:
    bool hovered = false;

    explicit LinuxButtonGroup(QWidget *groupWidget);
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void updateButtons();
};

/// A circular window button styled to match the system theme via QPalette.
/// Uses QIcon::fromTheme for icons with QPainter vector fallbacks.
class LinuxWindowButton : public QAbstractButton {
    Q_OBJECT
public:
    enum Role { Close, Minimize, Maximize };
    static constexpr int ButtonSize = 24;

    LinuxWindowButton(Role role, LinuxButtonGroup *group, QWidget *parent);

    Role role() const { return m_role; }
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QIcon themeIcon() const;

    Role m_role;
    LinuxButtonGroup *m_group;
};

/// Keeps left/right margins in sync with the computed vertical margin.
class LinuxSymmetricMarginFilter : public QObject {
    Q_OBJECT
public:
    LinuxSymmetricMarginFilter(QHBoxLayout *layout, int buttonSize, QObject *parent);
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QHBoxLayout *m_layout;
    int m_buttonSize;
};

#endif // LINUXWINDOWBUTTON_H
