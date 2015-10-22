/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef IISWINXPTASKPANELSCHEME_H
#define IISWINXPTASKPANELSCHEME_H

#include "iistaskpanelscheme.h"

#include "iistaskpanel_global.h"

class IISTASKPANEL_EXPORT iisWinXPTaskPanelScheme : public iisTaskPanelScheme
{
public:
	iisWinXPTaskPanelScheme(QObject *parent=0);
	~iisWinXPTaskPanelScheme();

	static iisTaskPanelScheme* defaultScheme();

protected:
	static iisWinXPTaskPanelScheme *myDefaultXPScheme;
};


class IISTASKPANEL_EXPORT iisWinXPTaskPanelScheme2 : public iisTaskPanelScheme
{
public:
	iisWinXPTaskPanelScheme2(QObject *parent=0);
	~iisWinXPTaskPanelScheme2();

	static iisTaskPanelScheme* defaultScheme();

protected:
	static iisWinXPTaskPanelScheme2 *myDefaultXPScheme;
};

#endif // IISWINXPTASKPANELSCHEME_H
