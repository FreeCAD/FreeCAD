/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "iistaskheader.h"
#include "iistaskpanelscheme.h"
#include "iisiconlabel.h"

iisTaskHeader::iisTaskHeader(const QIcon &icon, const QString &title, bool expandable, QWidget *parent)
	: QFrame(parent),
	myExpandable(expandable),
	m_over(false),
	m_buttonOver(false),
	m_fold(true),
	m_opacity(0.1),
	myButton(0)
{
	myTitle = new iisIconLabel(icon, title, this);
	myTitle->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	connect(myTitle, SIGNAL(activated()), this, SLOT(fold()));

	QHBoxLayout *hbl = new QHBoxLayout();
	hbl->setMargin(2);
	setLayout(hbl);

	hbl->addWidget(myTitle);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	setScheme(iisTaskPanelScheme::defaultScheme());
	myTitle->setSchemePointer(&myLabelScheme);


	if (myExpandable) {
		myButton = new QLabel(this);
		hbl->addWidget(myButton);
		myButton->installEventFilter(this);
		myButton->setFixedWidth(myScheme->headerButtonSize.width());
		changeIcons();
	}
}

iisTaskHeader::~iisTaskHeader()
{

}

bool iisTaskHeader::eventFilter(QObject *obj, QEvent *event)
{
	switch (event->type()) {
		case QEvent::MouseButtonPress:
			fold();
			return true;

		case QEvent::Enter:
			m_buttonOver = true;
			changeIcons();
			return true;

		case QEvent::Leave:
			m_buttonOver = false;
			changeIcons();
			return true;

		default:;
	}

	return QFrame::eventFilter(obj, event);
}

void iisTaskHeader::setScheme(iisTaskPanelScheme *scheme)
{
	if (scheme) {
		myScheme = scheme;
		myLabelScheme = &(scheme->headerLabelScheme);

		if (myExpandable) {
			setCursor(myLabelScheme->cursorOver ? Qt::PointingHandCursor : cursor());
			changeIcons();
		}

		setFixedHeight(scheme->headerSize);

		update();
	}
}

void iisTaskHeader::paintEvent ( QPaintEvent * event ) 
{
	QPainter p(this);

#if QT_VERSION >= 0x040203
	if (myScheme->headerAnimation)
		p.setOpacity(m_opacity+0.7);
#endif

	p.setPen(myScheme->headerBorder);
	p.setBrush(myScheme->headerBackground);
	if (myScheme->headerBorder.style() == Qt::NoPen)
		p.drawRect(rect());
	else
		p.drawRect(rect().adjusted(0,0,-1,-1));
}

void iisTaskHeader::animate()
{
	if (!myScheme->headerAnimation)
		return;

	if (!isEnabled()) {
		m_opacity = 0.1;
		update();
		return;
	}

	if (m_over) {
		if (m_opacity >= 0.3) {
			m_opacity = 0.3;
			return;
		}
		m_opacity += 0.05;
	} else {
		if (m_opacity <= 0.1) {
			m_opacity = 0.1;
			return;
		}
		m_opacity = qMax(0.1, m_opacity-0.05);
	}

	QTimer::singleShot(100, this, SLOT(animate()));
	update();
}

void iisTaskHeader::enterEvent ( QEvent * /*event*/ )
{
	m_over = true;

	if (isEnabled())
		QTimer::singleShot(100, this, SLOT(animate()));

	update();
}

void iisTaskHeader::leaveEvent ( QEvent * /*event*/ )
{
	m_over = false;
	
	if (isEnabled())
		QTimer::singleShot(100, this, SLOT(animate()));

	update();
}

void iisTaskHeader::fold()
{
	if (myExpandable) {
		emit activated();

		m_fold = !m_fold;
		changeIcons();
	}
}

void iisTaskHeader::changeIcons()
{
	if (!myButton)
		return;

	if (m_buttonOver)
	{
		if (m_fold)
			myButton->setPixmap(myScheme->headerButtonFoldOver.pixmap(myScheme->headerButtonSize));
		else
			myButton->setPixmap(myScheme->headerButtonUnfoldOver.pixmap(myScheme->headerButtonSize));
	} else 
	{
		if (m_fold)
			myButton->setPixmap(myScheme->headerButtonFold.pixmap(myScheme->headerButtonSize));
		else
			myButton->setPixmap(myScheme->headerButtonUnfold.pixmap(myScheme->headerButtonSize));
	}
}

void iisTaskHeader::mouseReleaseEvent ( QMouseEvent * event )
{
	if (event->button() == Qt::LeftButton) {
		emit activated();
	}
}

