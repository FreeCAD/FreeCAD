// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Standard_Version.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <NCollection_Vector.hxx>
#include <STEPCAFControl_Writer.hxx>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "WriterStep.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Stream.h>
#include <Base/Uuid.h>
#include <App/Application.h>
#include <Mod/Part/App/encodeFilename.h>
#include <Mod/Part/App/Interface.h>

using namespace Import;

namespace
{

constexpr std::size_t chunkSize = 64 * 1024;

// ISO 10303-21 restricts string literals to the basic alphabet and requires
// any other character to be encoded with the control directives \X2\ (UCS-2,
// 4 hex digits) and \X4\ (UCS-4, 8 hex digits), each closed by \X0\. OCCT
// however writes names as raw UTF-8 bytes, which conforming readers replace
// by '_'. This filter re-encodes every non-ASCII character inside a string
// literal. It works on a stream of bytes, so its memory use does not depend
// on the file size. Bytes that are not valid UTF-8 are taken as ISO 8859-1.
class StringLiteralEscaper
{
public:
    explicit StringLiteralEscaper(std::ostream& out)
        : out {out}
    {}

    void feed(const char* data, std::size_t size)
    {
        for (std::size_t i = 0; i < size; ++i) {
            feedByte(static_cast<unsigned char>(data[i]));
        }
        flushBuffer();
    }

    void finish()
    {
        dropPending();
        closeDirective();
        flushBuffer();
    }

    bool hasEscaped() const
    {
        return escaped;
    }

private:
    void feedByte(unsigned char byte)
    {
        if (byte < 0x80) {
            // OCCT wraps long literals at a fixed column, even inside a UTF-8
            // sequence. Line breaks are not significant in a literal, so keep
            // them back until the sequence is complete.
            if (pendingSize > 0 && (byte == '\r' || byte == '\n')) {
                lineBreaks += static_cast<char>(byte);
                return;
            }
            dropPending();
            putAscii(byte);
            return;
        }

        if (!inString) {
            // not expected outside of literals, leave it untouched
            buffer += static_cast<char>(byte);
            return;
        }

        if (pendingSize > 0) {
            if ((byte & 0xC0) == 0x80) {
                pending[pendingSize++] = byte;
                if (pendingSize == expected) {
                    completePending();
                }
                return;
            }
            dropPending();
        }
        startSequence(byte);
    }

    void startSequence(unsigned char lead)
    {
        if ((lead & 0xE0) == 0xC0) {
            expected = 2;
        }
        else if ((lead & 0xF0) == 0xE0) {
            expected = 3;
        }
        else if ((lead & 0xF8) == 0xF0) {
            expected = 4;
        }
        else {
            putCodePoint(lead);
            return;
        }
        pending[0] = lead;
        pendingSize = 1;
    }

    void completePending()
    {
        static constexpr std::array<unsigned char, 5> leadMask {0, 0, 0x1F, 0x0F, 0x07};
        static constexpr std::array<char32_t, 5> minimum {0, 0, 0x80, 0x800, 0x10000};

        char32_t value = pending[0] & leadMask[expected];
        for (std::size_t i = 1; i < expected; ++i) {
            value = (value << 6) | (pending[i] & 0x3F);
        }
        // reject overlong forms, UTF-16 surrogates and values beyond Unicode
        if (value < minimum[expected] || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
            dropPending();
            return;
        }

        pendingSize = 0;
        putCodePoint(value);
        putLineBreaks();
    }

    // Give up on an incomplete or invalid sequence and take its bytes as ISO 8859-1
    void dropPending()
    {
        for (std::size_t i = 0; i < pendingSize; ++i) {
            putCodePoint(pending[i]);
        }
        pendingSize = 0;
        putLineBreaks();
    }

    void putLineBreaks()
    {
        for (char lineBreak : lineBreaks) {
            putAscii(static_cast<unsigned char>(lineBreak));
        }
        lineBreaks.clear();
    }

    void putAscii(unsigned char byte)
    {
        closeDirective();
        // an embedded quote is written as '' and toggles twice
        if (byte == '\'') {
            inString = !inString;
        }
        buffer += static_cast<char>(byte);
    }

    void putCodePoint(char32_t codePoint)
    {
        static const char hexDigits[] = "0123456789ABCDEF";
        int needed = codePoint > 0xFFFF ? 4 : 2;
        if (needed != directive) {
            closeDirective();
            buffer += needed == 4 ? "\\X4\\" : "\\X2\\";
            directive = needed;
        }
        for (int shift = needed * 8 - 4; shift >= 0; shift -= 4) {
            buffer += hexDigits[(codePoint >> shift) & 0xF];
        }
        escaped = true;
    }

    void closeDirective()
    {
        if (directive != 0) {
            buffer += "\\X0\\";
            directive = 0;
        }
    }

    void flushBuffer()
    {
        out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        buffer.clear();
    }

    std::ostream& out;
    std::string buffer;
    std::array<unsigned char, 4> pending {};
    std::size_t pendingSize = 0;
    std::size_t expected = 0;
    std::string lineBreaks;
    int directive = 0;
    bool inString = false;
    bool escaped = false;
};

bool isNonAscii(char ch)
{
    return static_cast<unsigned char>(ch) >= 0x80;
}

// Run the written file through StringLiteralEscaper. The result goes to a
// temporary file next to it which then replaces the original, so a failure
// never damages the file written by OCCT. It is then kept unchanged and only
// a warning is given.
void escapeNonAsciiStrings(const Base::FileInfo& file)
{
    std::vector<char> chunk(chunkSize);
    auto readChunk = [&chunk](std::istream& in) {
        in.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        return static_cast<std::size_t>(in.gcount());
    };

    // most files are plain ASCII, check that first to avoid rewriting them
    bool found = false;
    {
        Base::ifstream in(file, std::ios::in | std::ios::binary);
        while (!found && in) {
            auto end = chunk.cbegin() + static_cast<std::ptrdiff_t>(readChunk(in));
            found = std::any_of(chunk.cbegin(), end, isNonAscii);
        }
        if (in.bad() || (!found && !in.eof())) {
            Base::Console().warning(
                "Cannot read STEP file to escape non-ASCII characters: {}\n",
                file.filePath()
            );
            return;
        }
    }
    if (!found) {
        return;
    }

    Base::FileInfo tmp(file.filePath() + "." + Base::Uuid::createUuid());
    bool written = false;
    bool escaped = false;
    {
        Base::ifstream in(file, std::ios::in | std::ios::binary);
        Base::ofstream out(tmp, std::ios::out | std::ios::trunc | std::ios::binary);
        if (in && out) {
            StringLiteralEscaper escaper(out);
            while (in) {
                std::size_t size = readChunk(in);
                escaper.feed(chunk.data(), size);
            }
            escaper.finish();
            escaped = escaper.hasEscaped();
            out.close();
            written = in.eof() && !in.bad() && !out.fail();
        }
    }

    if (written && !escaped) {
        // non-ASCII bytes outside of string literals only, nothing to replace
        tmp.deleteFile();
        return;
    }
    if (written && tmp.renameFile(file.filePath().c_str())) {
        return;
    }

    tmp.deleteFile();
    Base::Console().warning(
        "Cannot escape non-ASCII characters in STEP file, they are kept as UTF-8: {}\n",
        file.filePath()
    );
}

}  // namespace

WriterStep::WriterStep(const Base::FileInfo& file)  // NOLINT
    : file {file}
{}

void WriterStep::write(Handle(TDocStd_Document) hDoc) const  // NOLINT
{
    std::string utf8Name = file.filePath();
    std::string name8bit = Part::encodeFilename(utf8Name);

    STEPCAFControl_Writer writer;
#if OCC_VERSION_HEX >= 0x070900
    // Temporary workaround for OCCT 7.9+, see: https://github.com/Open-Cascade-SAS/OCCT/issues/1327
    // TODO: Remove or refine the guards or remove when the issue is fixed in OCCT 8.0.1
    writer.SetShapeFixParameters(DESTEP_Parameters::GetDefaultShapeFixParameters());
    ShapeProcess::OperationsFlags aFlags;
    aFlags.set(ShapeProcess::Operation::SplitCommonVertex);
    aFlags.set(ShapeProcess::Operation::DirectFaces);
    writer.SetShapeProcessFlags(aFlags);
#endif
    Part::Interface::writeStepAssembly(Part::Interface::Assembly::On);
    writer.Transfer(hDoc, STEPControl_AsIs);

    APIHeaderSection_MakeHeader makeHeader(writer.ChangeWriter().Model());
    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part")
                                             ->GetGroup("STEP");

    // Don't set name because STEP doesn't support UTF-8
    // https://forum.freecad.org/viewtopic.php?f=8&t=52967
    makeHeader.SetAuthorValue(
        1,
        new TCollection_HAsciiString(hGrp->GetASCII("Author", "Author").c_str())
    );
    makeHeader.SetOrganizationValue(1, new TCollection_HAsciiString(hGrp->GetASCII("Company").c_str()));
    makeHeader.SetOriginatingSystem(
        new TCollection_HAsciiString(App::Application::getExecutableName().c_str())
    );
    makeHeader.SetDescriptionValue(1, new TCollection_HAsciiString("FreeCAD Model"));
    IFSelect_ReturnStatus ret = writer.Write(name8bit.c_str());
    if (ret == IFSelect_RetError || ret == IFSelect_RetFail || ret == IFSelect_RetStop) {
        throw Base::FileException("Cannot open file: ", file);
    }

    escapeNonAsciiStrings(file);
}
