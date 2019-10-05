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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <curl/curl.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentPy.h>
#include <App/DocumentObserverPython.h>

#include <Base/Console.h>
#include <Base/PyObjectBase.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "AppCloud.h"

using namespace App;
using namespace std;
XERCES_CPP_NAMESPACE_USE

/* Python entry */
PyMOD_INIT_FUNC(Cloud)
{
    PyObject* mod = Cloud::initModule();
    Base::Console().Log("Loading Cloud module... done\n");
    PyMOD_Return(mod);
}

Py::Object Cloud::Module::sCloudUrl(const Py::Tuple& args)
{
    char *Url;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Url))     // convert args: Python->C
        return Py::None();
    if (this->Url.getStrValue() != Url) {
                this->Url.setValue(Url);
    }
    return Py::None();
}

Py::Object Cloud::Module::sCloudAccessKey(const Py::Tuple& args)
{    
    char *AccessKey;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8", &AccessKey))     // convert args: Python->C
        return Py::None();
    if (this->AccessKey.getStrValue() != AccessKey) {
           this->AccessKey.setValue(AccessKey);
    }
    return Py::None();
}

Py::Object Cloud::Module::sCloudSecretKey(const Py::Tuple& args)
{
    char *SecretKey;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8", &SecretKey))     // convert args: Python->C
        return Py::None();
    if (this->SecretKey.getStrValue() != SecretKey) {
             this->SecretKey.setValue(SecretKey);
    }
    return Py::None();
}

Py::Object Cloud::Module::sCloudTcpPort(const Py::Tuple& args)
{
    char *TcpPort;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8", &TcpPort))     // convert args: Python->C
        return Py::None();
    if (this->TcpPort.getStrValue() != TcpPort) {
            this->TcpPort.setValue(TcpPort);
    }
    return Py::None();
}

Py::Object Cloud::Module::sCloudSave(const Py::Tuple& args)
{
    char *pDoc;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8", &pDoc))     // convert args: Python->C
        return Py::None();
    cloudSave(pDoc);   
    return Py::None();
}


Py::Object Cloud::Module::sCloudRestore(const Py::Tuple& args)
{
    char *pDoc;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &pDoc))     // convert args: Python->C
        return Py::None();
    cloudRestore(pDoc);
    return Py::None();
}

struct data_buffer
{
        const char *ptr;
        size_t remaining_size;
};

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{

  struct data_buffer *local_ptr = (struct data_buffer *)stream;
  size_t data_to_transfer = size * nmemb;

  if(local_ptr->remaining_size) {
    // copy as much as possible from the source to the destination
    size_t copy_this_much = local_ptr->remaining_size;
    if(copy_this_much > data_to_transfer)
      copy_this_much = data_to_transfer;
    memcpy(ptr, local_ptr->ptr, copy_this_much);

    local_ptr->ptr += copy_this_much;
    local_ptr->remaining_size -= copy_this_much;
    return copy_this_much;
  }

  return 0;
}

void Cloud::CloudWriter::checkXML(DOMNode* node) {
     if (node) {
           switch (node->getNodeType()) {
           case DOMNode::ELEMENT_NODE:
                checkElement(static_cast<DOMElement*>(node));
                break;
           case DOMNode::TEXT_NODE:
                checkText(static_cast<DOMText*>(node));
                break;
           default:
                break;
           }
           DOMNode* child = node->getFirstChild();
           while (child) {
                DOMNode* next = child->getNextSibling();
                checkXML(child);
                child = next;
           }
     }
}

void Cloud::CloudWriter::checkElement(DOMElement* element) {
     char* name = XMLString::transcode(element->getTagName());
     if ( strcmp(name, "Code") == 0 )
        print=1;
     XMLString::release(&name);

}

void Cloud::CloudWriter::checkText(DOMText* text) {

     XMLCh* buffer = new XMLCh[XMLString::stringLen(text->getData()) + 1];
     XMLString::copyString(buffer, text->getData());
     XMLString::trim(buffer);
     char* content=XMLString::transcode(buffer);
     delete[] buffer;
     if ( print )
     {
		strcpy(errorCode,content);
     }
     print=0;
     XMLString::release(&content);
}


void Cloud::CloudWriter::createBucket()
{
	printf("CREATING BUCKET %s\n", this->Bucket);
        struct timeval tv;
        struct tm *tm;
        char date_formatted[256];
        char StringToSign[1024];
        unsigned char *digest;

        CURL *curl;
        CURLcode res;

        struct data_buffer curl_buffer;

#if defined(FC_OS_WIN32)
#else
        gettimeofday(&tv,NULL);
        tm = localtime(&tv.tv_sec);
        strftime(date_formatted,256,"%a, %d %b %Y %T %z", tm);
#endif

        // CHANGEME
        sprintf(StringToSign,"PUT\n\napplication/xml\n%s\n/%s/", date_formatted, this->Bucket);

        // We have to use HMAC encoding and SHA1
        digest=HMAC(EVP_sha1(),this->SecretKey,strlen(this->SecretKey),
                (const unsigned char *)&StringToSign,strlen(StringToSign),NULL,NULL);

        // Let's build the Header and call to curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if ( curl )
        {
                struct curl_slist *chunk = NULL;
                char header_data[1024];
                // Let's build our own header
                std::string strUrl(this->Url);
                eraseSubStr(strUrl,"http://");
                eraseSubStr(strUrl,"https://");

                sprintf(header_data,"Host: %s", strUrl.c_str());
                chunk = curl_slist_append(chunk, header_data);
                sprintf(header_data,"Date: %s", date_formatted);
                chunk = curl_slist_append(chunk, header_data);
                chunk = curl_slist_append(chunk, "Content-Type: application/xml");
                std::string digest_str;
                digest_str=Base::base64_encode(digest,strlen((const char *)digest));
                sprintf(header_data,"Authorization: AWS %s:%s", this->AccessKey,
                digest_str.c_str());
                chunk = curl_slist_append(chunk, header_data);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
//                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                sprintf(header_data,"%s:%s/%s/", this->Url,this->TcpPort,
                                                    this->Bucket);
                curl_easy_setopt(curl, CURLOPT_URL, header_data);
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                curl_easy_setopt(curl, CURLOPT_PUT, 1L);
                // curl read a file not a memory buffer (it shall be able to do it)
                curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

                curl_buffer.ptr = NULL;
                curl_buffer.remaining_size = (size_t) 0;

                curl_easy_setopt(curl, CURLOPT_READDATA, &curl_buffer);

                curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)0);
                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                      fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                curl_easy_cleanup(curl);
        }
}

Cloud::CloudWriter::CloudWriter(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket)
{
        struct timeval tv;
        struct tm *tm;
        char date_formatted[256];
        char StringToSign[1024];
        unsigned char *digest;
        CURL *curl;
        CURLcode res;

        std::string s;

        this->Url=Url;
        this->AccessKey=AccessKey;
        this->SecretKey=SecretKey;
        this->TcpPort=TcpPort;
        this->Bucket=Bucket;
        this->FileName="";
        // Amazon S3 and Swift require the timezone to be define to GMT.
        // As to simplify the conversion this is performed through the TZ
        // environment variable and a call to localtime as to convert output of gettimeofday
#if defined(FC_OS_WIN32)
        _putenv("TZ=GMT");
#else
        setenv("TZ","GMT",1);
#endif
	// We must check that the bucket exists or not. If not, we have to create it.
	// We must request the bucketlist if we receive a 404 error. This means that it
    // doesn't exist. The other option is that the content list is empty.
	// This piece of code is the same as the Reader except we do not interpret it !
#if defined(FC_OS_WIN32)
#else
        gettimeofday(&tv,NULL);
        tm = localtime(&tv.tv_sec);
        strftime(date_formatted,256,"%a, %d %b %Y %T %z", tm);
#endif

        sprintf(StringToSign,"GET\n\napplication/xml\n%s\n/%s/", date_formatted, this->Bucket);

        // We have to use HMAC encoding and SHA1
        digest=HMAC(EVP_sha1(),this->SecretKey,strlen(this->SecretKey),
                (const unsigned char *)&StringToSign,strlen(StringToSign),NULL,NULL);

        // Let's build the Header and call to curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if ( curl )
        {
                struct curl_slist *chunk = NULL;
                char header_data[1024];
                // Let's build our own header
                std::string strUrl(this->Url);
                eraseSubStr(strUrl,"http://");
                eraseSubStr(strUrl,"https://");

                sprintf(header_data,"Host: %s:%s", strUrl.c_str(),
                                                   this->TcpPort);
                chunk = curl_slist_append(chunk, header_data);
                sprintf(header_data,"Date: %s", date_formatted);
                chunk = curl_slist_append(chunk, header_data);
                chunk = curl_slist_append(chunk, "Content-Type: application/xml");
                std::string digest_str;
                digest_str=Base::base64_encode(digest,strlen((const char *)digest));
                sprintf(header_data,"Authorization: AWS %s:%s", this->AccessKey,
                digest_str.c_str());
                chunk = curl_slist_append(chunk, header_data);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                sprintf(header_data,"%s:%s/%s/", this->Url,this->TcpPort,
                                                    this->Bucket);
                curl_easy_setopt(curl, CURLOPT_URL, header_data);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
                // curl read a file not a memory buffer (it shall be able to do it)
                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                      fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                curl_easy_cleanup(curl);


                std::stringstream input(s);

                try { XMLPlatformUtils::Initialize(); }
                        catch (const XMLException& toCatch) {
                            char* message = XMLString::transcode(toCatch.getMessage());
                            cout << "Error during initialization! :\n"
                                 << message << "\n";
                     XMLString::release(&message);
                     return ;
                }

                XercesDOMParser* parser = new XercesDOMParser();
                parser->setValidationScheme(XercesDOMParser::Val_Always);
                parser->setDoNamespaces(true);

                xercesc::MemBufInputSource myxml_buf((const XMLByte *const)s.c_str(), s.size(),
                                     "myxml (in memory)");

                parser->parse(myxml_buf);
                auto* dom=parser->getDocument();
		// Is there an Error entry into the document ?
		// if yes then we must create the Bucket
                checkXML(dom);
		if ( strcmp(errorCode,"NoSuchBucket") == 0 )
		{
			// we must create the Bucket using a PUT request
			createBucket();
		}
	
	}

}

Cloud::CloudWriter::~CloudWriter()
{
}

size_t Cloud::CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size*nmemb;
    try
    {
        s->append((char*)contents, newLength);
    }
    catch(std::bad_alloc &)
    {
        //handle memory problem
        return 0;
    }
    return newLength;
}

void Cloud::CloudReader::checkElement(DOMElement* element) {
     char* name = XMLString::transcode(element->getTagName());
     if ( strcmp(name, "Key") == 0 )
        print=1;
     XMLString::release(&name);

}

void Cloud::CloudReader::checkText(DOMText* text) {

     XMLCh* buffer = new XMLCh[XMLString::stringLen(text->getData()) + 1];
     XMLString::copyString(buffer, text->getData());
     XMLString::trim(buffer);
     struct Cloud::CloudReader::FileEntry *new_entry;
     char* content=XMLString::transcode(buffer);
     delete[] buffer;
     if ( print )
     {
             new_entry=new Cloud::CloudReader::FileEntry;
             strcpy(new_entry->FileName,content);
             Cloud::CloudReader::FileList.push_back(new_entry);
     }
     print=0;
     XMLString::release(&content);
}

void Cloud::CloudReader::addFile(struct Cloud::CloudReader::FileEntry *new_entry)
{
             Cloud::CloudReader::FileList.push_back(new_entry);
}

void Cloud::CloudReader::checkXML(DOMNode* node) {
     if (node) {
           switch (node->getNodeType()) {
           case DOMNode::ELEMENT_NODE:
                checkElement(static_cast<DOMElement*>(node));
                break;
           case DOMNode::TEXT_NODE:
                checkText(static_cast<DOMText*>(node));
                break;
           default:
                break;
           }
           DOMNode* child = node->getFirstChild();
           while (child) {
                DOMNode* next = child->getNextSibling();
                checkXML(child);
                child = next;
           }
     }
}



Cloud::CloudReader::~CloudReader()
{
}

Cloud::CloudReader::CloudReader(const char* Url, const char* AccessKey, const char* SecretKey, const char* TcpPort, const char* Bucket) 
{
        struct timeval tv;
        struct tm *tm;
        char date_formatted[256];
        char StringToSign[1024];
        unsigned char *digest;
        CURL *curl;
        CURLcode res;

        std::string s;


        this->Url=Url;
        this->AccessKey=AccessKey;
        this->SecretKey=SecretKey;
        this->TcpPort=TcpPort;
        this->Bucket=Bucket;

        // Amazon S3 and Swift require the timezone to be define to
        // GMT. As to simplify the conversion this is performed through the TZ
        // environment variable and a call to localtime as to convert output of gettimeofday
#if defined(FC_OS_WIN32)
        _putenv("TZ=GMT");
#else
        setenv("TZ","GMT",1);
#endif

        // We must get the directory content


#if defined(FC_OS_WIN32)
#else
        gettimeofday(&tv,NULL);
        tm = localtime(&tv.tv_sec);
        strftime(date_formatted,256,"%a, %d %b %Y %T %z", tm);
#endif

        sprintf(StringToSign,"GET\n\napplication/xml\n%s\n/%s/", date_formatted, this->Bucket);

        // We have to use HMAC encoding and SHA1
        digest=HMAC(EVP_sha1(),this->SecretKey,strlen(this->SecretKey),
                (const unsigned char *)&StringToSign,strlen(StringToSign),NULL,NULL);

        // Let's build the Header and call to curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if ( curl )
        {
                struct curl_slist *chunk = NULL;
                char header_data[1024];
                // Let's build our own header
		std::string strUrl(this->Url);
                eraseSubStr(strUrl,"http://");
                eraseSubStr(strUrl,"https://");

                sprintf(header_data,"Host: %s:%s", strUrl.c_str(),
                                                   this->TcpPort);
                chunk = curl_slist_append(chunk, header_data);
                sprintf(header_data,"Date: %s", date_formatted);
                chunk = curl_slist_append(chunk, header_data);
                chunk = curl_slist_append(chunk, "Content-Type: application/xml");
                std::string digest_str;
                digest_str=Base::base64_encode(digest,strlen((const char *)digest));
                sprintf(header_data,"Authorization: AWS %s:%s", this->AccessKey,
                digest_str.c_str());
                chunk = curl_slist_append(chunk, header_data);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                sprintf(header_data,"%s:%s/%s/", this->Url,this->TcpPort,
                                                    this->Bucket);
                curl_easy_setopt(curl, CURLOPT_URL, header_data);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
                // curl read a file not a memory buffer (it shall be able to do it)

                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                      fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                curl_easy_cleanup(curl);
                std::stringstream input(s);

                try { XMLPlatformUtils::Initialize(); }
                        catch (const XMLException& toCatch) {
                            char* message = XMLString::transcode(toCatch.getMessage());
                            cout << "Error during initialization! :\n"
                                 << message << "\n";
                     XMLString::release(&message);
                     return ;
                }

                XercesDOMParser* parser = new XercesDOMParser();
                parser->setValidationScheme(XercesDOMParser::Val_Always);
                parser->setDoNamespaces(true);

                xercesc::MemBufInputSource myxml_buf((const XMLByte *const)s.c_str(), s.size(),
                                     "myxml (in memory)");

                parser->parse(myxml_buf);
                auto* dom=parser->getDocument();
                checkXML(dom);
                // Dumping the filelist name
        }

}

void Cloud::CloudReader::DownloadFile(Cloud::CloudReader::FileEntry *entry)
{
        struct timeval tv;
        struct tm *tm;
        char date_formatted[256];
        char StringToSign[1024];
        unsigned char *digest;
        CURL *curl;
        CURLcode res;

        std::string s;

        // We must get the directory content


#if defined(FC_OS_WIN32)
#else
        gettimeofday(&tv,NULL);
        tm = localtime(&tv.tv_sec);
        strftime(date_formatted,256,"%a, %d %b %Y %T %z", tm);
#endif

        // CHANGEME
        sprintf(StringToSign,"GET\n\napplication/octet-stream\n%s\n/%s/%s", date_formatted, this->Bucket, entry->FileName);
        // We have to use HMAC encoding and SHA1
        digest=HMAC(EVP_sha1(),this->SecretKey,strlen(this->SecretKey),
                (const unsigned char *)&StringToSign,strlen(StringToSign),NULL,NULL);

        // Let's build the Header and call to curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if ( curl )
        {
                struct curl_slist *chunk = NULL;
                char header_data[1024];
                // Let's build our own header
		std::string strUrl(this->Url);
                eraseSubStr(strUrl,"http://");
                eraseSubStr(strUrl,"https://");
                sprintf(header_data,"Host: %s:%s", strUrl.c_str(),
                                                   this->TcpPort);
                chunk = curl_slist_append(chunk, header_data);
                sprintf(header_data,"Date: %s", date_formatted);
                chunk = curl_slist_append(chunk, header_data);
                chunk = curl_slist_append(chunk, "Content-Type: application/octet-stream");
                std::string digest_str;
                digest_str=Base::base64_encode(digest,strlen((const char *)digest));
                sprintf(header_data,"Authorization: AWS %s:%s", this->AccessKey,
                digest_str.c_str());
                chunk = curl_slist_append(chunk, header_data);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                sprintf(header_data,"%s:%s/%s/%s", this->Url,this->TcpPort,
                                                    this->Bucket,entry->FileName);
                curl_easy_setopt(curl, CURLOPT_URL, header_data);
//                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
                // curl read a file not a memory buffer (it shall be able to do it)

                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                      fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                curl_easy_cleanup(curl);

                entry->FileStream << s;
        }
}

struct Cloud::CloudReader::FileEntry * Cloud::CloudReader::GetEntry(std::string FileName)
{
        struct Cloud::CloudReader::FileEntry *current_entry=NULL;
        list<FileEntry*>::const_iterator it1;

        for(it1 = FileList.begin(); it1 != FileList.end(); ++it1) {
                if ( strcmp(FileName.c_str(), (*it1)->FileName) == 0 )
                {
                        current_entry = (*it1);
                        break;
                }
        }

        if ( current_entry != NULL )
        {
                (*it1)->touch=1;
                DownloadFile(*it1);
        }

        return(current_entry);
}

int Cloud::CloudReader::isTouched(std::string FileName)
{
       list<FileEntry*>::const_iterator it1;
       for(it1 = FileList.begin(); it1 != FileList.end(); ++it1) {
                if ( strcmp(FileName.c_str(), (*it1)->FileName) == 0 )
                {
                        if ( (*it1)->touch )
                                return(1);
                        else
                                return(0);
                }
        }
        return(0);
}

void Cloud::eraseSubStr(std::string & Str, const std::string & toErase)
{
	size_t pos = Str.find(toErase);
	if (pos != std::string::npos)
	{
		Str.erase(pos, toErase.length());
	}
}


void Cloud::CloudWriter::putNextEntry(const char* file)
{
      this->FileName = file;
      this->FileStream.str("");
      this->FileStream << std::fixed;
      this->FileStream.precision(std::numeric_limits<double>::digits10 + 1);
      this->FileStream.setf(ios::fixed,ios::floatfield);
      this->FileStream.imbue(std::locale::classic());

}

bool Cloud::CloudWriter::shouldWrite(const std::string& , const Base::Persistence *) const
{
    return true;
}

void Cloud::CloudWriter::pushCloud(const char *FileName, const char *data, long size)
{
        struct timeval tv;
        struct tm *tm;
        char date_formatted[256];
        char StringToSign[1024];
        unsigned char *digest;
        // char my_data[1024]="a que coucou";

        CURL *curl;
        CURLcode res;

        struct data_buffer curl_buffer;

#if defined(FC_OS_WIN32)
#else
        gettimeofday(&tv,NULL);
        tm = localtime(&tv.tv_sec);
        strftime(date_formatted,256,"%a, %d %b %Y %T %z", tm);
#endif

        // CHANGEME
        sprintf(StringToSign,"PUT\n\napplication/octet-stream\n%s\n/%s/%s", date_formatted, this->Bucket, FileName);

        // We have to use HMAC encoding and SHA1
        digest=HMAC(EVP_sha1(),this->SecretKey,strlen(this->SecretKey),
                (const unsigned char *)&StringToSign,strlen(StringToSign),NULL,NULL);

        // Let's build the Header and call to curl
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        if ( curl )
        {
                struct curl_slist *chunk = NULL;
                char header_data[1024];
                // Let's build our own header
		std::string strUrl(this->Url);
		eraseSubStr(strUrl,"http://");
		eraseSubStr(strUrl,"https://");
				
                sprintf(header_data,"Host: %s:%s", strUrl.c_str(),
                                                   this->TcpPort);
                chunk = curl_slist_append(chunk, header_data);
                sprintf(header_data,"Date: %s", date_formatted);
                chunk = curl_slist_append(chunk, header_data);
                chunk = curl_slist_append(chunk, "Content-Type: application/octet-stream");
                std::string digest_str;
                digest_str=Base::base64_encode(digest,strlen((const char *)digest));
                sprintf(header_data,"Authorization: AWS %s:%s", this->AccessKey,
                digest_str.c_str());
                chunk = curl_slist_append(chunk, header_data);
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
//                curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
                sprintf(header_data,"%s:%s/%s/%s", this->Url,this->TcpPort,
                                                    this->Bucket,FileName);
                curl_easy_setopt(curl, CURLOPT_URL, header_data);
                curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
                curl_easy_setopt(curl, CURLOPT_PUT, 1L);
                // curl read a file not a memory buffer (it shall be able to do it)
                curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
                curl_buffer.ptr = data;
                curl_buffer.remaining_size = (size_t) size;

                curl_easy_setopt(curl, CURLOPT_READDATA, &curl_buffer);

                curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE,
                     (curl_off_t)size);


                res = curl_easy_perform(curl);
                if(res != CURLE_OK)
                      fprintf(stderr, "curl_easy_perform() failed: %s\n",
                        curl_easy_strerror(res));
                curl_easy_cleanup(curl);
        }
}

void Cloud::CloudWriter::writeFiles(void)
{

    // use a while loop because it is possible that while
    // processing the files new ones can be added
    std::string tmp="";
    char *cstr;
    size_t index = 0;
    if ( strlen(this->FileName.c_str()) > 1  )
    {
        // We must push the current buffer
        const std::string tmp = this->FileStream.str();
        const char* cstr = tmp.data();
        pushCloud((const char *)this->FileName.c_str(),cstr, tmp.size());
    }
    while (index < FileList.size()) {
        FileEntry entry = FileList.begin()[index];

        if (shouldWrite(entry.FileName, entry.Object)) {
            this->FileStream.str("");
            this->FileStream.precision(std::numeric_limits<double>::digits10 + 1);
            this->FileStream.setf(ios::fixed,ios::floatfield);
            this->FileStream.imbue(std::locale::classic());
            entry.Object->SaveDocFile(*this);
            tmp = this->FileStream.str();
            cstr = (char *)tmp.data();
            pushCloud((const char *)entry.FileName.c_str(), (const char *)cstr, tmp.size());
            this->FileStream.str("");
        }

        index++;
    }


}


bool Cloud::Module::cloudSave(const char *BucketName)
{
	Document* doc = GetApplication().getActiveDocument();

        auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");

        // Save the name of the tip object in order to handle in Restore()
        if (doc->Tip.getValue()) {
            doc->TipName.setValue(doc->Tip.getValue()->getNameInDocument());
        }

        std::string LastModifiedDateString = Base::TimeInfo::currentDateTimeString();
        doc->LastModifiedDate.setValue(LastModifiedDateString.c_str());
        // set author if needed
        bool saveAuthor = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document")->GetBool("prefSetAuthorOnSave",false);
        if (saveAuthor) {
            std::string Author = App::GetApplication().GetParameterGroupByPath
                ("User parameter:BaseApp/Preferences/Document")->GetASCII("prefAuthor","");
            doc->LastModifiedBy.setValue(Author.c_str());
        }
       if ( strcmp(BucketName, doc->Label.getValue()) != 0 )
                doc->Label.setValue(BucketName);

        Cloud::CloudWriter mywriter((const char*)this->Url.getStrValue().c_str(),
                                  (const char*)this->AccessKey.getStrValue().c_str(),
                                  (const char*)this->SecretKey.getStrValue().c_str(),
                                  (const char*)this->TcpPort.getStrValue().c_str(),
                                  BucketName);

        mywriter.putNextEntry("Document.xml");

        if (hGrp->GetBool("SaveBinaryBrep", false))
            mywriter.setMode("BinaryBrep");
        mywriter.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl
                        << "<!--" << endl
                        << " FreeCAD Document, see https://www.freecadweb.org for more information..." << endl
                        << "-->" << endl;
        doc->Save(mywriter);

        // Special handling for Gui document.
        doc->signalSaveDocument(mywriter);

        // write additional files
        mywriter.writeFiles();

        return(true);
}

void readFiles(Cloud::CloudReader reader, Base::XMLReader *xmlreader) 
{
    // It's possible that not all objects inside the document could be created, e.g. if a module
    // is missing that would know these object types. So, there may be data files inside the Cloud
    // file that cannot be read. We simply ignore these files.
    // On the other hand, however, it could happen that a file should be read that is not part of
    // the zip file. This happens e.g. if a document is written without GUI up but is read with GUI
    // up. In this case the associated GUI document asks for its file which is not part of the
    // file, then.
    // In either case it's guaranteed that the order of the files is kept.

    std::vector<Base::XMLReader::FileEntry>::const_iterator it = xmlreader->FileList.begin();
    while ( it != xmlreader->FileList.end()) {
        if ( reader.isTouched(it->FileName.c_str()) == 0 )
        {
                Base::Reader localreader(reader.GetEntry(it->FileName.c_str())->FileStream,it->FileName, xmlreader->FileVersion);
                it->Object->RestoreDocFile(localreader);
                if ( localreader.getLocalReader() != nullptr )
                {
            readFiles(reader, localreader.getLocalReader().get());
                }
        }
        it++;
    }
}


bool Cloud::Module::cloudRestore (const char *BucketName)
{

    Document* doc = GetApplication().getActiveDocument();
    // clean up if the document is not empty
    // !TODO mind exceptions while restoring!

    doc->clearUndos();

    doc->clearDocument();

    std::stringstream oss;

    Cloud::CloudReader myreader((const char*)this->Url.getStrValue().c_str(),
                                  (const char*)this->AccessKey.getStrValue().c_str(),
                                  (const char*)this->SecretKey.getStrValue().c_str(),
                                  (const char*)this->TcpPort.getStrValue().c_str(),
                                  BucketName);

    // we shall pass there the initial Document.xml file

    Base::XMLReader reader("Document.xml", myreader.GetEntry("Document.xml")->FileStream);

    if (!reader.isValid())
        throw Base::FileException("Error reading Document.xml file","Document.xml");


    GetApplication().signalStartRestoreDocument(*doc);
    doc->setStatus(Document::Restoring, true);
    
    try {
        // Document::Restore(reader);
        doc->Restore(reader);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("Invalid Document.xml: %s\n", e.what());
    }

    // Special handling for Gui document, the view representations must already
    // exist, what is done in Restore().
    // Note: This file doesn't need to be available if the document has been created
    // without GUI. But if available then follow after all data files of the App document.

      doc->signalRestoreDocument(reader);

      readFiles(myreader,&reader);

    // reset all touched

    doc->afterRestore(true);

    GetApplication().signalFinishRestoreDocument(*doc);
    doc->setStatus(Document::Restoring, false);
    return(true);

}
