/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "styles.h"

#include <iisTaskPanel>

test::test(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);

	tb1 = new iisTaskBox(QPixmap(":/images/win/filenew.png"), "Group of Tasks", true, this);
	ui.panel->addWidget(tb1);

	iisIconLabel *i1 = new iisIconLabel(QPixmap(":/images/win/zoomin.png"), "Do Task 1", tb1);
	tb1->addIconLabel(i1);
	connect(i1, SIGNAL(activated()), this, SLOT(task1()));
	//tb1->groupLayout()->addWidget(i1);
	iisIconLabel *i2 = new iisIconLabel(QPixmap(":/images/win/zoomout.png"), "Do Task 2", tb1);
	tb1->addIconLabel(i2);
	//tb1->groupLayout()->addWidget(i2);

	QHBoxLayout *hbl = new QHBoxLayout();
	tb1->groupLayout()->addLayout(hbl);

	iisIconLabel *i3 = new iisIconLabel(QPixmap(":/images/win/editcopy.png"), "Do Task 3", tb1);
	tb1->addIconLabel(i3, false);
	hbl->addWidget(i3);
	//tb1->groupLayout()->addWidget(i3);
	iisIconLabel *i4 = new iisIconLabel(QPixmap(":/images/win/editcut.png"), "Do Task 4", tb1);
	tb1->addIconLabel(i4, false);
	hbl->addWidget(i4);
	i4->setColors(Qt::red, Qt::green, Qt::gray);
	i4->setFocusPen(QPen());
	//tb1->groupLayout()->addWidget(i4);
	iisIconLabel *i5 = new iisIconLabel(QPixmap(":/images/win/editpaste.png"), "Do Task 5", tb1);
	tb1->addIconLabel(i5);
	//tb1->groupLayout()->addWidget(i5);

	tb2 = new iisTaskBox(QPixmap(":/images/win/fileprint.png"), "Non-expandable Group", false, this);
	ui.panel->addWidget(tb2);

	iisIconLabel *i21 = new iisIconLabel(QPixmap(":/images/win/filenew.png"), "Do Task 2.1", tb2);
	tb2->addIconLabel(i21);
	//tb2->groupLayout()->addWidget(i21);
	iisIconLabel *i22 = new iisIconLabel(QPixmap(":/images/win/fileopen.png"), "Do Task 2.2", tb2);
	tb2->addIconLabel(i22);
	i22->setEnabled(false);
	//tb2->groupLayout()->addWidget(i22);
	iisIconLabel *i23 = new iisIconLabel(QPixmap(":/images/win/filesave.png"), "Do Task 2.3", tb2);
	tb2->addIconLabel(i23);
	//tb2->groupLayout()->addWidget(i23);

	tb3 = new iisTaskBox(QPixmap(), "Group without Icons", true, this);
	ui.panel->addWidget(tb3);

	iisIconLabel *i31 = new iisIconLabel(QPixmap(), "Do Task 3.1", tb3);
	tb3->addIconLabel(i31);

	iisIconLabel *i32 = new iisIconLabel(QPixmap(), "Do Task 3.2", tb3);
	tb3->addIconLabel(i32);

	tb3->groupLayout()->addWidget(new QLabel("Widgets also allowed:", this));
	tb3->groupLayout()->addWidget(new QPushButton("A Button", this));


	// Other widgets can be also added to the panel
	QLabel *l1 = new QLabel("A group without header", this);
	ui.panel->addWidget(l1);


	tb4 = new iisTaskGroup(this);
	ui.panel->addWidget(tb4);

	iisIconLabel *i41 = new iisIconLabel(QPixmap(":/images/win/textbold.png"), "Do Task 4.1", tb4);
	tb4->addIconLabel(i41);

	iisIconLabel *i42 = new iisIconLabel(QPixmap(), "Do Task 4.2", tb4);
	tb4->addIconLabel(i42);

	ui.panel->addStretch();
}

test::~test()
{

}

void test::on_rbDefault_clicked()
{
	ui.panel->setScheme(iisTaskPanelScheme::defaultScheme());
	//tb1->setScheme(iisTaskPanelScheme::defaultScheme());
	//tb2->setScheme(iisTaskPanelScheme::defaultScheme());
	//tb3->setScheme(iisTaskPanelScheme::defaultScheme());
	//tb4->setScheme(iisTaskPanelScheme::defaultScheme());
}

void test::on_rbXP_clicked()
{
	ui.panel->setScheme(iisWinXPTaskPanelScheme::defaultScheme());
	//tb1->setScheme(iisWinXPTaskPanelScheme::defaultScheme());
	tb2->setScheme(iisWinXPTaskPanelScheme2::defaultScheme());
	tb3->setScheme(iisWinXPTaskPanelScheme2::defaultScheme());
	//tb4->setScheme(iisWinXPTaskPanelScheme::defaultScheme());
}

