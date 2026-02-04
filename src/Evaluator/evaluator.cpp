#include "Ast/Expressions/PostfixExprs.hpp"
#include <Ast/AccessModifier.hpp>
#include <Ast/Expressions/FunctionCall.hpp>
#include <Evaluator/Context/context.hpp>
#include <Evaluator/Core/ExprResult.hpp>
#include <Evaluator/Value/value.hpp>
#include <Module/builtins.hpp>
#include <Evaluator/Value/LvObject.hpp>
#include <Evaluator/Value/Type.hpp>
#include <Evaluator/Value/structInstance.hpp>
#include <Ast/astBase.hpp>
#include <Core/fig_string.hpp>
#include <Evaluator/Core/StatementResult.hpp>
#include <Evaluator/Value/value.hpp>
#include <Error/errorLog.hpp>
#include <Evaluator/evaluator.hpp>
#include <Evaluator/evaluator_error.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>

#include <Utils/utils.hpp>
#include <Parser/parser.hpp>

#ifndef SourceInfo
    #define SourceInfo(ptr) (ptr->sourcePath), (ptr->sourceLines)
#endif

namespace Fig
{

    bool Evaluator::isInterfaceSignatureMatch(const Ast::ImplementMethod &implMethod,
                                              const Ast::InterfaceMethod &ifMethod)
    {
        return implMethod.name == ifMethod.name && implMethod.paras == ifMethod.paras;
    }

    StatementResult Evaluator::evalBlockStatement(Ast::BlockStatement block, ContextPtr ctx)
    {
        StatementResult sr = StatementResult::normal();
        for (const Ast::Statement &stmt : block->stmts)
        {
            sr = evalStatement(stmt, ctx);
            if (!sr.isNormal()) { return sr; }
        }
        return sr;
    }

    ContextPtr Evaluator::loadModule(const std::filesystem::path &path)
    {
        static std::unordered_map<FString, std::pair<std::vector<FString>, std::vector<Ast::AstBase>>> mod_ast_cache{};

        FString modSourcePath(path.string());

        std::ifstream file(path);
        assert(file.is_open());

        std::vector<Ast::AstBase> asts;

        std::vector<FString> modSourceLines;

        if (mod_ast_cache.contains(modSourcePath))
        {
            auto &[_sl, _asts] = mod_ast_cache[modSourcePath];
            modSourceLines = _sl;
            asts = _asts;
        }
        else
        {
            std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            modSourceLines = Utils::splitSource(FString(source));

            Lexer lexer((FString(source)), modSourcePath, modSourceLines);
            Parser parser(lexer, modSourcePath, modSourceLines);

            asts = parser.parseAll();
            mod_ast_cache[modSourcePath] = {modSourceLines, asts};
        }

        Evaluator evaluator;
        evaluator.SetSourcePath(modSourcePath);
        evaluator.SetSourceLines(modSourceLines);

        ContextPtr modctx = std::make_shared<Context>(FString(std::format("<Module at {}>", path.string())), nullptr);

        evaluator.SetGlobalContext(modctx);
        evaluator.RegisterBuiltinsValue();
        evaluator.Run(asts); // error upward pass-by, log outside, we have already keep info in evaluator error

        return evaluator.global;
    }

    StatementResult Evaluator::evalImportSt(Ast::Import i, ContextPtr ctx)
    {
        const std::vector<FString> &pathVec = i->path;

        FString modName = pathVec.back(); // pathVec at least has 1 element
        if (modName == u8"_builtins")
        {
            RegisterBuiltins();
            return StatementResult::normal();
        }

        // std::cerr << modName.toBasicString() << '\n'; DEBUG

        if (ctx->containsInThisScope(modName))
        {
            throw EvaluatorError(
                u8"RedeclarationError", std::format("{} has already been declared.", modName.toBasicString()), i);
        }

        auto path = resolveModulePath(pathVec);
        ContextPtr modCtx = loadModule(path);

        // 冲突问题等impl存储改成 2xMap之后解决吧（咕咕咕
        ctx->getImplRegistry().insert(modCtx->getImplRegistry().begin(), modCtx->getImplRegistry().end()); // load impl

        for (auto &[type, opRecord] : modCtx->getOpRegistry())
        {
            if (ctx->getOpRegistry().contains(type))
            {
                throw EvaluatorError(
                    u8"DuplicateOperationOverload",
                    std::format("Module `{}` and current context `{}` have conflict operation overload for `{}` object",
                                modCtx->getScopeName().toBasicString(),
                                ctx->getScopeName().toBasicString(),
                                type.toString().toBasicString()),
                    i);
            }
            ctx->getOpRegistry()[type] = opRecord;
        }
        if (!i->names.empty())
        {
            for (const FString &symName : i->names)
            {
                LvObject tmp(modCtx->get(symName), modCtx);
                const ObjectPtr &value = tmp.get();

                ctx->def(symName, tmp.declaredType(), AccessModifier::Const, value);
            }
            return StatementResult::normal();
        }
        if (!i->rename.empty()) { modName = i->rename; }
        ctx->def(
            modName, ValueType::Module, AccessModifier::PublicConst, std::make_shared<Object>(Module(modName, modCtx)));

        return StatementResult::normal();
    }

    StatementResult Evaluator::Run(std::vector<Ast::AstBase> asts)
    {
        using Ast::AstType;
        StatementResult sr = StatementResult::normal();
        for (auto &ast : asts)
        {
            // statement, all stmt!
            Ast::Statement stmt = std::static_pointer_cast<Ast::StatementAst>(ast);
            assert(stmt != nullptr);
            sr = evalStatement(stmt, global);
            if (sr.isError()) { handle_error(sr, stmt, global); }
            if (!sr.isNormal()) { return sr; }
        }

        return sr;
    }

    void Evaluator::handle_error(const StatementResult &sr, const Ast::Statement &stmt, const ContextPtr &ctx)
    {
        assert(sr.isError());
        const ObjectPtr &result = sr.result;
        const TypeInfo &resultType = actualType(result);

        if (result->is<StructInstance>() && implements(resultType, Builtins::getErrorInterfaceTypeInfo(), ctx))
        {
            /*
                toString() -> String
                getErrorClass() -> String
                getErrorMessage() -> String
            */
            const StructInstance &resInst = result->as<StructInstance>();

            Function getErrorClassFn = ctx->getImplementedMethod(resultType, u8"getErrorClass");
            getErrorClassFn = Function(getErrorClassFn.name,
                                       getErrorClassFn.paras,
                                       getErrorClassFn.retType,
                                       getErrorClassFn.body,
                                       resInst.localContext);
            Function getErrorMessageFn = ctx->getImplementedMethod(resultType, u8"getErrorMessage");
            getErrorMessageFn = Function(getErrorMessageFn.name,
                                         getErrorMessageFn.paras,
                                         getErrorMessageFn.retType,
                                         getErrorMessageFn.body,
                                         resInst.localContext);

            const ExprResult &errorClassRes =
                executeFunction(getErrorClassFn, Ast::FunctionCallArgs{}, resInst.localContext);
            const ExprResult &errorMessageRes =
                executeFunction(getErrorMessageFn, Ast::FunctionCallArgs{}, resInst.localContext);

            if (errorClassRes.isError()) { handle_error(errorClassRes.toStatementResult(), getErrorClassFn.body, ctx); }
            if (errorMessageRes.isError())
            {
                handle_error(errorMessageRes.toStatementResult(), getErrorMessageFn.body, ctx);
            }

            // std::cerr << errorClassRes.unwrap()->toString().toBasicString() << "\n";
            // std::cerr << errorMessageRes.unwrap()->toString().toBasicString() << "\n";

            const FString &errorClass = errorClassRes.unwrap()->as<ValueType::StringClass>();
            const FString &errorMessage = errorMessageRes.unwrap()->as<ValueType::StringClass>();

            ErrorLog::logFigErrorInterface(errorClass, errorMessage);
            std::exit(1);
        }
        else
        {
            throw EvaluatorError(u8"UncaughtExceptionError",
                                 std::format("Uncaught exception: {}", sr.result->toString().toBasicString()),
                                 stmt);
        }
    }

    void Evaluator::printStackTrace()
    {
        if (global) global->printStackTrace();
    }
}; // namespace Fig