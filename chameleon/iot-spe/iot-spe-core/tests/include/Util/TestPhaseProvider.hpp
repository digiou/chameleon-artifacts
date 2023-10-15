/*
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        https://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef x_x_CORE_TESTS_INCLUDE_UTIL_TESTPHASEPROVIDER_HPP_
#define x_x_CORE_TESTS_INCLUDE_UTIL_TESTPHASEPROVIDER_HPP_

#include <QueryCompiler/Phases/DefaultPhaseFactory.hpp>
#include <QueryCompiler/QueryCompilationRequest.hpp>
#include <QueryCompiler/QueryCompiler.hpp>

namespace x::TestUtils {
class TestPhaseProvider : public QueryCompilation::Phases::DefaultPhaseFactory {
  public:
    QueryCompilation::LowerToExecutableQueryPlanPhasePtr
    createLowerToExecutableQueryPlanPhase(QueryCompilation::QueryCompilerOptionsPtr options, bool) override;
};
}// namespace x::TestUtils

#endif//x_x_CORE_TESTS_INCLUDE_UTIL_TESTPHASEPROVIDER_HPP_
