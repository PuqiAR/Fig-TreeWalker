#pragma once

#include <Ast/astBase.hpp>

#include <vector>

namespace Fig::Ast
{
    /*
    import std.io as stdio; // io --> stdio;
    import std.io {print, println};
    */

    class ImportSt final : public StatementAst
    {
    public:
        std::vector<FString> path;
        std::vector<FString> names;

        FString rename;
        
        ImportSt()
        {
            type = AstType::ImportSt;
        }

        ImportSt(std::vector<FString> _path, std::vector<FString> _names, const FString &_rename) :
            path(std::move(_path)), names(std::move(_names)), rename(_rename)
        {
            type = AstType::ImportSt;
        }
    };

    using Import = std::shared_ptr<ImportSt>;
};