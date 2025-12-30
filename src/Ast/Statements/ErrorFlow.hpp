#pragma once

#include <Ast/astBase.hpp>

namespace Fig::Ast
{
    class ThrowSt final : public StatementAst
    {
    public:
        Expression value;

        ThrowSt()
        {
            type = AstType::ThrowSt;
        }

        ThrowSt(Expression _value) :
            value(std::move(_value))
        {
            type = AstType::ThrowSt;
        }
    };
    using Throw = std::shared_ptr<ThrowSt>;

    struct Catch
    {
        FString errVarName;
        bool hasType = false;
        FString errVarType;
        BlockStatement body;

        Catch() {}
        Catch(FString _errVarName, FString _errVarType, BlockStatement _body) :
            errVarName(std::move(_errVarName)), errVarType(std::move(_errVarType)), body(std::move(_body))
        {
            hasType = true;
        }
        Catch(FString _errVarName, BlockStatement _body) :
            errVarName(std::move(_errVarName)), body(std::move(_body))
        {
            hasType = false;
        }
    };

    class TrySt final : public StatementAst
    {
    public:
        BlockStatement body;
        std::vector<Catch> catches;
        BlockStatement finallyBlock = nullptr;

        TrySt()
        {
            type = AstType::TrySt;
        }
        TrySt(BlockStatement _body, std::vector<Catch> _catches, BlockStatement _finallyBlock) :
            body(std::move(_body)), catches(std::move(_catches)), finallyBlock(std::move(_finallyBlock))
        {
            type = AstType::TrySt;
        }
    };

    using Try = std::shared_ptr<TrySt>;
} // namespace Fig::Ast