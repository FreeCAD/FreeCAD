#include <string>
#include <sstream>
#include <vector>

#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace
{

llvm::cl::OptionCategory ToolCategory("property-accessor-rewriter options");

enum class LineEndingType : uint8_t
{
    DOS,
    UNIX
};

class RewriteException: public std::runtime_error
{
private:
    SourceLocation loc;

public:
    RewriteException(const std::string& msg, SourceLocation loc)
        : std::runtime_error(msg)
        , loc(loc)
    {}

    SourceLocation getLocation() const
    {
        return loc;
    }
};


LineEndingType detectLineEndingType(const std::string& source)
{
    bool hasCRLF = false;
    bool hasLF = false;

    for (size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\r') {
            if (i + 1 < source.size() && source[i + 1] == '\n') {
                hasCRLF = true;
                ++i;
            }
            else {
                throw std::runtime_error("Mixed or unknown line endings detected.");
            }
        }
        else if (source[i] == '\n') {
            hasLF = true;
        }
    }

    if (hasCRLF && !hasLF) {
        return LineEndingType::DOS;
    }

    if (hasLF && !hasCRLF) {
        return LineEndingType::UNIX;
    }

    throw std::runtime_error("Mixed or unknown line endings detected.");
}

std::string getLineEnding(const std::string& source)
{
    switch (detectLineEndingType(source)) {
        case LineEndingType::DOS:
            return "\r\n";
        case LineEndingType::UNIX:
            return "\n";
        default:
            throw std::runtime_error("Unknown line ending type.");
    }
}

const char* GetterAnnotation = "fc_property_getter";
const char* SetterAnnotation = "fc_property_setter";
const char* GetterFunctionName = "getWithContext";
const char* SetterFunctionName = "setWithContext";
const char* AccessorLabel = "potentialAccessor";
constexpr size_t TabWidthUnix = 8;
constexpr size_t IndentAccessorBody = 8;
constexpr size_t IndentationWithinBody = 4;

/// Whether a method has the given attribute annotation.
bool hasAttr(const CXXMethodDecl* methodDecl, const char* annotation)
{
    if (!methodDecl) {
        return false;
    }

    auto attributes = methodDecl->specific_attrs<AnnotateAttr>();

    return std::ranges::any_of(attributes, [annotation](const AnnotateAttr* attr) {
        return attr->getAnnotation() == annotation;
    });
}

/// Whether a method is marked as a property getter.
bool hasPropGetterAttr(const CXXMethodDecl* methodDecl)
{
    return hasAttr(methodDecl, GetterAnnotation);
}

/// Whether a method is marked as a property getter.
bool hasPropSetterAttr(const CXXMethodDecl* methodDecl)
{
    return hasAttr(methodDecl, SetterAnnotation);
}

/**
 * @brief AST visitor to check if a method is a context accessor.
 */
class ContainsContextAccessor: public RecursiveASTVisitor<ContainsContextAccessor>
{
public:
    ContainsContextAccessor() = default;

    bool VisitCallExpr(CallExpr* callExpr)
    {
        const FunctionDecl* functionDecl = callExpr->getDirectCallee();
        if (!functionDecl) {
            return true;
        }
        std::string functionName = functionDecl->getNameAsString();
        if (functionName == GetterFunctionName || functionName == SetterFunctionName) {
            containsContextAccessor = true;
        }
        return true;
    }

    bool found() const
    {
        return containsContextAccessor;
    }

private:
    bool containsContextAccessor {false};
};

bool alreadyWrapped(Stmt* body)
{
    if (!body) {
        return false;
    }
    ContainsContextAccessor v;
    v.TraverseStmt(body);
    return v.found();
}

std::string stripOuterBraces(const std::string& body)
{
    std::string trimmed = body;
    size_t start = 0;
    size_t end = trimmed.size();

    auto isSpace = [](char c) {
        return c == ' ' || c == '\t';
    };

    auto isLineChar = [](char c) {
        return c == '\n' || c == '\r';
    };

    auto isWhiteSpace = [&](char c) {
        return isSpace(c) || isLineChar(c);
    };

    auto trimLeadingWS = [&]() {
        while (start < end && isWhiteSpace(trimmed[start])) {
            ++start;
        }
    };
    auto trimTrailingWS = [&]() {
        while (end > start && isWhiteSpace(trimmed[end - 1])) {
            --end;
        }
    };

    auto trimLine = [&]() {
        while (start < end && isSpace(trimmed[start])) {
            start++;
        }
        while (start < end && isLineChar(trimmed[start])) {
            start++;
        }
    };

    trimLeadingWS();
    trimTrailingWS();

    if (end <= start) {
        return "";
    }

    if (trimmed[start] == '{' && trimmed[end - 1] == '}') {
        ++start;
        --end;
        trimLine();
        trimTrailingWS();
    }

    return trimmed.substr(start, end - start);
}

std::string shiftIndent(const std::string& body, size_t spaces)
{
    std::istringstream input(body);
    std::ostringstream shifted;
    std::string line;
    std::string indent(spaces, ' ');

    bool firstLine = true;
    while (std::getline(input, line)) {
        if (!firstLine) {
            shifted << "\n";
        }
        shifted << indent << line;
        firstLine = false;
    }

    return shifted.str();
}

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct AccessorInfo
{
    std::string className;
    std::string methodName;
    bool isOverloaded;
    std::string retType;
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::vector<std::string> captureNames;
    std::string innerBody;
};

std::string createParamList(const std::vector<std::string>& list)
{
    std::string paramList;
    for (size_t i = 0; i < list.size(); ++i) {
        if (i) {
            paramList += ", ";
        }
        paramList += list[i];
    }
    return paramList;
}

struct UsedParamVisitor: public RecursiveASTVisitor<UsedParamVisitor>
{
    llvm::DenseSet<const ParmVarDecl*> usedParams;

    bool VisitDeclRefExpr(DeclRefExpr* declRef)
    {
        if (const auto* paramDecl = llvm::dyn_cast<ParmVarDecl>(declRef->getDecl())) {
            usedParams.insert(paramDecl);
        }
        return true;
    }
};

void setParameters(AccessorInfo& info, const CXXMethodDecl* method)
{
    UsedParamVisitor v;
    v.TraverseStmt(method->getBody());

    for (const ParmVarDecl* p : method->parameters()) {
        std::string name = p->getNameAsString();
        if (name.empty()) {
            throw RewriteException("Cannot rewrite method with unnamed parameter.", p->getLocation());
        }

        QualType type = p->getType();
        std::string prefix = type->isReferenceType() ? "&" : "";
        // Only caputre parameters if they are used in the body.  Prefix them
        // if they are references.
        info.captureNames.push_back(v.usedParams.contains(p) ? prefix + name : "");
        info.paramNames.push_back(name);
        info.paramTypes.push_back(p->getType().getAsString());
    }
}

std::size_t countIndentColumns(const std::string& input, std::size_t tabWidth = TabWidthUnix)
{
    std::size_t cols = 0;
    for (char c : input) {
        if (c == ' ') {
            cols += 1;
        }
        else if (c == '\t') {
            cols += tabWidth - (cols % tabWidth);
        }
        else {
            break;
        }
    }
    return cols;
}

std::string computeIndentationMethod(const std::string& linesOfBody)
{
    // The input should consist of multiple lines representing the body of a
    // method.  In FreeCAD, the lines are indented by the indentation of the
    // method, and 4 spaces as indentation within the method.  We subtract the
    // indentation of 4 spaces within the method to obtain the indentation of
    // the method.
    std::size_t cols = countIndentColumns(linesOfBody);
    return std::string(cols - IndentationWithinBody, ' ');
}

bool isOverloaded(const CXXMethodDecl* decl)
{
    if (!decl) {
        return false;
    }

    auto* parent = decl->getParent();
    if (!parent) {
        return false;
    }

    auto name = decl->getDeclName();
    if (name.isEmpty()) {
        return false;
    }

    clang::DeclContext::lookup_result res = parent->lookup(name);
    unsigned count = 0;
    for (clang::NamedDecl* nd : res) {
        if (llvm::isa<clang::FunctionDecl>(nd) || llvm::isa<clang::CXXMethodDecl>(nd)) {
            ++count;
            if (count > 1) {
                return true;
            }
        }
    }
    return false;
}

template<typename GenBodyFunc>
std::string generateAccessorBody(const CXXMethodDecl* method, ASTContext& ctx, GenBodyFunc genBody)
{
    const auto* cls = llvm::dyn_cast<CXXRecordDecl>(method->getParent());
    if (!cls) {
        return "{}";
    }

    AccessorInfo info;

    info.className = cls->getNameAsString();
    info.methodName = method->getNameAsString();
    info.isOverloaded = isOverloaded(method);

    QualType returnType = method->getReturnType();
    info.retType = returnType.getAsString(ctx.getPrintingPolicy());

    setParameters(info, method);

    // Get original body source and strip braces
    const Stmt* body = method->getBody();
    auto& sourceMgr = ctx.getSourceManager();
    auto& langOpts = ctx.getLangOpts();
    SourceRange bodyRange = body->getSourceRange();
    const std::string originalBody
        = Lexer::getSourceText(CharSourceRange::getTokenRange(bodyRange), sourceMgr, langOpts).str();
    const std::string nl = getLineEnding(originalBody);
    info.innerBody = stripOuterBraces(originalBody);
    const std::string indentation = computeIndentationMethod(info.innerBody);
    info.innerBody = shiftIndent(info.innerBody, IndentAccessorBody);

    return genBody(info, indentation, nl);
}

std::string generateSetterBody(const CXXMethodDecl* method, ASTContext& ctx)
{
    return generateAccessorBody(
        method,
        ctx,
        [](AccessorInfo& info, const std::string& baseIndent, const std::string& nl) {
            std::ostringstream code;
            std::string templateArgs = info.className;
            code << "{" << nl;
            if (info.isOverloaded) {
                code << baseIndent << "    using FuncType = void (" << info.className << "::*)(";
                code << createParamList(info.paramTypes) << ");" << nl;
                templateArgs += ", FuncType";
            }
            code << baseIndent << "    " << SetterFunctionName << "<" << templateArgs << ">(" << nl;
            code << baseIndent << "        this," << nl;
            code << baseIndent << "        &" << info.className << "::" << info.methodName << ","
                 << nl;
            code << baseIndent << "        [this";

            for (const auto& name : info.captureNames) {
                if (!name.empty()) {
                    code << ", " << name;
                }
            }
            code << "]() {" << nl;
            // the body maintains its own indentation and we've already shifted it earlier.
            code << info.innerBody + nl;
            code << baseIndent << "        }";
            if (!info.paramNames.empty()) {
                code << "," << nl;
                code << baseIndent << "        " + createParamList(info.paramNames) + nl;
            }
            else {
                code << nl;
            }
            code << baseIndent << "    );" << nl;
            code << baseIndent << "}" << nl;

            return code.str();
        }
    );
}

std::string generateGetterBody(const CXXMethodDecl* method, ASTContext& ctx)
{
    return generateAccessorBody(
        method,
        ctx,
        [](AccessorInfo& info, const std::string& baseIndent, const std::string& nl) {
            // Build new body
            std::ostringstream code;
            code << "{" << nl;
            code << baseIndent << "    return " << GetterFunctionName << "<" << info.className
                 << ", " << info.retType << ">(" << nl;
            code << baseIndent << "        this," << nl;
            code << baseIndent << "        &" << info.className << "::" << info.methodName << ","
                 << nl;
            code << baseIndent << "        [this";
            for (const auto& name : info.captureNames) {
                if (!name.empty()) {
                    code << ", " << name;
                }
            }
            code << "]() -> " << info.retType << " {" << nl;
            // the body maintains its own indentation and we've already shifted it earlier.
            code << info.innerBody + nl;
            code << baseIndent << "        }";
            if (!info.paramNames.empty()) {
                code << "," << nl;
                code << baseIndent << "        " + createParamList(info.paramNames) + nl;
            }
            else {
                code << nl;
            }
            code << baseIndent << "    );" << nl;
            code << baseIndent << "}" << nl;

            return code.str();
        }
    );
}

class PropertyAccessorRewriter: public MatchFinder::MatchCallback
{
public:
    explicit PropertyAccessorRewriter(Rewriter& rewriter)
        : rewriter(rewriter)
    {}

    void report(DiagnosticsEngine& diagEngine, const RewriteException& ex, const CXXMethodDecl* method)
    {
        unsigned errorID = diagEngine.getCustomDiagID(
            DiagnosticsEngine::Error,
            "Failed to rewrite property accessor: %0"
        );
        SourceLocation loc = ex.getLocation().isValid() ? ex.getLocation() : method->getLocation();
        DiagnosticBuilder errorBuilder = diagEngine.Report(loc, errorID);
        errorBuilder.AddString(ex.what());

        unsigned noteID
            = diagEngine.getCustomDiagID(DiagnosticsEngine::Note, "skipping rewrite of method '%0'");
        DiagnosticBuilder noteBuilder = diagEngine.Report(method->getLocation(), noteID);
        noteBuilder.AddString(method->getQualifiedNameAsString());
    }

    void run(const MatchFinder::MatchResult& result) override
    {
        const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>(AccessorLabel);
        if (!method || !method->hasBody()) {
            return;
        }

        if (!hasPropGetterAttr(method) && !hasPropSetterAttr(method)) {
            return;
        }

        Stmt* body = method->getBody();
        if (alreadyWrapped(body)) {
            return;
        }

        ASTContext& ctx = *result.Context;
        std::string newBody;
        try {
            if (hasPropGetterAttr(method)) {
                newBody = generateGetterBody(method, ctx);
            }
            else if (hasPropSetterAttr(method)) {
                newBody = generateSetterBody(method, ctx);
            }
            else {
                return;
            }
            SourceRange bodyRange = body->getSourceRange();
            rewriter.ReplaceText(bodyRange, newBody);

            llvm::errs() << "Rewrote accessor: " << method->getQualifiedNameAsString() << "\n";
        }
        catch (const RewriteException& ex) {
            report(ctx.getDiagnostics(), ex, method);
        }
    }

private:
    // The rewriter is a member of PropertyGetterAction.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    Rewriter& rewriter;
};

class PropertyGetterAction: public ASTFrontendAction
{
public:
    PropertyGetterAction() = default;

    void EndSourceFileAction() override
    {
        auto& sourceMgr = rewriter.getSourceMgr();
        for (auto it = rewriter.buffer_begin(); it != rewriter.buffer_end(); ++it) {
            FileID fid = it->first;
            const llvm::RewriteBuffer& rewriteBuffer = it->second;

            const FileEntry* fileEntry = sourceMgr.getFileEntryForID(fid);
            if (!fileEntry) {
                // This might be a buffer without a real file (e.g. <stdin>); skip it.
                continue;
            }

            StringRef path = sourceMgr.getFilename(sourceMgr.getLocForStartOfFile(fid));
            if (path.empty()) {
                continue;
            }

            std::error_code errorCode;
            llvm::raw_fd_ostream out(path, errorCode, llvm::sys::fs::OF_None);
            if (errorCode) {
                llvm::errs() << "Error writing to " << path << ": " << errorCode.message() << "\n";
                continue;
            }
            rewriteBuffer.write(out);
        }
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler, llvm::StringRef /*inFile*/) override
    {
        rewriter.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());

        auto consumer = std::make_unique<ASTConsumer>();
        finder = std::make_unique<MatchFinder>();
        callback = std::make_unique<PropertyAccessorRewriter>(rewriter);

        finder->addMatcher(cxxMethodDecl(isDefinition()).bind(AccessorLabel), callback.get());

        return finder->newASTConsumer();
    }

private:
    Rewriter rewriter;
    std::unique_ptr<MatchFinder> finder;
    std::unique_ptr<PropertyAccessorRewriter> callback;
};

}  // namespace

int main(int argc, const char** argv)
{
    auto expectedParser = CommonOptionsParser::create(argc, argv, ToolCategory);
    if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
    }
    CommonOptionsParser& optionsParser = expectedParser.get();
    ClangTool tool(optionsParser.getCompilations(), optionsParser.getSourcePathList());

    tool.appendArgumentsAdjuster([](const CommandLineArguments& args, StringRef /*stringRef*/) {
        CommandLineArguments adjustedArgs;
        for (const auto& arg : args) {
            if (arg != "-mno-direct-extern-access") {
                adjustedArgs.push_back(arg);
            }
        }
        // C++ standard library dirs (from g++ -v)
        adjustedArgs.emplace_back("-isystem");
        adjustedArgs.emplace_back(
            "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../include/c++/15.2.1"
        );

        adjustedArgs.emplace_back("-isystem");
        adjustedArgs
            .emplace_back("/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../include/c++/15.2.1/x86_64-pc-linux-gnu");

        adjustedArgs.emplace_back("-isystem");
        adjustedArgs.emplace_back(
            "/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../include/c++/15.2.1/backward"
        );
        adjustedArgs.emplace_back("-isystem");
        adjustedArgs.emplace_back("/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include");

        adjustedArgs.emplace_back("-isystem");
        adjustedArgs.emplace_back("/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include-fixed");

        adjustedArgs.emplace_back("-isystem");
        adjustedArgs.emplace_back("/usr/include");
        adjustedArgs.emplace_back("-Wno-unused-parameter");

        return adjustedArgs;
    });

    return tool.run(newFrontendActionFactory<PropertyGetterAction>().get());
}
