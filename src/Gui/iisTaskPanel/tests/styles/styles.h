/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef TEST_H
#define TEST_H

#include <QtGui/QMainWindow>
#include "ui_styles.h"

class iisTaskBox;
class iisTaskGroup;

class test : public QMainWindow
{
	Q_OBJECT

public:
	test(QWidget *parent = 0, Qt::WFlags flags = 0);
	~test();

protected slots:
	void on_rbDefault_clicked();
	void on_rbXP_clicked();

private:
	Ui::testClass ui;

	iisTaskBox *tb1, *tb2, *tb3;
	iisTaskGroup *tb4;
};

#endif // TEST_H
