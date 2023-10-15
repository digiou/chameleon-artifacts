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

#ifndef x_CORE_INCLUDE_PLANS_UTILS_PLANIDGENERATOR_HPP_
#define x_CORE_INCLUDE_PLANS_UTILS_PLANIDGENERATOR_HPP_

#include <Common/Identifiers.hpp>

namespace x {

/**
 * @brief This class is responsible for generating identifiers for query plans
 */
class PlanIdGenerator {

  public:
    /**
     * @brief Returns the next free global query Id
     * @return global query id
     */
    static SharedQueryId getNextSharedQueryId();

    /**
     * @brief Returns the next free query Sub Plan Id
     * @return query sub plan id
     */
    static QuerySubPlanId getNextQuerySubPlanId();

    /**
     * @brief Returns the next free Query id
     * @return query id
     */
    static QueryId getNextQueryId();
};
}// namespace x
#endif// x_CORE_INCLUDE_PLANS_UTILS_PLANIDGENERATOR_HPP_
