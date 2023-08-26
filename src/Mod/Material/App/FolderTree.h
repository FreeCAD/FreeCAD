/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_FOLDERTREE_H
#define MATERIAL_FOLDERTREE_H

namespace Materials {

template <class T>
class FolderTreeNode
{
public:
    enum NodeType {
        DataNode,
        FolderNode
    };

    explicit FolderTreeNode() : _folder(nullptr) {}
    virtual ~FolderTreeNode() {}

    NodeType getType(void) const { return _type; }
    void setType(NodeType type) { _type = type; }

    const std::map<QString, FolderTreeNode<T>*> *getFolder(void) const { return _folder; }
    std::map<QString, FolderTreeNode<T>*> *getFolder(void) { return _folder; }
    const T *getData(void) const { return _data; }

    void setFolder(std::map<QString, FolderTreeNode<T>*> *folder)
    {
        setType(FolderNode);
        _folder = folder;
    }
    void setData(const T *data)
    {
        setType(DataNode);
        _data = data;
    }

private:
    NodeType    _type;
    std::map<QString, FolderTreeNode<T> *> *_folder;
    const T *_data;
};

} // namespace Materials

#endif // MATERIAL_FOLDERTREE_H
