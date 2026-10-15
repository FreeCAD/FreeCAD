// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <gtest/gtest.h>
#include "gmock/gmock.h"

#include <memory>
#include <sstream>
#include <string>

#include <App/Application.h>
#include <App/Document.h>
#include <App/VRMLObject.h>
#include <Base/FileInfo.h>
#include <Base/ZipOutputStream.h>
#include <Base/ZipReader.h>
#include <src/App/InitApplication.h>

// NOLINTBEGIN

class VRMLObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        document = App::GetApplication().openDocument(fileName().c_str());
    }

    void TearDown() override
    {
        if (document) {
            App::GetApplication().closeDocument(document->getName());
        }
    }

    std::string fileName() const
    {
        std::string resDir(DATADIR);
        resDir.append("/tests/TestVRMLTextures.FCStd");
        return resDir;
    }

    App::Document* getDocument() const
    {
        return document;
    }

private:
    App::Document* document {};
};

TEST_F(VRMLObjectTest, loadVRMLWithTextures)
{
    App::Document* doc = getDocument();
    ASSERT_TRUE(doc);

    auto vrml = dynamic_cast<App::VRMLObject*>(doc->getActiveObject());
    ASSERT_TRUE(vrml);

    auto res = vrml->Resources.getValues();
    EXPECT_EQ(res.size(), 6);
    EXPECT_EQ(res[0], std::string("FreeCAD/FreeCAD1.png"));
    EXPECT_EQ(res[1], std::string("FreeCAD/FreeCAD2.png"));
    EXPECT_EQ(res[2], std::string("FreeCAD/FreeCAD3.png"));
    EXPECT_EQ(res[3], std::string("FreeCAD/FreeCAD4.png"));
    EXPECT_EQ(res[4], std::string("FreeCAD/FreeCAD5.png"));
    EXPECT_EQ(res[5], std::string("FreeCAD/FreeCAD6.png"));

    auto url = vrml->Urls.getValues();
    EXPECT_EQ(url.size(), 6);
    for (const auto& it : url) {
        Base::FileInfo fi(it);
        EXPECT_TRUE(fi.isFile());
        EXPECT_TRUE(fi.exists());
    }
}

// Regression tests for GHSA-9624-pf2m-8cgg: a crafted .FCStd must not be able to write a VRML
// resource outside the document transient directory. The document controls both the resource
// name in Document.xml and the matching ZIP member, so a traversal name such as
// "FreeCAD/../marker.txt" reaches VRMLObject::restoreTextureFinished() unchanged (the pre-existing
// fixRelativePath() only rewrites the first path component when it differs from the object name).
class VRMLObjectSecurityTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void TearDown() override
    {
        if (_document) {
            App::GetApplication().closeDocument(_document->getName());
        }
        for (const auto& marker : _markers) {
            Base::FileInfo(marker).deleteFile();
        }
        if (!_craftedFile.empty()) {
            Base::FileInfo(_craftedFile).deleteFile();
        }
    }

    std::string baseFileName() const
    {
        return std::string(DATADIR) + "/tests/TestVRMLTextures.FCStd";
    }

    std::string craftTraversalDocument(const std::string& evilResource)
    {
        const std::string original("FreeCAD/FreeCAD1.png");
        std::string output = Base::FileInfo::getTempFileName() + ".FCStd";

        Base::ZipReader input {Base::FileInfo(baseFileName())};
        Base::ZipOutputStream stream {Base::FileInfo(output)};
        for (std::string name : input.entryNames()) {
            std::unique_ptr<std::istream> in(input.getInputStream(name));
            std::ostringstream buffer;
            buffer << in->rdbuf();
            std::string data = buffer.str();

            if (name == "Document.xml") {
                std::string::size_type pos = data.find(original);
                EXPECT_NE(pos, std::string::npos) << "base test file layout changed";
                if (pos != std::string::npos) {
                    data.replace(pos, original.size(), evilResource);
                }
            }
            else if (name == original) {
                name = evilResource;
                data = "VRML_RESOURCE_PATH_TRAVERSAL_MARKER\n";
            }

            stream.putNextEntry(name);
            stream.write(data.data(), static_cast<std::streamsize>(data.size()));
            stream.closeEntry();
        }
        stream.close();

        _craftedFile = output;
        return output;
    }

    App::VRMLObject* openCraftedDocument(const std::string& evilResource)
    {
        std::string crafted = craftTraversalDocument(evilResource);
        _document = App::GetApplication().openDocument(crafted.c_str());
        return _document ? dynamic_cast<App::VRMLObject*>(_document->getActiveObject()) : nullptr;
    }

    App::Document* _document {};
    std::string _craftedFile;
    std::vector<std::string> _markers;
};

TEST_F(VRMLObjectSecurityTest, singleLevelTraversalDoesNotEscapeResourceSubdirectory)
{
    App::VRMLObject* vrml = openCraftedDocument("FreeCAD/../vrml-escape-marker.txt");
    ASSERT_TRUE(vrml);

    std::string transientDir = _document->TransientDir.getValue();
    std::string escaped = transientDir + "/vrml-escape-marker.txt";
    _markers.push_back(escaped);

    EXPECT_FALSE(Base::FileInfo(escaped).exists()) << "resource escaped its subdirectory";
    EXPECT_TRUE(vrml->Urls.getValues()[0].empty()) << "rejected resource must have no URL";
}

TEST_F(VRMLObjectSecurityTest, deepTraversalIsRejected)
{
    App::VRMLObject* vrml = openCraftedDocument("FreeCAD/../../../vrml-deep-marker.txt");
    ASSERT_TRUE(vrml);

    EXPECT_TRUE(vrml->Urls.getValues()[0].empty()) << "rejected resource must have no URL";
}

TEST_F(VRMLObjectSecurityTest, legitimateResourcesStillLoadAlongsideRejectedEntry)
{
    App::VRMLObject* vrml = openCraftedDocument("FreeCAD/../vrml-escape-marker.txt");
    ASSERT_TRUE(vrml);
    _markers.push_back(std::string(_document->TransientDir.getValue()) + "/vrml-escape-marker.txt");

    auto urls = vrml->Urls.getValues();
    ASSERT_EQ(urls.size(), 6);
    for (std::size_t index = 1; index < urls.size(); ++index) {
        Base::FileInfo fi(urls[index]);
        EXPECT_TRUE(fi.exists()) << "unrelated resource " << index << " failed to load";
    }
}

// NOLINTEND
