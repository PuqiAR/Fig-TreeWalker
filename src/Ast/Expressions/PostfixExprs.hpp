#pragma once

#include <Ast/astBase.hpp>

namespace Fig::Ast
{
    // actually, function call is postfix, too
    // but it's too long, so use a single file (FunctionCall.hpp)

    class MemberExprAst final : public ExpressionAst
    {
    public:
        Expression base;
        FString member;

        MemberExprAst()
        {
            type = AstType::MemberExpr;
        }

        MemberExprAst(Expression _base, FString _member) :
            base(std::move(_base)), member(std::move(_member))
        {
            type = AstType::MemberExpr;
        }
    };

    using MemberExpr = std::shared_ptr<MemberExprAst>;

    class IndexExprAst final : public ExpressionAst
    {
    public:
        Expression base;
        Expression index;

        IndexExprAst()
        {
            type = AstType::IndexExpr;
        }

        IndexExprAst(Expression _base, Expression _index) :
            base(std::move(_base)), index(std::move(_index))
        {
            type = AstType::IndexExpr;
        }
    };

    using IndexExpr = std::shared_ptr<IndexExprAst>;
}; // namespace Fig::Ast