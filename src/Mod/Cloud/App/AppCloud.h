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

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Base64.h>
#include <Base/TimeInfo.h>
#include <xlocale.h>

#include <App/PropertyContainer.h>
#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/Document.h>

#include <xercesc/framework/XMLPScanToken.hpp>
#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

XERCES_CPP_NAMESPACE_BEGIN
    class DOMNode;
    class DOMText;
XERCES_CPP_NAMESPACE_END

namespace Cloud {

struct AmzData {
	std::string digest;
	char dateFormatted[256];
	char ContentType[256];
	char Host[256];
	char *MD5;
};

void eraseSubStr(std::string & Str, const std::string & toErase);
size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
struct AmzData *ComputeDigestAmzS3v2(char *operation, char *data_type, const char *target, const char *Secret, const char *ptr, long size);
struct curl_slist *BuildHeaderAmzS3v2(const char *Url, const char *TcpPort, const char *PublicKey, struct AmzData *Data);
char *MD5Sum(const char *ptr, long size);

class CloudAppExport CloudReader
{
public:
    CloudReader(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket);
    virtual ~CloudReader();
    int file=0;
    int continuation=0;
    int truncated=0;

    struct FileEntry
    {
        char FileName[1024];
        std::stringstream FileStream;
        int touch=0;
    };
    void checkText(XERCES_CPP_NAMESPACE_QUALIFIER DOMText* text);
    void checkXML(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node);
    void checkElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* element);
    void addFile(struct Cloud::CloudReader::FileEntry *new_entry);
    struct FileEntry *GetEntry(std::string FileName);
    void DownloadFile(Cloud::CloudReader::FileEntry *entry);
    int isTouched(std::string FileName);
protected:
    std::list<Cloud::CloudReader::FileEntry*> FileList;
    char* NextFileName;
    const char* Url;
    const char* TcpPort;
    const char* AccessKey;
    const char* SecretKey;
    const char* Bucket;
};

class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Cloud")
    {
	add_varargs_method("cloudurl",&Module::sCloudUrl,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );

	add_varargs_method("cloudaccesskey",&Module::sCloudAccessKey,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );

	add_varargs_method("cloudsecretkey",&Module::sCloudSecretKey,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );
	
	add_varargs_method("cloudtcpport",&Module::sCloudTcpPort,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );

	add_varargs_method("cloudsave",&Module::sCloudSave,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );

        add_varargs_method("cloudrestore",&Module::sCloudRestore,
            "cloudurl(string) -- Connect to a Cloud Storage service."
        );

        initialize("This module is the Cloud module."); // register with Python
    }

    virtual ~Module() {}

    App::PropertyString Url;
    App::PropertyString TcpPort;
    App::PropertyString AccessKey;
    App::PropertyString SecretKey;
    bool cloudSave(const char* BucketName);
    bool cloudRestore(const char* BucketName);

private:
    Py::Object sCloudUrl  (const Py::Tuple& args);
    Py::Object sCloudAccessKey  (const Py::Tuple& args);
    Py::Object sCloudSecretKey  (const Py::Tuple& args);
    Py::Object sCloudTcpPort  (const Py::Tuple& args);
    Py::Object sCloudSave  (const Py::Tuple& args);
    Py::Object sCloudRestore  (const Py::Tuple& args);


};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}


// This class is writing files to an S3 cloud storage class

class CloudAppExport CloudWriter : public Base::Writer
{
public:
    int print=0;
    char errorCode[1024]="";
    CloudWriter(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket);
    virtual ~CloudWriter();
    void pushCloud(const char *FileName, const char *data, long size);
    void putNextEntry(const char* file);
    void createBucket();
    virtual void writeFiles(void);

    virtual std::ostream &Stream(void){return FileStream;}
    virtual bool shouldWrite(const std::string& name, const Base::Persistence *Object) const;
    void checkText(XERCES_CPP_NAMESPACE_QUALIFIER DOMText* text);
    void checkXML(XERCES_CPP_NAMESPACE_QUALIFIER DOMNode* node);
    void checkElement(XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* element);

protected:
    std::string FileName;
    const char* Url;
    const char* TcpPort;
    const char* AccessKey;
    const char* SecretKey;
    const char* Bucket;
    std::stringstream FileStream;
};


}

void readFiles(Cloud::CloudReader reader, Base::XMLReader *xmlreader);
