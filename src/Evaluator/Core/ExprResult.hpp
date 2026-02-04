#pragma once

#include <Core/fig_string.hpp>
#include <Evaluator/Value/value_forward.hpp>
#include <Evaluator/Value/LvObject.hpp>

#include <variant>
#include <cassert>

namespace Fig
{
    struct StatementResult;
    struct ExprResult
    {
        std::variant<LvObject, RvObject> result;
        enum class Flow
        {
            Normal,
            Error,
        } flow;

        ExprResult(ObjectPtr _result, Flow _flow = Flow::Normal) : result(_result), flow(_flow) {}
        ExprResult(const LvObject &_result, Flow _flow = Flow::Normal) : result(_result), flow(_flow) {}

        static ExprResult normal(ObjectPtr _result) { return ExprResult(_result); }
        static ExprResult normal(const LvObject &_result) { return ExprResult(_result); }

        static ExprResult error(ObjectPtr _result) { return ExprResult(_result, Flow::Error); }
        static ExprResult error(const LvObject &_result) { return ExprResult(_result, Flow::Error); }

        bool isNormal() const { return flow == Flow::Normal; }

        bool isError() const { return flow == Flow::Error; }

        bool isResultLv() const { return std::holds_alternative<LvObject>(result); }

        ObjectPtr &unwrap()
        {
            if (!isNormal()) { assert(false && "unwrap abnormal ExprResult!"); }
            return std::get<RvObject>(result);
        }

        const ObjectPtr &unwrap() const
        {
            if (!isNormal()) { assert(false && "unwrap abnormal ExprResult!"); }
            return std::get<RvObject>(result);
        }

        const LvObject &unwrap_lv() const { return std::get<LvObject>(result); }

        StatementResult toStatementResult() const;
    };
#define check_unwrap(expr)                                                                                             \
    ({                                                                                                                 \
        auto _r = (expr);                                                                                              \
        if (_r.isError()) return _r;                                                                                   \
        _r.unwrap();                                                                                                   \
    })
#define check_unwrap_lv(expr)                                                                                          \
    ({                                                                                                                 \
        auto _r = (expr);                                                                                              \
        if (_r.isError()) return _r;                                                                                   \
        _r.unwrap_lv();                                                                                                \
    })
#define check_unwrap_stres(expr)                                                                                       \
    ({                                                                                                                 \
        auto _r = (expr);                                                                                              \
        if (_r.isError()) return _r.toStatementResult();                                                               \
        _r.unwrap();                                                                                                   \
    })

}; // namespace Fig