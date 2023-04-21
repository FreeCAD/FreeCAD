# SPDX-License-Identifier: MIT
# Copyright 2020 Andy Maloney <asmaloney@gmail.com>

target_sources( E57Format
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/BlobNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/BlobNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CheckedFile.h
        ${CMAKE_CURRENT_LIST_DIR}/CheckedFile.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Common.h
        ${CMAKE_CURRENT_LIST_DIR}/Common.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorReaderImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorReaderImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorWriterImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/CompressedVectorWriterImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/DecodeChannel.h
        ${CMAKE_CURRENT_LIST_DIR}/DecodeChannel.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Decoder.h
        ${CMAKE_CURRENT_LIST_DIR}/Decoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Encoder.h
        ${CMAKE_CURRENT_LIST_DIR}/Encoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/FloatNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/FloatNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/IntegerNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/IntegerNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/NodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/NodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/Packet.h
        ${CMAKE_CURRENT_LIST_DIR}/Packet.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ImageFileImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ImageFileImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/ReaderImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ReaderImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/ScaledIntegerNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/ScaledIntegerNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/SectionHeaders.h
        ${CMAKE_CURRENT_LIST_DIR}/SectionHeaders.cpp
        ${CMAKE_CURRENT_LIST_DIR}/SourceDestBufferImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/SourceDestBufferImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/StringNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/StringNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/StructureNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/StructureNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/VectorNodeImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/VectorNodeImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/WriterImpl.cpp
        ${CMAKE_CURRENT_LIST_DIR}/WriterImpl.h
        ${CMAKE_CURRENT_LIST_DIR}/E57Exception.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57Format.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57SimpleData.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57SimpleReader.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57SimpleWriter.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57Version.h
        ${CMAKE_CURRENT_LIST_DIR}/E57XmlParser.cpp
        ${CMAKE_CURRENT_LIST_DIR}/E57XmlParser.h
)

target_include_directories( E57Format
	PRIVATE
	    ${CMAKE_CURRENT_SOURCE_DIR}
)
