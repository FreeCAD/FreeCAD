/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef IISTASKPANELSCHEME_H
#define IISTASKPANELSCHEME_H

#include <QtGui>

#include "iistaskpanel_global.h"

struct IISTASKPANEL_EXPORT iisIconLabelScheme
{
	QColor text, textOver, textOff;
	QPen focusPen;
	QFont font;
	int iconSize;
	bool underlineOver, cursorOver;
};

class IISTASKPANEL_EXPORT iisTaskPanelScheme : public QObject
{
public:
	iisTaskPanelScheme(QObject *parent = 0);
	~iisTaskPanelScheme();

	static iisTaskPanelScheme* defaultScheme();

	QBrush panelBackground;

	QBrush headerBackground;
	iisIconLabelScheme headerLabelScheme;
	QPen headerBorder;

	int headerSize;

	bool headerAnimation;

	QIcon headerButtonFold, headerButtonFoldOver, headerButtonUnfold, headerButtonUnfoldOver;
	QSize headerButtonSize;

	QBrush groupBackground;
	QPen groupBorder;
	int groupFoldSteps, groupFoldDelay;
	iisIconLabelScheme taskLabelScheme;

protected:
	static iisTaskPanelScheme *myDefaultScheme;
};

#endif // IISTASKPANELSCHEME_H
