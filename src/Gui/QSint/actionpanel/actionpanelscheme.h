/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef TASKPANELSCHEME_H
#define TASKPANELSCHEME_H

#include <QPixmap>
#include <QSize>
#include <QString>
#include "qsint_global.h"


namespace QSint
{


/**
    \brief Class representing color scheme for ActionPanel and ActionGroup.
    \since 0.2

    \image html ActionPanel1.png Default ActionPanel scheme
*/
class QSINT_EXPORT ActionPanelScheme
{
public:
    /** \enum TaskPanelFoldEffect
        \brief Animation effect during expanding/collapsing of the ActionGroup's contents.
      */
    enum TaskPanelFoldEffect
    {
        /// No folding effect
        NoFolding,
        /// Folding by scaling group's contents
        ShrunkFolding,
        /// Folding by sliding group's contents
        SlideFolding
    };


  ActionPanelScheme();

  /** Returns a pointer to the default scheme object.
    Must be reimplemented in the own schemes.
    */
  static ActionPanelScheme* defaultScheme();

  /// Height of the header in pixels.
  int headerSize;
  /// If set to \a true, moving mouse over the header results in changing its opacity slowly.
  bool headerAnimation;

  /// Image of folding button when the group is expanded.
  QPixmap headerButtonFold;
  /// Image of folding button when the group is expanded and mouse cursor is over the button.
  QPixmap headerButtonFoldOver;
  /// Image of folding button when the group is collapsed.
  QPixmap headerButtonUnfold;
  /// Image of folding button when the group is collapsed and mouse cursor is over the button.
  QPixmap headerButtonUnfoldOver;

  QSize headerButtonSize;

  /// Number of steps made for expanding/collapsing animation (default 20).
  int groupFoldSteps;
  /// Delay in ms between steps made for expanding/collapsing animation (default 15).
  int groupFoldDelay;
  /// Sets folding effect during expanding/collapsing.
  TaskPanelFoldEffect groupFoldEffect;
  /// If set to \a true, changes group's opacity slowly during expanding/collapsing.
  bool groupFoldThaw;

  /// The CSS for the ActionPanel/ActionGroup elements.
  QString actionStyle;
};


}

#endif // TASKPANELSCHEME_H
