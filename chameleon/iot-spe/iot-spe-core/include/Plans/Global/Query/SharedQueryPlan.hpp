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

#ifndef x_CORE_INCLUDE_PLANS_GLOBAL_QUERY_SHAREDQUERYPLAN_HPP_
#define x_CORE_INCLUDE_PLANS_GLOBAL_QUERY_SHAREDQUERYPLAN_HPP_

#include <Common/Identifiers.hpp>
#include <Util/PlacementStrategy.hpp>
#include <Util/SharedQueryPlanStatus.hpp>
#include <memory>
#include <queue>
#include <set>
#include <vector>

namespace x {

class Node;
using NodePtr = std::shared_ptr<Node>;

class LogicalOperatorNode;
using LogicalOperatorNodePtr = std::shared_ptr<LogicalOperatorNode>;

class QueryPlan;
using QueryPlanPtr = std::shared_ptr<QueryPlan>;

class SharedQueryPlan;
using SharedQueryPlanPtr = std::shared_ptr<SharedQueryPlan>;

namespace Optimizer {

class MatchedOperatorPair;
using MatchedOperatorPairPtr = std::unique_ptr<MatchedOperatorPair>;

namespace Experimental {
class ChangeLog;
using ChangeLogPtr = std::unique_ptr<ChangeLog>;

class ChangeLogEntry;
using ChangeLogEntryPtr = std::shared_ptr<ChangeLogEntry>;
}// namespace Experimental
}// namespace Optimizer

using Timestamp = uint64_t;

struct RemovedEdge {
    TopologyNodeId downstreamNodeId;
    TopologyNodeId upstreamNodeId;
};

using ChangeLogEntries = std::vector<std::pair<Timestamp, Optimizer::Experimental::ChangeLogEntryPtr>>;

const std::string PINNED_NODE_ID = "PINNED_NODE_ID";

/**
 * @brief This class holds a query plan shared by multiple queryIdAndCatalogEntryMapping i.e. from its source nodes we can reach the sink nodes of all
 * the queryIdAndCatalogEntryMapping participating in the shared query plan. A Global Query Plan can consists of multiple Shared Query Plans.
 * Additionally, a query con share only one Shared Query Plan within a Global Query Plan.
 *
 * Example:
 *
 * Example Query Plan:
 *
 *               {Q1}             {Q2}
 *             (Sink1)           (Sink2)
 *                |                 |
 *             (Map1)            (Map1)
 *                |                 |
 *            (Filter1)         (Filter1)
 *               |                 |
 *         (Source(Car))      (Source(Car))
 *
 * Shared Query Plan:
 *
 * sharedQueryPlanId: 1
 * queryPlan:
 *             (Sink1)       (Sink2)
 *                \          /
 *                  \      /
 *                     |
 *                  (Map1)
 *                    |
 *                (Filter1)
 *                   |
 *             (Source(Car))
 *
 * sharedQueryPlanId:
 *
 * queryIdToSinkOperatorMap:  {Q1:Sink1,Q2:Sink2}
 * queryIds: [Q1, Q2]
 * sinkOperators: [Sink1, Sink2]
 * deployed: false
 * newMetaData: true
 *
 * In the above Shared query plan, we have two QueryPlans with Sink1 and sink2 as respective sink nodes.
 * A SharedQueryPlan consists of a unique:
 *
 * - sharedQueryPlanId : this id is equivalent to the Query Id assigned to the user queryId. Since, there can be more than one query that can be merged
 *                      together we generate a unique Shared Query Id that can be associated to more than one Query Ids.
 * - queryPlan : the query plan for the shared query plan
 * - queryIdToSinkOperatorMap: the mapping among the query and the sink operator
 * - queryIds : A vector of original Query Ids participating in the shared query plan
 * - sinkOperators: A vector of sink operators in the query plan.
 * - deployed : A boolean flag indicating if the query plan is deployed or not.
 * - newMetaData : A boolean flag indicating if the meta data is a newly created one (i.e. it was never deployed before).
 *
 */
class SharedQueryPlan {

  public:
    static SharedQueryPlanPtr create(const QueryPlanPtr& queryPlan);

    /**
     * @brief: Add all downstream operators of the query with input id starting from the matched operator
     * @param queryId : id of the input query
     * @param matchedOperatorPairs : the matched operator pairs
     */
    void addQuery(QueryId queryId, const std::vector<Optimizer::MatchedOperatorPairPtr>& matchedOperatorPairs);

    /**
     * @brief Remove a Query, the associated exclusive operators, and clear sink and query id vectors
     * @param queryId : the original query Id
     * @return true if successful
     */
    bool removeQuery(QueryId queryId);

    /**
     * @brief Mark all operators between (excluding) upstream and downstream operators for re-operator placement
     * @param upstreamOperatorIds: upstream operator ids
     * @param downstreamOperatorIds: downstream Operator ids
     */
    void performReOperatorPlacement(const std::set<uint64_t>& upstreamOperatorIds,
                                    const std::set<uint64_t>& downstreamOperatorIds);

    /**
     * @brief Method to update the placement information and state of the shared query plan operators
     * @param updatedOperators: operators with new mappings
     */
    void updateOperators(const std::set<LogicalOperatorNodePtr>& updatedOperators);

    /**
     * @brief Clear all MetaData information
     */
    void clear();

    /**
     * @brief Get the Query plan for deployment build from the information within this metadata
     * @return the query plan with all the interconnected logical operators
     */
    QueryPlanPtr getQueryPlan();

    /**
     * @brief Check if the metadata is empty
     * @return true if vector of queryIds is empty
     */
    bool isEmpty();

    /**
     * @brief Get the vector of sink operators sharing common upstream operators
     * @return the vector of Sink Operators
     */
    std::vector<LogicalOperatorNodePtr> getSinkOperators();

    /**
     * @brief Get the collection of registered query ids and their sink operators
     * @return map of registered query ids and their sink operators
     */
    std::map<QueryId, std::set<LogicalOperatorNodePtr>> getQueryIdToSinkOperatorMap();

    /**
     * @brief Get the shared query id
     * @return shared query id
     */
    [[nodiscard]] SharedQueryId getId() const;

    /**
     * @brief Get all query ids part of the SharedQueryPlan
     * @return vector of query ids
     */
    std::vector<QueryId> getQueryIds();

    /**
     * @brief Get the change log entries of the shared query plan change until the timestamp.
     * @param timestamp: the timestamp until the change log entries need to be retrieved
     * @return the change log entries with timestamp of their creation
     */
    ChangeLogEntries getChangeLogEntries(Timestamp timestamp);

    /**
     * @brief: update the timestamp till which the changes have been processed
     * @param timestamp: the new timestamp in micro seconds
     */
    void updateProcessedChangeLogTimestamp(Timestamp timestamp);

    /**
     * @brief Get the hash based signature for the shared query plan
     * @return collection of hash based signatures
     */
    std::map<size_t, std::set<std::string>> getHashBasedSignature();

    /**
     * Get the status of the shared query plan
     * @return Current status of the query plan
     */
    [[nodiscard]] SharedQueryPlanStatus getStatus() const;

    /**
     * Set the status of the shared query plan
     * @param newStatus : the status of the shared query plan
     */
    void setStatus(SharedQueryPlanStatus newStatus);

    /**
     * @brief Get the placement strategy for the shared query plan
     * @return placement strategy
     */
    [[nodiscard]] Optimizer::PlacementStrategy getPlacementStrategy() const;

  private:
    explicit SharedQueryPlan(const QueryPlanPtr& queryPlan);

    /**
     * @brief Recursively remove the operator and all its subsequent upstream operators. The function terminates upon encountering
     * an upstream operator that is connected to another downstream operator and returns it as output.
     * @param operatorToRemove : the operator to remove
     * @return last upstream operators that are not removed
     */
    std::set<LogicalOperatorNodePtr> removeOperator(const LogicalOperatorNodePtr& operatorToRemove);

    /**
     * @brief Update the hash based signatures with new values
     * @param hashValue: The hash value
     * @param stringSignature: The string signature
     */
    void updateHashBasedSignature(size_t hashValue, const std::string& stringSignature);

    SharedQueryId sharedQueryId;
    SharedQueryPlanStatus sharedQueryPlanStatus;
    QueryPlanPtr queryPlan;
    std::map<QueryId, std::set<LogicalOperatorNodePtr>> queryIdToSinkOperatorMap;
    std::vector<QueryId> queryIds;
    //FIXME: #2274 We have to figure out a way to change it once a query is removed
    std::map<size_t, std::set<std::string>> hashBasedSignatures;
    Optimizer::PlacementStrategy placementStrategy;
    Optimizer::Experimental::ChangeLogPtr changeLog;
};
}// namespace x

#endif// x_CORE_INCLUDE_PLANS_GLOBAL_QUERY_SHAREDQUERYPLAN_HPP_
