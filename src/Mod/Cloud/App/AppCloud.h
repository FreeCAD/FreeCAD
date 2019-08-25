#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Base64.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <curl/curl.h>
#include <sys/time.h>
#include <time.h>
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

using namespace App;
using namespace std;
using namespace xercesc;

namespace Cloud {

class BaseExport CloudReader
{
public:
    CloudReader(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket);
    virtual ~CloudReader();
    int print=0;

    struct FileEntry
    {
        char FileName[1024];
        std::stringstream FileStream;
        int touch=0;
    };
    void checkText(DOMText* text);
    void checkXML(DOMNode* node);
    void checkElement(DOMElement* element);
    void addFile(struct Cloud::CloudReader::FileEntry *new_entry);
    struct FileEntry *GetEntry(std::string FileName);
    void DownloadFile(Cloud::CloudReader::FileEntry *entry);
    int isTouched(std::string FileName);
protected:
    std::list<Cloud::CloudReader::FileEntry*> FileList;
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

class BaseExport CloudWriter : public Base::Writer
{
public:
    CloudWriter(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket);
    virtual ~CloudWriter();
    void pushCloud(const char *FileName, const char *data, long size);
    void putNextEntry(const char* file);
    virtual void writeFiles(void);

    virtual std::ostream &Stream(void){return FileStream;}
    virtual bool shouldWrite(const std::string& name, const Base::Persistence *Object) const;

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
