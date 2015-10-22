/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef IISICONLABEL_H
#define IISICONLABEL_H

#include <QtGui>

#include "iistaskpanel_global.h"

struct iisIconLabelScheme;

class IISTASKPANEL_EXPORT iisIconLabel : public QWidget
{
	Q_OBJECT

public:
	iisIconLabel(const QIcon &icon, const QString &title, QWidget *parent = 0);
	virtual ~iisIconLabel();

	void setColors(const QColor &color, const QColor &colorOver, const QColor &colorOff);
	void setFont(const QFont &font);
	void setFocusPen(const QPen &pen);

	void setSchemePointer(iisIconLabelScheme **pointer);

	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

Q_SIGNALS:
	void pressed();
	void released();
	void clicked();
	void activated();
	void contextMenu();

protected:
	virtual void paintEvent ( QPaintEvent * event );
	virtual void enterEvent ( QEvent * event );
	virtual void leaveEvent ( QEvent * event );

	virtual void mousePressEvent ( QMouseEvent * event );
	virtual void mouseReleaseEvent ( QMouseEvent * event );
	virtual void keyPressEvent ( QKeyEvent * event );

	QIcon myPixmap;
	QString myText;

	QColor myColor, myColorOver, myColorDisabled;
	QFont myFont;
	QPen myPen;

	iisIconLabelScheme **mySchemePointer;

	bool m_over, m_pressed;

	bool m_changeCursorOver, m_underlineOver;
};

#endif // IISICONLABEL_H
