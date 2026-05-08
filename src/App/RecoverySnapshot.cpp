// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <sstream>
#include <utility>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Base/XMLTools.h>

#include "Application.h"
#include "Document.h"
#include "RecoverySnapshot.h"

namespace
{

class ScopedSaveThumbnailPreference
{
public:
    ScopedSaveThumbnailPreference(ParameterGrp::handle params, bool enabled)
        : params(std::move(params))
        , originalValue(this->params->GetBool("SaveThumbnail", true))
    {
        this->params->SetBool("SaveThumbnail", enabled);
    }

    ScopedSaveThumbnailPreference(const ScopedSaveThumbnailPreference&) = delete;
    ScopedSaveThumbnailPreference& operator=(const ScopedSaveThumbnailPreference&) = delete;

    ~ScopedSaveThumbnailPreference()
    {
        params->SetBool("SaveThumbnail", originalValue);
    }

private:
    ParameterGrp::handle params;
    bool originalValue;
};

std::string recoveryDirectoryFor(const App::Document& doc)
{
    std::string dirName = doc.TransientDir.getValue();
    dirName += "/fc_recovery_files";
    return dirName;
}

void writeRecoveryMetadataFile(const App::Document& doc)
{
    std::string fileName = doc.TransientDir.getValue();
    fileName += "/fc_recovery_file.xml";
    const auto escapedLabel = XMLTools::escapeXml(doc.Label.getValue());
    const auto escapedFileName = XMLTools::escapeXml(doc.FileName.getValue());

    Base::FileInfo fileInfo(fileName);
    Base::ofstream file(fileInfo, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw Base::FileException("Failed to open auto-recovery metadata file", fileInfo);
    }

    file << "<?xml version='1.0' encoding='utf-8'?>\n"
         << "<AutoRecovery SchemaVersion=\"1\">\n"
         << "  <Status>Created</Status>\n"
         << "  <Label>" << escapedLabel << "</Label>\n"
         << "  <FileName>" << escapedFileName << "</FileName>\n"
         << "</AutoRecovery>\n";
}

template<typename WriterT>
void writeRecoverySnapshotContents(const App::Document& doc, WriterT& writer)
{
    writer.putNextEntry("Document.xml");
    doc.Save(writer);

    // Special handling for Gui document state.
    doc.signalSaveDocument(writer);
    writer.writeFiles();

    if (writer.hasErrors()) {
        std::stringstream message;
        message << "Failed to write all data to auto-recovery output ";
        message << writer.getErrors().front();
        throw Base::FileException(message.str().c_str());
    }
}

void writeUncompressedRecoverySnapshot(const App::Document& doc, bool saveBinaryBrep)
{
    std::string dirName = recoveryDirectoryFor(doc);
    Base::FileInfo dir(dirName);
    if (!dir.exists() && !dir.createDirectory()) {
        throw Base::FileException("Failed to create auto-recovery directory", dir);
    }

    Base::FileWriter writer(dirName.c_str());
    if (saveBinaryBrep) {
        writer.setMode("BinaryBrep");
    }

    writeRecoverySnapshotContents(doc, writer);
}

void writeCompressedRecoverySnapshot(const App::Document& doc, bool saveBinaryBrep)
{
    std::string fileName = doc.TransientDir.getValue();
    fileName += "/fc_recovery_file.fcstd";

    Base::FileInfo fileInfo(fileName);
    Base::ofstream file(fileInfo, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        throw Base::FileException("Failed to open auto-recovery archive", fileInfo);
    }

    Base::ZipWriter writer(file);
    if (saveBinaryBrep) {
        writer.setMode("BinaryBrep");
    }

    writer.setComment("AutoRecovery file");
    writer.setLevel(1);  // Prefer lower latency over compression ratio for autosave.
    writeRecoverySnapshotContents(doc, writer);
}

}  // namespace

namespace App
{

bool writeRecoverySnapshotToTransientDir(const Document& doc,
                                         const RecoverySnapshotSaveOptions& options)
{
    if (!doc.canWriteRecoverySnapshot()) {
        std::stringstream message;
        message << "Document '" << doc.Label.getValue()
                << "' is not in a stable App state for recovery write";
        throw Base::RuntimeError(message.str().c_str());
    }

    auto params = GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document"
    );
    ScopedSaveThumbnailPreference saveThumbnailPreference(params, options.saveThumbnail);

    writeRecoveryMetadataFile(doc);

    if (!options.compressed) {
        writeUncompressedRecoverySnapshot(doc, options.saveBinaryBrep);
        return true;
    }

    writeCompressedRecoverySnapshot(doc, options.saveBinaryBrep);
    return true;
}

}  // namespace App
