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
#include <Parsers/IoTSPEPSL/IoTSPEPSLPattern.hpp>

namespace x::Parsers {

//Getter and Setter for the map/list entries of each clause
const std::map<int32_t, std::string>& IoTSPEPSLPattern::getSources() const { return this->sourceList; }
void IoTSPEPSLPattern::setSources(const std::map<int32_t, std::string>& sources) { this->sourceList = sources; }
const std::map<int32_t, IoTSPEPSLOperatorNode>& IoTSPEPSLPattern::getOperatorList() const { return this->operatorList; }
void IoTSPEPSLPattern::setOperatorList(const std::map<int32_t, IoTSPEPSLOperatorNode>& operatorList) {
    this->operatorList = operatorList;
}
const std::list<ExpressionNodePtr>& IoTSPEPSLPattern::getExpressions() const { return this->expressionList; }
void IoTSPEPSLPattern::setExpressions(const std::list<ExpressionNodePtr>& expressions) { this->expressionList = expressions; }
const std::vector<ExpressionNodePtr>& IoTSPEPSLPattern::getProjectionFields() const { return this->projectionFields; }
void IoTSPEPSLPattern::setProjectionFields(const std::vector<ExpressionNodePtr>& projectionFields) {
    this->projectionFields = projectionFields;
}
const std::list<SinkDescriptorPtr>& IoTSPEPSLPattern::getSinks() const { return this->sinkList; }
void IoTSPEPSLPattern::setSinks(const std::list<SinkDescriptorPtr>& sinks) { this->sinkList = sinks; }
const std::pair<std::string, int32_t>& IoTSPEPSLPattern::getWindow() const { return this->window; }
void IoTSPEPSLPattern::setWindow(const std::pair<std::string, int32_t>& window) { this->window = window; }
// methods to update the clauses maps/lists
void IoTSPEPSLPattern::addSource(std::pair<int32_t, std::basic_string<char>> sourcePair) { this->sourceList.insert(sourcePair); }
void IoTSPEPSLPattern::updateSource(const int32_t key, std::string sourceName) { this->sourceList[key] = sourceName; }
void IoTSPEPSLPattern::addExpression(ExpressionNodePtr expressionNode) {
    auto pos = this->expressionList.begin();
    this->expressionList.insert(pos, expressionNode);
}
void IoTSPEPSLPattern::addSink(SinkDescriptorPtr sinkDescriptor) { this->sinkList.push_back(sinkDescriptor); }
void IoTSPEPSLPattern::addProjectionField(ExpressionNodePtr expressionNode) { this->projectionFields.push_back(expressionNode); }
void IoTSPEPSLPattern::addOperatorNode(IoTSPEPSLOperatorNode operatorNode) {
    this->operatorList.insert(std::pair<uint32_t, IoTSPEPSLOperatorNode>(operatorNode.getId(), operatorNode));
}
}// namespace x::Parsers
