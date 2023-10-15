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
#include <Plans/Query/QueryPlan.hpp>
#include <QueryCompiler/Exceptions/QueryCompilationException.hpp>
#include <QueryCompiler/Operators/OperatorPipeline.hpp>
#include <QueryCompiler/Operators/PhysicalOperators/PhysicalOperator.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <numeric>
#include <utility>
namespace x::QueryCompilation {

OperatorPipeline::OperatorPipeline(uint64_t pipelineId, Type pipelineType)
    : id(pipelineId), queryPlan(QueryPlan::create()), pipelineType(pipelineType) {}

OperatorPipelinePtr OperatorPipeline::create() {
    return std::make_shared<OperatorPipeline>(OperatorPipeline(Util::getNextPipelineId(), Type::OperatorPipelineType));
}

OperatorPipelinePtr OperatorPipeline::createSinkPipeline() {
    return std::make_shared<OperatorPipeline>(OperatorPipeline(Util::getNextPipelineId(), Type::SinkPipelineType));
}

OperatorPipelinePtr OperatorPipeline::createSourcePipeline() {
    return std::make_shared<OperatorPipeline>(OperatorPipeline(Util::getNextPipelineId(), Type::SourcePipelineType));
}

void OperatorPipeline::setType(Type pipelineType) { this->pipelineType = pipelineType; }

bool OperatorPipeline::isOperatorPipeline() const { return pipelineType == Type::OperatorPipelineType; }

bool OperatorPipeline::isSinkPipeline() const { return pipelineType == Type::SinkPipelineType; }

bool OperatorPipeline::isSourcePipeline() const { return pipelineType == Type::SourcePipelineType; }

void OperatorPipeline::addPredecessor(const OperatorPipelinePtr& pipeline) {
    pipeline->successorPipelix.emplace_back(shared_from_this());
    this->predecessorPipelix.emplace_back(pipeline);
}

void OperatorPipeline::addSuccessor(const OperatorPipelinePtr& pipeline) {
    if (pipeline) {
        pipeline->predecessorPipelix.emplace_back(weak_from_this());
        this->successorPipelix.emplace_back(pipeline);
    }
}

std::vector<OperatorPipelinePtr> OperatorPipeline::getPredecessors() const {
    std::vector<OperatorPipelinePtr> predecessors;
    for (const auto& predecessor : predecessorPipelix) {
        predecessors.emplace_back(predecessor.lock());
    }
    return predecessors;
}

bool OperatorPipeline::hasOperators() const { return !this->queryPlan->getRootOperators().empty(); }

void OperatorPipeline::clearPredecessors() {
    for (const auto& pre : predecessorPipelix) {
        if (auto prePipeline = pre.lock()) {
            prePipeline->removeSuccessor(shared_from_this());
        }
    }
    predecessorPipelix.clear();
}

void OperatorPipeline::removePredecessor(const OperatorPipelinePtr& pipeline) {
    for (auto iter = predecessorPipelix.begin(); iter != predecessorPipelix.end(); ++iter) {
        if (iter->lock().get() == pipeline.get()) {
            predecessorPipelix.erase(iter);
            return;
        }
    }
}
void OperatorPipeline::removeSuccessor(const OperatorPipelinePtr& pipeline) {
    for (auto iter = successorPipelix.begin(); iter != successorPipelix.end(); ++iter) {
        if (iter->get() == pipeline.get()) {
            successorPipelix.erase(iter);
            return;
        }
    }
}
void OperatorPipeline::clearSuccessors() {
    for (const auto& succ : successorPipelix) {
        succ->removePredecessor(shared_from_this());
    }
    successorPipelix.clear();
}

std::vector<OperatorPipelinePtr> const& OperatorPipeline::getSuccessors() const { return successorPipelix; }

void OperatorPipeline::prependOperator(OperatorNodePtr newRootOperator) {
    if (!this->isOperatorPipeline() && this->hasOperators()) {
        throw QueryCompilationException("Sink and Source pipelix can have more then one operator");
    }
    if (newRootOperator->hasProperty("LogicalOperatorId")) {
        operatorIds.push_back(std::any_cast<uint64_t>(newRootOperator->getProperty("LogicalOperatorId")));
    }
    this->queryPlan->appendOperatorAsNewRoot(std::move(newRootOperator));
}

uint64_t OperatorPipeline::getPipelineId() const { return id; }

QueryPlanPtr OperatorPipeline::getQueryPlan() { return queryPlan; }
const std::vector<uint64_t>& OperatorPipeline::getOperatorIds() const { return operatorIds; }

std::string OperatorPipeline::toString() const {
    auto successorsStr = std::accumulate(successorPipelix.begin(),
                                         successorPipelix.end(),
                                         std::string(),
                                         [](const std::string& result, const OperatorPipelinePtr& succPipeline) {
                                             auto succPipelineId = std::to_string(succPipeline->id);
                                             return result.empty() ? succPipelineId : result + ", " + succPipelineId;
                                         });

    auto predeccesorsStr =
        std::accumulate(predecessorPipelix.begin(),
                        predecessorPipelix.end(),
                        std::string(),
                        [](const std::string& result, const std::weak_ptr<OperatorPipeline>& predecessorPipeline) {
                            auto predecessorPipelineId = std::to_string(predecessorPipeline.lock()->id);
                            return result.empty() ? predecessorPipelineId : result + ", " + predecessorPipelineId;
                        });

    std::ostringstream oss;
    oss << "- Id: " << id << ", Type: " << magic_enum::enum_name(pipelineType) << ", Successors: " << successorsStr
        << ", Predecessors: " << predeccesorsStr << std::endl
        << "- Queryplan: " << queryPlan->toString();

    return oss.str();
}
}// namespace x::QueryCompilation