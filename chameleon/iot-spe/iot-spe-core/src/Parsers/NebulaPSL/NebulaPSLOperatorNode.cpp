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

#include <Parsers/IoTSPEPSL/IoTSPEPSLOperatorNode.hpp>

namespace x::Parsers {
// creates a instance of the Operator Node (for as AST Tree node) with a unique identifier
IoTSPEPSLOperatorNode::IoTSPEPSLOperatorNode(int32_t id) { this->id = id; }
//Getter and Setter
int32_t IoTSPEPSLOperatorNode::getId() const { return this->id; }
void IoTSPEPSLOperatorNode::setId(int32_t id) { this->id = id; }
const std::string& IoTSPEPSLOperatorNode::getOperatorName() const { return this->operatorName; }
void IoTSPEPSLOperatorNode::setOperatorName(const std::string& operatorName) { this->operatorName = operatorName; }
int32_t IoTSPEPSLOperatorNode::getRightChildId() const { return this->rightChildId; }
void IoTSPEPSLOperatorNode::setRightChildId(int32_t rightChildId) { this->rightChildId = rightChildId; }
int32_t IoTSPEPSLOperatorNode::getLeftChildId() const { return this->leftChildId; }
void IoTSPEPSLOperatorNode::setLeftChildId(int32_t leftChildId) { this->leftChildId = leftChildId; }
const std::pair<int32_t, int32_t>& IoTSPEPSLOperatorNode::getMinMax() const { return this->minMax; }
void IoTSPEPSLOperatorNode::setMinMax(const std::pair<int32_t, int32_t>& minMax) { this->minMax = minMax; }
int32_t IoTSPEPSLOperatorNode::getParentNodeId() const { return this->parentNodeId; }
void IoTSPEPSLOperatorNode::setParentNodeId(int32_t parentNodeId) { this->parentNodeId = parentNodeId; }

}// namespace x::Parsers