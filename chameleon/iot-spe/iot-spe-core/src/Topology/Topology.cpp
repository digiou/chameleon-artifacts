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

#include <Common/Identifiers.hpp>
#include <Nodes/Util/Iterators/BreadthFirstNodeIterator.hpp>
#include <Nodes/Util/Iterators/DepthFirstNodeIterator.hpp>
#include <Topology/Topology.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Experimental/SpatialType.hpp>
#include <Util/Logger/Logger.hpp>
#include <algorithm>
#include <deque>
#include <utility>

namespace x {

Topology::Topology() : rootNode(nullptr) {}

TopologyPtr Topology::create() { return std::shared_ptr<Topology>(new Topology()); }

bool Topology::addNewTopologyNodeAsChild(const TopologyNodePtr& parent, const TopologyNodePtr& newNode) {
    std::unique_lock lock(topologyLock);
    uint64_t newNodeId = newNode->getId();
    if (indexOnNodeIds.find(newNodeId) == indexOnNodeIds.end()) {
        x_INFO("Topology: Adding New Node {} to the catalog of nodes.", newNode->toString());
        indexOnNodeIds[newNodeId] = newNode;
    }
    x_INFO("Topology: Adding Node {} as child to the node {}", newNode->toString(), parent->toString());
    return parent->addChild(newNode);
}

bool Topology::removePhysicalNode(const TopologyNodePtr& nodeToRemove) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Removing Node {}", nodeToRemove->toString());

    uint64_t idOfNodeToRemove = nodeToRemove->getId();
    if (indexOnNodeIds.find(idOfNodeToRemove) == indexOnNodeIds.end()) {
        x_WARNING("Topology: The physical node {} doesn't exists in the system.", idOfNodeToRemove);
        return true;
    }

    if (!rootNode) {
        x_WARNING("Topology: No root node exists in the topology");
        return false;
    }

    if (rootNode->getId() == idOfNodeToRemove) {
        x_WARNING("Topology: Attempt to remove the root node. Removing root node is not allowed.");
        return false;
    }

    nodeToRemove->removeAllParent();
    nodeToRemove->removeChildren();
    indexOnNodeIds.erase(idOfNodeToRemove);
    x_DEBUG("Topology: Successfully removed the node.");
    return true;
}

std::vector<TopologyNodePtr> Topology::findPathBetween(const std::vector<TopologyNodePtr>& sourceNodes,
                                                       const std::vector<TopologyNodePtr>& destinationNodes) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Finding path between set of start and destination nodes");
    std::vector<TopologyNodePtr> startNodesOfGraph;
    for (const auto& sourceNode : sourceNodes) {
        x_TRACE("Topology: Finding all paths between the source node {} and a set of destination nodes",
                  sourceNode->toString());
        std::map<uint64_t, TopologyNodePtr> mapOfUniqueNodes;
        TopologyNodePtr startNodeOfGraph = find(sourceNode, destinationNodes, mapOfUniqueNodes);
        x_TRACE("Topology: Validate if all destination nodes reachable");
        for (const auto& destinationNode : destinationNodes) {
            if (mapOfUniqueNodes.find(destinationNode->getId()) == mapOfUniqueNodes.end()) {
                x_ERROR("Topology: Unable to find path between source node {} and destination node{}",
                          sourceNode->toString(),
                          destinationNode->toString());
                return {};
            }
        }
        x_TRACE("Topology: Push the start node of the graph into a collection of start nodes");
        startNodesOfGraph.push_back(startNodeOfGraph);
    }
    x_TRACE("Topology: Merge all found sub-graphs together to create a single sub graph and return the set of start nodes of "
              "the merged graph.");
    return mergeSubGraphs(startNodesOfGraph);
}

std::vector<TopologyNodePtr> Topology::mergeSubGraphs(const std::vector<TopologyNodePtr>& startNodes) {
    x_INFO("Topology: Merge {} sub-graphs to create a single sub-graph", startNodes.size());

    x_DEBUG("Topology: Compute a map storing number of times a node occurred in different sub-graphs");
    std::map<uint64_t, uint32_t> nodeCountMap;
    for (const auto& startNode : startNodes) {
        x_TRACE("Topology: Fetch all ancestor nodes of the given start node");
        const std::vector<NodePtr> family = startNode->getAndFlattenAllAncestors();
        x_TRACE(
            "Topology: Iterate over the family members and add the information in the node count map about the node occurrence");
        for (const auto& member : family) {
            uint64_t nodeId = member->as<TopologyNode>()->getId();
            if (nodeCountMap.find(nodeId) != nodeCountMap.end()) {
                x_TRACE("Topology: Family member already present increment the occurrence count");
                uint32_t count = nodeCountMap[nodeId];
                nodeCountMap[nodeId] = count + 1;
            } else {
                x_TRACE("Topology: Add family member into the node count map");
                nodeCountMap[nodeId] = 1;
            }
        }
    }

    x_DEBUG("Topology: Iterate over each sub-graph and compute a single merged sub-graph");
    std::vector<TopologyNodePtr> result;
    std::map<uint64_t, TopologyNodePtr> mergedGraphNodeMap;
    for (const auto& startNode : startNodes) {
        x_DEBUG(
            "Topology: Check if the node already present in the new merged graph and add a copy of the node if not present");
        if (mergedGraphNodeMap.find(startNode->getId()) == mergedGraphNodeMap.end()) {
            TopologyNodePtr copyOfStartNode = startNode->copy();
            x_DEBUG("Topology: Add the start node to the list of start nodes for the new merged graph");
            result.push_back(copyOfStartNode);
            mergedGraphNodeMap[startNode->getId()] = copyOfStartNode;
        }
        x_DEBUG("Topology: Iterate over the ancestry of the start node and add the eligible nodes to new merged graph");
        TopologyNodePtr childNode = startNode;
        while (childNode) {
            x_TRACE("Topology: Get all parents of the child node to select the next parent to traverse.");
            std::vector<NodePtr> parents = childNode->getParents();
            TopologyNodePtr selectedParent;
            if (parents.size() > 1) {
                x_TRACE("Topology: Found more than one parent for the node");
                x_TRACE("Topology: Iterate over all parents and select the parent node that has the max cost value.");
                double maxCost = 0;
                for (auto& parent : parents) {

                    x_TRACE("Topology: Get all ancestor of the node and aggregate their occurrence counts.");
                    std::vector<NodePtr> family = parent->getAndFlattenAllAncestors();
                    double occurrenceCount = 0;
                    for (auto& member : family) {
                        occurrenceCount = occurrenceCount + nodeCountMap[member->as<TopologyNode>()->getId()];
                    }

                    x_TRACE("Topology: Compute cost by multiplying aggregate occurrence count with base multiplier and "
                              "dividing the result by the number of nodes in the path.");
                    double cost = (occurrenceCount * BASE_MULTIPLIER) / family.size();

                    if (cost > maxCost) {
                        x_TRACE("Topology: The cost is more than max cost found till now.");
                        if (selectedParent) {
                            x_TRACE("Topology: Remove the previously selected parent as parent to the current child node.");
                            childNode->removeParent(selectedParent);
                        }
                        maxCost = cost;
                        x_TRACE("Topology: Mark this parent as next selected parent.");
                        selectedParent = parent->as<TopologyNode>();
                    } else {
                        x_TRACE("Topology: The cost is less than max cost found till now.");
                        if (selectedParent) {
                            x_TRACE("Topology: Remove this parent as parent to the current child node.");
                            childNode->removeParent(parent);
                        }
                    }
                }
            } else if (parents.size() == 1) {
                x_TRACE("Topology: Found only one parent for the current child node");
                x_TRACE("Topology: Set the parent as next parent to traverse");
                selectedParent = parents[0]->as<TopologyNode>();
            }

            if (selectedParent) {
                x_TRACE("Topology: Found a new next parent to traverse");
                if (mergedGraphNodeMap.find(selectedParent->getId()) != mergedGraphNodeMap.end()) {
                    x_TRACE("Topology: New next parent is already present in the new merged graph.");
                    TopologyNodePtr equivalentParentNode = mergedGraphNodeMap[selectedParent->getId()];
                    TopologyNodePtr equivalentChildNode = mergedGraphNodeMap[childNode->getId()];
                    x_TRACE("Topology: Add the existing node, with id same as new next parent, as parent to the existing node "
                              "with id same as current child node");
                    equivalentChildNode->addParent(equivalentParentNode);
                } else {
                    x_TRACE("Topology: New next parent is not present in the new merged graph.");
                    x_TRACE(
                        "Topology: Add copy of new next parent as parent to the existing child node in the new merged graph.");
                    TopologyNodePtr copyOfSelectedParent = selectedParent->copy();
                    TopologyNodePtr equivalentChildNode = mergedGraphNodeMap[childNode->getId()];
                    equivalentChildNode->addParent(copyOfSelectedParent);
                    mergedGraphNodeMap[selectedParent->getId()] = copyOfSelectedParent;
                }
            }
            x_TRACE("Topology: Assign new selected parent as next child node to traverse.");
            childNode = selectedParent;
        }
    }

    return result;
}

std::optional<TopologyNodePtr> Topology::findAllPathBetween(const TopologyNodePtr& startNode,
                                                            const TopologyNodePtr& destinationNode) {
    std::unique_lock lock(topologyLock);
    x_DEBUG("Topology: Finding path between {} and {}", startNode->toString(), destinationNode->toString());

    std::optional<TopologyNodePtr> result;
    std::vector<TopologyNodePtr> searchedNodes{destinationNode};
    std::map<uint64_t, TopologyNodePtr> mapOfUniqueNodes;
    TopologyNodePtr found = find(startNode, searchedNodes, mapOfUniqueNodes);
    if (found) {
        x_DEBUG("Topology: Found path between {} and {}", startNode->toString(), destinationNode->toString());
        return found;
    }
    x_WARNING("Topology: Unable to find path between {} and {}", startNode->toString(), destinationNode->toString());
    return result;
}

TopologyNodePtr Topology::find(TopologyNodePtr testNode,
                               std::vector<TopologyNodePtr> searchedNodes,
                               std::map<uint64_t, TopologyNodePtr>& uniqueNodes) {

    x_TRACE("Topology: check if test node is one of the searched node");
    auto found = std::find_if(searchedNodes.begin(), searchedNodes.end(), [&](const TopologyNodePtr& searchedNode) {
        return searchedNode->getId() == testNode->getId();
    });

    if (found != searchedNodes.end()) {
        x_DEBUG("Topology: found the destination node");
        if (uniqueNodes.find(testNode->getId()) == uniqueNodes.end()) {
            x_TRACE("Topology: Insert the information about the test node in the unique node map");
            const TopologyNodePtr copyOfTestNode = testNode->copy();
            uniqueNodes[testNode->getId()] = copyOfTestNode;
        }
        x_TRACE("Topology: Insert the information about the test node in the unique node map");
        return uniqueNodes[testNode->getId()];
    }

    std::vector<NodePtr> parents = testNode->getParents();
    std::vector<NodePtr> updatedParents;
    //filters out all parents that are marked for maintenance, as these should be ignored during path finding
    for (auto& parent : parents) {
        if (!parent->as<TopologyNode>()->isUnderMaintenance()) {
            updatedParents.push_back(parent);
        }
    }

    if (updatedParents.empty()) {
        x_WARNING("Topology: reached end of the tree but destination node not found.");
        return nullptr;
    }

    TopologyNodePtr foundNode = nullptr;
    for (auto& parent : updatedParents) {
        TopologyNodePtr foundInParent = find(parent->as<TopologyNode>(), searchedNodes, uniqueNodes);
        if (foundInParent) {
            x_TRACE("Topology: found the destination node as the parent of the physical node.");
            if (!foundNode) {
                //TODO: SZ I don't understand how we can end up here
                if (uniqueNodes.find(testNode->getId()) == uniqueNodes.end()) {
                    const TopologyNodePtr copyOfTestNode = testNode->copy();
                    uniqueNodes[testNode->getId()] = copyOfTestNode;
                }
                foundNode = uniqueNodes[testNode->getId()];
            }
            x_TRACE("Topology: Adding found node as parent to the copy of testNode.");
            foundNode->addParent(foundInParent);
        }
    }
    return foundNode;
}

std::string Topology::toString() {
    std::unique_lock lock(topologyLock);

    if (!rootNode) {
        x_WARNING("Topology: No root node found");
        return "";
    }

    std::stringstream topologyInfo;
    topologyInfo << std::endl;

    // store pair of TopologyNodePtr and its depth in when printed
    std::deque<std::pair<TopologyNodePtr, uint64_t>> parentToPrint{std::make_pair(rootNode, 0)};

    // indent offset
    int indent = 2;

    // perform dfs traverse
    while (!parentToPrint.empty()) {
        std::pair<TopologyNodePtr, uint64_t> nodeToPrint = parentToPrint.front();
        parentToPrint.pop_front();
        for (std::size_t i = 0; i < indent * nodeToPrint.second; i++) {
            if (i % indent == 0) {
                topologyInfo << '|';
            } else {
                if (i >= indent * nodeToPrint.second - 1) {
                    topologyInfo << std::string(indent, '-');
                } else {
                    topologyInfo << std::string(indent, ' ');
                }
            }
        }
        topologyInfo << nodeToPrint.first->toString() << std::endl;

        for (const auto& child : nodeToPrint.first->getChildren()) {
            parentToPrint.emplace_front(child->as<TopologyNode>(), nodeToPrint.second + 1);
        }
    }
    return topologyInfo.str();
}

void Topology::print() { x_DEBUG("Topology print:{}", toString()); }

bool Topology::nodeWithWorkerIdExists(TopologyNodeId workerId) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Finding if a physical node with worker id {} exists.", workerId);
    if (!rootNode) {
        x_WARNING("Topology: Root node not found.");
        return false;
    }
    x_TRACE("Topology: Traversing the topology using BFS.");
    BreadthFirstNodeIterator bfsIterator(rootNode);
    for (auto itr = bfsIterator.begin(); itr != x::BreadthFirstNodeIterator::end(); ++itr) {
        auto physicalNode = (*itr)->as<TopologyNode>();
        if (physicalNode->getId() == workerId) {
            x_TRACE("Topology: Found a physical node {} with worker id {}", physicalNode->toString(), workerId);
            return true;
        }
    }
    return false;
}

TopologyNodePtr Topology::getRoot() {
    std::unique_lock lock(topologyLock);
    return rootNode;
}

TopologyNodePtr Topology::findNodeWithId(uint64_t nodeId) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Finding a physical node with id {}", nodeId);
    if (indexOnNodeIds.find(nodeId) != indexOnNodeIds.end()) {
        x_DEBUG("Topology: Found a physical node with id {}", nodeId);
        return indexOnNodeIds[nodeId];
    }
    x_WARNING("Topology: Unable to find a physical node with id {}", nodeId);
    return nullptr;
}

void Topology::setAsRoot(const TopologyNodePtr& physicalNode) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Setting physical node {} as root to the topology.", physicalNode->toString());
    indexOnNodeIds[physicalNode->getId()] = physicalNode;
    rootNode = physicalNode;
}

bool Topology::removeNodeAsChild(const TopologyNodePtr& parentNode, const TopologyNodePtr& childNode) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Removing node {} as child to the node {}", childNode->toString(), parentNode->toString());
    return parentNode->remove(childNode);
}

bool Topology::reduceResources(uint64_t nodeId, uint16_t amountToReduce) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Reduce {} resources from node with id {}", amountToReduce, nodeId);
    if (indexOnNodeIds.find(nodeId) == indexOnNodeIds.end()) {
        x_WARNING("Topology: Unable to find node with id {}", nodeId);
        return false;
    }
    indexOnNodeIds[nodeId]->reduceResources(amountToReduce);
    return true;
}

bool Topology::increaseResources(uint64_t nodeId, uint16_t amountToIncrease) {
    std::unique_lock lock(topologyLock);
    x_INFO("Topology: Increase {} resources from node with id {}", amountToIncrease, nodeId);
    if (indexOnNodeIds.find(nodeId) == indexOnNodeIds.end()) {
        x_WARNING("Topology: Unable to find node with id {}", nodeId);
        return false;
    }
    indexOnNodeIds[nodeId]->increaseResources(amountToIncrease);
    return true;
}

TopologyNodePtr Topology::findCommonAncestor(std::vector<TopologyNodePtr> topologyNodes) {

    x_DEBUG("Topology: find common node for a set of topology nodes.");

    if (topologyNodes.empty()) {
        x_ERROR("Topology: Input topology node list was empty.");
        return nullptr;
    }

    //Check if one of the input node is a root node of the topology
    auto found = std::find_if(topologyNodes.begin(), topologyNodes.end(), [&](const TopologyNodePtr& topologyNode) {
        return rootNode->getId() == topologyNode->getId();
    });

    // If a root node found in the input nodes then return the root topology node
    if (found != topologyNodes.end()) {
        return *found;
    }

    x_DEBUG("Topology: Selecting a start node to identify the common ancestor.");
    TopologyNodePtr startNode = topologyNodes[0];
    bool foundAncestor = false;
    TopologyNodePtr resultAncestor;
    x_TRACE("Topology: Adding selected node to the deque for further processing.");
    std::deque<NodePtr> nodesToProcess{startNode};
    while (!nodesToProcess.empty()) {
        TopologyNodePtr candidateNode = nodesToProcess.front()->as<TopologyNode>();
        nodesToProcess.pop_front();
        x_TRACE(
            "Topology: Check if the children topology node of the node under consideration contains all input topology nodes.");
        std::vector<NodePtr> children = candidateNode->getAndFlattenAllChildren(false);
        for (auto& nodeToLook : topologyNodes) {
            auto found = std::find_if(children.begin(), children.end(), [&](const NodePtr& child) {
                return nodeToLook->getId() == child->as<TopologyNode>()->getId();
            });

            if (found == children.end()) {
                x_TRACE("Topology: Unable to find the input topology node as child of the node under consideration.");
                foundAncestor = false;
                break;
            }
            foundAncestor = true;
        }

        if (foundAncestor) {
            x_TRACE("Topology: The node under consideration contains all input node as its children.");
            return candidateNode;
        }

        x_TRACE("Topology: Add parent of the the node under consideration to the deque for further processing.");
        auto parents = candidateNode->getParents();
        for (const auto& parent : parents) {
            if (!parent->as<TopologyNode>()->isUnderMaintenance())
                nodesToProcess.push_back(parent);
        }
    }

    x_ERROR("Topology: Unable to find a common ancestor topology node for the input topology nodes.");
    return nullptr;
}

TopologyNodePtr Topology::findCommonChild(std::vector<TopologyNodePtr> topologyNodes) {
    x_INFO("Topology: find common child node for a set of parent topology nodes.");

    if (topologyNodes.empty()) {
        x_WARNING("Topology: Input topology node list was empty.");
        return nullptr;
    }

    x_DEBUG("Topology: Selecting a start node to identify the common child.");
    TopologyNodePtr startNode = topologyNodes[0];
    bool foundAncestor = false;
    TopologyNodePtr resultAncestor;
    x_TRACE("Topology: Adding selected node to the deque for further processing.");
    std::deque<NodePtr> nodesToProcess{startNode};
    while (!nodesToProcess.empty()) {
        TopologyNodePtr candidateNode = nodesToProcess.front()->as<TopologyNode>();
        nodesToProcess.pop_front();
        x_TRACE(
            "Topology: Check if the parent topology node of the node under consideration contains all input topology nodes.");
        std::vector<NodePtr> parents = candidateNode->getAndFlattenAllAncestors();
        for (auto& nodeToLook : topologyNodes) {
            auto found = std::find_if(parents.begin(), parents.end(), [&](const NodePtr& parent) {
                return nodeToLook->getId() == parent->as<TopologyNode>()->getId();
            });

            if (found == parents.end()) {
                x_TRACE("Topology: Unable to find the input topology node as parent of the node under consideration.");
                foundAncestor = false;
                break;
            }
            foundAncestor = true;
        }

        if (foundAncestor) {
            x_TRACE("Topology: The node under consideration contains all input node as its parent.");
            return candidateNode;
        }

        x_TRACE("Topology: Add children of the the node under consideration to the deque for further processing.");
        auto children = candidateNode->getChildren();
        for (const auto& child : children) {
            if (!child->as<TopologyNode>()->isUnderMaintenance()) {
                nodesToProcess.push_back(child);
            }
        }
    }
    x_WARNING("Topology: Unable to find a common child topology node for the input topology nodes.");
    return nullptr;
}

TopologyNodePtr Topology::findCommonNodeBetween(std::vector<TopologyNodePtr> childNodes,
                                                std::vector<TopologyNodePtr> parenNodes) {
    x_DEBUG("Topology: Find a common ancestor node for the input children nodes.");
    TopologyNodePtr commonAncestorForChildren = findCommonAncestor(std::move(childNodes));
    if (!commonAncestorForChildren) {
        x_WARNING("Topology: Unable to find a common ancestor node for the input child node.");
        return nullptr;
    }

    x_DEBUG("Topology: Find a common child node for the input parent nodes.");
    TopologyNodePtr commonChildForParents = findCommonChild(std::move(parenNodes));
    if (!commonChildForParents) {
        x_WARNING("Topology: Unable to find a common child node for the input parent nodes.");
        return nullptr;
    }

    if (commonChildForParents->getId() == commonAncestorForChildren->getId()) {
        x_DEBUG("Topology: Both common child and ancestor are same node. Returning as result.");
        return commonChildForParents;
    }
    if (commonChildForParents->containAsChild(commonAncestorForChildren)) {
        x_DEBUG("Topology: Returning the common children of the parent topology nodes");
        return commonChildForParents;
    } else if (!commonChildForParents->containAsParent(commonAncestorForChildren)) {
        x_WARNING("Topology: Common child is not connected to the common ancestor.");
        return nullptr;
    }
    x_DEBUG("Topology: Returning common ancestor as result.");
    return commonAncestorForChildren;
}

std::vector<TopologyNodePtr> Topology::findNodesBetween(const TopologyNodePtr& sourceNode,
                                                        const TopologyNodePtr& destinationNode) {

    x_DEBUG("Topology: Find topology nodes between source and destination nodes.");
    if (sourceNode->getId() == destinationNode->getId()) {
        x_DEBUG("Topology: Both source and destination are same node.");
        return {sourceNode};
    }
    if (!sourceNode->containAsParent(destinationNode)) {
        x_WARNING("Topology: source node is not connected to the destination node.");
        return {};
    }

    std::vector<TopologyNodePtr> nodesBetween;
    x_DEBUG("Topology: iterate over parent of the source node and find path between its parent and destination nodes.");
    auto parents = sourceNode->getParents();
    for (const auto& sourceParent : parents) {
        std::vector<TopologyNodePtr> foundBetweenNodes = findNodesBetween(sourceParent->as<TopologyNode>(), destinationNode);
        if (!foundBetweenNodes.empty()) {
            x_TRACE("Topology: found a path between source nodes parent and destination nodes.");
            nodesBetween.push_back(sourceNode);
            nodesBetween.insert(nodesBetween.end(), foundBetweenNodes.begin(), foundBetweenNodes.end());
            return nodesBetween;
        }
    }
    x_DEBUG("Topology: return the found path between source and destination nodes.");
    return nodesBetween;
}

std::vector<TopologyNodePtr> Topology::findNodesBetween(std::vector<TopologyNodePtr> sourceNodes,
                                                        std::vector<TopologyNodePtr> destinationNodes) {
    x_DEBUG("Topology: Find a common ancestor node for the input children nodes.");
    TopologyNodePtr commonAncestorForChildren = findCommonAncestor(std::move(sourceNodes));
    if (!commonAncestorForChildren) {
        x_WARNING("Topology: Unable to find a common ancestor node for the input child node.");
        return {};
    }

    x_DEBUG("Topology: Find a common child node for the input parent nodes.");
    TopologyNodePtr commonChildForParents = findCommonChild(std::move(destinationNodes));
    if (!commonChildForParents) {
        x_WARNING("Topology: Unable to find a common child node for the input parent nodes.");
        return {};
    }

    return findNodesBetween(commonAncestorForChildren, commonChildForParents);
}

TopologyNodePtr Topology::findTopologyNodeInSubgraphById(uint64_t id, const std::vector<TopologyNodePtr>& sourceNodes) {
    //First look in all source nodes
    for (const auto& sourceNode : sourceNodes) {
        if (sourceNode->getId() == id) {
            return sourceNode;
        }
    }

    //Perform DFS from the root node to find TopologyNode with matching identifier
    auto rootNodes = sourceNodes[0]->getAllRootNodes();
    //TODO: When we support more than 1 sink location please change the logic
    if (rootNodes.size() > 1) {
        x_NOT_IMPLEMENTED();
    }
    auto topologyIterator = x::DepthFirstNodeIterator(rootNodes[0]).begin();
    while (topologyIterator != x::DepthFirstNodeIterator::end()) {
        auto currentTopologyNode = (*topologyIterator)->as<TopologyNode>();
        if (currentTopologyNode->getId() == id) {
            return currentTopologyNode;
        }
        ++topologyIterator;
    }
    return nullptr;
}
}// namespace x