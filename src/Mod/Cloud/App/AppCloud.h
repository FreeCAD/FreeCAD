/***************************************************************************
 *   Copyright (c) 2019 Jean-Marie Verdun         jmverdun3@gmail.com      *
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

#include <App/Document.h>

#include <Base/Base64.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>


XERCES_CPP_NAMESPACE_BEGIN
class DOMNode;
class DOMText;
XERCES_CPP_NAMESPACE_END

namespace Cloud
{

struct AmzData
{
    std::string digest;
    char dateFormatted[256];
    char ContentType[256];
    char Host[256];
    char* MD5;
};

struct AmzDatav4
{
    std::string digest;
    char dateFormattedS[256];
    char dateFormattedD[256];
    char ContentType[256];
    char Host[256];
    std::string Region;
    char* MD5;
    char* SHA256Sum;
};

std::string getHexValue(unsigned char* input, unsigned int HMACLength);
void eraseSubStr(std::string& Str, const std::string& toErase);
size_t CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s);
struct AmzData* ComputeDigestAmzS3v2(char* operation,
                                     char* data_type,
                                     const char* target,
                                     const char* Secret,
                                     const char* ptr,
                                     long size);
struct AmzDatav4* ComputeDigestAmzS3v4(char* operation,
                                       const char* server,
                                       char* data_type,
                                       const char* target,
                                       const char* Secret,
                                       const char* ptr,
                                       long size,
                                       char* parameters,
                                       std::string Region);
struct curl_slist* BuildHeaderAmzS3v2(const char* URL,
                                      const char* TCPPort,
                                      const char* PublicKey,
                                      struct AmzData* Data);
struct curl_slist*
BuildHeaderAmzS3v4(const char* URL, const char* PublicKey, struct AmzDatav4* Data);
char* MD5Sum(const char* ptr, long size);
char* SHA256Sum(const char* ptr, long size);

class CloudAppExport CloudReader
{
public:
    CloudReader(const char* URL,
                const char* AccessKey,
                const char* SecretKey,
                const char* TCPPort,
                const char* Bucket,
                std::string ProtocolVersion,
                std::string Region);
    virtual ~CloudReader();
    int file = 0;
    int continuation = 0;
    int truncated = 0;

    struct FileEntry
    {
        char FileName[1024];
        std::stringstream FileStream;
        int touch = 0;
    };
    void checkText(XERCES_CPP_NAMESPACE_QUALIFIER DOMText* text);
    void checkXML(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node);
    void checkElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* element);
    void addFile(struct Cloud::CloudReader::FileEntry* new_entry);
    struct FileEntry* GetEntry(std::string FileName);
    void DownloadFile(Cloud::CloudReader::FileEntry* entry);
    int isTouched(std::string FileName);

protected:
    std::list<Cloud::CloudReader::FileEntry*> FileList;
    char* NextFileName;
    const char* URL;
    const char* TCPPort;
    const char* TokenAuth;
    const char* TokenSecret;
    const char* Bucket;
    std::string ProtocolVersion;
    std::string Region;
};

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("Cloud")
    {
        add_varargs_method("URL",
                           &Module::sCloudURL,
                           "URL(string) -- Connect to a Cloud Storage service.");

        add_varargs_method("TokenAuth",
                           &Module::sCloudTokenAuth,
                           "TokenAuth(string) -- Token Authorization string.");

        add_varargs_method("TokenSecret",
                           &Module::sCloudTokenSecret,
                           "TokenSecret(string) -- Token Secret string.");

        add_varargs_method("TCPPort", &Module::sCloudTCPPort, "TCPPort(string) -- Port number.");

        add_varargs_method("Save",
                           &Module::sCloudSave,
                           "Save(string) -- Save the active document to the Cloud.");

        add_varargs_method("Restore",
                           &Module::sCloudRestore,
                           "Restore(string) -- Restore to the active document from the Cloud.");

        add_varargs_method(
            "ProtocolVersion",
            &Module::sCloudProtocolVersion,
            "ProtocolVersion(string) -- Specify Amazon s3 protocol version (2 or 4)");
        add_varargs_method("Region",
                           &Module::sCloudRegion,
                           "Region(string) -- Specify Amazon s3 Region");

        initialize("This module is the Cloud module.");  // register with Python
    }

    virtual ~Module()
    {}

    App::PropertyString URL;
    App::PropertyString TCPPort;
    App::PropertyString TokenAuth;
    App::PropertyString TokenSecret;
    App::PropertyString ProtocolVersion;
    App::PropertyString Region;
    bool cloudSave(const char* BucketName);
    bool cloudRestore(const char* BucketName);
    void LinkXSetValue(std::string filename);

private:
    Py::Object sCloudURL(const Py::Tuple& args);
    Py::Object sCloudTokenAuth(const Py::Tuple& args);
    Py::Object sCloudTokenSecret(const Py::Tuple& args);
    Py::Object sCloudTCPPort(const Py::Tuple& args);
    Py::Object sCloudSave(const Py::Tuple& args);
    Py::Object sCloudRestore(const Py::Tuple& args);
    Py::Object sCloudProtocolVersion(const Py::Tuple& args);
    Py::Object sCloudRegion(const Py::Tuple& args);
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}


// This class is writing files to an S3 cloud storage class

class CloudAppExport CloudWriter: public Base::Writer
{
public:
    int print = 0;
    char errorCode[1024] = "";
    CloudWriter(const char* URL,
                const char* TokenAuth,
                const char* TokenSecret,
                const char* TCPPort,
                const char* Bucket,
                std::string ProtocolVersion,
                std::string Region);
    virtual ~CloudWriter();
    void pushCloud(const char* FileName, const char* data, long size);
    void putNextEntry(const char* file);
    void createBucket();
    virtual void writeFiles(void);

    virtual std::ostream& Stream(void)
    {
        return FileStream;
    }
    virtual bool shouldWrite(const std::string& name, const Base::Persistence* Object) const;
    void checkText(XERCES_CPP_NAMESPACE_QUALIFIER DOMText* text);
    void checkXML(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node);
    void checkElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* element);

protected:
    std::string FileName;
    const char* URL;
    const char* TCPPort;
    const char* TokenAuth;
    const char* TokenSecret;
    const char* Bucket;
    std::string ProtocolVersion;
    std::string Region;
    std::stringstream FileStream;
};


}  // namespace Cloud

void readFiles(Cloud::CloudReader reader, Base::XMLReader* xmlreader);
