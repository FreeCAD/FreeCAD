// SPDX-License-Identifier: LGPL-3.0-only
/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QToolButton>
#include "qsint_global.h"

namespace QSint
{
/**
 * @brief Represents an action, similar to a Windows Vista/7 control panel item.
 *
 * An ActionLabel typically displays an icon and text. It supports tooltips, status tips,
 * clickability, checkability, and other features similar to a QToolButton.
 *
 * Customization via CSS: The class name `QSint--ActionLabel` is used.
 */
class QSINT_EXPORT ActionLabel : public QToolButton
{
    Q_OBJECT

public:
    /**
     * @brief Constructs an ActionLabel.
     * @param parent The parent widget.
     */
    explicit ActionLabel(QWidget *parent = nullptr);

    /**
     * @brief Constructs an ActionLabel from a QAction.
     * @param action The QAction to represent.
     * @param parent The parent widget.
     */
    explicit ActionLabel(QAction *action, QWidget *parent = nullptr);

    /**
     * @brief Destroys the ActionLabel.
     */
    ~ActionLabel() override = default;

    /**
     * @brief Returns the recommended size for the label.
     * @return The size hint.
     */
    QSize sizeHint() const override;

    /**
     * @brief Returns the minimum size the label can be.
     * @return The minimum size hint.
     */
    QSize minimumSizeHint() const override;

private:
    void paintEvent(QPaintEvent *event) override;

protected:
    /**
     * @brief Initializes the ActionLabel.
     */
    void init();
};

} // namespace
