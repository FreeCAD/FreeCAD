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

#if defined(FC_OS_WIN32)
#include <Windows.h>
#include <cstdint>
#endif

#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include <curl/curl.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentPy.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/PyObjectBase.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include "AppCloud.h"


using namespace App;
using namespace std;
using namespace boost::placeholders;
XERCES_CPP_NAMESPACE_USE

/* Python entry */
PyMOD_INIT_FUNC(Cloud)
{
    PyObject* mod = Cloud::initModule();
    Base::Console().Log("Loading Cloud module... done\n");
    PyMOD_Return(mod);
}

Py::Object Cloud::Module::sCloudURL(const Py::Tuple& args)
{
    char* URL;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &URL)) {
        throw Py::Exception();
    }

    std::string strURL = URL;
    PyMem_Free(URL);
    if (this->URL.getStrValue() != strURL) {
        this->URL.setValue(strURL);
    }

    return Py::None();
}

Py::Object Cloud::Module::sCloudTokenAuth(const Py::Tuple& args)
{
    char* TokenAuth;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &TokenAuth)) {
        throw Py::Exception();
    }

    std::string strTokenAuth = TokenAuth;
    PyMem_Free(TokenAuth);
    if (this->TokenAuth.getStrValue() != strTokenAuth) {
        this->TokenAuth.setValue(strTokenAuth);
    }

    return Py::None();
}

Py::Object Cloud::Module::sCloudTokenSecret(const Py::Tuple& args)
{
    char* TokenSecret;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &TokenSecret)) {
        throw Py::Exception();
    }

    std::string strTokenSecret = TokenSecret;
    PyMem_Free(TokenSecret);
    if (this->TokenSecret.getStrValue() != strTokenSecret) {
        this->TokenSecret.setValue(strTokenSecret);
    }

    return Py::None();
}

Py::Object Cloud::Module::sCloudTCPPort(const Py::Tuple& args)
{
    char* TCPPort;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &TCPPort)) {
        throw Py::Exception();
    }

    std::string strTCPPort = TCPPort;
    PyMem_Free(TCPPort);
    if (this->TCPPort.getStrValue() != strTCPPort) {
        this->TCPPort.setValue(strTCPPort);
    }

    return Py::None();
}

Py::Object Cloud::Module::sCloudSave(const Py::Tuple& args)
{
    char* pDoc;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &pDoc)) {
        throw Py::Exception();
    }

    std::string strpDoc = pDoc;
    PyMem_Free(pDoc);
    cloudSave(strpDoc.c_str());

    return Py::None();
}


Py::Object Cloud::Module::sCloudRestore(const Py::Tuple& args)
{
    char* pDoc;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &pDoc)) {
        throw Py::Exception();
    }

    std::string strpDoc = pDoc;
    PyMem_Free(pDoc);
    cloudRestore(strpDoc.c_str());

    return Py::None();
}

Py::Object Cloud::Module::sCloudProtocolVersion(const Py::Tuple& args)
{
    char* ProtocolVersion;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &ProtocolVersion)) {
        throw Py::Exception();
    }

    std::string strProtocolVersion = ProtocolVersion;
    PyMem_Free(ProtocolVersion);
    if (this->ProtocolVersion.getStrValue() != strProtocolVersion) {
        this->ProtocolVersion.setValue(strProtocolVersion);
    }

    return Py::None();
}

Py::Object Cloud::Module::sCloudRegion(const Py::Tuple& args)
{
    char* Region;
    if (!PyArg_ParseTuple(args.ptr(), "et", "utf-8", &Region)) {
        throw Py::Exception();
    }

    std::string strRegion = Region;
    PyMem_Free(Region);
    if (this->Region.getStrValue() != strRegion) {
        this->Region.setValue(strRegion);
    }

    return Py::None();
}

struct data_buffer
{
    const char* ptr;
    size_t remaining_size;
};

static size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream)
{

    struct data_buffer* local_ptr = (struct data_buffer*)stream;
    size_t data_to_transfer = size * nmemb;

    if (local_ptr->remaining_size) {
        // copy as much as possible from the source to the destination
        size_t copy_this_much = local_ptr->remaining_size;
        if (copy_this_much > data_to_transfer) {
            copy_this_much = data_to_transfer;
        }
        memcpy(ptr, local_ptr->ptr, copy_this_much);

        local_ptr->ptr += copy_this_much;
        local_ptr->remaining_size -= copy_this_much;
        return copy_this_much;
    }

    return 0;
}

void Cloud::CloudWriter::checkXML(DOMNode* node)
{
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

void Cloud::CloudWriter::checkElement(DOMElement* element)
{
    char* name = XMLString::transcode(element->getTagName());
    if (strcmp(name, "Code") == 0) {
        print = 1;
    }
    XMLString::release(&name);
}

void Cloud::CloudWriter::checkText(DOMText* text)
{

    XMLCh* buffer = new XMLCh[XMLString::stringLen(text->getData()) + 1];
    XMLString::copyString(buffer, text->getData());
    XMLString::trim(buffer);
    char* content = XMLString::transcode(buffer);
    delete[] buffer;
    if (print) {
        strcpy(errorCode, content);
    }
    print = 0;
    XMLString::release(&content);
}


void Cloud::CloudWriter::createBucket()
{
    struct Cloud::AmzData* RequestData;
    struct Cloud::AmzDatav4* RequestDatav4;
    CURL* curl;
    CURLcode res;

    struct data_buffer curl_buffer;

    char path[1024];
    sprintf(path, "/%s/", this->Bucket);
    std::string strURL(this->URL);
    eraseSubStr(strURL, "http://");
    eraseSubStr(strURL, "https://");

    if (this->ProtocolVersion == "2") {
        RequestData = Cloud::ComputeDigestAmzS3v2("PUT",
                                                  "application/xml",
                                                  path,
                                                  this->TokenSecret,
                                                  nullptr,
                                                  0);
    }
    else {
        RequestDatav4 = Cloud::ComputeDigestAmzS3v4("PUT",
                                                    strURL.c_str(),
                                                    "application/xml",
                                                    path,
                                                    this->TokenSecret,
                                                    nullptr,
                                                    0,
                                                    nullptr,
                                                    this->Region);
    }

    // Let's build the Header and call to curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
#ifdef ALLOW_SELF_SIGNED_CERTIFICATE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif

    if (curl) {
        struct curl_slist* chunk = nullptr;
        char URL[256];
        // Let's build our own header
        std::string strURL(this->URL);
        eraseSubStr(strURL, "http://");
        eraseSubStr(strURL, "https://");
        if (this->ProtocolVersion == "2") {
            chunk = Cloud::BuildHeaderAmzS3v2(strURL.c_str(),
                                              this->TCPPort,
                                              this->TokenAuth,
                                              RequestData);
            delete RequestData;
        }
        else {
            chunk = Cloud::BuildHeaderAmzS3v4(strURL.c_str(), this->TokenAuth, RequestDatav4);
            delete RequestDatav4;
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // Lets build the URL for our Curl call

        sprintf(URL, "%s:%s/%s/", this->URL, this->TCPPort, this->Bucket);
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        // curl read a file not a memory buffer (it shall be able to do it)
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

        curl_buffer.ptr = nullptr;
        curl_buffer.remaining_size = (size_t)0;

        curl_easy_setopt(curl, CURLOPT_READDATA, &curl_buffer);

        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)0);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
}
//
// #if defined(FC_OS_WIN32)
//
// #include <chrono>
// #undef timezone
//
//
// int gettimeofday( time_t* tp, struct timezone* tzp) {
//  namespace sc = std::chrono;
//  sc::system_clock::duration d = sc::system_clock::now().time_since_epoch();
//  sc::seconds s = sc::duration_cast<sc::seconds>(d);
//  tp->tv_sec = s.count();
//  tp->tv_usec = sc::duration_cast<sc::microseconds>(d - s).count();
//
//  return 0;
//}
// #endif

struct Cloud::AmzDatav4* Cloud::ComputeDigestAmzS3v4(char* operation,
                                                     const char* server,
                                                     char* data_type,
                                                     const char* target,
                                                     const char* Secret,
                                                     const char* ptr,
                                                     long size,
                                                     char* parameters,
                                                     std::string Region)
{
    struct AmzDatav4* returnData;
    returnData = new Cloud::AmzDatav4;
    struct tm* tm;
    char* canonical_request;
    char* canonicalRequestHash;
    char* stringToSign;

    strcpy(returnData->ContentType, data_type);

#if defined(FC_OS_WIN32)
    _putenv("TZ=GMT");
    time_t rawtime;

    time(&rawtime);
    tm = localtime(&rawtime);
#else
    struct timeval tv;
    setenv("TZ", "GMT", 1);
    gettimeofday(&tv, nullptr);
    tm = localtime(&tv.tv_sec);
#endif
    strftime(returnData->dateFormattedD, 256, "%Y%m%d", tm);
    strftime(returnData->dateFormattedS, 256, "%Y%m%dT%H%M%SZ", tm);
    returnData->MD5 = nullptr;

    // We must evaluate the canonical request
    canonical_request = (char*)malloc(4096 * (sizeof(char*)));
    strcpy(canonical_request, operation);
    strcat(canonical_request, "\n");
    strcat(canonical_request, target);
    strcat(canonical_request, "\n");
    if (parameters == nullptr) {
        strcat(canonical_request, "\n");
    }
    else {
        strcat(canonical_request, parameters);
        strcat(canonical_request, "\n");
    }
    strcat(canonical_request, "host:");
    strcat(canonical_request, server);
    strcat(canonical_request, "\n");
    strcat(canonical_request, "x-amz-date:");
    strcat(canonical_request, returnData->dateFormattedS);
    strcat(canonical_request, "\n\n");
    strcat(canonical_request, "host;x-amz-date\n");
    // We must add there the file SHA256 Hash
    returnData->SHA256Sum = nullptr;
    if (strcmp(operation, "PUT") == 0) {
        if (ptr != nullptr) {
            returnData->SHA256Sum = Cloud::SHA256Sum(ptr, size);
            strcat(canonical_request, returnData->SHA256Sum);
        }
        else {
            strcat(canonical_request, "UNSIGNED-PAYLOAD");
        }
    }
    else {
        strcat(canonical_request, "UNSIGNED-PAYLOAD");
    }
    canonicalRequestHash = Cloud::SHA256Sum(canonical_request, strlen(canonical_request));
    // returnData->digest = string(digest);


    // We need now to sign a string which contain the digest
    // The format is as follow
    // ${authType}
    // ${dateValueL}
    // ${dateValueS}/${Region}/${service}/aws4_request
    // ${canonicalRequestHash}"

    stringToSign = (char*)malloc(4096 * sizeof(char));
    strcat(stringToSign, "AWS4-HMAC-SHA256");
    strcat(stringToSign, "\n");
    strcat(stringToSign, returnData->dateFormattedS);
    strcat(stringToSign, "\n");
    strcat(stringToSign, returnData->dateFormattedD);
    strcat(stringToSign, "/us/s3/aws4_request");
    strcat(stringToSign, "\n");
    strcat(stringToSign, canonicalRequestHash);
    strcat(stringToSign, "\0");

    // We must now compute the signature
    // Everything starts with the secret key and an SHA256 HMAC encryption
    char kSecret[256];
    unsigned char *kDate, *kRegion, *kService, *kSigned, *kSigning;

    strcpy(kSecret, "AWS4");
    strcat(kSecret, Secret);
    unsigned int HMACLength;

    std::string temporary;

    kDate = HMAC(EVP_sha256(),
                 kSecret,
                 strlen(kSecret),
                 (const unsigned char*)returnData->dateFormattedD,
                 strlen(returnData->dateFormattedD),
                 nullptr,
                 &HMACLength);

    temporary = getHexValue(kDate, HMACLength);
    temporary = getHexValue(kDate, HMACLength);

    // We can now compute the remaining parts
    kRegion = HMAC(EVP_sha256(),
                   kDate,
                   HMACLength,
                   (const unsigned char*)Region.c_str(),
                   strlen(Region.c_str()),
                   nullptr,
                   &HMACLength);

    temporary = getHexValue(kRegion, HMACLength);

    kService = HMAC(EVP_sha256(),
                    kRegion,
                    HMACLength,
                    (const unsigned char*)"s3",
                    strlen("s3"),
                    nullptr,
                    &HMACLength);

    temporary = getHexValue(kService, HMACLength);

    kSigning = HMAC(EVP_sha256(),
                    kService,
                    HMACLength,
                    (const unsigned char*)"aws4_request",
                    strlen("aws4_request"),
                    nullptr,
                    &HMACLength);

    temporary = getHexValue(kService, HMACLength);


    kSigned = HMAC(EVP_sha256(),
                   kSigning,
                   HMACLength,
                   (const unsigned char*)stringToSign,
                   strlen(stringToSign),
                   nullptr,
                   &HMACLength);

    temporary = getHexValue(kSigned, HMACLength);

    returnData->digest = string(temporary);
    returnData->Region = Region;
    free(canonical_request);
    return (returnData);
}

std::string Cloud::getHexValue(unsigned char* input, unsigned int HMACLength)
{
    char* Hex;
    unsigned char* ptr = input;
    std::string resultReadable;
    Hex = (char*)malloc(HMACLength * 2 * sizeof(char) + 1);
    for (unsigned int i = 0; i < HMACLength; i++) {
        sprintf(Hex, "%02x", *ptr);
        ptr++;
        Hex[2] = '\0';
        resultReadable += Hex;
    }
    return resultReadable;
}

struct Cloud::AmzData* Cloud::ComputeDigestAmzS3v2(char* operation,
                                                   char* data_type,
                                                   const char* target,
                                                   const char* Secret,
                                                   const char* ptr,
                                                   long size)
{
    struct AmzData* returnData;
    // struct timeval tv;
    struct tm* tm;
    char date_formatted[256];
    char StringToSign[1024];
    unsigned char* digest;
    unsigned int HMACLength;
    // Amazon S3 and Swift require the timezone to be define to GMT.
    // As to simplify the conversion this is performed through the TZ
    // environment variable and a call to localtime as to convert output of gettimeofday
    returnData = new Cloud::AmzData;
    strcpy(returnData->ContentType, data_type);
#if defined(FC_OS_WIN32)
    _putenv("TZ=GMT");
    time_t rawtime;

    time(&rawtime);
    tm = localtime(&rawtime);
#else
    struct timeval tv;
    setenv("TZ", "GMT", 1);
    gettimeofday(&tv, nullptr);
    tm = localtime(&tv.tv_sec);
#endif
    strftime(date_formatted, 256, "%a, %d %b %Y %T %z", tm);
    returnData->MD5 = nullptr;
    if (strcmp(operation, "PUT") == 0) {
        if (ptr != nullptr) {
            returnData->MD5 = Cloud::MD5Sum(ptr, size);
            sprintf(StringToSign,
                    "%s\n%s\n%s\n%s\n%s",
                    operation,
                    returnData->MD5,
                    data_type,
                    date_formatted,
                    target);
        }
        else {
            sprintf(StringToSign, "%s\n\n%s\n%s\n%s", operation, data_type, date_formatted, target);
        }
    }
    else {
        sprintf(StringToSign, "%s\n\n%s\n%s\n%s", operation, data_type, date_formatted, target);
    }
    // We have to use HMAC encoding and SHA1
    digest = HMAC(EVP_sha1(),
                  Secret,
                  strlen(Secret),
                  (const unsigned char*)&StringToSign,
                  strlen(StringToSign),
                  nullptr,
                  &HMACLength);
    returnData->digest = Base::base64_encode(digest, HMACLength);
    strcpy(returnData->dateFormatted, date_formatted);
    return returnData;
}

char* Cloud::SHA256Sum(const char* ptr, long size)
{
    char* output;
    std::string local;
    std::string resultReadable;
    unsigned char result[SHA256_DIGEST_LENGTH];
    output = (char*)malloc(2 * SHA256_DIGEST_LENGTH * sizeof(char) + 1);
    SHA256((unsigned char*)ptr, size, result);

    strcpy(output, getHexValue(result, SHA256_DIGEST_LENGTH).c_str());

    return (output);
}

char* Cloud::MD5Sum(const char* ptr, long size)
{
    char* output;
    std::string local;
    unsigned char result[MD5_DIGEST_LENGTH];
    output = (char*)malloc(2 * MD5_DIGEST_LENGTH * sizeof(char) + 1);
    MD5((unsigned char*)ptr, size, result);
    local = Base::base64_encode(result, MD5_DIGEST_LENGTH);
    strcpy(output, local.c_str());
    return (output);
}

struct curl_slist*
Cloud::BuildHeaderAmzS3v4(const char* URL, const char* PublicKey, struct Cloud::AmzDatav4* Data)
{
    char header_data[1024];
    struct curl_slist* chunk = nullptr;

    // Build the Host: entry
    // sprintf(header_data,"Host: %s:%s", URL, TCPPort);
    sprintf(header_data, "Host: %s", URL);
    chunk = curl_slist_append(chunk, header_data);

    // Build the Date entry

    sprintf(header_data, "X-Amz-Date: %s", Data->dateFormattedS);
    chunk = curl_slist_append(chunk, header_data);

    // Build the Content-Type entry

    sprintf(header_data, "Content-Type:%s", Data->ContentType);
    chunk = curl_slist_append(chunk, header_data);

    // If ptr is not null we must compute the MD5-Sum as to validate later the ETag
    // and add the MD5-Content: entry to the header
    if (Data->MD5 != nullptr) {
        sprintf(header_data, "Content-MD5: %s", Data->MD5);
        chunk = curl_slist_append(chunk, header_data);
        // We don't need it anymore we can free it
        free((void*)Data->MD5);
    }

    if (Data->SHA256Sum != nullptr) {
        sprintf(header_data, "x-amz-content-sha256: %s", Data->SHA256Sum);
        chunk = curl_slist_append(chunk, header_data);
        // We don't need it anymore we can free it
        free((void*)Data->SHA256Sum);
    }
    else {
        chunk = curl_slist_append(chunk, "x-amz-content-sha256: UNSIGNED-PAYLOAD");
    }

    // build the Auth entry
    sprintf(header_data,
            "Authorization: AWS4-HMAC-SHA256 Credential=%s/%s/%s/%s/aws4_request, "
            "SignedHeaders=%s, Signature=%s",
            PublicKey,
            Data->dateFormattedD,
            "us",
            "s3",
            "host;x-amz-date",
            Data->digest.c_str());
    chunk = curl_slist_append(chunk, header_data);

    return chunk;
}

struct curl_slist* Cloud::BuildHeaderAmzS3v2(const char* URL,
                                             const char* TCPPort,
                                             const char* PublicKey,
                                             struct Cloud::AmzData* Data)
{
    char header_data[1024];
    struct curl_slist* chunk = nullptr;

    // Build the Host: entry

    sprintf(header_data, "Host: %s:%s", URL, TCPPort);
    chunk = curl_slist_append(chunk, header_data);
    // Build the Date entry

    sprintf(header_data, "Date: %s", Data->dateFormatted);
    chunk = curl_slist_append(chunk, header_data);
    // Build the Content-Type entry

    sprintf(header_data, "Content-Type:%s", Data->ContentType);
    chunk = curl_slist_append(chunk, header_data);

    // If ptr is not null we must compute the MD5-Sum as to validate later the ETag
    // and add the MD5-Content: entry to the header
    if (Data->MD5 != nullptr) {
        sprintf(header_data, "Content-MD5: %s", Data->MD5);
        chunk = curl_slist_append(chunk, header_data);
        // We don't need it anymore we can free it
        free((void*)Data->MD5);
    }

    // build the Auth entry

    sprintf(header_data, "Authorization: AWS %s:%s", PublicKey, Data->digest.c_str());
    chunk = curl_slist_append(chunk, header_data);

    return chunk;
}

Cloud::CloudWriter::CloudWriter(const char* URL,
                                const char* TokenAuth,
                                const char* TokenSecret,
                                const char* TCPPort,
                                const char* Bucket,
                                std::string ProtocolVersion,
                                std::string Region)
{
    struct Cloud::AmzData* RequestData;
    struct Cloud::AmzDatav4* RequestDatav4;
    CURL* curl;
    CURLcode res;

    std::string s;

    this->URL = URL;
    this->TokenAuth = TokenAuth;
    this->TokenSecret = TokenSecret;
    this->TCPPort = TCPPort;
    this->Bucket = Bucket;
    if (!ProtocolVersion.empty()) {
        this->ProtocolVersion = ProtocolVersion;
    }
    else {
        this->ProtocolVersion = "2";
    }
    this->Region = Region;
    this->FileName = "";
    char path[1024];
    sprintf(path, "/%s/", this->Bucket);
    std::string strURL(this->URL);
    eraseSubStr(strURL, "http://");
    eraseSubStr(strURL, "https://");
    if (this->ProtocolVersion == "2") {
        RequestData = Cloud::ComputeDigestAmzS3v2("GET",
                                                  "application/xml",
                                                  path,
                                                  this->TokenSecret,
                                                  nullptr,
                                                  0);
    }
    else {
        RequestDatav4 = Cloud::ComputeDigestAmzS3v4("GET",
                                                    strURL.c_str(),
                                                    "application/xml",
                                                    path,
                                                    this->TokenSecret,
                                                    nullptr,
                                                    0,
                                                    nullptr,
                                                    this->Region);
    }
    // Let's build the Header and call to curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
#ifdef ALLOW_SELF_SIGNED_CERTIFICATE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif
    if (curl) {
        // Let's build our own header
        struct curl_slist* chunk = nullptr;
        char URL[256];
        std::string strURL(this->URL);
        eraseSubStr(strURL, "http://");
        eraseSubStr(strURL, "https://");
        if (this->ProtocolVersion == "2") {
            chunk = Cloud::BuildHeaderAmzS3v2(strURL.c_str(),
                                              this->TCPPort,
                                              this->TokenAuth,
                                              RequestData);
            delete RequestData;
        }
        else {
            chunk = Cloud::BuildHeaderAmzS3v4(strURL.c_str(), this->TokenAuth, RequestDatav4);
            delete RequestDatav4;
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // Lets build the URL for our Curl call

        sprintf(URL, "%s:%s/%s/", this->URL, this->TCPPort, this->Bucket);
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        // curl read a file not a memory buffer (it shall be able to do it)
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
        createBucket();
        // Lets dump temporarily for debug purposes of s3v4 implementation
        std::stringstream input(s);

        try {
            XMLPlatformUtils::Initialize();
        }
        catch (const XMLException& toCatch) {
            char* message = XMLString::transcode(toCatch.getMessage());
            cout << "Error during initialization! :\n" << message << "\n";
            XMLString::release(&message);
            return;
        }

        XercesDOMParser* parser = new XercesDOMParser();
        parser->setValidationScheme(XercesDOMParser::Val_Always);
        parser->setDoNamespaces(true);

        xercesc::MemBufInputSource myxml_buf((const XMLByte* const)s.c_str(),
                                             s.size(),
                                             "myxml (in memory)");

        parser->parse(myxml_buf);
        auto* dom = parser->getDocument();
        // Is there an Error entry into the document ?
        // if yes, then we must create the Bucket
        checkXML(dom);
        if (strcmp(errorCode, "NoSuchBucket") == 0) {
            // we must create the Bucket using a PUT request
            createBucket();
        }
    }
}

Cloud::CloudWriter::~CloudWriter()
{}

size_t
Cloud::CurlWrite_CallbackFunc_StdString(void* contents, size_t size, size_t nmemb, std::string* s)
{
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    }
    catch (std::bad_alloc&) {
        // handle memory problem
        return 0;
    }
    return newLength;
}

void Cloud::CloudReader::checkElement(DOMElement* element)
{
    char* name = XMLString::transcode(element->getTagName());
    if (strcmp(name, "Key") == 0) {
        file = 1;
    }
    if (strcmp(name, "NextContinuationToken") == 0) {
        continuation = 1;
    }
    if (strcmp(name, "IsTruncated") == 0) {
        truncated = 1;
    }
    XMLString::release(&name);
}

void Cloud::CloudReader::checkText(DOMText* text)
{

    XMLCh* buffer = new XMLCh[XMLString::stringLen(text->getData()) + 1];
    XMLString::copyString(buffer, text->getData());
    XMLString::trim(buffer);
    struct Cloud::CloudReader::FileEntry* new_entry;
    char* content = XMLString::transcode(buffer);
    delete[] buffer;
    if (file) {
        new_entry = new Cloud::CloudReader::FileEntry;
        strcpy(new_entry->FileName, content);
        Cloud::CloudReader::FileList.push_back(new_entry);
    }
    file = 0;
    if (continuation == 1) {
        strcpy(Cloud::CloudReader::NextFileName, content);
        continuation = 0;
    }
    if (truncated == 1) {
        if (strncmp(content, "true", 4) != 0) {
            truncated = 0;
        }
        else {
            truncated = 2;
        }
    }
    XMLString::release(&content);
}

void Cloud::CloudReader::addFile(struct Cloud::CloudReader::FileEntry* new_entry)
{
    Cloud::CloudReader::FileList.push_back(new_entry);
}

void Cloud::CloudReader::checkXML(DOMNode* node)
{
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
{}

Cloud::CloudReader::CloudReader(const char* URL,
                                const char* TokenAuth,
                                const char* TokenSecret,
                                const char* TCPPort,
                                const char* Bucket,
                                std::string ProtocolVersion,
                                std::string Region)
{
    struct Cloud::AmzData* RequestData;
    struct Cloud::AmzDatav4* RequestDatav4;
    CURL* curl;
    CURLcode res;
    bool GetBucketContentList = true;
    struct curl_slist* chunk = nullptr;
    char parameters[1024];


    this->URL = URL;
    this->TokenAuth = TokenAuth;
    this->TokenSecret = TokenSecret;
    this->TCPPort = TCPPort;
    this->Bucket = Bucket;
    if (!ProtocolVersion.empty()) {
        this->ProtocolVersion = ProtocolVersion;
    }
    else {
        this->ProtocolVersion = "2";
    }
    this->Region = Region;

    char path[1024];
    sprintf(path, "/%s/", this->Bucket);
    Cloud::CloudReader::NextFileName = (char*)malloc(sizeof(char) * 1024);
    for (int i = 0; i < 1024; i++) {
        NextFileName[i] = '\0';
    }


    // Let's build the Header and call to curl
    curl_global_init(CURL_GLOBAL_ALL);
    while (GetBucketContentList) {
        std::string s;
        curl = curl_easy_init();
#ifdef ALLOW_SELF_SIGNED_CERTIFICATE
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif
        if (curl) {
            // Let's build our own header
            char URL[256];
            std::string strURL(this->URL);
            eraseSubStr(strURL, "http://");
            eraseSubStr(strURL, "https://");
            if (strlen(NextFileName) == 0) {
                sprintf(parameters, "list-type=2");
            }
            else {
                sprintf(parameters, "list-type=2&continuation-token=%s", NextFileName);
            }
            sprintf(URL, "%s:%s/%s/?%s", this->URL, this->TCPPort, this->Bucket, parameters);
            curl_easy_setopt(curl, CURLOPT_URL, URL);

            if (this->ProtocolVersion == "2") {
                RequestData = Cloud::ComputeDigestAmzS3v2("GET",
                                                          "application/xml",
                                                          path,
                                                          this->TokenSecret,
                                                          nullptr,
                                                          0);
                chunk = Cloud::BuildHeaderAmzS3v2(strURL.c_str(),
                                                  this->TCPPort,
                                                  this->TokenAuth,
                                                  RequestData);
                delete RequestData;
            }
            else {
                RequestDatav4 = Cloud::ComputeDigestAmzS3v4("GET",
                                                            strURL.c_str(),
                                                            "application/xml",
                                                            path,
                                                            this->TokenSecret,
                                                            nullptr,
                                                            0,
                                                            (char*)&parameters[0],
                                                            this->Region);
                chunk = Cloud::BuildHeaderAmzS3v4(strURL.c_str(), this->TokenAuth, RequestDatav4);
                delete RequestDatav4;
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
            // curl read a file not a memory buffer (it shall be able to do it)

            res = curl_easy_perform(curl);

            for (int i = 0; i < 1024; i++) {
                NextFileName[i] = '\0';
            }
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            }
            curl_easy_cleanup(curl);


            std::stringstream input(s);

            try {
                XMLPlatformUtils::Initialize();
            }
            catch (const XMLException& toCatch) {
                char* message = XMLString::transcode(toCatch.getMessage());
                cout << "Error during initialization! :\n" << message << "\n";
                XMLString::release(&message);
                return;
            }

            XercesDOMParser* parser = new XercesDOMParser();
            parser->setValidationScheme(XercesDOMParser::Val_Always);
            parser->setDoNamespaces(true);

            xercesc::MemBufInputSource myxml_buf((const XMLByte* const)s.c_str(),
                                                 s.size(),
                                                 "myxml (in memory)");

            parser->parse(myxml_buf);
            auto* dom = parser->getDocument();
            checkXML(dom);
        }
        if (truncated == 0) {
            GetBucketContentList = false;
        }
        else {
            truncated = 0;
            continuation = 0;
            file = 0;
        }
    }
}

void Cloud::CloudReader::DownloadFile(Cloud::CloudReader::FileEntry* entry)
{
    struct Cloud::AmzData* RequestData;
    struct Cloud::AmzDatav4* RequestDatav4;
    CURL* curl;
    CURLcode res;

    std::string s;

    // We must get the directory content
    char path[1024];
    sprintf(path, "/%s/%s", this->Bucket, entry->FileName);
    std::string strURL(this->URL);
    eraseSubStr(strURL, "http://");
    eraseSubStr(strURL, "https://");
    if (this->ProtocolVersion == "2") {
        RequestData = Cloud::ComputeDigestAmzS3v2("GET",
                                                  "application/octet-stream",
                                                  path,
                                                  this->TokenSecret,
                                                  nullptr,
                                                  0);
    }
    else {
        RequestDatav4 = Cloud::ComputeDigestAmzS3v4("GET",
                                                    strURL.c_str(),
                                                    "application/octet-stream",
                                                    path,
                                                    this->TokenSecret,
                                                    nullptr,
                                                    0,
                                                    nullptr,
                                                    this->Region);
    }

    // Let's build the Header and call to curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
#ifdef ALLOW_SELF_SIGNED_CERTIFICATE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif
    if (curl) {
        struct curl_slist* chunk = nullptr;
        char URL[256];
        // Let's build our own header
        std::string strURL(this->URL);
        eraseSubStr(strURL, "http://");
        eraseSubStr(strURL, "https://");
        if (this->ProtocolVersion == "2") {
            chunk = Cloud::BuildHeaderAmzS3v2(strURL.c_str(),
                                              this->TCPPort,
                                              this->TokenAuth,
                                              RequestData);
            delete RequestData;
        }
        else {
            chunk = Cloud::BuildHeaderAmzS3v4(strURL.c_str(), this->TokenAuth, RequestDatav4);
            delete RequestDatav4;
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        sprintf(URL, "%s:%s/%s/%s", this->URL, this->TCPPort, this->Bucket, entry->FileName);
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        // curl read a file not a memory buffer (it shall be able to do it)

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);

        entry->FileStream << s;
    }
}

struct Cloud::CloudReader::FileEntry* Cloud::CloudReader::GetEntry(std::string FileName)
{
    struct Cloud::CloudReader::FileEntry* current_entry = nullptr;
    list<FileEntry*>::const_iterator it1;

    for (it1 = FileList.begin(); it1 != FileList.end(); ++it1) {
        if (strcmp(FileName.c_str(), (*it1)->FileName) == 0) {
            current_entry = (*it1);
            break;
        }
    }

    if (current_entry != nullptr) {
        (*it1)->touch = 1;
        DownloadFile(*it1);
    }

    return (current_entry);
}

int Cloud::CloudReader::isTouched(std::string FileName)
{
    list<FileEntry*>::const_iterator it1;
    for (it1 = FileList.begin(); it1 != FileList.end(); ++it1) {
        if (strcmp(FileName.c_str(), (*it1)->FileName) == 0) {
            if ((*it1)->touch) {
                return (1);
            }
            else {
                return (0);
            }
        }
    }
    return (0);
}

void Cloud::eraseSubStr(std::string& Str, const std::string& toErase)
{
    size_t pos = Str.find(toErase);
    if (pos != std::string::npos) {
        Str.erase(pos, toErase.length());
    }
}


void Cloud::CloudWriter::putNextEntry(const char* file)
{
    this->FileName = file;
    this->FileStream.str("");
    this->FileStream << std::fixed;
    this->FileStream.precision(std::numeric_limits<double>::digits10 + 1);
    this->FileStream.setf(ios::fixed, ios::floatfield);
    this->FileStream.imbue(std::locale::classic());
}

bool Cloud::CloudWriter::shouldWrite(const std::string&, const Base::Persistence*) const
{
    return true;
}

void Cloud::CloudWriter::pushCloud(const char* FileName, const char* data, long size)
{
    struct Cloud::AmzData* RequestData;
    struct Cloud::AmzDatav4* RequestDatav4;
    CURL* curl;
    CURLcode res;
    struct data_buffer curl_buffer;

    char path[1024];
    sprintf(path, "/%s/%s", this->Bucket, FileName);

    std::string strURL(this->URL);
    eraseSubStr(strURL, "http://");
    eraseSubStr(strURL, "https://");
    if (this->ProtocolVersion == "2") {
        RequestData = Cloud::ComputeDigestAmzS3v2("PUT",
                                                  "application/octet-stream",
                                                  path,
                                                  this->TokenSecret,
                                                  data,
                                                  size);
    }
    else {
        RequestDatav4 = Cloud::ComputeDigestAmzS3v4("PUT",
                                                    strURL.c_str(),
                                                    "application/octet-stream",
                                                    path,
                                                    this->TokenSecret,
                                                    data,
                                                    size,
                                                    nullptr,
                                                    this->Region);
    }

    // Let's build the Header and call to curl
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
#ifdef ALLOW_SELF_SIGNED_CERTIFICATE
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif
    if (curl) {
        struct curl_slist* chunk = nullptr;
        char URL[256];
        // Let's build our own header
        std::string strURL(this->URL);
        eraseSubStr(strURL, "http://");
        eraseSubStr(strURL, "https://");
        if (this->ProtocolVersion == "2") {
            chunk = Cloud::BuildHeaderAmzS3v2(strURL.c_str(),
                                              this->TCPPort,
                                              this->TokenAuth,
                                              RequestData);
            delete RequestData;
        }
        else {
            chunk = Cloud::BuildHeaderAmzS3v4(strURL.c_str(), this->TokenAuth, RequestDatav4);
            delete RequestDatav4;
        }

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // Lets build the URL for our Curl call

        sprintf(URL, "%s:%s/%s/%s", this->URL, this->TCPPort, this->Bucket, FileName);
        curl_easy_setopt(curl, CURLOPT_URL, URL);

        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_PUT, 1L);
        // curl read a file not a memory buffer (it shall be able to do it)
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_buffer.ptr = data;
        curl_buffer.remaining_size = (size_t)size;

        curl_easy_setopt(curl, CURLOPT_READDATA, &curl_buffer);

        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)size);


        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
}

void Cloud::CloudWriter::writeFiles(void)
{

    // use a while loop because it is possible that while
    // processing the files, new ones can be added
    std::string tmp = "";
    char* cstr;
    size_t index = 0;
    if (strlen(this->FileName.c_str()) > 1) {
        // We must push the current buffer
        const std::string tmp = this->FileStream.str();
        const char* cstr = tmp.data();
        pushCloud((const char*)this->FileName.c_str(), cstr, tmp.size());
    }
    while (index < FileList.size()) {
        FileEntry entry = FileList.begin()[index];

        if (shouldWrite(entry.FileName, entry.Object)) {
            this->FileStream.str("");
            this->FileStream.precision(std::numeric_limits<double>::digits10 + 1);
            this->FileStream.setf(ios::fixed, ios::floatfield);
            this->FileStream.imbue(std::locale::classic());
            entry.Object->SaveDocFile(*this);
            tmp = this->FileStream.str();
            cstr = (char*)tmp.data();
            pushCloud((const char*)entry.FileName.c_str(), (const char*)cstr, tmp.size());
            this->FileStream.str("");
        }

        index++;
    }
}


bool Cloud::Module::cloudSave(const char* BucketName)
{
    Document* doc = GetApplication().getActiveDocument();

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document");

    // Save the name of the tip object in order to handle in Restore()
    if (doc->Tip.getValue()) {
        doc->TipName.setValue(doc->Tip.getValue()->getNameInDocument());
    }

    std::string LastModifiedDateString = Base::Tools::currentDateTimeString();
    doc->LastModifiedDate.setValue(LastModifiedDateString.c_str());
    // set author if needed
    bool saveAuthor = App::GetApplication()
                          .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                          ->GetBool("prefSetAuthorOnSave", false);
    if (saveAuthor) {
        std::string Author =
            App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                ->GetASCII("prefAuthor", "");
        doc->LastModifiedBy.setValue(Author.c_str());
    }
    if (strcmp(BucketName, doc->Label.getValue()) != 0) {
        doc->Label.setValue(BucketName);
    }

    Cloud::CloudWriter mywriter((const char*)this->URL.getStrValue().c_str(),
                                (const char*)this->TokenAuth.getStrValue().c_str(),
                                (const char*)this->TokenSecret.getStrValue().c_str(),
                                (const char*)this->TCPPort.getStrValue().c_str(),
                                BucketName,
                                (const char*)this->ProtocolVersion.getStrValue().c_str(),
                                this->Region.getStrValue());

    mywriter.putNextEntry("Document.xml");

    if (hGrp->GetBool("SaveBinaryBrep", false)) {
        mywriter.setMode("BinaryBrep");
    }
    mywriter.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << endl
                      << "<!--" << endl
                      << " FreeCAD Document, see https://www.freecad.org for more information..."
                      << endl
                      << "-->" << endl;
    doc->Save(mywriter);

    // Special handling for Gui document.
    doc->signalSaveDocument(mywriter);

    // write additional files
    mywriter.writeFiles();

    return (true);
}

void readFiles(Cloud::CloudReader reader, Base::XMLReader* xmlreader)
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
    while (it != xmlreader->FileList.end()) {
        if (reader.isTouched(it->FileName.c_str()) == 0) {
            Base::Reader localreader(reader.GetEntry(it->FileName.c_str())->FileStream,
                                     it->FileName,
                                     xmlreader->FileVersion);
            // for debugging only purpose
            if (false) {
                std::stringstream ss;
                ss << localreader.getStream().rdbuf();
                auto aString = ss.str();
                aString = "";
            }
            it->Object->RestoreDocFile(localreader);
            if (localreader.getLocalReader() != nullptr) {
                readFiles(reader, localreader.getLocalReader().get());
            }
        }
        it++;
    }
}

void Cloud::Module::LinkXSetValue(std::string filename)
{
    // Need to check if the document exist
    // if not we have to create a new one
    // and Restore the associated Document
    //
    std::vector<Document*> Documents;
    Documents = App::GetApplication().getDocuments();
    for (std::vector<Document*>::iterator it = Documents.begin(); it != Documents.end(); it++) {
        if ((*it)->FileName.getValue() == filename) {
            // The document exist
            // we can exit
            return;
        }
    }

    size_t header = filename.find_first_of(":");
    string protocol = filename.substr(0, header);
    string url_new = filename.substr(header + 3);  // url_new is the url excluding the http part
    size_t part2 = url_new.find_first_of("/");
    string path = url_new.substr(part2 + 1);
    // Starting from here the Document doesn't exist we must create it
    // and make it the active Document before Restoring
    App::Document* newDoc;
    string newName;
    Document* currentDoc = GetApplication().getActiveDocument();
    newName = GetApplication().getUniqueDocumentName("unnamed");
    newDoc = GetApplication().newDocument(newName.c_str(), (const char*)path.c_str(), true);
    GetApplication().setActiveDocument(newDoc);
    this->cloudRestore((const char*)path.c_str());
    GetApplication().setActiveDocument(currentDoc);
}

bool Cloud::Module::cloudRestore(const char* BucketName)
{

    Document* doc = GetApplication().getActiveDocument();
    // clean up if the document is not empty
    // !TODO: mind exceptions while restoring!
    doc->signalLinkXsetValue.connect(boost::bind(&Cloud::Module::LinkXSetValue, this, _1));

    doc->clearUndos();

    doc->clearDocument();

    std::stringstream oss;

    Cloud::CloudReader myreader((const char*)this->URL.getStrValue().c_str(),
                                (const char*)this->TokenAuth.getStrValue().c_str(),
                                (const char*)this->TokenSecret.getStrValue().c_str(),
                                (const char*)this->TCPPort.getStrValue().c_str(),
                                BucketName,
                                (const char*)this->ProtocolVersion.getStrValue().c_str(),
                                this->Region.getStrValue());

    // we shall pass there the initial Document.xml file

    Base::XMLReader reader("Document.xml", myreader.GetEntry("Document.xml")->FileStream);

    if (!reader.isValid()) {
        throw Base::FileException("Error reading Document.xml file", "Document.xml");
    }


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

    readFiles(myreader, &reader);

    // reset all touched

    doc->afterRestore(true);

    GetApplication().signalFinishRestoreDocument(*doc);
    doc->setStatus(Document::Restoring, false);
    doc->Label.setValue(BucketName);
    // The FileName shall be an URI format
    string uri;
    uri = this->URL.getStrValue() + ":" + this->TCPPort.getStrValue() + "/" + string(BucketName);
    doc->FileName.setValue(uri);
    doc->signalLinkXsetValue.disconnect(boost::bind(&Cloud::Module::LinkXSetValue, this, _1));
    return (true);
}
