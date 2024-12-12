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

#include <map>
#include <memory>

#include <QString>

namespace Materials
{

template<class T>
class FolderTreeNode
{
public:
    enum NodeType
    {
        UnknownNode,
        DataNode,
        FolderNode
    };

    FolderTreeNode()
        :_type(UnknownNode)
    {}
    virtual ~FolderTreeNode() = default;

    NodeType getType() const
    {
        assert(_type == DataNode || _type == FolderNode);
        return _type;
    }
    void setType(NodeType type)
    {
        _type = type;
    }

    const std::shared_ptr<std::map<QString, std::shared_ptr<FolderTreeNode<T>>>> getFolder() const
    {
        assert(_type == FolderNode);
        return _folder;
    }
    std::shared_ptr<std::map<QString, std::shared_ptr<FolderTreeNode<T>>>> getFolder()
    {
        assert(_type == FolderNode);
        return _folder;
    }
    std::shared_ptr<T> getData() const
    {
        assert(_type == DataNode);
        return _data;
    }
    QString getUUID() const
    {
        assert(_type == DataNode);
        return _uuid;
    }

    void setFolder(std::shared_ptr<std::map<QString, std::shared_ptr<FolderTreeNode<T>>>> folder)
    {
        setType(FolderNode);
        _folder = folder;
    }
    void setData(std::shared_ptr<T> data)
    {
        setType(DataNode);
        _data = data;
    }
    void setUUID(const QString uuid)
    {
        setType(DataNode);
        _uuid = uuid;
    }

private:
    NodeType _type;
    std::shared_ptr<std::map<QString, std::shared_ptr<FolderTreeNode<T>>>> _folder;
    QString _uuid;
    std::shared_ptr<T> _data;
};

}  // namespace Materials

#endif  // MATERIAL_FOLDERTREE_H
