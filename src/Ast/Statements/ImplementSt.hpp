#pragma once
#include <Ast/functionParameters.hpp>
#include <Ast/Statements/FunctionDefSt.hpp>
#include <Ast/astBase.hpp>

namespace Fig::Ast
{
    /*
    
    impl Readable for File
    {
        read() -> String
        {
            ...
    }
    
    */
    struct ImplementMethod
    {
        FString name;
        FunctionParameters paras;
        BlockStatement body;
    };

    class ImplementAst final : public StatementAst
    {
    public:
        FString interfaceName;
        FString structName;

        std::vector<ImplementMethod> methods;

        ImplementAst()
        {
            type = AstType::ImplementSt;
        }

        ImplementAst(FString _interfaceName, FString _structName, std::vector<ImplementMethod> _methods) :
            interfaceName(std::move(_interfaceName)), structName(std::move(_structName)), methods(std::move(_methods))
        {
            type = AstType::ImplementSt;
        }
    };

    using Implement = std::shared_ptr<ImplementAst>;
};