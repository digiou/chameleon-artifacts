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

#ifndef x_CORE_INCLUDE_NODES_UTIL_DUMPCONTEXT_HPP_
#define x_CORE_INCLUDE_NODES_UTIL_DUMPCONTEXT_HPP_

#include <memory>
#include <string>
#include <vector>

namespace x {

class Node;
using NodePtr = std::shared_ptr<Node>;

class DumpHandler;
using DebugDumpHandlerPtr = std::shared_ptr<DumpHandler>;

class DumpContext;
using DumpContextPtr = std::shared_ptr<DumpContext>;

class QueryPlan;
using QueryPlanPtr = std::shared_ptr<QueryPlan>;

namespace QueryCompilation {
class PipelineQueryPlan;
using PipelineQueryPlanPtr = std::shared_ptr<PipelineQueryPlan>;
}// namespace QueryCompilation

/**
 * @brief The dump context is used to dump a node graph to multiple dump handler at the same time.
 * To this end, it manages the local registered dump handlers.
 */
class DumpContext {
  public:
    static DumpContextPtr create();
    static DumpContextPtr create(const std::string& contextIdentifier);
    explicit DumpContext(std::string contextIdentifier);
    /**
     * @brief Registers a new dump handler to the context.
     * @param debugDumpHandler
     */
    void registerDumpHandler(const DebugDumpHandlerPtr& debugDumpHandler);

    /**
     * @brief Dumps the passed node and its children on all registered dump handlers.
     * @param node
     */
    void dump(NodePtr const& node);

    /**
    * @brief Dumps the passed query plan on all registered dump handlers.
    * @param defix the scope of this plan.
    * @param queryPlan
    */
    void dump(std::string const& scope, QueryPlanPtr const& queryPlan);

    /**
    * @brief Dumps the passed pipeline query plan on all registered dump handlers.
    * @param defix the scope of this plan.
    * @param queryPlan
    */
    void dump(std::string const& scope, QueryCompilation::PipelineQueryPlanPtr const& queryPlan);

  private:
    std::string context;
    std::vector<DebugDumpHandlerPtr> dumpHandlers;
};

}// namespace x

#endif// x_CORE_INCLUDE_NODES_UTIL_DUMPCONTEXT_HPP_
