#ifndef WINVISTAPANELSCHEME_H
#define WINVISTAPANELSCHEME_H


#include "actionpanelscheme.h"


namespace QSint
{


/**
    \brief Windows Vista-like color scheme for ActionPanel and ActionGroup.
    \since 0.2

    \image html ActionPanel4.png Example of the scheme
*/
class WinVistaPanelScheme : public ActionPanelScheme
{
public:
    WinVistaPanelScheme();

    static ActionPanelScheme* defaultScheme()
    {
        static WinVistaPanelScheme scheme;
        return &scheme;
    }
};


}


#endif // WINVISTAPANELSCHEME_H
