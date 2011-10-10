/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "iiswinxptaskpanelscheme.h"

iisWinXPTaskPanelScheme* iisWinXPTaskPanelScheme::myDefaultXPScheme = 0;

iisWinXPTaskPanelScheme::iisWinXPTaskPanelScheme(QObject *parent)
	: iisTaskPanelScheme(parent)
{
	QLinearGradient panelBackgroundGrd(0,0, 0,300);
	panelBackgroundGrd.setColorAt(0, 0x7ba2e7);
	panelBackgroundGrd.setColorAt(1, 0x6375d6);
	panelBackground = panelBackgroundGrd;

	headerBackground = QBrush(0x225aca);

	headerBorder = QColor(0x225aca);
	headerSize = 25;
	headerAnimation = false;

	headerLabelScheme.text = QColor(0xffffff);
	headerLabelScheme.textOver = QColor(0x428eff);
	headerLabelScheme.iconSize = 22;

	headerButtonFold = QPixmap(":/Resources/headerButtonFold_XPBlue1.png");
	headerButtonFoldOver = QPixmap(":/Resources/headerButtonFoldOver_XPBlue1.png");
	headerButtonUnfold = QPixmap(":/Resources/headerButtonUnfold_XPBlue1.png");
	headerButtonUnfoldOver = QPixmap(":/Resources/headerButtonUnfoldOver_XPBlue1.png");
	headerButtonSize = QSize(17,17);

	groupBackground = QBrush(0xeff3ff); 
	groupBorder = QColor(0xffffff);

	taskLabelScheme.text = QColor(0x215dc6);
	taskLabelScheme.textOver = QColor(0x428eff);
}

iisWinXPTaskPanelScheme::~iisWinXPTaskPanelScheme()
{

}

iisTaskPanelScheme* iisWinXPTaskPanelScheme::defaultScheme()	
{ 
	if (!myDefaultXPScheme)
		myDefaultXPScheme = new iisWinXPTaskPanelScheme();

	return myDefaultXPScheme; 
}



iisWinXPTaskPanelScheme2* iisWinXPTaskPanelScheme2::myDefaultXPScheme = 0;

iisWinXPTaskPanelScheme2::iisWinXPTaskPanelScheme2(QObject *parent)
	: iisTaskPanelScheme(parent)
{
	QLinearGradient panelBackgroundGrd(0,0, 0,300);
	panelBackgroundGrd.setColorAt(0, 0x7ba2e7);
	panelBackgroundGrd.setColorAt(1, 0x6375d6);
	panelBackground = panelBackgroundGrd;

	QLinearGradient headerBackgroundGrd(0,0, 300,0);
	headerBackgroundGrd.setColorAt(0, 0xffffff);
	headerBackgroundGrd.setColorAt(1, 0xc6d3f7);
	headerBackground = headerBackgroundGrd;

	headerBorder = QPen(Qt::NoPen);
	headerSize = 25;
	headerAnimation = false;

	headerLabelScheme.text = QColor(0x215dc6);
	headerLabelScheme.textOver = QColor(0x428eff);
	headerLabelScheme.iconSize = 22;

	headerButtonFold = QPixmap(":/Resources/headerButtonFold_XPBlue2.png");
	headerButtonFoldOver = QPixmap(":/Resources/headerButtonFoldOver_XPBlue2.png");
	headerButtonUnfold = QPixmap(":/Resources/headerButtonUnfold_XPBlue2.png");
	headerButtonUnfoldOver = QPixmap(":/Resources/headerButtonUnfoldOver_XPBlue2.png");
	headerButtonSize = QSize(17,17);

	groupBackground = QBrush(0xd6dff7); 
	groupBorder = QColor(0xffffff);

	taskLabelScheme.text = QColor(0x215dc6);
	taskLabelScheme.textOver = QColor(0x428eff);
}

iisWinXPTaskPanelScheme2::~iisWinXPTaskPanelScheme2()
{

}

iisTaskPanelScheme* iisWinXPTaskPanelScheme2::defaultScheme()	
{ 
	if (!myDefaultXPScheme)
		myDefaultXPScheme = new iisWinXPTaskPanelScheme2();

	return myDefaultXPScheme; 
}

