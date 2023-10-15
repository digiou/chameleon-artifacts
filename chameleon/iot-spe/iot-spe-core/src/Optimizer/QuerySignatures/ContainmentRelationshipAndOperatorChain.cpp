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

#include <Optimizer/QuerySignatures/ContainmentRelationshipAndOperatorChain.hpp>

namespace x::Optimizer {

ContainmentRelationshipAndOperatorChainPtr
ContainmentRelationshipAndOperatorChain::create(ContainmentRelationship containmentRelationship,
                                                std::vector<LogicalOperatorNodePtr> containedOperatorChain) {
    return std::make_unique<ContainmentRelationshipAndOperatorChain>(
        ContainmentRelationshipAndOperatorChain(containmentRelationship, containedOperatorChain));
}

ContainmentRelationshipAndOperatorChain::ContainmentRelationshipAndOperatorChain(
    ContainmentRelationship containmentRelationship,
    std::vector<LogicalOperatorNodePtr> containedOperatorChain)
    : containmentRelationship(containmentRelationship), containedOperatorChain(containedOperatorChain) {}

}// namespace x::Optimizer