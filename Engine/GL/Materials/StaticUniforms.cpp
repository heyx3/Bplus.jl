#include "StaticUniforms.h"

using namespace Bplus;
using namespace Bplus::GL;
using namespace Bplus::GL::Uniforms;

/*
using BoolExprParams_Args = std::vector<StaticIntExpr>;
using BoolExprParams_ChildExprs = std::vector<StaticBoolExpr>;

namespace
{
}


bool StaticBoolExpr::Evaluate(const StaticUniformDefs& uniforms,
                              const StaticUniformValues& values) const
{
    switch (Type)
    {
        #define COMPARE_STATIC_BOOL_EXPR(enumName, op) \
            case StaticBoolExprTypes::enumName: { \
                const auto& uniformDef = uniforms.Definitions.find( \
                int64_t val1, val2; \
                if (std::holds_alternative<std::string>(Params)) { \
                    val1 = std::get<std::string>(Params); \
                } \
            } break;

        case StaticBoolExprTypes::Equal:
            BP_ASSERT(std::holds_alternative<BoolExprParams_Args>(Params),
                      "'Equal' op expects a pair of values to compare");
            
        break;
    }
}

*/