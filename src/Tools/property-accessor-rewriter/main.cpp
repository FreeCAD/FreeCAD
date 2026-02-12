// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <algorithm>
#include <string>
#include <vector>
#include <unordered_set>

#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/raw_ostream.h>

#include "line_endings.h"
#include "indentation.h"
#include "visitors.h"


using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace Visitors;
using namespace Indentation;

namespace
{


llvm::cl::OptionCategory ToolCategory("property-accessor-rewriter options");

const char* AccessorLabel = "potentialAccessor";

const std::unordered_set<std::string> PropertyMethodsToIgnore
    = {"init", "create", "getClassTypeId", "getTypeId"};


bool isAlreadyRewritten(Stmt* body)
{
    if (!body) {
        return false;
    }
    ContainsContextAccessor v;
    v.TraverseStmt(body);
    return v.found();
}


bool isQualified(const MemberExpr* memberExpr)
{
    if (!memberExpr) {
        return false;
    }

    return memberExpr->getQualifier() != nullptr;
}

bool shouldRewriteMemberExpr(const MemberExpr* memberExpr)
{
    if (!memberExpr) {
        return false;
    }

    if (isQualified(memberExpr)) {
        return false;
    }

    if (!memberExpr->isImplicitAccess()) {
        const Expr* base = memberExpr->getBase();
        if (!base) {
            return false;
        }
        if (!ThisRefCollector::isBaseThis(base)) {
            return false;
        }
    }

    return true;
}

bool isChildOfDerefUnary(const CXXThisExpr* thisExpr, ASTContext& ctx)
{
    auto parents = ctx.getParents(*thisExpr);
    return std::ranges::any_of(parents, [](const auto& p) {
        if (const auto* stmt = p.template get<Stmt>()) {
            if (const auto* unaryOp = dyn_cast<UnaryOperator>(stmt)) {
                return unaryOp->getOpcode() == UO_Deref;
            }
        }
        return false;
    });
}

const Stmt* getFirstNonWrapperParentStmt(const Stmt* stmt, ASTContext& ctx)
{
    if (!stmt) {
        return nullptr;
    }

    auto isWrapper = [](const Stmt* x) {
        return isa<ImplicitCastExpr>(x) || isa<ParenExpr>(x) || isa<CStyleCastExpr>(x)
            || isa<MaterializeTemporaryExpr>(x) || isa<ExprWithCleanups>(x)
            || isa<CXXBindTemporaryExpr>(x);
    };

    const Stmt* cur = stmt;
    while (cur && isWrapper(cur)) {
        auto pp = ctx.getParents(*cur);
        if (pp.empty()) {
            return cur;
        }

        const Stmt* next = nullptr;
        for (const auto& p : pp) {
            next = p.get<Stmt>();
            if (next) {
                break;
            }
        }
        if (!next) {
            return cur;
        }
        cur = next;
    }
    return cur;
}

bool isBaseOfMemberExpr(const CXXThisExpr* thisExpr, ASTContext& ctx)
{
    auto parents = ctx.getParents(*thisExpr);
    for (const auto& p : parents) {
        if (const auto* S = p.get<Stmt>()) {
            const Stmt* top = getFirstNonWrapperParentStmt(S, ctx);

            if (const auto* ME = dyn_cast<MemberExpr>(top)) {
                const Expr* base = ME->getBase();
                if (base && base->IgnoreParenImpCasts() == thisExpr) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool shouldRewriteThisToSelfPtr(const CXXThisExpr* thisExpr, ASTContext& ctx)
{
    if (!thisExpr) {
        return false;
    }

    if (isChildOfDerefUnary(thisExpr, ctx)) {
        return false;
    }

    if (isBaseOfMemberExpr(thisExpr, ctx)) {
        return false;
    }

    return true;
}

bool candidateForRewriting(const MatchFinder::MatchResult& result)
{
    const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>(AccessorLabel);
    if (!method || !method->hasBody()) {
        return false;
    }

    const CXXRecordDecl* parent = method->getParent();
    if (!parent) {
        return false;
    }

    // For now
    if (parent->getQualifiedNameAsString() != "App::PropertyInteger"
        && parent->getQualifiedNameAsString() != "App::PropertyVector") {
        return false;
    }

    if (PropertyMethodsToIgnore.contains(method->getNameAsString())) {
        return false;
    }

    Stmt* body = method->getBody();
    if (isAlreadyRewritten(body)) {
        return false;
    }

    auto* compoundBody = llvm::dyn_cast<CompoundStmt>(body);
    return compoundBody != nullptr;
}


class PropertyAccessorRewriter: public MatchFinder::MatchCallback
{
public:
    explicit PropertyAccessorRewriter(Rewriter& rewriter)
        : rewriter(rewriter)
    {}

    void run(const MatchFinder::MatchResult& result) override
    {
        if (!candidateForRewriting(result)) {
            return;
        }

        // checks for null have been done in candidateForRewriting
        const auto* method = result.Nodes.getNodeAs<CXXMethodDecl>(AccessorLabel);
        Stmt* body = method->getBody();
        const CXXRecordDecl* parent = method->getParent();

        ASTContext& ctx = *result.Context;

        ThisRefCollector collector(ctx, parent);
        collector.TraverseStmt(body);

        bool hasWrite = collector.hasWriteToThisField || collector.hasWriteToThis;
        bool hasRead = !collector.getMembers().empty() || collector.hasReadFromThis;

        Role role = None;
        if (hasWrite) {
            role = Setter;
        }
        else if (hasRead) {
            role = Getter;
        }
        else {
            return;
        }

        rewrite(method, body, parent, collector, role, ctx);
    }

private:
    struct Edit
    {
        SourceLocation begin, end;
        std::string replacement;
        bool isInsertBefore;
    };

    enum Role : uint8_t
    {
        None,
        Getter,
        Setter
    };

    void createSelfDeclEdit(
        std::vector<Edit>& edits,
        const Stmt* body,
        const CXXRecordDecl* parent,
        Role role,
        const std::string& nl,
        const std::string& indentation
    )
    {
        // check for null has been done in candidateForRewriting
        auto* compoundBody = llvm::dyn_cast<CompoundStmt>(body);

        // Build the self-declaration text
        std::string qualClass = parent->getQualifiedNameAsString();  // e.g. "App::PropertyInteger"
        std::string selfDecl = nl + indentation;
        if (role == Getter) {
            // const qualifier for getters
            selfDecl += std::string("auto& self = propGetterSelf<const ") + qualClass + ">(*this);"
                + nl;
        }
        else {
            selfDecl += std::string("auto& self = propSetterSelf<") + qualClass + ">(*this);" + nl;
        }


        // Insert selfDecl after the opening brace of the body.
        SourceLocation lbraceLoc = compoundBody->getLBracLoc();
        // compute insertion point: after '{' token
        SourceLocation insertLoc = lbraceLoc.getLocWithOffset(1);
        // Better: advance to next token to preserve formatting; for simplicity we use offset(1).
        edits.push_back(
            Edit {.begin = insertLoc, .end = insertLoc, .replacement = selfDecl, .isInsertBefore = true}
        );
    }

    void createMemberExprEdits(std::vector<Edit>& edits, const ThisRefCollector& collector)
    {
        for (const MemberExpr* memberExpr : collector.getMembers()) {
            if (!shouldRewriteMemberExpr(memberExpr)) {
                continue;
            }

            const Expr* obj = memberExpr->getBase();
            bool isImplicit = memberExpr->isImplicitAccess();

            SourceLocation begin;
            SourceLocation end;

            if (isImplicit) {
                begin = end = memberExpr->getMemberLoc();
            }
            else {
                SourceLocation opLoc = memberExpr->getOperatorLoc();  // location of '.' or '->'
                begin = obj->getBeginLoc();
                end = opLoc;
            }

            edits.push_back(
                Edit {.begin = begin, .end = end, .replacement = std::string("self."), .isInsertBefore = true}
            );
        }
    }

    void createDerefThisExprEdits(std::vector<Edit>& edits, const ThisRefCollector& collector)
    {
        for (const auto* derefThis : collector.derefThisExprs) {
            SourceLocation begin = derefThis->getBeginLoc();
            SourceLocation end = derefThis->getEndLoc();
            edits.push_back(
                Edit {.begin = begin, .end = end, .replacement = "self", .isInsertBefore = false}
            );
        }
    }

    void createThisRefEdits(
        std::vector<Edit>& edits,
        const ThisRefCollector& collector,
        const SourceManager& sourceMgr,
        const LangOptions& langOpts,
        ASTContext& ctx
    )
    {
        for (const auto* thisExpr : collector.thisRefs) {
            if (!shouldRewriteThisToSelfPtr(thisExpr, ctx)) {
                continue;
            }

            SourceLocation begin = thisExpr->getBeginLoc();
            // thisExpr->getEndLoc() does not yield right result.
            SourceLocation end
                = Lexer::getLocForEndOfToken(begin, 0, sourceMgr, langOpts).getLocWithOffset(-1);

            edits.push_back(
                Edit {.begin = begin, .end = end, .replacement = "&self", .isInsertBefore = false}
            );
        }
    }

    void rewrite(
        const CXXMethodDecl* method,
        Stmt* body,
        const CXXRecordDecl* parent,
        ThisRefCollector& collector,
        Role role,
        ASTContext& ctx
    )
    {

        SourceManager& sourceMgr = ctx.getSourceManager();
        LangOptions langOpts = ctx.getLangOpts();
        SourceRange bodyRange = body->getSourceRange();
        const std::string originalBody
            = Lexer::getSourceText(CharSourceRange::getTokenRange(bodyRange), sourceMgr, langOpts).str();
        const std::string nl = LineEndings::findLineEnding(originalBody);
        const std::string indentation = computeIndentationMethod(stripOuterBraces(originalBody));

        std::vector<Edit> edits;

        createSelfDeclEdit(edits, body, parent, role, nl, indentation);
        createMemberExprEdits(edits, collector);
        createDerefThisExprEdits(edits, collector);
        createThisRefEdits(edits, collector, sourceMgr, langOpts, ctx);

        if (edits.empty()) {
            return;
        }

        std::ranges::sort(edits, [&](const Edit& a, const Edit& b) {
            return sourceMgr.isBeforeInTranslationUnit(b.begin, a.begin);
        });

        for (const Edit& e : edits) {
            if (e.begin == e.end) {
                rewriter.InsertText(
                    e.begin,
                    e.replacement,
                    /*insertAfter=*/false,
                    /*indentNewLines=*/false
                );
            }
            else {
                CharSourceRange range = CharSourceRange::getTokenRange(e.begin, e.end);
                rewriter.ReplaceText(range, e.replacement);
            }
        }

        llvm::errs() << "Rewrote accessor (" << (role == Getter ? "getter" : "setter")
                     << "): " << method->getQualifiedNameAsString() << "\n";
    }

private:
    // The rewriter is a member of PropertyGetterAction.
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    Rewriter& rewriter;
};

class PropertyAccessorAction: public ASTFrontendAction
{
public:
    PropertyAccessorAction() = default;

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

        finder->addMatcher(
            cxxMethodDecl(
                isDefinition(),
                unless(isImplicit()),
                unless(cxxConstructorDecl()),
                unless(cxxDestructorDecl()),
                ofClass(cxxRecordDecl(isDerivedFrom("App::Property")))
            )
                .bind(AccessorLabel),
            callback.get()
        );

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

    return tool.run(newFrontendActionFactory<PropertyAccessorAction>().get());
}
