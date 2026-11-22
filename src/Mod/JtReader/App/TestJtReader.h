// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2007 Juergen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

#include "JrJt/JtReader.h"


class TestJtReader: public JtReader
{
public:
    TestJtReader();
    ~TestJtReader();

    void read(void);
};
