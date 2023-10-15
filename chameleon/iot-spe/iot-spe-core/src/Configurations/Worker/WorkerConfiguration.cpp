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

#include "Util/yaml/Yaml.hpp"
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/PhysicalSourceType.hpp>
#include <Configurations/ConfigurationOption.hpp>
#include <Configurations/Coordinator/CoordinatorConfiguration.hpp>
#include <Configurations/Worker/PhysicalSourceFactory.hpp>
#include <Configurations/Worker/WorkerConfiguration.hpp>
#include <Configurations/details/EnumOptionDetails.hpp>
#include <Util/Core.hpp>
#include <Util/Logger/Logger.hpp>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>

namespace x::Configurations {

template class EnumOption<x::Spatial::Experimental::SpatialType>;
template class EnumOption<x::QueryCompilation::QueryCompilerOptions::QueryCompiler>;
template class EnumOption<x::QueryCompilation::QueryCompilerOptions::CompilationStrategy>;
template class EnumOption<x::QueryCompilation::QueryCompilerOptions::PipeliningStrategy>;
template class EnumOption<x::QueryCompilation::QueryCompilerOptions::OutputBufferOptimizationLevel>;
template class EnumOption<x::QueryCompilation::QueryCompilerOptions::WindowingStrategy>;
template class EnumOption<x::Runtime::QueryExecutionMode>;
template class EnumOption<x::Spatial::Mobility::Experimental::LocationProviderType>;
template class EnumOption<x::Optimizer::QueryMergerRule>;
template class EnumOption<x::LogLevel>;
template class EnumOption<x::Optimizer::MemoryLayoutSelectionPhase::MemoryLayoutPolicy>;

}// namespace x::Configurations