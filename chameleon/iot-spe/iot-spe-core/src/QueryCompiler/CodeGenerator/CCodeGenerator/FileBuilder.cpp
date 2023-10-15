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

#include <string>

#include <QueryCompiler/CodeGenerator/CCodeGenerator/Declarations/FunctionDeclaration.hpp>
#include <QueryCompiler/CodeGenerator/CCodeGenerator/FileBuilder.hpp>

namespace x::QueryCompilation {

FileBuilder FileBuilder::create(const std::string&) {
    FileBuilder builder;

    builder.declations << "#include <API/Schema.hpp>\n"
                          "#include <Operators/LogicalOperators/InferModelOperatorHandler.hpp>\n"
                          "#include <Common/ExecutableType/Array.hpp>\n"
                          "#include <QueryCompiler/Operators/PhysicalOperators/CEP/CEPOperatorHandler/CEPOperatorHandler.hpp>\n";
#ifdef TFDEF
    builder.declations << "#include <QueryCompiler/CodeGenerator/CCodeGenerator/TensorflowAdapter.hpp>\n";
#endif
    builder.declations
        << "#include <cstdint>\n"
           "#include <string.h>\n"
           "#include <State/StateVariable.hpp>\n"
           "#include <Windowing/LogicalWindowDefinition.hpp>\n"
           "#include <Windowing/WindowHandler/AggregationWindowHandler.hpp>\n"
           "#include <Windowing/WindowHandler/WindowOperatorHandler.hpp>\n"
           "#include <Windowing/WindowActions/BaseExecutableJoinAction.hpp>\n"
           "#include <Windowing/Runtime/WindowManager.hpp>\n"
           "#include <Windowing/Runtime/WindowSliceStore.hpp>\n"
           "#include <Runtime/TupleBuffer.hpp>\n"
           "#include <Runtime/ExecutionResult.hpp>\n"
           "#include <Runtime/QueryTerminationType.hpp>\n"
           "#include <Runtime/WorkerContext.hpp>\n"
           "#include <Runtime/Execution/PipelineExecutionContext.hpp>\n"
           "#include <Runtime/Execution/ExecutablePipelixtage.hpp>\n"
           "#include <Windowing/WindowHandler/BatchJoinOperatorHandler.hpp>\n"
           "#include <Windowing/WindowHandler/JoinOperatorHandler.hpp>\n"
           "#include <Windowing/WindowPolicies/ExecutableOnTimeTriggerPolicy.hpp>\n"
           "#include <Windowing/WindowPolicies/ExecutableOnTimeTriggerPolicy.hpp>\n"
           "#include <Windowing/WindowPolicies/ExecutableOnWatermarkChangeTriggerPolicy.hpp>\n"
           "#include <Windowing/WindowHandler/BatchJoinHandler.hpp>\n"
           "#include <Windowing/WindowHandler/JoinHandler.hpp>\n"
           "#include <Windowing/WindowActions/ExecutablextedLoopJoinTriggerAction.hpp>\n"
           "#include <Windowing/Runtime/WindowedJoinSliceListStore.hpp>\n"
           "#include <Windowing/Runtime/WindowedJoinSliceListStore.hpp>\n"
           "#include <Util/Experimental/HashMap.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedSlice.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedGlobalSliceStoreAppendOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedSliceMergingOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedSlidingWindowSinkOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedThreadLocalPreAggregationOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedThreadLocalSliceStore.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSliceMergingOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSlidingWindowSinkOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedThreadLocalSliceStore.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedThreadLocalPreAggregationOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedGlobalSliceStoreAppendOperatorHandler.hpp>\n"
           "#include <Windowing/Experimental/KeyedTimeWindow/KeyedSliceStaging.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSlice.hpp>\n"
           "#include <Windowing/Experimental/NonKeyedTimeWindow/NonKeyedSliceStaging.hpp>\n"
           "#include <Windowing/Experimental/WindowProcessingTasks.hpp>\n"
           "#include <Windowing/Experimental/GlobalSliceStore.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableCountAggregation.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableSumAggregation.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableMinAggregation.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableMaxAggregation.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableAVGAggregation.hpp>\n"
           "#include <Windowing/WindowAggregations/ExecutableMedianAggregation.hpp>\n"
           "#include <Windowing/WindowActions/ExecutableSliceAggregationTriggerAction.hpp>\n"
           "#include <Windowing/WindowActions/ExecutableCompleteAggregationTriggerAction.hpp>\n"
           "using namespace x::QueryCompilation;"
        << std::endl;
    return builder;
}

FileBuilder& FileBuilder::addDeclaration(const DeclarationPtr& declaration) {
    auto const code = declaration->getCode();
    declations << code << ";";
    return *this;
}

CodeFile FileBuilder::build() {
    CodeFile file;
    file.code = declations.str();
    return file;
}
}// namespace x::QueryCompilation