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

#ifndef x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_ELEGANTCONFIGURATIONS_HPP_
#define x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_ELEGANTCONFIGURATIONS_HPP_

#include <Configurations/BaseConfiguration.hpp>
#include <Configurations/ConfigurationsNames.hpp>
#include <memory>

namespace x::Configurations {

/**
 * Define all ELEGANT related configuration parameters
 */
class ElegantConfigurations : public BaseConfiguration {

  public:
    ElegantConfigurations() : BaseConfiguration(){};

    ElegantConfigurations(const std::string& name, const std::string& description) : BaseConfiguration(name, description){};

    /**
     * @brief Accelerate java UDFs.
     */
    BoolOption accelerateJavaUDFs = {ACCELERATE_JAVA_UDFS, false, "Accelerate java UDFs."};

    /**
     * @brief ELEGANT external planner service URL. Example: https://localhost:8081/plan
     */
    StringOption plannerServiceURL = {PLANNER_SERVICE_URL, "Complete URL for connecting with the ELEGANT planner."};

    /**
     * @brief ELEGANT external planner service URL. Example: https://localhost:8081/plan
     */
    StringOption accelerationServiceURL = {ACCELERATION_SERVICE_URL, "Complete URL for connecting with the ELEGANT planner."};

  private:
    std::vector<Configurations::BaseOption*> getOptions() override { return {&accelerateJavaUDFs}; }
};
}// namespace x::Configurations
#endif// x_CORE_INCLUDE_CONFIGURATIONS_COORDINATOR_ELEGANTCONFIGURATIONS_HPP_
