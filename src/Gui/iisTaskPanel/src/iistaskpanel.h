/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

/*
 Change log:
 * jriegel (03-01-2010)
  - add method removeWidget()
 * jriegel (05-01-2010)
  - add method removeStretch()
  - use QSpacerItem directly
*/

#ifndef IISTASKPANEL_H
#define IISTASKPANEL_H

#include <QtGui>
#include <QtCore>
#include <QSpacerItem>

#include "iistaskpanel_global.h"

class iisTaskPanelScheme;

class IISTASKPANEL_EXPORT iisTaskPanel : public QWidget
{
public:
	iisTaskPanel(QWidget *parent = 0);
	~iisTaskPanel();

	void addWidget(QWidget *w);
	void removeWidget(QWidget *w);
	void addStretch(int s = 0);
	void removeStretch();

	void setScheme(iisTaskPanelScheme *scheme);

protected:
	virtual void paintEvent ( QPaintEvent * event );

	iisTaskPanelScheme *myScheme;
	QSpacerItem *mySpacer;
};

#endif // IISTASKPANEL_H
