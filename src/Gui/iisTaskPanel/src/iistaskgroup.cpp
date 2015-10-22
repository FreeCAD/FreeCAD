/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "iistaskgroup.h"
#include "iistaskpanelscheme.h"
#include "iisiconlabel.h"

iisTaskGroup::iisTaskGroup(QWidget *parent, bool hasHeader)
	: QFrame(parent),
	myHasHeader(hasHeader)
{
	//setMinimumHeight(32);

	setScheme(iisTaskPanelScheme::defaultScheme());

	QVBoxLayout *vbl = new QVBoxLayout();
	vbl->setMargin(4);
	vbl->setSpacing(0);
	setLayout(vbl);
}

iisTaskGroup::~iisTaskGroup()
{

}

void iisTaskGroup::setScheme(iisTaskPanelScheme *scheme)
{
	if (scheme) {
		myScheme = scheme;
		myLabelScheme = &(scheme->taskLabelScheme);
		update();
	}
}

void iisTaskGroup::addIconLabel(iisIconLabel *label, bool addToLayout)
{
	if (!label) return;

	if (addToLayout) {
		layout()->addWidget(label);
	}

	label->setSchemePointer(&myLabelScheme);
}

void iisTaskGroup::paintEvent ( QPaintEvent * event ) 
{
	QPainter p(this);

	//p.setOpacity(/*m_opacity+*/0.7);
	//p.fillRect(rect(), myScheme->groupBackground);

	p.setBrush(myScheme->groupBackground);
	p.setPen(myScheme->groupBorder);
	p.drawRect(rect().adjusted(0,-(int)myHasHeader,-1,-1));
}
