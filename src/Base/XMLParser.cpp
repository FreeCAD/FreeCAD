#include "XMLParser.h"

#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <format>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/util/XMLException.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XercesVersion.hpp>


#include "Exception.h"
#include "XMLTools.h"

using namespace XERCES_CPP_NAMESPACE;

namespace
{

/**
 * Custom error handler, stores errors and fatal errors,
 * That can then be retrieved using `getErrors`.
 */
class DOMTreeErrorReporter: public ErrorHandler
{
public:
    void warning(const SAXParseException& toCatch) override
    {
        (void)toCatch;  // Ignore all warnings.
    }
    void fatalError(const SAXParseException& toCatch) override
    {
        this->handleError(toCatch, "Fatal error");
    }
    void error(const SAXParseException& toCatch) override
    {
        this->handleError(toCatch, "Error");
    }
    void resetErrors() override
    {
        errors.clear();
    }

    std::vector<std::string> getErrors()
    {
        return errors;
    }

private:
    std::vector<std::string> errors;
    void handleError(const SAXParseException& toCatch, const std::string& errorType)
    {
        std::stringstream str;
        str << errorType << " at file \"" << StrX(toCatch.getSystemId()) << "\", line "
            << toCatch.getLineNumber() << ", column " << toCatch.getColumnNumber()
            << "\n   Message: " << StrX(toCatch.getMessage()) << "\n";
        errors.emplace_back(str.str());
    }
};


class DOMPrintErrorHandler: public DOMErrorHandler
{
public:
    DOMPrintErrorHandler() = default;
    ~DOMPrintErrorHandler() override = default;

    bool handleError(const DOMError& domError) override
    {
        // Display whatever error message passed from the serializer
        char* msg = XMLString::transcode(domError.getMessage());
        errors.emplace_back(msg);
        XMLString::release(&msg);

        // Instructs the serializer to continue serialization if possible.
        return true;
    }
    void resetErrors()
    {
        errors.clear();
    }

    std::vector<std::string> getErrors()
    {
        return errors;
    }

    /* Unimplemented constructors and operators */
    DOMPrintErrorHandler(const DOMPrintErrorHandler&) = delete;
    DOMPrintErrorHandler(DOMPrintErrorHandler&&) = delete;
    void operator=(const DOMPrintErrorHandler&) = delete;
    void operator=(DOMPrintErrorHandler&&) = delete;

private:
    std::vector<std::string> errors;
};


void InitXercesC()
{
    XMLPlatformUtils::Initialize();
}

std::unique_ptr<Base::XMLElement> toXMLElement(DOMNode* document)
{
    auto element = std::make_unique<Base::XMLElement>();
    if (!document) {
        return element;
    }
    element->tag = StrX(document->getNodeName()).c_str();
    if (document->hasAttributes()) {
        DOMNamedNodeMap* attrs = document->getAttributes();
        for (XMLSize_t i = 0; i < attrs->getLength(); i++) {
            DOMNode* attr = attrs->item(i);
            element->attrs[StrX(attr->getNodeName()).c_str()] = StrX(attr->getNodeValue()).c_str();
        }
    }

    const DOMNodeList* children = document->getChildNodes();
    for (XMLSize_t i = 0; i < children->getLength(); i++) {
        DOMNode* child = children->item(i);
        switch (child->getNodeType()) {
            case DOMNode::ELEMENT_NODE:
                element->children.emplace_back(::toXMLElement(child));
                break;
            case DOMNode::TEXT_NODE:
                element->content = StrXUTF8(child->getNodeValue()).c_str();
                break;
            default:
                break;
        }
    }
    return element;
}

void toDOMDocument(const Base::XMLElement& source, DOMElement* root)
{
    for (const auto& attr : source.attrs) {
        root->setAttribute(
            XStr(attr.first.c_str()).unicodeForm(),
            XStr(attr.second.c_str()).unicodeForm()
        );
    }
    if (!source.content.empty()) {
        root->setTextContent(XStr(source.content.c_str()).unicodeForm());
    }
    for (const auto& child : source.children) {
        auto* childNode = root->getOwnerDocument()->createElement(
            XStr(child->tag.c_str()).unicodeForm()
        );
        root->appendChild(childNode);
        ::toDOMDocument(*child, childNode);
    }
}

void saveDocumentToTarget(DOMDocument* doc, XMLFormatTarget* myFormTarget)
{
    if (!doc) {
        return;
    }

    DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(
        XUTF8StrLiteral("LS").unicodeForm()
    );
    DOMLSSerializer* theSerializer = static_cast<DOMImplementationLS*>(impl)->createLSSerializer();
    DOMLSOutput* theOutput = static_cast<DOMImplementationLS*>(impl)->createLSOutput();

    // Plug in our own error handler
    DOMPrintErrorHandler myErrorHandler;
    DOMConfiguration* config = theSerializer->getDomConfig();

    config->setParameter(XMLUni::fgDOMErrorHandler, &myErrorHandler);

    // set feature if the serializer supports the feature/mode
    if (config->canSetParameter(XMLUni::fgDOMWRTSplitCdataSections, true)) {
        config->setParameter(XMLUni::fgDOMWRTSplitCdataSections, true);
    }

    if (config->canSetParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true)) {
        config->setParameter(XMLUni::fgDOMWRTDiscardDefaultContent, true);
    }

    if (config->canSetParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true)) {
        config->setParameter(XMLUni::fgDOMWRTFormatPrettyPrint, true);
    }

    theOutput->setByteStream(myFormTarget);
    theSerializer->write(doc, theOutput);

    theOutput->release();
    theSerializer->release();
}
}  // namespace


namespace Base
{
std::unique_ptr<XMLElement> XMLElement::clone() const
{
    auto result = std::make_unique<XMLElement>();

    result->tag = tag;
    result->attrs = attrs;
    result->content = content;

    result->children.reserve(children.size());

    for (const auto& child : children) {
        result->children.push_back(child->clone());
    }

    return result;
}

std::unique_ptr<XMLElement> ParseXMLFile(const fs::path& path)
{
    try {
        InitXercesC();

#if defined(FC_OS_WIN32)
        std::wstring name = path.wstring();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        LocalFileInputSource inputSource(reinterpret_cast<const XMLCh*>(name.c_str()));
#else
        LocalFileInputSource inputSource(XStr(path.string().c_str()).unicodeForm());
#endif

        XercesDOMParser parser;
        DOMTreeErrorReporter errReporter {};
        parser.setErrorHandler(&errReporter);
        parser.parse(inputSource);

        DOMDocument* document = parser.adoptDocument();

        if (!document || !document->getDocumentElement()) {
            throw XMLBaseException("Malformed document: Invalid document");
        }

        if (!errReporter.getErrors().empty()) {
            std::stringstream errorMessage;
            errorMessage << "An error occured during the parsing of the XML file " << path << ": ";
            for (auto& err : errReporter.getErrors()) {
                errorMessage << err << "\n";
            }
            throw XMLBaseException {errorMessage.str()};
        }

        DOMElement* rootElement = document->getDocumentElement();
        auto xmlContent = ::toXMLElement(rootElement);
        document->release();

        return xmlContent;
    }
    catch (const XMLException& e) {
        throw XMLBaseException {
            std::format("An error occurred during parsing: {}", StrX(e.getMessage()).c_str())
        };
    }
    catch (const DOMException& e) {
        throw XMLBaseException {
            std::format("A DOM error occurred during parsing. DOMException code: {}", e.code)
        };
    }

    return ::toXMLElement(nullptr);  // technically unreachable
}

void SaveXMLFile(const fs::path& path, const XMLElement& xmlTree)
{
    try {
        InitXercesC();

        DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(
            XUTF8StrLiteral("Core LS").unicodeForm()
        );
        DOMDocument* doc
            = impl->createDocument(nullptr, XStr(xmlTree.tag.c_str()).unicodeForm(), nullptr);
        DOMElement* root = doc->getDocumentElement();

        ::toDOMDocument(xmlTree, root);

#if defined(FC_OS_WIN32)
        std::wstring name = path.wstring();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        LocalFileFormatTarget myFormTarget {reinterpret_cast<const XMLCh*>(name.c_str())};
#else
        LocalFileFormatTarget myFormTarget {LocalFileFormatTarget(path.string().c_str())};
#endif

        ::saveDocumentToTarget(doc, &myFormTarget);
        doc->release();
    }
    catch (XMLException& e) {
        throw Base::XMLBaseException {std::format(
            "An error occurred during creation of output transcoder. Msg is:\n{}\n",
            StrX(e.getMessage()).c_str()
        )};
    }
}

std::optional<std::vector<std::string>> CheckXMLDocument(
    const XMLElement& xmlTree,
    const std::string& xsdString
)
{
    try {
        InitXercesC();

        DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(
            XUTF8StrLiteral("Core LS").unicodeForm()
        );
        DOMDocument* doc
            = impl->createDocument(nullptr, XStr(xmlTree.tag.c_str()).unicodeForm(), nullptr);
        DOMElement* root = doc->getDocumentElement();

        ::toDOMDocument(xmlTree, root);

        //
        // Plug in a format target to receive the resultant
        // XML stream from the serializer.
        //
        // LocalFileFormatTarget prints the resultant XML stream
        // to a file once it receives any thing from the serializer.
        //
        MemBufFormatTarget myFormTarget;
        ::saveDocumentToTarget(doc, &myFormTarget);
        doc->release();

        // Either use the file saved on disk or write the current XML into a buffer in memory
        MemBufInputSource xmlFile(myFormTarget.getRawBuffer(), myFormTarget.getLen(), "(memory)");

        // Either load the XSD file from disk or use the built-in string
        std::string xsdStr(xsdString);  // NOLINT
        MemBufInputSource xsdFile(
            reinterpret_cast<const XMLByte*>(xsdStr.c_str()),
            xsdStr.size(),
            "Schema.xsd"
        );

        XercesDOMParser parser;
        Grammar* grammar = parser.loadGrammar(xsdFile, Grammar::SchemaGrammarType, true);
        if (!grammar) {
            return std::vector<std::string> {"Grammar file cannot be loaded"};
        }

        parser.setExternalNoNamespaceSchemaLocation("Schema.xsd");
        parser.cacheGrammarFromParse(true);
        parser.setValidationScheme(XercesDOMParser::Val_Auto);
        parser.setDoNamespaces(true);
        parser.setDoSchema(true);
        parser.setDisableDefaultEntityResolution(true);

        DOMTreeErrorReporter errHandler;
        parser.setErrorHandler(&errHandler);
        parser.parse(xmlFile);

        if (parser.getErrorCount() > 0) {
            std::stringstream str;
            str << "Unexpected XML structure detected: " << parser.getErrorCount();
            for (auto& err : errHandler.getErrors()) {
                str << err << "\n";
            }

            return std::vector<std::string> {str.str()};
        }
    }
    catch (XMLException& e) {
        return std::vector<std::string> {
            std::format("An error occurred while checking document: {}", StrX(e.getMessage()).c_str())
        };
    }
    return std::nullopt;
}

}  // namespace Base
