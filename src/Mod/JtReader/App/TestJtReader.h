// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) Juergen Riegel 2007    <juergen.riegel@web.de>          *
 *   LGPL                                                                  *
 ***************************************************************************/

#include "JrJt/JtReader.h"


class TestJtReader: public JtReader
{
public:
    TestJtReader();
    ~TestJtReader();

    void read(void);
};
