/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef IISTASKBOX_H
#define IISTASKBOX_H

#include <QtGui>

#include "iistaskpanel_global.h"

class iisTaskHeader;
class iisTaskGroup;
class iisTaskPanelScheme;
class iisIconLabel;

class IISTASKPANEL_EXPORT iisTaskBox : public QFrame
{
	Q_OBJECT

public:
	iisTaskBox(const QString &title, bool expandable = true, QWidget *parent = 0);
	iisTaskBox(const QPixmap &icon, const QString &title, bool expandable = true, QWidget *parent = 0);
	virtual ~iisTaskBox();

	void setScheme(iisTaskPanelScheme *pointer);

	QBoxLayout* groupLayout();

	void addIconLabel(iisIconLabel *label, bool addToLayout = true);

public Q_SLOTS:
	void showHide();

protected Q_SLOTS:
	void processHide();
	void processShow();

protected:
	void init(); 

	virtual void paintEvent ( QPaintEvent * event );
	

	double m_foldStep, m_foldDelta, m_fullHeight, m_tempHeight;
	int m_foldDirection;

	QPixmap m_foldPixmap;

	iisTaskHeader *myHeader;
	iisTaskGroup *myGroup;
	QWidget *myDummy;

	iisTaskPanelScheme *myScheme;
};

#endif // IISTASKBOX_H
