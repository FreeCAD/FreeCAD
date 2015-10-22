/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "iistaskbox.h"
#include "iistaskheader.h"
#include "iistaskgroup.h"
#include "iistaskpanelscheme.h"


iisTaskBox::iisTaskBox(const QString &title, bool expandable, QWidget *parent)
	: QFrame(parent)
{
	myHeader = new iisTaskHeader(QPixmap(), title, expandable, this);
	init();
}

iisTaskBox::iisTaskBox(const QPixmap &icon, const QString &title, bool expandable, QWidget *parent)
	: QFrame(parent)
{
	myHeader = new iisTaskHeader(icon, title, expandable, this);
	init();
}

void iisTaskBox::init()
{
	m_foldStep = 0;

	myScheme = iisTaskPanelScheme::defaultScheme();

	QVBoxLayout *vbl = new QVBoxLayout();
	vbl->setMargin(0);
	vbl->setSpacing(0);
	setLayout(vbl);

	vbl->addWidget(myHeader);

	myGroup = new iisTaskGroup(this, true);
	vbl->addWidget(myGroup);

	myDummy = new QWidget(this);
	vbl->addWidget(myDummy);
	myDummy->hide();

	connect(myHeader, SIGNAL(activated()), this, SLOT(showHide()));
}

iisTaskBox::~iisTaskBox()
{

}

void iisTaskBox::setScheme(iisTaskPanelScheme *pointer)
{
	myScheme = pointer;
	myHeader->setScheme(pointer);
	myGroup->setScheme(pointer);
	update();
}

QBoxLayout* iisTaskBox::groupLayout()
{
	return myGroup->groupLayout();
}

void iisTaskBox::addIconLabel(iisIconLabel *label, bool addToLayout)
{
	myGroup->addIconLabel(label, addToLayout);
}


void iisTaskBox::showHide()
{
	if (m_foldStep)
		return;

	m_foldPixmap = QPixmap::grabWidget(myGroup, myGroup->rect());

	if (myGroup->isVisible()) {
		m_tempHeight = m_fullHeight = myGroup->height();
		m_foldDelta = m_fullHeight / myScheme->groupFoldSteps;
		m_foldStep = myScheme->groupFoldSteps;
		m_foldDirection = -1;

		myGroup->hide();
		myDummy->setFixedSize(myGroup->size());
		myDummy->show();

		QTimer::singleShot(myScheme->groupFoldDelay, this, SLOT(processHide()));
	}
	else {
		m_foldStep = myScheme->groupFoldSteps;
		m_foldDirection = 1;
		m_tempHeight = 0;

		QTimer::singleShot(myScheme->groupFoldDelay, this, SLOT(processShow()));
	}

	myDummy->show();
}

void iisTaskBox::processHide()
{
	if (!--m_foldStep) {
		myDummy->setFixedHeight(0);
		myDummy->hide();
		m_foldPixmap = QPixmap();
		setFixedHeight(myHeader->height());
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		return;
	}

	setUpdatesEnabled(false);

	m_tempHeight -= m_foldDelta; 
	myDummy->setFixedHeight(m_tempHeight);
	setFixedHeight(myDummy->height()+myHeader->height());

	QTimer::singleShot(myScheme->groupFoldDelay, this, SLOT(processHide()));

	setUpdatesEnabled(true);
}

void iisTaskBox::processShow()
{
	if (!--m_foldStep) {
		myDummy->hide();
		m_foldPixmap = QPixmap();
		myGroup->show();
		setFixedHeight(m_fullHeight+myHeader->height());
		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		setMaximumHeight(QWIDGETSIZE_MAX);
		setMinimumHeight(0);
		return;
	}

	setUpdatesEnabled(false);

	m_tempHeight += m_foldDelta; 
	myDummy->setFixedHeight(m_tempHeight);
	setFixedHeight(myDummy->height()+myHeader->height());

	QTimer::singleShot(myScheme->groupFoldDelay, this, SLOT(processShow()));

	setUpdatesEnabled(true);
}

void iisTaskBox::paintEvent ( QPaintEvent * event ) 
{
	QPainter p(this);

	if (myDummy->isVisible()) {
#if QT_VERSION >= 0x040202
		if (m_foldDirection < 0)
			p.setOpacity((double)m_foldStep / myScheme->groupFoldSteps);
		else
			p.setOpacity((double)(myScheme->groupFoldSteps-m_foldStep) / myScheme->groupFoldSteps);
#endif

		p.drawPixmap(myDummy->x(),myDummy->y(),m_foldPixmap);

		return;
	}

}
