// SPDX-FileCopyrightText: 2026 Benjamin Nauck
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef FOLDABLEMENUBAR_H
#define FOLDABLEMENUBAR_H

#include <QWidget>
#include <memory>

#include "customtitlebarkit/export.h"

class QMenu;
class QMenuBar;

class CUSTOMTITLEBARKIT_EXPORT FoldableMenuBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(bool foldable READ isFoldable WRITE setFoldable)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(int revealWidth READ revealWidth WRITE setRevealWidth)

public:
    explicit FoldableMenuBar(QWidget *parent = nullptr);
    ~FoldableMenuBar() override;

    /// Add a menu with the given title. Returns the QMenu to populate.
    QMenu *addMenu(const QString &title);

    /// Replace the internal QMenuBar with an external one.
    /// The FoldableMenuBar takes ownership of the menu bar.
    void setMenuBar(QMenuBar *menuBar);

    /// Set a brand widget (logo/icon) shown at the left edge.
    /// When foldable, this is always visible; menu items expand on hover.
    void setBrandWidget(QWidget *brand);
    QWidget *brandWidget() const;

    /// Foldable mode: collapsed shows only brand, hover expands menu items.
    void setFoldable(bool foldable);
    bool isFoldable() const;

    /// Overlay expand: when true (default), the menu expands as a floating
    /// overlay instead of pushing adjacent widgets (no layout shift).
    void setOverlayExpand(bool overlay);
    bool isOverlayExpand() const;

    /// Widgets whose content should be masked (clipped) in the overlay region.
    /// Typically the titlebar's leftArea and rightArea — so the titlebar
    /// background (gradient, etc.) shows through while widgets are hidden.
    void setOverlayMaskTargets(const QList<QWidget *> &targets);

    void setExpanded(bool expanded);
    bool isExpanded() const;

    int revealWidth() const;
    void setRevealWidth(int w);

Q_SIGNALS:
    void expandedChanged(bool expanded);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FOLDABLEMENUBAR_H
