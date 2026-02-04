#include <Ast/Expressions/BinaryExpr.hpp>
#include <Evaluator/Value/value.hpp>
#include <Evaluator/Value/Type.hpp>
#include <Evaluator/Value/Type.hpp>
#include <Evaluator/Value/LvObject.hpp>
#include <Evaluator/Value/IntPool.hpp>
#include <Evaluator/evaluator.hpp>
#include <Evaluator/evaluator_error.hpp>
#include <Evaluator/Core/ExprResult.hpp>

#include <exception>
#include <memory>
#include <functional>

namespace Fig
{
    ExprResult Evaluator::evalBinary(Ast::BinaryExpr bin, ContextPtr ctx)
    {
        using Ast::Operator;
        Operator op = bin->op;
        Ast::Expression lexp = bin->lexp, rexp = bin->rexp;

        const auto &tryInvokeOverloadFn =
            [ctx, op](const ObjectPtr &lhs, const ObjectPtr &rhs, const std::function<ExprResult()> &rollback) {
                if (lhs->is<StructInstance>() && lhs->getTypeInfo() == rhs->getTypeInfo())
                {
                    // 运算符重载
                    const TypeInfo &type = actualType(lhs);
                    if (ctx->hasOperatorImplemented(type, op))
                    {
                        const auto &fnOpt = ctx->getBinaryOperatorFn(type, op);
                        return (*fnOpt)(lhs, rhs);
                    }
                }
                return rollback();
            };

        switch (op)
        {
            case Operator::Add: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() + rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(*lhs + *rhs);
                });
            }
            case Operator::Subtract: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() - rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(*lhs - *rhs);
                });
            }
            case Operator::Multiply: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() * rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(*lhs * *rhs);
                });
            }
            case Operator::Divide: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs / *rhs); });
            }
            case Operator::Modulo: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass lv = lhs->as<ValueType::IntClass>();
                        ValueType::IntClass rv = rhs->as<ValueType::IntClass>();
                        if (rv == 0) { throw ValueError(FString(std::format("Modulo by zero: {} % {}", lv, rv))); }
                        ValueType::IntClass result = lv / rv;
                        ValueType::IntClass r = lv % rv;
                        if (r != 0 && ((lv < 0) != (rv < 0))) { result -= 1; }
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(*lhs % *rhs);
                });
            }

            case Operator::Is: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs, ctx, bin]() {
                    const TypeInfo &lhsType = lhs->getTypeInfo();
                    const TypeInfo &rhsType = rhs->getTypeInfo();

                    if (lhs->is<StructInstance>() && rhs->is<StructType>())
                    {
                        const StructInstance &si = lhs->as<StructInstance>();
                        const StructType &st = rhs->as<StructType>();
                        return std::make_shared<Object>(si.parentType == st.type);
                    }
                    if (lhs->is<StructInstance>() && rhs->is<InterfaceType>())
                    {
                        const StructInstance &si = lhs->as<StructInstance>();
                        const InterfaceType &it = rhs->as<InterfaceType>();
                        return std::make_shared<Object>(implements(si.parentType, it.type, ctx));
                    }

                    if (ValueType::isTypeBuiltin(lhsType) && rhsType == ValueType::StructType)
                    {
                        const StructType &st = rhs->as<StructType>();
                        const TypeInfo &type = st.type;
                        /*
                            如果是内置类型(e.g. Int, String)
                            那么 eval出来String这个字，出来的是StructType
                            而出来的StructType.type就不会是一个独立的TypeInfo,而是内置的ValueType::String
                            依次我们可以判断内置类型

                            e.g:
                                "123" is String
                                  L   OP    R

                            其中 L 类型为 String
                            而 R 类型为 StructType (builtins.hpp) 中注册
                            拿到 R 的 StructType, 其中的 type 为 String
                        */
                        if (lhs->getTypeInfo() == type) { return Object::getTrueInstance(); }
                        return Object::getFalseInstance();
                    }

                    throw EvaluatorError(u8"TypeError",
                                         std::format("Unsupported operator `is` for '{}' && '{}'",
                                                     prettyType(lhs).toBasicString(),
                                                     prettyType(rhs).toBasicString()),
                                         bin->lexp);
                });
            }

            case Operator::As: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs, ctx, bin, this]() -> ExprResult {
                    if (!rhs->is<StructType>())
                    {
                        throw EvaluatorError(
                            u8"OperatorError",
                            std::format("Operator `as` requires right hand side operand a struct type, but got '{}'",
                                        prettyType(rhs).toBasicString()),
                            bin->rexp);
                    }
                    const StructType &targetStructType = rhs->as<StructType>();
                    const TypeInfo &targetType = targetStructType.type;
                    const TypeInfo &sourceType = lhs->getTypeInfo();
                    if (targetType == sourceType) { return lhs; }
                    if (targetType == ValueType::String) { return std::make_shared<Object>(lhs->toStringIO()); }
                    if (sourceType == ValueType::Int)
                    {
                        if (targetType == ValueType::Double)
                        {
                            return std::make_shared<Object>(
                                static_cast<ValueType::DoubleClass>(lhs->as<ValueType::IntClass>()));
                        }
                    }
                    else if (sourceType == ValueType::Double)
                    {
                        if (targetType == ValueType::Int)
                        {
                            return IntPool::getInstance().createInt(
                                static_cast<ValueType::IntClass>(lhs->as<ValueType::DoubleClass>()));
                        }
                    }
                    else if (sourceType == ValueType::String)
                    {
                        const FString &str = lhs->as<ValueType::StringClass>();
                        if (targetType == ValueType::Int)
                        {
                            try
                            {
                                return IntPool::getInstance().createInt(
                                    static_cast<ValueType::IntClass>(std::stoll(str.toBasicString())));
                            }
                            catch (std::exception &e)
                            {
                                return ExprResult::error(
                                    genTypeError(FString(std::format("Cannot cast type `{}` to `{}`, bad int string {}",
                                                                     prettyType(lhs).toBasicString(),
                                                                     prettyType(rhs).toBasicString(),
                                                                     str.toBasicString())),
                                                 bin->rexp,
                                                 ctx));
                            }
                        }
                        if (targetType == ValueType::Double)
                        {
                            try
                            {
                                return std::make_shared<Object>(std::stod(str.toBasicString()));
                            }
                            catch (std::exception &e)
                            {
                                return ExprResult::error(genTypeError(
                                    FString(std::format("Cannot cast type `{}` to `{}`, bad double string {}",
                                                        prettyType(lhs).toBasicString(),
                                                        prettyType(rhs).toBasicString(),
                                                        str.toBasicString())),
                                    bin->rexp,
                                    ctx));
                            }
                        }
                        if (targetType == ValueType::Bool)
                        {
                            if (str == u8"true") { return Object::getTrueInstance(); }
                            else if (str == u8"false") { return Object::getFalseInstance(); }
                            return ExprResult::error(
                                genTypeError(FString(std::format("Cannot cast type `{}` to `{}`, bad bool string {}",
                                                                 prettyType(lhs).toBasicString(),
                                                                 prettyType(rhs).toBasicString(),
                                                                str.toBasicString())),
                                             bin->rexp,
                                             ctx));
                        }
                    }
                    else if (sourceType == ValueType::Bool)
                    {
                        if (targetType == ValueType::Int)
                        {
                            return IntPool::getInstance().createInt(static_cast<ValueType::IntClass>(lhs->as<ValueType::BoolClass>()));
                        }
                        if (targetType == ValueType::Double)
                        {
                            return std::make_shared<Object>(static_cast<ValueType::DoubleClass>(lhs->as<ValueType::BoolClass>()));
                        }
                    }

                    return ExprResult::error(genTypeError(FString(std::format("Cannot cast type `{}` to `{}`",
                                                                              prettyType(lhs).toBasicString(),
                                                                              prettyType(rhs).toBasicString())),
                                                          bin->rexp,
                                                          ctx));
                });
            }

            case Operator::BitAnd: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() & rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(bit_and(*lhs, *rhs));
                });
            }

            case Operator::BitOr: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() | rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(bit_or(*lhs, *rhs));
                });
            }

            case Operator::BitXor: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() ^ rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(bit_xor(*lhs, *rhs));
                });
            }

            case Operator::ShiftLeft: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() << rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(shift_left(*lhs, *rhs));
                });
            }
            case Operator::ShiftRight: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));

                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() {
                    if (lhs->is<ValueType::IntClass>() && rhs->is<ValueType::IntClass>())
                    {
                        ValueType::IntClass result = lhs->as<ValueType::IntClass>() >> rhs->as<ValueType::IntClass>();
                        return IntPool::getInstance().createInt(result);
                    }
                    return std::make_shared<Object>(shift_right(*lhs, *rhs));
                });
            }

            case Operator::Assign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                lv.set(rhs);
                return rhs;
            }

            case Operator::And: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                if (lhs->is<bool>() && !isBoolObjectTruthy(lhs)) { return Object::getFalseInstance(); }
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs && *rhs); });
            }

            case Operator::Or: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                if (lhs->is<bool>() && isBoolObjectTruthy(lhs)) { return Object::getTrueInstance(); }
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs || *rhs); });
            }

            case Operator::Equal: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs == *rhs); });
            }

            case Operator::NotEqual: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs != *rhs); });
            }

            case Operator::Less: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs < *rhs); });
            }

            case Operator::LessEqual: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs <= *rhs); });
            }

            case Operator::Greater: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs > *rhs); });
            }

            case Operator::GreaterEqual: {
                ObjectPtr lhs = check_unwrap(eval(lexp, ctx));
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                return tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs >= *rhs); });
            }

            case Operator::PlusAssign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                const ObjectPtr &lhs = lv.get();
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                const ObjectPtr &result = check_unwrap(
                    tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs + *rhs); }));
                lv.set(result);
                return rhs;
            }

            case Operator::MinusAssign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                const ObjectPtr &lhs = lv.get();
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                const ObjectPtr &result = check_unwrap(
                    tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs - *rhs); }));
                lv.set(result);
                return rhs;
            }

            case Operator::AsteriskAssign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                const ObjectPtr &lhs = lv.get();
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                const ObjectPtr &result = check_unwrap(
                    tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs * *rhs); }));
                lv.set(result);
                return rhs;
            }

            case Operator::SlashAssign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                const ObjectPtr &lhs = lv.get();
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                const ObjectPtr &result = check_unwrap(
                    tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs / *rhs); }));
                lv.set(result);
                return rhs;
            }

            case Operator::PercentAssign: {
                LvObject lv = check_unwrap_lv(evalLv(lexp, ctx));
                const ObjectPtr &lhs = lv.get();
                ObjectPtr rhs = check_unwrap(eval(rexp, ctx));
                const ObjectPtr &result = check_unwrap(
                    tryInvokeOverloadFn(lhs, rhs, [lhs, rhs]() { return std::make_shared<Object>(*lhs % *rhs); }));
                lv.set(result);
                return rhs;
            }

            default:
                throw EvaluatorError(u8"UnsupportedOp",
                                     std::format("Unsupport operator '{}' for binary", magic_enum::enum_name(op)),
                                     bin);
        }
    }
}; // namespace Fig