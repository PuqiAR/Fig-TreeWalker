#pragma once
#include "Ast/AccessModifier.hpp"
#include "Ast/Expressions/BinaryExpr.hpp"
#include "Ast/Expressions/ValueExpr.hpp"
#include "Ast/Expressions/VarExpr.hpp"
#include "Ast/Statements/ControlSt.hpp"
#include "Ast/astBase.hpp"
#include "Ast/functionParameters.hpp"
#include "Value/value.hpp"
#include <Ast/Expressions/FunctionCall.hpp>
#include <Ast/Expressions/InitExpr.hpp>
#include <Ast/Statements/ImplementSt.hpp>
#include <Ast/Statements/InterfaceDefSt.hpp>
#include <Evaluator/Value/Type.hpp>
#include <Ast/ast.hpp>

#include <Evaluator/Context/context.hpp>
#include <Error/error.hpp>
#include <Module/builtins.hpp>
#include <Evaluator/Value/LvObject.hpp>
#include <cstddef>
#include <filesystem>

#include <Evaluator/Core/StatementResult.hpp>
#include <Evaluator/Core/ExprResult.hpp>
#include <memory>
#include <source_location>

namespace Fig
{
    class Evaluator
    {
    private:
        ContextPtr global;

    public:
        FString sourcePath;
        std::vector<FString> sourceLines;

        void SetSourcePath(const FString &sp) { sourcePath = sp; }

        void SetSourceLines(const std::vector<FString> &sl) { sourceLines = sl; }

        void SetGlobalContext(ContextPtr ctx)
        {
            assert(ctx != nullptr);
            global = ctx;
        }

        void CreateGlobalContext() { global = std::make_shared<Context>(FString(u8"<Global>")); }

        void RegisterBuiltins() // only function
        {
            assert(global != nullptr);

            for (auto &[name, fn] : Builtins::getBuiltinFunctions())
            {
                int argc = Builtins::getBuiltinFunctionParamCount(name);
                Function f(name, fn, argc);
                global->def(name, ValueType::Function, AccessModifier::Const, std::make_shared<Object>(f));
            }

            global->setImplRecord(
                Builtins::getTypeErrorStructTypeInfo(),
                Builtins::getErrorInterfaceTypeInfo(),
                ImplRecord{
                    .interfaceType = Builtins::getErrorInterfaceTypeInfo(),
                    .structType = Builtins::getTypeErrorStructTypeInfo(),
                    .implMethods = {
                        {u8"toString",
                         Function(
                             u8"toString",
                             Ast::FunctionParameters{},
                             ValueType::String,
                             std::make_shared<Ast::BlockStatementAst>(std::vector<Ast::Statement>(
                                 {std::make_shared<Ast::ReturnSt>(std::make_shared<Ast::BinaryExprAst>(
                                     std::make_shared<Ast::ValueExprAst>(std::make_shared<Object>(u8"TypeError: ")),
                                     Ast::Operator::Add,
                                     std::make_shared<Ast::FunctionCallExpr>(
                                         std::make_shared<Ast::VarExprAst>(u8"getErrorMessage"),
                                         Ast::FunctionArguments{})))})),
                             nullptr)},
                        {u8"getErrorClass",
                         Function(u8"getErrorClass",
                                  Ast::FunctionParameters{},
                                  ValueType::String,
                                  std::make_shared<Ast::BlockStatementAst>(std::vector<Ast::Statement>(
                                      {std::make_shared<Ast::ReturnSt>(std::make_shared<Ast::ValueExprAst>(
                                          std::make_shared<Object>(FString(u8"TypeError"))))})),
                                  nullptr)},
                        {u8"getErrorMessage",
                         Function(u8"getErrorMessage",
                                  Ast::FunctionParameters{},
                                  ValueType::String,
                                  std::make_shared<Ast::BlockStatementAst>(std::vector<Ast::Statement>(
                                      {std::make_shared<Ast::ReturnSt>(std::make_shared<Ast::VarExprAst>(u8"msg"))})),
                                  nullptr)},
                    }});
        }

        void RegisterBuiltinsValue()
        {
            assert(global != nullptr);

            for (auto &[name, val] : Builtins::getBuiltinValues())
            {
                global->def(name, val->getTypeInfo(), AccessModifier::Const, val);
            }
        }

        bool isInterfaceSignatureMatch(const Ast::ImplementMethod &, const Ast::InterfaceMethod &);

        ObjectPtr genTypeError(const FString &_msg,
                               const Ast::AstBase &_ast,
                               ContextPtr ctx,
                               std::source_location loc = std::source_location::current())
        {
            ContextPtr stCtx = std::make_shared<Context>(u8"<TypeError Instance>");
            stCtx->def(u8"msg", ValueType::String, AccessModifier::Const, std::make_shared<Object>(_msg));
            return std::make_shared<Object>(StructInstance(Builtins::getTypeErrorStructTypeInfo(), stCtx));
        }

        /* Left-value eval*/
        ExprResult evalVarExpr(Ast::VarExpr, ContextPtr);       // identifier: a, b, c
        ExprResult evalMemberExpr(Ast::MemberExpr, ContextPtr); // a.b
        ExprResult evalIndexExpr(Ast::IndexExpr, ContextPtr);   // a[b]

        ExprResult evalLv(Ast::Expression, ContextPtr); // for access: a.b / index a[b]

        /* Right-value eval*/
        ExprResult evalInitExpr(Ast::InitExpr, ContextPtr);
        ExprResult evalBinary(Ast::BinaryExpr, ContextPtr);   // normal binary expr: +, -, *....
        ExprResult evalUnary(Ast::UnaryExpr, ContextPtr);     // unary expr
        ExprResult evalTernary(Ast::TernaryExpr, ContextPtr); // ternary expr

        ExprResult executeFunction(const Function &fn, const Ast::FunctionCallArgs &, ContextPtr); // fn, fn context

        ExprResult evalFunctionCall(const Ast::FunctionCall &,
                                    ContextPtr); // function call

        ExprResult eval(Ast::Expression, ContextPtr);

        StatementResult evalBlockStatement(Ast::BlockStatement, ContextPtr); // block
        StatementResult evalStatement(Ast::Statement, ContextPtr);           // statement

        std::filesystem::path resolveModulePath(const std::vector<FString> &);
        ContextPtr loadModule(const std::filesystem::path &);

        StatementResult evalImportSt(Ast::Import, ContextPtr);

        StatementResult Run(std::vector<Ast::AstBase>); // Entry

        void handle_error(const StatementResult &, const Ast::Statement &, const ContextPtr &);

        void printStackTrace();
    };
}; // namespace Fig