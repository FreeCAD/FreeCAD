/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include "FeatureClone.h"

#include <TopExp_Explorer.hxx>
#include <Base/Console.h>
#include <AttachExtension.h>

using namespace Part;

PROPERTY_SOURCE(Part::Clone, Part::Feature)

Clone::Clone()
{
    ADD_PROPERTY_TYPE(Objects, (0), "Clone", App::Prop_None, "The objects included in this clone");
    Objects.setSize(0);

    ADD_PROPERTY_TYPE(Scale, (Base::Vector3d(1.0, 1.0, 1.0)), "Clone", App::Prop_None, "The scale factor of this clone");
    ADD_PROPERTY_TYPE(Fuse, (0), "Clone", App::Prop_None, "If Clone includes several objects,\nset True for fusion or False for compound");
}

short Clone::mustExecute() const
{
    if (Objects.isTouched() ||
        Scale.isTouched() ||
        Fuse.isTouched()) {
        return 1;
    }

    return 0;
}

TopoDS_Shape Clone::joinShapes(std::vector<TopoDS_Shape> &shapes)
{
    std::vector<TopoDS_Shape> temp;
    std::vector<TopoDS_Shape> *eff = &shapes;

    if (Fuse.getValue()) {
        TopAbs_ShapeEnum types[3] = { TopAbs_SOLID, TopAbs_FACE, TopAbs_EDGE };

        for (int i = 0; i < 3; ++i) {
            for (std::vector<TopoDS_Shape>::iterator it = shapes.begin(); it != shapes.end(); ++it) {
                TopExp_Explorer explorer(*it, types[i]);
                while (explorer.More()) {
                    temp.push_back(explorer.Current());
                    explorer.Next();
                }
            }

            if (temp.size() > 0) {
                break;
            }
        }

        eff = &temp;
    }

    if (eff->size() > 1) {
        if (Fuse.getValue()) {
            try {
                TopoDS_Shape fusion = TopoShape((*eff)[0]).fuse(std::vector<TopoDS_Shape>(eff->begin() + 1, eff->end()));
                return TopoShape(fusion).removeSplitter();
            }
            catch (...) {
                // Fusion failed, will return a compound
            }
        }

        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);

        for (std::vector<TopoDS_Shape>::iterator it = eff->begin(); it != eff->end(); ++it) {
            if (!it->IsNull()) {
                builder.Add(compound, *it);
            }
        }

        return compound;
    }

    return (*eff)[0];
}

App::DocumentObjectExecReturn *Clone::execute(void)
{
    Base::Placement placement = this->Placement.getValue();

    std::vector<TopoDS_Shape> shapes;
    std::vector<App::DocumentObject *> objects = this->Objects.getValues();

    for (std::vector<App::DocumentObject *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        TopoDS_Shape shape = Feature::getShape(*it);
        if (!shape.IsNull()) {
            shapes.push_back(shape);
        }
    }

    if (shapes.size() > 0) {
        TopoShape shape = TopoShape(joinShapes(shapes));

        if (!shape.isNull() && this->Scale.getValue() != Base::Vector3d(1.0, 1.0, 1.0)) {
            Base::Placement origPlacement = shape.getPlacement();
            shape.setPlacement(Base::Placement());

            Base::Matrix4D mx;
            mx.scale(this->Scale.getValue());
            if (this->Scale.getValue().x == this->Scale.getValue().y
                && this->Scale.getValue().y == this->Scale.getValue().z) {
                shape.transformShape(mx, false);
            }
            else {
                shape.transformGeometry(mx);
            }

            shape.setPlacement(origPlacement);
        }

        this->Shape.setValue(shape.getShape());
    }

    Placement.setValue(placement);

    return App::DocumentObject::StdReturn;
}
