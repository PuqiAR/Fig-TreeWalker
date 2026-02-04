#include <Evaluator/Core/ExprResult.hpp>
#include <Evaluator/Core/StatementResult.hpp>

namespace Fig
{
    StatementResult ExprResult::toStatementResult() const
    {
        if (isError())
        {
            if (isResultLv())
            {
                return StatementResult::errorFlow(std::get<LvObject>(result).get());
            }
            return StatementResult::errorFlow(std::get<RvObject>(result));
        }
        if (isResultLv()) { return StatementResult::normal(std::get<LvObject>(result).get()); }
        return StatementResult::normal(std::get<RvObject>(result));
    }
};