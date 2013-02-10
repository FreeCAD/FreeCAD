/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include <QtGui/QApplication>
#include "styles.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	test w;
	w.show();
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	return a.exec();
}
