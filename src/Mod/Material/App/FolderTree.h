/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MATERIAL_FOLDERTREE_H
#define MATERIAL_FOLDERTREE_H

#include <QString>
#include <map>
#include <memory>


namespace Materials
{

template<class T>
class FolderTreeNode
{
public:
    enum NodeType
    {
        DataNode,
        FolderNode
    };

    FolderTreeNode()
    {}
    virtual ~FolderTreeNode() = default;

    NodeType getType() const
    {
        return _type;
    }
    void setType(NodeType type)
    {
        _type = type;
    }

    const std::shared_ptr<std::map<QString, FolderTreeNode<T>*>> getFolder() const
    {
        return _folder;
    }
    std::shared_ptr<std::map<QString, FolderTreeNode<T>*>> getFolder()
    {
        return _folder;
    }
    const T* getData() const
    {
        return _data;
    }

    void setFolder(std::shared_ptr<std::map<QString, FolderTreeNode<T>*>> folder)
    {
        setType(FolderNode);
        _folder = folder;
    }
    void setData(const T* data)
    {
        setType(DataNode);
        _data = data;
    }

private:
    NodeType _type;
    std::shared_ptr<std::map<QString, FolderTreeNode<T>*>> _folder;
    const T* _data;
};

}  // namespace Materials

#endif  // MATERIAL_FOLDERTREE_H
