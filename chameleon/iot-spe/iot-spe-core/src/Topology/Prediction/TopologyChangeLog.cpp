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
#include <Topology/Prediction/Edge.hpp>
#include <Topology/Prediction/TopologyChangeLog.hpp>
#include <Topology/Prediction/TopologyDelta.hpp>
#include <Util/Logger/Logger.hpp>

namespace x::Experimental::TopologyPrediction {

void TopologyChangeLog::updateChangelog(const std::unordered_map<TopologyNodeId, std::vector<TopologyNodeId>>& newMap,
                                        std::unordered_map<TopologyNodeId, std::vector<TopologyNodeId>>& additionTarget,
                                        std::unordered_map<TopologyNodeId, std::vector<TopologyNodeId>>& toSubtract) {
    //iterate over the parent ids and corresponding lists of children
    for (const auto& [parentToAdd, childrenToAdd] : newMap) {
        //get the list of children to which new children will be added
        auto& additionTargetChildren = additionTarget[parentToAdd];
        //get the children which are not to be added even if they appead in the list of children to add
        auto& childrenToSubtract = toSubtract[parentToAdd];
        for (const auto childToAdd : childrenToAdd) {
            auto childToSubtract = std::find(childrenToSubtract.begin(), childrenToSubtract.end(), childToAdd);
            if (childToSubtract != childrenToSubtract.end()) {
                //if the child that is to be added, is also subtracted, remove it from the subtraction list and do not add it to the addition target
                childrenToSubtract.erase(childToSubtract);
                continue;
            }

            //if compiled with debug configuration, check for duplicates
#ifndef NDEBUG
            //if the edge to be added is also added as part of another changelog, this means our data is corrupted
            if (std::find(additionTargetChildren.begin(), additionTargetChildren.end(), childToAdd)
                != additionTargetChildren.end()) {
                x_ERROR("Duplicate edge {}->{} found in topology changelog", childToAdd, parentToAdd);
            }
#endif
            additionTargetChildren.push_back(childToAdd);
        }
        //remove empty vectors in target
        if (additionTargetChildren.empty()) {
            additionTarget.erase(parentToAdd);
        }
        if (childrenToSubtract.empty()) {
            toSubtract.erase(parentToAdd);
        }
    }
}

void TopologyChangeLog::add(const TopologyChangeLog& newChangeLog) {
    /* add added edges from the addedChangelog to the list of added edges at this object unless they already exist in the list of
     * removed edges at this object. In case the added edge exists in the list of removed edges at this object, remove it from the list.
     */
    updateChangelog(newChangeLog.addedLinks, this->addedLinks, this->removedLinks);
    /* add removed edges from the addedChangelog to the list of removed edges at this object unless they already exist in the list of
     * added edges at this object. In case the removed edge exists in the list of added edges at this object, remove it from the list.
     */
    updateChangelog(newChangeLog.removedLinks, this->removedLinks, this->addedLinks);
}

std::vector<TopologyNodeId> TopologyChangeLog::getAddedChildren(TopologyNodeId nodeId) const {
    auto it = addedLinks.find(nodeId);
    if (it != addedLinks.end()) {
        return it->second;
    }
    return {};
}

std::vector<TopologyNodeId> TopologyChangeLog::getRemovedChildren(TopologyNodeId nodeId) const {
    auto it = removedLinks.find(nodeId);
    if (it != removedLinks.end()) {
        return it->second;
    }
    return {};
}

void TopologyChangeLog::update(const TopologyDelta& newDelta) {
    for (auto addedEdge : newDelta.getAdded()) {
        addedLinks[addedEdge.downstreamTopologyNode].push_back(addedEdge.upstreamTopologyNode);
    }
    for (auto removedEdge : newDelta.getRemoved()) {
        removedLinks[removedEdge.downstreamTopologyNode].push_back(removedEdge.upstreamTopologyNode);
    }
}

void TopologyChangeLog::erase(const TopologyDelta& delta) {
    removeLinksFromMap(addedLinks, delta.getAdded());
    removeLinksFromMap(removedLinks, delta.getRemoved());
}

void TopologyChangeLog::removeLinksFromMap(std::unordered_map<TopologyNodeId, std::vector<TopologyNodeId>>& map,
                                           const std::vector<Edge>& edges) {
    for (auto edge : edges) {
        auto& children = map[edge.downstreamTopologyNode];
        auto iterator = std::find(children.begin(), children.end(), edge.upstreamTopologyNode);
        if (iterator != children.end()) {
            children.erase(iterator);
        } else if (!children.empty()) {
            x_THROW_RUNTIME_ERROR("Trying to remove edge " + edge.toString() + " which does not exist in the changelog");
        }
        if (children.empty()) {
            map.erase(edge.downstreamTopologyNode);
        }
    }
}

bool TopologyChangeLog::empty() { return addedLinks.empty() && removedLinks.empty(); }
}// namespace x::Experimental::TopologyPrediction