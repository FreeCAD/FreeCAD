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

#ifndef VISITORS_H
#define VISITORS_H

#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/Debug.h>

#include "debug.h"

namespace Visitors
{

namespace cl = clang;

inline constexpr std::string_view GetterFunctionName = "propGetterSelf";
inline constexpr std::string_view SetterFunctionName = "propSetterSelf";


/// Visitor to check if a function body contains calls to the context accessor
/// functions.
class ContainsContextAccessor: public cl::RecursiveASTVisitor<ContainsContextAccessor>
{


public:
    ContainsContextAccessor() = default;

    bool VisitCallExpr(const cl::CallExpr* callExpr)
    {
        const cl::FunctionDecl* functionDecl = callExpr->getDirectCallee();
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


/**
 * @brief Visitor to collect all references to `this` in a method body.
 *
 * It detects both explicit and implicit uses of `this`, including member accesses, calls to
 * non-const member functions, and passing `this` or its fields to other functions.
 */
struct ThisRefCollector: public cl::RecursiveASTVisitor<ThisRefCollector>
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    cl::ASTContext& context;
    const cl::CXXRecordDecl* methodParent;

    std::vector<const cl::CXXThisExpr*> thisRefs;
    std::vector<const cl::MemberExpr*> fieldMembers;
    std::vector<const cl::MemberExpr*> methodMembers;

    bool hasWriteToThisField {false};

    bool hasWriteToThis {false};
    bool hasReadFromThis {false};

    std::vector<const cl::UnaryOperator*> derefThisExprs;


    ThisRefCollector(cl::ASTContext& context, const cl::CXXRecordDecl* decl)
        : context(context)
        , methodParent(decl)
    {}

    // this->field, this->method(), field, method()
    bool VisitMemberExpr(const cl::MemberExpr* memberExpr)
    {
        Debug::out() << "VisitMemberExpr\n";
        Debug::out() << "  memberExpr: " << Debug::toSourceText(memberExpr, context) << "\n";

        const cl::Decl* memberDecl = memberExpr->getMemberDecl();
        if (!memberDecl) {
            return true;
        }

        const auto* fieldDecl = dyn_cast_or_null<cl::FieldDecl>(memberDecl);
        const auto* methodDecl = dyn_cast_or_null<cl::CXXMethodDecl>(memberDecl);
        if (!fieldDecl && !methodDecl) {
            return true;
        }

        const cl::CXXRecordDecl* memberParent = nullptr;
        if (fieldDecl) {
            memberParent = dyn_cast<cl::CXXRecordDecl>(fieldDecl->getParent());
        }
        else {
            memberParent = methodDecl->getParent();
        }

        if (!memberParent || !isFromClassOrBase(memberParent, methodParent)) {
            return true;
        }

        if (!memberExpr->isImplicitAccess() && !isBaseThis(memberExpr->getBase())) {
            return true;
        }

        if (fieldDecl) {
            fieldMembers.push_back(memberExpr);
            Debug::out() << "  field member\n";
        }
        else {
            methodMembers.push_back(memberExpr);
            Debug::out() << "  method member\n";
        }
        return true;
    }

    // field = ..., field += ..., this->field = ...,
    bool VisitBinaryOperator(const cl::BinaryOperator* binaryOp)
    {
        Debug::out() << "VisitBinaryOperator\n";
        Debug::out() << "  binaryOp: " << Debug::toSourceText(binaryOp, context) << "\n";

        if (!binaryOp || hasWriteToThisField) {
            return true;
        }

        if ((binaryOp->getOpcode() == cl::BO_Assign || binaryOp->isCompoundAssignmentOp())
            && getRootThisField(binaryOp->getLHS(), methodParent)) {

            Debug::out() << "  write to this field detected\n";
            hasWriteToThisField = true;
        }
        return true;
    }

    // Overloaded assignment / compound assignment (operator=, operator+=, etc.)
    bool VisitCXXOperatorCallExpr(const cl::CXXOperatorCallExpr* opCall)
    {
        Debug::out() << "VisitCXXOperatorCallExpr\n";
        Debug::out() << "  opCall: " << Debug::toSourceText(opCall, context) << "\n";

        if (!opCall || hasWriteToThisField) {
            return true;
        }

        const auto op = opCall->getOperator();

        auto isWriteOp = [&](clang::OverloadedOperatorKind K) {
            switch (K) {
                case cl::OO_Equal:
                case cl::OO_PlusEqual:
                case cl::OO_MinusEqual:
                case cl::OO_StarEqual:
                case cl::OO_SlashEqual:
                case cl::OO_PercentEqual:
                case cl::OO_LessLessEqual:
                case cl::OO_GreaterGreaterEqual:
                case cl::OO_AmpEqual:
                case cl::OO_PipeEqual:
                case cl::OO_CaretEqual:
                    return true;
                default:
                    return false;
            }
        };

        if (isWriteOp(op) && opCall->getNumArgs() >= 1
            && getRootThisField(opCall->getArg(0), methodParent)) {

            Debug::out() << "  write to this field detected\n";
            hasWriteToThisField = true;
        }

        return true;
    }

    // ++field, field++, --field, field--, *this
    bool VisitUnaryOperator(const cl::UnaryOperator* unaryOp)
    {
        Debug::out() << "VisitUnaryOperator\n";
        Debug::out() << "  unaryOp: " << Debug::toSourceText(unaryOp, context) << "\n";

        if (!unaryOp || hasWriteToThisField) {
            return true;
        }

        if (unaryOp->isIncrementDecrementOp()
            && getRootThisField(unaryOp->getSubExpr(), methodParent)) {

            Debug::out() << "  write to this field detected\n";
            hasWriteToThisField = true;
        }

        if (unaryOp->getOpcode() == cl::UO_Deref) {
            const cl::Expr* sub = ignoreWrappers(unaryOp->getSubExpr());
            if (isa<cl::CXXThisExpr>(sub)) {
                derefThisExprs.push_back(unaryOp);
            }
        }
        return true;
    }

    // this->method(), method()
    bool VisitCXXMemberCallExpr(const cl::CXXMemberCallExpr* memberCall)
    {
        Debug::out() << "VisitCXXMemberCallExpr\n";
        Debug::out() << "  memberCall: " << Debug::toSourceText(memberCall, context) << "\n";

        if (Debug::toSourceText(memberCall, context) == "setValue(boost::any_cast<long>(value))") {
            Debug::out() << "  we are there\n";
        }

        if (!memberCall || hasWriteToThisField) {
            return true;
        }

        const cl::CXXMethodDecl* callee = memberCall->getMethodDecl();
        if (!callee) {
            return true;
        }

        // Heuristic: non-const member function may mutate the object
        if (callee->isConst()) {
            return true;
        }

        const cl::Expr* obj = memberCall->getImplicitObjectArgument();
        if (!obj) {
            return true;
        }
        obj = ignoreWrappers(obj);

        if (llvm::isa<cl::CXXThisExpr>(obj) || getRootThisField(obj, methodParent)) {
            Debug::out() << " non-const member call on this field detected\n";
            hasWriteToThis = true;
        }

        return true;
    }

    bool argHasWriteToThisField(const cl::CallExpr* call, unsigned i, const cl::FunctionDecl* callee) const
    {
        Debug::out() << "argHasWriteToThisField: arg " << i << " of call to "
                     << callee->getQualifiedNameAsString() << "\n";
        const cl::Expr* arg = call->getArg(i);
        if (!arg) {
            return false;
        }

        Debug::out() << "  arg: " << Debug::toSourceText(arg, context) << "\n";

        const cl::Expr* argNoWrap = ignoreWrappers(arg);
        bool hasAddressOf = false;

        if (const auto* unaryOp = dyn_cast<cl::UnaryOperator>(argNoWrap);
            unaryOp && unaryOp->getOpcode() == cl::UO_AddrOf) {

            hasAddressOf = true;
            argNoWrap = ignoreWrappers(unaryOp->getSubExpr());
        }

        const cl::FieldDecl* root = getRootThisField(argNoWrap, methodParent);
        if (!root) {
            return false;
        }

        if (i >= callee->getNumParams()) {
            return false;
        }

        cl::QualType typeParamDecl = callee->getParamDecl(i)->getType();

        if (!hasAddressOf) {
            if (const auto* refType = typeParamDecl->getAs<cl::LValueReferenceType>();
                refType && !refType->getPointeeType().isConstQualified()) {

                Debug::out() << "  arg is non-const reference\n";

                return true;
            }
        }
        else {
            if (const auto* ptrType = typeParamDecl->getAs<cl::PointerType>();
                ptrType && !ptrType->getPointeeType().isConstQualified()) {

                Debug::out() << "  arg is non-const pointer\n";

                return true;
            }
        }
        return false;
    }

    // field, method(), this->field, this->method() passed as argument
    bool VisitCallExpr(const cl::CallExpr* call)
    {
        Debug::out() << "VisitCallExpr\n";
        Debug::out() << "  call: " << Debug::toSourceText(call, context) << "\n";

        if (!call || hasWriteToThisField) {
            return true;
        }

        const cl::FunctionDecl* callee = call->getDirectCallee();
        if (!callee) {
            return true;
        }

        for (unsigned i = 0; i < call->getNumArgs(); ++i) {
            if (!hasWriteToThisField && argHasWriteToThisField(call, i, callee)) {

                Debug::out() << "  call argument may write to this field\n";
                hasWriteToThisField = true;
            }

            const cl::Expr* arg = call->getArg(i);
            if (!arg) {
                continue;
            }

            if (isDerefThisExpr(arg)) {
                Debug::out() << "  argument is deref of this\n";
                if (i < callee->getNumParams()) {
                    cl::QualType paramType = callee->getParamDecl(i)->getType();
                    classifyThisBinding(paramType, hasReadFromThis, hasWriteToThis);
                }
            }
        }

        return true;
    }

    // this passed as argument to constructor initializer list: Ctor(this, ...), new Ctor(this, ...)
    bool VisitCXXConstructExpr(const cl::CXXConstructExpr* constructExpr)
    {
        if (!constructExpr) {
            return true;
        }

        const cl::CXXConstructorDecl* ctor = constructExpr->getConstructor();
        if (!ctor) {
            return true;
        }

        for (unsigned i = 0; i < constructExpr->getNumArgs(); ++i) {
            const cl::Expr* arg = constructExpr->getArg(i);
            if (!arg) {
                continue;
            }

            if (!isDerefThisExpr(arg)) {
                continue;
            }

            if (i >= ctor->getNumParams()) {
                continue;
            }

            cl::QualType paramType = ctor->getParamDecl(i)->getType();
            classifyThisBinding(paramType, hasReadFromThis, hasWriteToThis);
        }

        return true;
    }

    // this expressions themselves
    bool VisitCXXThisExpr(const cl::CXXThisExpr* thisExpr)
    {
        if (!thisExpr) {
            return true;
        }

        if (thisExpr->isImplicit()) {
            return true;
        }

        thisRefs.push_back(thisExpr);
        return true;
    }

    std::vector<const cl::MemberExpr*> getMembers() const
    {
        std::vector<const cl::MemberExpr*> members;
        members.insert(members.end(), fieldMembers.begin(), fieldMembers.end());
        members.insert(members.end(), methodMembers.begin(), methodMembers.end());
        return members;
    }

    static bool isBaseThis(const cl::Expr* base)
    {
        if (!base) {
            return false;
        }
        base = base->IgnoreParenImpCasts();

        if (isa<cl::CXXThisExpr>(base)) {
            return true;
        }

        if (const auto* uo = dyn_cast<cl::UnaryOperator>(base);
            uo && uo->getOpcode() == cl::UO_Deref) {

            const cl::Expr* sub = uo->getSubExpr()->IgnoreParenImpCasts();
            return isa<cl::CXXThisExpr>(sub);
        }
        return false;
    }

private:
    static bool isFromClassOrBase(const cl::CXXRecordDecl* candidate, const cl::CXXRecordDecl* methodParent)
    {
        if (!candidate || !methodParent) {
            return false;
        }
        return (candidate == methodParent) || methodParent->isDerivedFrom(candidate);
    }

    static const cl::FieldDecl* getRootThisField(
        const cl::Expr* expr,
        const cl::CXXRecordDecl* methodParent
    )
    {
        expr = ignoreWrappers(expr);
        if (!expr || !methodParent) {
            return nullptr;
        }

        // Walk "down" the member chain: ((_cVec).x).y ...
        // We want to find the first MemberExpr in the chain whose member is a FieldDecl on this.
        const cl::Expr* cur = expr;

        while (true) {
            cur = ignoreWrappers(cur);
            if (!cur) {
                return nullptr;
            }

            // Handle member expr: base.member
            if (const auto* memberExpr = dyn_cast<cl::MemberExpr>(cur)) {
                const cl::Expr* base = ignoreWrappers(memberExpr->getBase());

                if (const cl::FieldDecl* fieldDecl = fieldWithBaseThis(base, memberExpr, methodParent)) {
                    return fieldDecl;
                }

                cur = base;
                continue;
            }

            // Handle array subscript: base[idx]  (for built-in arrays / pointers)
            if (const auto* ASE = dyn_cast<cl::ArraySubscriptExpr>(cur)) {
                cur = ASE->getBase();
                continue;
            }

            // Other expression kind: stop
            return nullptr;
        }
    }

    static const cl::Expr* ignoreWrappers(const cl::Expr* E)
    {
        return E ? E->IgnoreParenImpCasts() : nullptr;
    }

    static const cl::FieldDecl* fieldWithBaseThis(
        const cl::Expr* base,
        const cl::MemberExpr* memberExpr,
        const cl::CXXRecordDecl* methodParent
    )
    {
        if (!base || !memberExpr || !methodParent) {
            return nullptr;
        }

        if (const auto* fieldDecl = dyn_cast<cl::FieldDecl>(memberExpr->getMemberDecl())) {
            const auto* fieldParent = dyn_cast<cl::CXXRecordDecl>(fieldDecl->getParent());
            if (fieldParent
                && (fieldParent == methodParent || methodParent->isDerivedFrom(fieldParent))) {
                if (memberExpr->isImplicitAccess()) {
                    return fieldDecl;
                }
                // explicit access: require base is `this` or `(*this)`
                if (base
                    && (isa<cl::CXXThisExpr>(base)
                        || (isa<cl::UnaryOperator>(base)
                            && cast<cl::UnaryOperator>(base)->getOpcode() == cl::UO_Deref
                            && isa<cl::CXXThisExpr>(
                                ignoreWrappers(cast<cl::UnaryOperator>(base)->getSubExpr())
                            )))) {
                    return fieldDecl;
                }
            }
        }
        return nullptr;
    }

    static bool isDerefThisExpr(const cl::Expr* expr)
    {
        expr = expr ? expr->IgnoreParenImpCasts() : nullptr;
        const auto* unaryOp = dyn_cast_or_null<cl::UnaryOperator>(expr);
        if (!unaryOp || unaryOp->getOpcode() != cl::UO_Deref) {
            return false;
        }
        return isa<cl::CXXThisExpr>(unaryOp->getSubExpr()->IgnoreParenImpCasts());
    }

    static void classifyThisBinding(cl::QualType paramType, bool& hasReadFromThis, bool& hasWriteToThis)
    {
        // References
        if (const auto* refType = paramType->getAs<cl::LValueReferenceType>()) {
            cl::QualType pointee = refType->getPointeeType();
            if (pointee.isConstQualified()) {
                hasReadFromThis = true;
            }
            else {
                hasWriteToThis = true;
            }
            return;
        }

        // Pointers
        if (const auto* ptrType = paramType->getAs<cl::PointerType>()) {
            cl::QualType pointee = ptrType->getPointeeType();
            if (pointee.isConstQualified()) {
                hasReadFromThis = true;
            }
            else {
                hasWriteToThis = true;
            }
            return;
        }

        hasReadFromThis = true;
    }
};


}  // namespace Visitors


#endif
