/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ANDROIDPANELSCHEME_H
#define ANDROIDPANELSCHEME_H

#include "actionpanelscheme.h"


namespace QSint
{


/**
    \brief Android-like color scheme for ActionPanel and ActionGroup.
    \since 0.2.1

    \image html ActionPanel5.png Example of the scheme
*/
class QSINT_EXPORT AndroidPanelScheme : public ActionPanelScheme
{
public:
    AndroidPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static AndroidPanelScheme scheme;
        return &scheme;
    }
};


}


#endif // ANDROIDPANELSCHEME_H
