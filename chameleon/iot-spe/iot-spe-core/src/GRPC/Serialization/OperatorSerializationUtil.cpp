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
#include <API/AttributeField.hpp>
#include <API/Expressions/Expressions.hpp>
#include <API/Schema.hpp>
#include <GRPC/Serialization/ExpressionSerializationUtil.hpp>
#include <GRPC/Serialization/OperatorSerializationUtil.hpp>
#include <GRPC/Serialization/SchemaSerializationUtil.hpp>
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Operators/LogicalOperators/BroadcastLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/FilterLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/InferModelLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/JoinLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/LimitLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/MapLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/MapUDFLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/OpenCLLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/RenameSourceOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/MaterializedViewSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/MonitoringSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/NetworkSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/NullOutputSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/PrintSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/SinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/ZmqSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/ArrowSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/BinarySourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/CsvSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/DefaultSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/LogicalSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/NetworkSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SenseSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sources/TCPSourceDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/ZmqSourceDescriptor.hpp>
#include <Operators/LogicalOperators/UnionLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Windowing/CentralWindowOperator.hpp>
#include <Operators/LogicalOperators/Windowing/SliceCreationOperator.hpp>
#include <Operators/LogicalOperators/Windowing/SliceMergingOperator.hpp>
#include <Operators/LogicalOperators/Windowing/WindowComputationOperator.hpp>
#include <Windowing/LogicalBatchJoinDefinition.hpp>
#include <Windowing/LogicalJoinDefinition.hpp>
#include <Windowing/LogicalWindowDefinition.hpp>

#include <Operators/OperatorNode.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Windowing/DistributionCharacteristic.hpp>
#include <Windowing/TimeCharacteristic.hpp>
#include <Windowing/WindowAggregations/AvgAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/CountAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/MaxAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/MedianAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/MinAggregationDescriptor.hpp>
#include <Windowing/WindowAggregations/SumAggregationDescriptor.hpp>

#include <Windowing/WindowPolicies/OnBufferTriggerPolicyDescription.hpp>
#include <Windowing/WindowPolicies/OnRecordTriggerPolicyDescription.hpp>
#include <Windowing/WindowPolicies/OnTimeTriggerPolicyDescription.hpp>
#include <Windowing/WindowPolicies/OnWatermarkChangeTriggerPolicyDescription.hpp>

#include <Operators/LogicalOperators/BatchJoinLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/JoinLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/ProjectionLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/WatermarkAssignerLogicalOperatorNode.hpp>
#include <Windowing/Watermark/EventTimeWatermarkStrategyDescriptor.hpp>
#include <Windowing/Watermark/IngestionTimeWatermarkStrategyDescriptor.hpp>
#include <Windowing/WindowActions/BaseJoinActionDescriptor.hpp>
#include <Windowing/WindowActions/BaseWindowActionDescriptor.hpp>
#include <Windowing/WindowActions/CompleteAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/LazyxtLoopJoinTriggerActionDescriptor.hpp>
#include <Windowing/WindowActions/SliceAggregationTriggerActionDescriptor.hpp>
#include <Windowing/WindowTypes/SlidingWindow.hpp>
#include <Windowing/WindowTypes/ThresholdWindow.hpp>
#include <Windowing/WindowTypes/TumblingWindow.hpp>
#include <Windowing/WindowTypes/WindowType.hpp>

#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <GRPC/Serialization/UDFSerializationUtil.hpp>
#include <Operators/LogicalOperators/CEP/IterationLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/MQTTSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/MonitoringSourceDescriptor.hpp>

#include <fstream>
#ifdef ENABLE_OPC_BUILD
#include <Operators/LogicalOperators/Sinks/OPCSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/OPCSourceDescriptor.hpp>
#endif
#ifdef ENABLE_MQTT_BUILD
#include <Operators/LogicalOperators/Sources/MQTTSourceDescriptor.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <fstream>

#endif

namespace x {

SerializableOperator OperatorSerializationUtil::serializeOperator(const OperatorNodePtr& operatorNode, bool isClientOriginated) {
    x_TRACE("OperatorSerializationUtil:: serialize operator {}", operatorNode->toString());
    SerializableOperator serializedOperator = SerializableOperator();
    if (operatorNode->instanceOf<SourceLogicalOperatorNode>()) {
        // serialize source operator
        serializeSourceOperator(*operatorNode->as<SourceLogicalOperatorNode>(), serializedOperator, isClientOriginated);

    } else if (operatorNode->instanceOf<SinkLogicalOperatorNode>()) {
        // serialize sink operator
        serializeSinkOperator(*operatorNode->as<SinkLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<FilterLogicalOperatorNode>()) {
        // serialize filter operator
        serializeFilterOperator(*operatorNode->as<FilterLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<ProjectionLogicalOperatorNode>()) {
        // serialize projection operator
        serializeProjectionOperator(*operatorNode->as<ProjectionLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<UnionLogicalOperatorNode>()) {
        // serialize union operator
        x_TRACE("OperatorSerializationUtil:: serialize to UnionLogicalOperatorNode");
        auto unionDetails = SerializableOperator_UnionDetails();
        serializedOperator.mutable_details()->PackFrom(unionDetails);

    } else if (operatorNode->instanceOf<BroadcastLogicalOperatorNode>()) {
        // serialize broadcast operator
        x_TRACE("OperatorSerializationUtil:: serialize to BroadcastLogicalOperatorNode");
        auto broadcastDetails = SerializableOperator_BroadcastDetails();
        serializedOperator.mutable_details()->PackFrom(broadcastDetails);

    } else if (operatorNode->instanceOf<MapLogicalOperatorNode>()) {
        // serialize map operator
        serializeMapOperator(*operatorNode->as<MapLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<InferModel::InferModelLogicalOperatorNode>()) {
#ifdef TFDEF
        // serialize infer model
        serializeInferModelOperator(*operatorNode->as<InferModel::InferModelLogicalOperatorNode>(), serializedOperator);
#endif// TFDEF

    } else if (operatorNode->instanceOf<IterationLogicalOperatorNode>()) {
        // serialize CEPIteration operator
        serializeCEPIterationOperator(*operatorNode->as<IterationLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<CentralWindowOperator>()) {
        // serialize window operator
        serializeWindowOperator(*operatorNode->as<CentralWindowOperator>(), serializedOperator);

    } else if (operatorNode->instanceOf<SliceCreationOperator>()) {
        // serialize window operator
        serializeWindowOperator(*operatorNode->as<SliceCreationOperator>(), serializedOperator);

    } else if (operatorNode->instanceOf<SliceMergingOperator>()) {
        // serialize slice merging operator
        serializeWindowOperator(*operatorNode->as<SliceMergingOperator>(), serializedOperator);

    } else if (operatorNode->instanceOf<WindowComputationOperator>()) {
        // serialize window operator
        serializeWindowOperator(*operatorNode->as<WindowComputationOperator>(), serializedOperator);

    } else if (operatorNode->instanceOf<JoinLogicalOperatorNode>()) {
        // serialize streaming join operator
        serializeJoinOperator(*operatorNode->as<JoinLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<Experimental::BatchJoinLogicalOperatorNode>()) {
        // serialize batch join operator
        serializeBatchJoinOperator(*operatorNode->as<Experimental::BatchJoinLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<LimitLogicalOperatorNode>()) {
        // serialize limit operator
        serializeLimitOperator(*operatorNode->as<LimitLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<WatermarkAssignerLogicalOperatorNode>()) {
        // serialize watermarkAssigner operator
        serializeWatermarkAssignerOperator(*operatorNode->as<WatermarkAssignerLogicalOperatorNode>(), serializedOperator);

    } else if (operatorNode->instanceOf<RenameSourceOperatorNode>()) {
        // Serialize rename source operator
        x_TRACE("OperatorSerializationUtil:: serialize to RenameSourceOperatorNode");
        auto renameDetails = SerializableOperator_RenameDetails();
        renameDetails.set_newsourcename(operatorNode->as<RenameSourceOperatorNode>()->getNewSourceName());
        serializedOperator.mutable_details()->PackFrom(renameDetails);

    } else if (operatorNode->instanceOf<MapUDFLogicalOperatorNode>()) {
        // Serialize Map Java UDF operator
        serializeJavaUDFOperator<MapUDFLogicalOperatorNode, SerializableOperator_MapJavaUdfDetails>(
            *operatorNode->as<MapUDFLogicalOperatorNode>(),
            serializedOperator);

    } else if (operatorNode->instanceOf<FlatMapUDFLogicalOperatorNode>()) {
        // Serialize FlatMap Java UDF operator
        serializeJavaUDFOperator<FlatMapUDFLogicalOperatorNode, SerializableOperator_FlatMapJavaUdfDetails>(
            *operatorNode->as<FlatMapUDFLogicalOperatorNode>(),
            serializedOperator);

    } else if (operatorNode->instanceOf<OpenCLLogicalOperatorNode>()) {
        // Serialize map udf operator
        serializeOpenCLOperator(*operatorNode->as<OpenCLLogicalOperatorNode>(), serializedOperator);
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not serialize this operator: {}", operatorNode->toString());
    }

    // serialize input schema
    serializeInputSchema(operatorNode, serializedOperator);

    // serialize output schema
    SchemaSerializationUtil::serializeSchema(operatorNode->getOutputSchema(), serializedOperator.mutable_outputschema());

    // serialize operator id
    serializedOperator.set_operatorid(operatorNode->getId());

    // serialize and append children if the node has any
    for (const auto& child : operatorNode->getChildren()) {
        serializedOperator.add_childrenids(child->as<OperatorNode>()->getId());
    }

    // serialize and append origin id
    if (operatorNode->isBinaryOperator()) {
        auto binaryOperator = operatorNode->as<BinaryOperatorNode>();
        for (const auto& originId : binaryOperator->getLeftInputOriginIds()) {
            serializedOperator.add_leftoriginids(originId);
        }
        for (const auto& originId : binaryOperator->getRightInputOriginIds()) {
            serializedOperator.add_rightoriginids(originId);
        }
    } else if (operatorNode->isExchangeOperator()) {
        auto exchangeOperator = operatorNode->as<ExchangeOperatorNode>();
        for (const auto& originId : exchangeOperator->getOutputOriginIds()) {
            serializedOperator.add_originids(originId);
        }
    } else {
        auto unaryOperator = operatorNode->as<UnaryOperatorNode>();
        for (const auto& originId : unaryOperator->getInputOriginIds()) {
            serializedOperator.add_originids(originId);
        }
    }

    x_TRACE("OperatorSerializationUtil:: serialize {} to {}",
              operatorNode->toString(),
              serializedOperator.details().type_url());
    return serializedOperator;
}

OperatorNodePtr OperatorSerializationUtil::deserializeOperator(SerializableOperator serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: de-serialize {}", serializedOperator.DebugString());
    auto details = serializedOperator.details();
    LogicalOperatorNodePtr operatorNode;
    if (details.Is<SerializableOperator_SourceDetails>()) {
        // de-serialize source operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to SourceLogicalOperator");
        auto serializedSourceDescriptor = SerializableOperator_SourceDetails();
        details.UnpackTo(&serializedSourceDescriptor);
        operatorNode = deserializeSourceOperator(serializedSourceDescriptor);

    } else if (details.Is<SerializableOperator_SinkDetails>()) {
        // de-serialize sink operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to SinkLogicalOperator");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails();
        details.UnpackTo(&serializedSinkDescriptor);
        operatorNode = deserializeSinkOperator(serializedSinkDescriptor);

    } else if (details.Is<SerializableOperator_FilterDetails>()) {
        // de-serialize filter operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to FilterLogicalOperator");
        auto serializedFilterOperator = SerializableOperator_FilterDetails();
        details.UnpackTo(&serializedFilterOperator);
        operatorNode = deserializeFilterOperator(serializedFilterOperator);

    } else if (details.Is<SerializableOperator_ProjectionDetails>()) {
        // de-serialize projection operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to ProjectionLogicalOperator");
        auto serializedProjectionOperator = SerializableOperator_ProjectionDetails();
        details.UnpackTo(&serializedProjectionOperator);
        operatorNode = deserializeProjectionOperator(serializedProjectionOperator);

    } else if (details.Is<SerializableOperator_CEPIterationDetails>()) {
        // de-serialize CEPIteration operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to CEPIterationLogicalOperator");
        auto serializedCEPIterationOperator = SerializableOperator_CEPIterationDetails();
        details.UnpackTo(&serializedCEPIterationOperator);
        operatorNode = deserializeCEPIterationOperator(serializedCEPIterationOperator);

    } else if (details.Is<SerializableOperator_UnionDetails>()) {
        // de-serialize union operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to UnionLogicalOperator");
        auto serializedUnionDescriptor = SerializableOperator_UnionDetails();
        details.UnpackTo(&serializedUnionDescriptor);
        operatorNode = LogicalOperatorFactory::createUnionOperator(Util::getNextOperatorId());

    } else if (details.Is<SerializableOperator_BroadcastDetails>()) {
        // de-serialize broadcast operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to BroadcastLogicalOperator");
        auto serializedBroadcastDescriptor = SerializableOperator_BroadcastDetails();
        details.UnpackTo(&serializedBroadcastDescriptor);
        // de-serialize broadcast descriptor
        operatorNode = LogicalOperatorFactory::createBroadcastOperator(Util::getNextOperatorId());

    } else if (details.Is<SerializableOperator_MapDetails>()) {
        // de-serialize map operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to MapLogicalOperator");
        auto serializedMapOperator = SerializableOperator_MapDetails();
        details.UnpackTo(&serializedMapOperator);
        operatorNode = deserializeMapOperator(serializedMapOperator);

    } else if (details.Is<SerializableOperator_InferModelDetails>()) {
#ifdef TFDEF
        // de-serialize infer model operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to InferModelLogicalOperator");
        auto serializedInferModelOperator = SerializableOperator_InferModelDetails();
        details.UnpackTo(&serializedInferModelOperator);
        operatorNode = deserializeInferModelOperator(serializedInferModelOperator);
#endif// TFDEF

    } else if (details.Is<SerializableOperator_WindowDetails>()) {
        // de-serialize window operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to WindowLogicalOperator");
        auto serializedWindowOperator = SerializableOperator_WindowDetails();
        details.UnpackTo(&serializedWindowOperator);
        auto windowNode = deserializeWindowOperator(serializedWindowOperator, Util::getNextOperatorId());
        operatorNode = windowNode;

    } else if (details.Is<SerializableOperator_JoinDetails>()) {
        // de-serialize streaming join operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to JoinLogicalOperator");
        auto serializedJoinOperator = SerializableOperator_JoinDetails();
        details.UnpackTo(&serializedJoinOperator);
        operatorNode = deserializeJoinOperator(serializedJoinOperator, Util::getNextOperatorId());

    } else if (details.Is<SerializableOperator_BatchJoinDetails>()) {
        // de-serialize batch join operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to BatchJoinLogicalOperator");
        auto serializedBatchJoinOperator = SerializableOperator_BatchJoinDetails();
        details.UnpackTo(&serializedBatchJoinOperator);
        operatorNode = deserializeBatchJoinOperator(serializedBatchJoinOperator, Util::getNextOperatorId());

    } else if (details.Is<SerializableOperator_WatermarkStrategyDetails>()) {
        // de-serialize watermark assigner operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to watermarkassigner operator");
        auto serializedWatermarkStrategyDetails = SerializableOperator_WatermarkStrategyDetails();
        details.UnpackTo(&serializedWatermarkStrategyDetails);
        operatorNode = deserializeWatermarkAssignerOperator(serializedWatermarkStrategyDetails);

    } else if (details.Is<SerializableOperator_LimitDetails>()) {
        // de-serialize limit operator
        x_TRACE("OperatorSerializationUtil:: de-serialize to limit operator");
        auto serializedLimitDetails = SerializableOperator_LimitDetails();
        details.UnpackTo(&serializedLimitDetails);
        operatorNode = deserializeLimitOperator(serializedLimitDetails);

    } else if (details.Is<SerializableOperator_RenameDetails>()) {
        // Deserialize rename source operator.
        x_TRACE("OperatorSerializationUtil:: deserialize to rename source operator");
        auto renameDetails = SerializableOperator_RenameDetails();
        details.UnpackTo(&renameDetails);
        operatorNode = LogicalOperatorFactory::createRenameSourceOperator(renameDetails.newsourcename());

    } else if (details.Is<SerializableOperator_MapJavaUdfDetails>()) {
        x_TRACE("Deserialize Map Java UDF operator.");
        auto mapJavaUDFDetails = SerializableOperator_MapJavaUdfDetails();
        details.UnpackTo(&mapJavaUDFDetails);
        operatorNode = deserializeMapJavaUDFOperator(mapJavaUDFDetails);

    } else if (details.Is<SerializableOperator_FlatMapJavaUdfDetails>()) {
        x_TRACE("Deserialize FlatMap Java UDF operator.");
        auto flatMapJavaUDFDetails = SerializableOperator_FlatMapJavaUdfDetails();
        details.UnpackTo(&flatMapJavaUDFDetails);
        operatorNode = deserializeFlatMapJavaUDFOperator(flatMapJavaUDFDetails);

    } else if (details.Is<SerializableOperator_OpenCLOperatorDetails>()) {
        x_TRACE("Deserialize Open CL operator.");
        auto openCLDetails = SerializableOperator_OpenCLOperatorDetails();
        details.UnpackTo(&openCLDetails);
        operatorNode = deserializeOpenCLOperator(openCLDetails);

    } else {
        x_THROW_RUNTIME_ERROR("OperatorSerializationUtil: could not de-serialize this serialized operator: ");
    }

    // de-serialize operator output schema
    operatorNode->setOutputSchema(SchemaSerializationUtil::deserializeSchema(serializedOperator.outputschema()));
    // deserialize input schema
    deserializeInputSchema(operatorNode, serializedOperator);

    if (details.Is<SerializableOperator_JoinDetails>()) {
        auto joinOp = operatorNode->as<JoinLogicalOperatorNode>();
        joinOp->getJoinDefinition()->updateSourceTypes(joinOp->getLeftInputSchema(), joinOp->getRightInputSchema());
        joinOp->getJoinDefinition()->updateOutputDefinition(joinOp->getOutputSchema());
    }

    if (details.Is<SerializableOperator_BatchJoinDetails>()) {
        auto joinOp = operatorNode->as<Experimental::BatchJoinLogicalOperatorNode>();
        joinOp->getBatchJoinDefinition()->updateInputSchemas(joinOp->getLeftInputSchema(), joinOp->getRightInputSchema());
        joinOp->getBatchJoinDefinition()->updateOutputDefinition(joinOp->getOutputSchema());
    }

    // de-serialize and append origin id
    if (operatorNode->isBinaryOperator()) {
        auto binaryOperator = operatorNode->as<BinaryOperatorNode>();
        std::vector<uint64_t> leftOriginIds;
        for (const auto& originId : serializedOperator.leftoriginids()) {
            leftOriginIds.push_back(originId);
        }
        binaryOperator->setLeftInputOriginIds(leftOriginIds);
        std::vector<uint64_t> rightOriginIds;
        for (const auto& originId : serializedOperator.rightoriginids()) {
            rightOriginIds.push_back(originId);
        }
        binaryOperator->setRightInputOriginIds(rightOriginIds);
    } else if (operatorNode->isExchangeOperator()) {
        auto exchangeOperator = operatorNode->as<ExchangeOperatorNode>();
        std::vector<uint64_t> originIds;
        for (const auto& originId : serializedOperator.originids()) {
            originIds.push_back(originId);
        }
        exchangeOperator->setInputOriginIds(originIds);
    } else {
        auto unaryOperator = operatorNode->as<UnaryOperatorNode>();
        std::vector<uint64_t> originIds;
        for (const auto& originId : serializedOperator.originids()) {
            originIds.push_back(originId);
        }
        unaryOperator->setInputOriginIds(originIds);
    }
    x_TRACE("OperatorSerializationUtil:: de-serialize {} to {}", serializedOperator.DebugString(), operatorNode->toString());
    return operatorNode;
}

void OperatorSerializationUtil::serializeSourceOperator(const SourceLogicalOperatorNode& sourceOperator,
                                                        SerializableOperator& serializedOperator,
                                                        bool isClientOriginated) {

    x_TRACE("OperatorSerializationUtil:: serialize to SourceLogicalOperatorNode");

    auto sourceDetails = SerializableOperator_SourceDetails();
    auto sourceDescriptor = sourceOperator.getSourceDescriptor();
    serializeSourceDescriptor(*sourceDescriptor, sourceDetails, isClientOriginated);
    sourceDetails.set_sourceoriginid(sourceOperator.getOriginId());

    serializedOperator.mutable_details()->PackFrom(sourceDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeSourceOperator(const SerializableOperator_SourceDetails& sourceDetails) {
    auto sourceDescriptor = deserializeSourceDescriptor(sourceDetails);
    return LogicalOperatorFactory::createSourceOperator(sourceDescriptor,
                                                        Util::getNextOperatorId(),
                                                        sourceDetails.sourceoriginid());
}

void OperatorSerializationUtil::serializeFilterOperator(const FilterLogicalOperatorNode& filterOperator,
                                                        SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to FilterLogicalOperatorNode");
    auto filterDetails = SerializableOperator_FilterDetails();
    ExpressionSerializationUtil::serializeExpression(filterOperator.getPredicate(), filterDetails.mutable_predicate());
    serializedOperator.mutable_details()->PackFrom(filterDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeFilterOperator(const SerializableOperator_FilterDetails& filterDetails) {
    auto filterExpression = ExpressionSerializationUtil::deserializeExpression(filterDetails.predicate());
    return LogicalOperatorFactory::createFilterOperator(filterExpression, Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeProjectionOperator(const ProjectionLogicalOperatorNode& projectionOperator,
                                                            SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to ProjectionLogicalOperatorNode");
    auto projectionDetail = SerializableOperator_ProjectionDetails();
    for (auto& exp : projectionOperator.getExpressions()) {
        auto* mutableExpression = projectionDetail.mutable_expression()->Add();
        ExpressionSerializationUtil::serializeExpression(exp, mutableExpression);
    }

    serializedOperator.mutable_details()->PackFrom(projectionDetail);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeProjectionOperator(const SerializableOperator_ProjectionDetails& projectionDetails) {
    // serialize and append children if the node has any
    std::vector<ExpressionNodePtr> exps;
    for (auto serializedExpression : projectionDetails.expression()) {
        auto projectExpression = ExpressionSerializationUtil::deserializeExpression(serializedExpression);
        exps.push_back(projectExpression);
    }

    return LogicalOperatorFactory::createProjectionOperator(exps, Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeSinkOperator(const SinkLogicalOperatorNode& sinkOperator,
                                                      SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to SinkLogicalOperatorNode");
    auto sinkDetails = SerializableOperator_SinkDetails();
    auto sinkDescriptor = sinkOperator.getSinkDescriptor();
    serializeSinkDescriptor(*sinkDescriptor, sinkDetails, sinkOperator.getInputOriginIds().size());
    serializedOperator.mutable_details()->PackFrom(sinkDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeSinkOperator(const SerializableOperator_SinkDetails& sinkDetails) {
    auto sinkDescriptor = deserializeSinkDescriptor(sinkDetails);
    return LogicalOperatorFactory::createSinkOperator(sinkDescriptor, Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeMapOperator(const MapLogicalOperatorNode& mapOperator,
                                                     SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to MapLogicalOperatorNode");
    auto mapDetails = SerializableOperator_MapDetails();
    ExpressionSerializationUtil::serializeExpression(mapOperator.getMapExpression(), mapDetails.mutable_expression());
    serializedOperator.mutable_details()->PackFrom(mapDetails);
}

LogicalUnaryOperatorNodePtr OperatorSerializationUtil::deserializeMapOperator(const SerializableOperator_MapDetails& mapDetails) {
    auto fieldAssignmentExpression = ExpressionSerializationUtil::deserializeExpression(mapDetails.expression());
    return LogicalOperatorFactory::createMapOperator(fieldAssignmentExpression->as<FieldAssignmentExpressionNode>(),
                                                     Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeWindowOperator(const WindowOperatorNode& windowOperator,
                                                        SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to WindowOperatorNode");
    auto windowDetails = SerializableOperator_WindowDetails();
    auto windowDefinition = windowOperator.getWindowDefinition();

    if (windowDefinition->isKeyed()) {
        for (auto& key : windowDefinition->getKeys()) {
            auto expression = windowDetails.mutable_keys()->Add();
            ExpressionSerializationUtil::serializeExpression(key, expression);
        }
    }
    windowDetails.set_origin(windowOperator.getOriginId());
    windowDetails.set_allowedlatexs(windowDefinition->getAllowedLatexs());
    auto windowType = windowDefinition->getWindowType();

    if (windowType->isTimeBasedWindowType()) {
        auto timeBasedWindowType = Windowing::WindowType::asTimeBasedWindowType(windowType);
        auto timeCharacteristic = timeBasedWindowType->getTimeCharacteristic();
        auto timeCharacteristicDetails = SerializableOperator_TimeCharacteristic();
        if (timeCharacteristic->getType() == Windowing::TimeCharacteristic::Type::EventTime) {
            timeCharacteristicDetails.set_type(SerializableOperator_TimeCharacteristic_Type_EventTime);
            timeCharacteristicDetails.set_field(timeCharacteristic->getField()->getName());
        } else if (timeCharacteristic->getType() == Windowing::TimeCharacteristic::Type::IngestionTime) {
            timeCharacteristicDetails.set_type(SerializableOperator_TimeCharacteristic_Type_IngestionTime);
        } else {
            x_ERROR("OperatorSerializationUtil: Cant serialize window Time Characteristic");
        }
        timeCharacteristicDetails.set_multiplier(timeCharacteristic->getTimeUnit().getMultiplier());

        if (timeBasedWindowType->getTimeBasedSubWindowType() == Windowing::TimeBasedWindowType::TUMBLINGWINDOW) {
            auto tumblingWindow = std::dynamic_pointer_cast<Windowing::TumblingWindow>(windowType);
            auto tumblingWindowDetails = SerializableOperator_TumblingWindow();
            tumblingWindowDetails.mutable_timecharacteristic()->CopyFrom(timeCharacteristicDetails);
            tumblingWindowDetails.set_size(tumblingWindow->getSize().getTime());
            windowDetails.mutable_windowtype()->PackFrom(tumblingWindowDetails);
        } else if (timeBasedWindowType->getTimeBasedSubWindowType() == Windowing::TimeBasedWindowType::SLIDINGWINDOW) {
            auto slidingWindow = std::dynamic_pointer_cast<Windowing::SlidingWindow>(windowType);
            auto slidingWindowDetails = SerializableOperator_SlidingWindow();
            slidingWindowDetails.mutable_timecharacteristic()->CopyFrom(timeCharacteristicDetails);
            slidingWindowDetails.set_size(slidingWindow->getSize().getTime());
            slidingWindowDetails.set_slide(slidingWindow->getSlide().getTime());
            windowDetails.mutable_windowtype()->PackFrom(slidingWindowDetails);
        } else {
            x_ERROR("OperatorSerializationUtil: Cant serialize window Time Type");
        }
    } else if (windowType->isContentBasedWindowType()) {
        auto contentBasedWindowType = Windowing::WindowType::asContentBasedWindowType(windowType);
        if (contentBasedWindowType->getContentBasedSubWindowType() == Windowing::ContentBasedWindowType::THRESHOLDWINDOW) {
            auto thresholdWindow = std::dynamic_pointer_cast<Windowing::ThresholdWindow>(windowType);
            auto thresholdWindowDetails = SerializableOperator_ThresholdWindow();
            ExpressionSerializationUtil::serializeExpression(thresholdWindow->getPredicate(),
                                                             thresholdWindowDetails.mutable_predicate());
            thresholdWindowDetails.set_minimumcount(thresholdWindow->getMinimumCount());
            windowDetails.mutable_windowtype()->PackFrom(thresholdWindowDetails);
        } else {
            x_ERROR("OperatorSerializationUtil: Cant serialize this content based window Type");
        }
    }

    // serialize aggregation
    for (auto aggregation : windowDefinition->getWindowAggregation()) {
        auto* windowAggregation = windowDetails.mutable_windowaggregations()->Add();
        ExpressionSerializationUtil::serializeExpression(aggregation->as(), windowAggregation->mutable_asfield());
        ExpressionSerializationUtil::serializeExpression(aggregation->on(), windowAggregation->mutable_onfield());

        switch (aggregation->getType()) {
            case Windowing::WindowAggregationDescriptor::Type::Count:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_COUNT);
                break;
            case Windowing::WindowAggregationDescriptor::Type::Max:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_MAX);
                break;
            case Windowing::WindowAggregationDescriptor::Type::Min:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_MIN);
                break;
            case Windowing::WindowAggregationDescriptor::Type::Sum:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_SUM);
                break;
            case Windowing::WindowAggregationDescriptor::Type::Avg:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_AVG);
                break;
            case Windowing::WindowAggregationDescriptor::Type::Median:
                windowAggregation->set_type(SerializableOperator_WindowDetails_Aggregation_Type_MEDIAN);
                break;
            default: x_FATAL_ERROR("OperatorSerializationUtil: could not cast aggregation type");
        }
    }
    auto* windowTrigger = windowDetails.mutable_triggerpolicy();

    switch (windowDefinition->getTriggerPolicy()->getPolicyType()) {
        case Windowing::TriggerType::triggerOnTime: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnTime);
            Windowing::OnTimeTriggerDescriptionPtr triggerDesc =
                std::dynamic_pointer_cast<Windowing::OnTimeTriggerPolicyDescription>(windowDefinition->getTriggerPolicy());
            windowTrigger->set_timeinms(triggerDesc->getTriggerTimeInMs());
            break;
        }
        case Windowing::TriggerType::triggerOnRecord: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnRecord);
            break;
        }
        case Windowing::TriggerType::triggerOnBuffer: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnBuffer);
            break;
        }
        case Windowing::TriggerType::triggerOnWatermarkChange: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnWatermarkChange);
            break;
        }
        default: {
            x_FATAL_ERROR("OperatorSerializationUtil: could not cast aggregation type");
        }
    }

    auto* windowAction = windowDetails.mutable_action();
    switch (windowDefinition->getTriggerAction()->getActionType()) {
        case Windowing::ActionType::WindowAggregationTriggerAction: {
            windowAction->set_type(SerializableOperator_TriggerAction_Type_Complete);
            break;
        }
        case Windowing::ActionType::SliceAggregationTriggerAction: {
            windowAction->set_type(SerializableOperator_TriggerAction_Type_Slicing);
            break;
        }
        default: {
            x_FATAL_ERROR("OperatorSerializationUtil: could not cast action type");
        }
    }

    if (windowDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Complete) {
        windowDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Complete);
    } else if (windowDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Combining) {
        windowDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Combining);
    } else if (windowDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Slicing) {
        windowDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Slicing);
    } else if (windowDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Merging) {
        windowDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Merging);
    } else {
        x_NOT_IMPLEMENTED();
    }

    serializedOperator.mutable_details()->PackFrom(windowDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeWindowOperator(const SerializableOperator_WindowDetails& windowDetails,
                                                     OperatorId operatorId) {
    auto serializedWindowAggregations = windowDetails.windowaggregations();
    auto serializedTriggerPolicy = windowDetails.triggerpolicy();
    auto serializedAction = windowDetails.action();

    auto serializedWindowType = windowDetails.windowtype();

    std::vector<Windowing::WindowAggregationPtr> aggregation;
    for (auto serializedWindowAggregation : serializedWindowAggregations) {
        auto onField = ExpressionSerializationUtil::deserializeExpression(serializedWindowAggregation.onfield())
                           ->as<FieldAccessExpressionNode>();
        auto asField = ExpressionSerializationUtil::deserializeExpression(serializedWindowAggregation.asfield())
                           ->as<FieldAccessExpressionNode>();
        if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_SUM) {
            aggregation.emplace_back(Windowing::SumAggregationDescriptor::create(onField, asField));
        } else if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_MAX) {
            aggregation.emplace_back(Windowing::MaxAggregationDescriptor::create(onField, asField));
        } else if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_MIN) {
            aggregation.emplace_back(Windowing::MinAggregationDescriptor::create(onField, asField));
        } else if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_COUNT) {
            aggregation.emplace_back(Windowing::CountAggregationDescriptor::create(onField, asField));
        } else if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_AVG) {
            aggregation.emplace_back(Windowing::AvgAggregationDescriptor::create(onField, asField));
        } else if (serializedWindowAggregation.type() == SerializableOperator_WindowDetails_Aggregation_Type_MEDIAN) {
            aggregation.emplace_back(Windowing::MedianAggregationDescriptor::create(onField, asField));
        } else {
            x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window aggregation: {}",
                            serializedWindowAggregation.DebugString());
        }
    }

    Windowing::WindowTriggerPolicyPtr trigger;
    if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnTime) {
        trigger = Windowing::OnTimeTriggerPolicyDescription::create(serializedTriggerPolicy.timeinms());
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnBuffer) {
        trigger = Windowing::OnBufferTriggerPolicyDescription::create();
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnRecord) {
        trigger = Windowing::OnRecordTriggerPolicyDescription::create();
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnWatermarkChange) {
        trigger = Windowing::OnWatermarkChangeTriggerPolicyDescription::create();
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize trigger: {}", serializedTriggerPolicy.DebugString());
    }

    Windowing::WindowActionDescriptorPtr action;
    if (serializedAction.type() == SerializableOperator_TriggerAction_Type_Complete) {
        action = Windowing::CompleteAggregationTriggerActionDescriptor::create();
    } else if (serializedAction.type() == SerializableOperator_TriggerAction_Type_Slicing) {
        action = Windowing::SliceAggregationTriggerActionDescriptor::create();
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize action: {}", serializedAction.DebugString());
    }

    Windowing::WindowTypePtr window;
    if (serializedWindowType.Is<SerializableOperator_TumblingWindow>()) {
        auto serializedTumblingWindow = SerializableOperator_TumblingWindow();
        serializedWindowType.UnpackTo(&serializedTumblingWindow);
        auto serializedTimeCharacteristic = serializedTumblingWindow.timecharacteristic();
        if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_EventTime) {
            auto field = Attribute(serializedTimeCharacteristic.field());
            auto multiplier = serializedTimeCharacteristic.multiplier();
            window = Windowing::TumblingWindow::of(
                Windowing::TimeCharacteristic::createEventTime(field, Windowing::TimeUnit(multiplier)),
                Windowing::TimeMeasure(serializedTumblingWindow.size()));
        } else if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_IngestionTime) {
            window = Windowing::TumblingWindow::of(Windowing::TimeCharacteristic::createIngestionTime(),
                                                   Windowing::TimeMeasure(serializedTumblingWindow.size()));
        } else {
            x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window time characteristic: {}",
                            serializedTimeCharacteristic.DebugString());
        }
    } else if (serializedWindowType.Is<SerializableOperator_SlidingWindow>()) {
        auto serializedSlidingWindow = SerializableOperator_SlidingWindow();
        serializedWindowType.UnpackTo(&serializedSlidingWindow);
        auto serializedTimeCharacteristic = serializedSlidingWindow.timecharacteristic();
        if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_EventTime) {
            auto field = Attribute(serializedTimeCharacteristic.field());
            window = Windowing::SlidingWindow::of(Windowing::TimeCharacteristic::createEventTime(field),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.size()),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.slide()));
        } else if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_IngestionTime) {
            window = Windowing::SlidingWindow::of(Windowing::TimeCharacteristic::createIngestionTime(),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.size()),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.slide()));
        } else {
            x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window time characteristic: {}",
                            serializedTimeCharacteristic.DebugString());
        }
    } else if (serializedWindowType.Is<SerializableOperator_ThresholdWindow>()) {
        auto serializedThresholdWindow = SerializableOperator_ThresholdWindow();
        serializedWindowType.UnpackTo(&serializedThresholdWindow);
        auto thresholdExpression = ExpressionSerializationUtil::deserializeExpression(serializedThresholdWindow.predicate());
        auto thresholdMinimumCount = serializedThresholdWindow.minimumcount();
        window = Windowing::ThresholdWindow::of(thresholdExpression, thresholdMinimumCount);
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window type: {}", serializedWindowType.DebugString());
    }

    auto distrChar = windowDetails.distrchar();
    Windowing::DistributionCharacteristicPtr distChar;
    if (distrChar.distr() == SerializableOperator_DistributionCharacteristic_Distribution_Unset) {
        // `Unset' indicates that the logical operator has just been deserialized from a client.
        // We change it to `Complete' which is the default used in `Query::window' and `Query::windowByKey'.
        // TODO This logic should be revisited when #2884 is fixed.
        x_DEBUG("OperatorSerializationUtil::deserializeWindowOperator: "
                  "SerializableOperator_WindowDetails_DistributionCharacteristic_Distribution_Unset");
        distChar = Windowing::DistributionCharacteristic::createCompleteWindowType();
    } else if (distrChar.distr() == SerializableOperator_DistributionCharacteristic_Distribution_Complete) {
        x_DEBUG("OperatorSerializationUtil::deserializeWindowOperator: "
                  "SerializableOperator_WindowDetails_DistributionCharacteristic_Distribution_Complete");
        distChar = Windowing::DistributionCharacteristic::createCompleteWindowType();
    } else if (distrChar.distr() == SerializableOperator_DistributionCharacteristic_Distribution_Combining) {
        x_DEBUG("OperatorSerializationUtil::deserializeWindowOperator: "
                  "SerializableOperator_WindowDetails_DistributionCharacteristic_Distribution_Combining");
        distChar =
            std::make_shared<Windowing::DistributionCharacteristic>(Windowing::DistributionCharacteristic::Type::Combining);
    } else if (distrChar.distr() == SerializableOperator_DistributionCharacteristic_Distribution_Slicing) {
        x_DEBUG("OperatorSerializationUtil::deserializeWindowOperator: "
                  "SerializableOperator_WindowDetails_DistributionCharacteristic_Distribution_Slicing");
        distChar = std::make_shared<Windowing::DistributionCharacteristic>(Windowing::DistributionCharacteristic::Type::Slicing);
    } else if (distrChar.distr() == SerializableOperator_DistributionCharacteristic_Distribution_Merging) {
        x_DEBUG("OperatorSerializationUtil::deserializeWindowOperator: "
                  "SerializableOperator_WindowDetails_DistributionCharacteristic_Distribution_Merging");
        distChar = std::make_shared<Windowing::DistributionCharacteristic>(Windowing::DistributionCharacteristic::Type::Merging);
    } else {
        x_NOT_IMPLEMENTED();
    }

    auto allowedLatexs = windowDetails.allowedlatexs();
    std::vector<FieldAccessExpressionNodePtr> keyAccessExpression;
    for (auto& key : windowDetails.keys()) {
        keyAccessExpression.emplace_back(
            ExpressionSerializationUtil::deserializeExpression(key)->as<FieldAccessExpressionNode>());
    }
    auto windowDef = Windowing::LogicalWindowDefinition::create(keyAccessExpression,
                                                                aggregation,
                                                                window,
                                                                distChar,
                                                                trigger,
                                                                action,
                                                                allowedLatexs);
    windowDef->setOriginId(windowDetails.origin());

    switch (distrChar.distr()) {
        case SerializableOperator_DistributionCharacteristic_Distribution_Unset:
            return LogicalOperatorFactory::createWindowOperator(windowDef, operatorId)->as<WindowOperatorNode>();
        case SerializableOperator_DistributionCharacteristic_Distribution_Complete:
            return LogicalOperatorFactory::createCentralWindowSpecializedOperator(windowDef, operatorId)
                ->as<CentralWindowOperator>();
        case SerializableOperator_DistributionCharacteristic_Distribution_Combining:
            return LogicalOperatorFactory::createWindowComputationSpecializedOperator(windowDef, operatorId)
                ->as<WindowComputationOperator>();
        case SerializableOperator_DistributionCharacteristic_Distribution_Merging:
            return LogicalOperatorFactory::createSliceMergingSpecializedOperator(windowDef, operatorId)
                ->as<SliceMergingOperator>();
        case SerializableOperator_DistributionCharacteristic_Distribution_Slicing:
            return LogicalOperatorFactory::createSliceCreationSpecializedOperator(windowDef, operatorId)
                ->as<SliceCreationOperator>();
        default: x_NOT_IMPLEMENTED();
    }
}

void OperatorSerializationUtil::serializeJoinOperator(const JoinLogicalOperatorNode& joinOperator,
                                                      SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to JoinLogicalOperatorNode");
    auto joinDetails = SerializableOperator_JoinDetails();
    auto joinDefinition = joinOperator.getJoinDefinition();

    ExpressionSerializationUtil::serializeExpression(joinDefinition->getLeftJoinKey(), joinDetails.mutable_onleftkey());
    ExpressionSerializationUtil::serializeExpression(joinDefinition->getRightJoinKey(), joinDetails.mutable_onrightkey());

    auto windowType = joinDefinition->getWindowType();
    auto timeBasedWindowType = Windowing::WindowType::asTimeBasedWindowType(windowType);
    auto timeCharacteristic = timeBasedWindowType->getTimeCharacteristic();
    auto timeCharacteristicDetails = SerializableOperator_TimeCharacteristic();
    if (timeCharacteristic->getType() == Windowing::TimeCharacteristic::Type::EventTime) {
        timeCharacteristicDetails.set_type(SerializableOperator_TimeCharacteristic_Type_EventTime);
        timeCharacteristicDetails.set_field(timeCharacteristic->getField()->getName());
    } else if (timeCharacteristic->getType() == Windowing::TimeCharacteristic::Type::IngestionTime) {
        timeCharacteristicDetails.set_type(SerializableOperator_TimeCharacteristic_Type_IngestionTime);
    } else {
        x_ERROR("OperatorSerializationUtil: Cant serialize window Time Characteristic");
    }
    if (timeBasedWindowType->getTimeBasedSubWindowType() == Windowing::TimeBasedWindowType::TUMBLINGWINDOW) {
        auto tumblingWindow = std::dynamic_pointer_cast<Windowing::TumblingWindow>(windowType);
        auto tumblingWindowDetails = SerializableOperator_TumblingWindow();
        tumblingWindowDetails.mutable_timecharacteristic()->CopyFrom(timeCharacteristicDetails);
        tumblingWindowDetails.set_size(tumblingWindow->getSize().getTime());
        joinDetails.mutable_windowtype()->PackFrom(tumblingWindowDetails);
    } else if (timeBasedWindowType->getTimeBasedSubWindowType() == Windowing::TimeBasedWindowType::SLIDINGWINDOW) {
        auto slidingWindow = std::dynamic_pointer_cast<Windowing::SlidingWindow>(windowType);
        auto slidingWindowDetails = SerializableOperator_SlidingWindow();
        slidingWindowDetails.mutable_timecharacteristic()->CopyFrom(timeCharacteristicDetails);
        slidingWindowDetails.set_size(slidingWindow->getSize().getTime());
        slidingWindowDetails.set_slide(slidingWindow->getSlide().getTime());
        joinDetails.mutable_windowtype()->PackFrom(slidingWindowDetails);
    } else {
        x_ERROR("OperatorSerializationUtil: Cant serialize window Time Type");
    }

    auto* windowTrigger = joinDetails.mutable_triggerpolicy();
    switch (joinDefinition->getTriggerPolicy()->getPolicyType()) {
        case Windowing::TriggerType::triggerOnTime: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnTime);
            Windowing::OnTimeTriggerDescriptionPtr triggerDesc =
                std::dynamic_pointer_cast<Windowing::OnTimeTriggerPolicyDescription>(joinDefinition->getTriggerPolicy());
            windowTrigger->set_timeinms(triggerDesc->getTriggerTimeInMs());
            break;
        }
        case Windowing::TriggerType::triggerOnRecord: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnRecord);
            break;
        }
        case Windowing::TriggerType::triggerOnBuffer: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnBuffer);
            break;
        }
        case Windowing::TriggerType::triggerOnWatermarkChange: {
            windowTrigger->set_type(SerializableOperator_TriggerPolicy_Type_triggerOnWatermarkChange);
            break;
        }
        default: {
            x_THROW_RUNTIME_ERROR("OperatorSerializationUtil: could not cast aggregation type");
        }
    }

    auto* windowAction = joinDetails.mutable_action();
    switch (joinDefinition->getTriggerAction()->getActionType()) {
        case Join::JoinActionType::LazyxtedLoopJoin: {
            windowAction->set_type(SerializableOperator_JoinDetails_JoinTriggerAction_Type_LazyxtedLoop);
            break;
        }
        default: {
            x_THROW_RUNTIME_ERROR("OperatorSerializationUtil: could not cast action type");
        }
    }

    if (joinDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Complete) {
        joinDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Complete);
    } else if (joinDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Combining) {
        joinDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Combining);
    } else if (joinDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Slicing) {
        joinDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Slicing);
    } else if (joinDefinition->getDistributionType()->getType() == Windowing::DistributionCharacteristic::Type::Merging) {
        joinDetails.mutable_distrchar()->set_distr(SerializableOperator_DistributionCharacteristic_Distribution_Merging);
    } else {
        x_NOT_IMPLEMENTED();
    }

    joinDetails.set_numberofinputedgesleft(joinDefinition->getNumberOfInputEdgesLeft());
    joinDetails.set_numberofinputedgesright(joinDefinition->getNumberOfInputEdgesRight());
    joinDetails.set_windowstartfieldname(joinOperator.getWindowStartFieldName());
    joinDetails.set_windowendfieldname(joinOperator.getWindowEndFieldName());
    joinDetails.set_windowkeyfieldname(joinOperator.getWindowKeyFieldName());
    joinDetails.set_origin(joinOperator.getOutputOriginIds()[0]);

    if (joinDefinition->getJoinType() == Join::LogicalJoinDefinition::JoinType::INNER_JOIN) {
        joinDetails.mutable_jointype()->set_jointype(SerializableOperator_JoinDetails_JoinTypeCharacteristic_JoinType_INNER_JOIN);
    } else if (joinDefinition->getJoinType() == Join::LogicalJoinDefinition::JoinType::CARTESIAN_PRODUCT) {
        joinDetails.mutable_jointype()->set_jointype(
            SerializableOperator_JoinDetails_JoinTypeCharacteristic_JoinType_CARTESIAN_PRODUCT);
    }

    serializedOperator.mutable_details()->PackFrom(joinDetails);
}

JoinLogicalOperatorNodePtr OperatorSerializationUtil::deserializeJoinOperator(const SerializableOperator_JoinDetails& joinDetails,
                                                                              OperatorId operatorId) {
    auto serializedTriggerPolicy = joinDetails.triggerpolicy();
    auto serializedAction = joinDetails.action();

    auto serializedWindowType = joinDetails.windowtype();

    Windowing::WindowTriggerPolicyPtr trigger;
    if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnTime) {
        trigger = Windowing::OnTimeTriggerPolicyDescription::create(serializedTriggerPolicy.timeinms());
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnBuffer) {
        trigger = Windowing::OnBufferTriggerPolicyDescription::create();
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnRecord) {
        trigger = Windowing::OnRecordTriggerPolicyDescription::create();
    } else if (serializedTriggerPolicy.type() == SerializableOperator_TriggerPolicy_Type_triggerOnWatermarkChange) {
        trigger = Windowing::OnWatermarkChangeTriggerPolicyDescription::create();
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize trigger: {}", serializedTriggerPolicy.DebugString());
    }

    auto serializedJoinType = joinDetails.jointype();
    // check which jointype is set
    // default: JoinType::INNER_JOIN
    Join::LogicalJoinDefinition::JoinType joinType = Join::LogicalJoinDefinition::JoinType::INNER_JOIN;
    // with Cartesian Product is set, change join type
    if (serializedJoinType.jointype() == SerializableOperator_JoinDetails_JoinTypeCharacteristic_JoinType_CARTESIAN_PRODUCT) {
        joinType = Join::LogicalJoinDefinition::JoinType::CARTESIAN_PRODUCT;
    }

    Join::BaseJoinActionDescriptorPtr action;
    if (serializedAction.type() == SerializableOperator_JoinDetails_JoinTriggerAction_Type_LazyxtedLoop) {
        action = Join::LazyxtLoopJoinTriggerActionDescriptor::create();
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize action: {}", serializedAction.DebugString());
    }

    Windowing::WindowTypePtr window;
    if (serializedWindowType.Is<SerializableOperator_TumblingWindow>()) {
        auto serializedTumblingWindow = SerializableOperator_TumblingWindow();
        serializedWindowType.UnpackTo(&serializedTumblingWindow);
        auto serializedTimeCharacteristic = serializedTumblingWindow.timecharacteristic();
        if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_EventTime) {
            auto field = Attribute(serializedTimeCharacteristic.field());
            window = Windowing::TumblingWindow::of(Windowing::TimeCharacteristic::createEventTime(field),
                                                   Windowing::TimeMeasure(serializedTumblingWindow.size()));
        } else if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_IngestionTime) {
            window = Windowing::TumblingWindow::of(Windowing::TimeCharacteristic::createIngestionTime(),
                                                   Windowing::TimeMeasure(serializedTumblingWindow.size()));
        } else {
            x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window time characteristic: {}",
                            serializedTimeCharacteristic.DebugString());
        }
    } else if (serializedWindowType.Is<SerializableOperator_SlidingWindow>()) {
        auto serializedSlidingWindow = SerializableOperator_SlidingWindow();
        serializedWindowType.UnpackTo(&serializedSlidingWindow);
        auto serializedTimeCharacteristic = serializedSlidingWindow.timecharacteristic();
        if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_EventTime) {
            auto field = Attribute(serializedTimeCharacteristic.field());
            window = Windowing::SlidingWindow::of(Windowing::TimeCharacteristic::createEventTime(field),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.size()),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.slide()));
        } else if (serializedTimeCharacteristic.type() == SerializableOperator_TimeCharacteristic_Type_IngestionTime) {
            window = Windowing::SlidingWindow::of(Windowing::TimeCharacteristic::createIngestionTime(),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.size()),
                                                  Windowing::TimeMeasure(serializedSlidingWindow.slide()));
        } else {
            x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window time characteristic: {}",
                            serializedTimeCharacteristic.DebugString());
        }
    } else {
        x_FATAL_ERROR("OperatorSerializationUtil: could not de-serialize window type: {}", serializedWindowType.DebugString());
    }

    LogicalOperatorNodePtr ptr;
    auto distChar = Windowing::DistributionCharacteristic::createCompleteWindowType();
    auto leftKeyAccessExpression =
        ExpressionSerializationUtil::deserializeExpression(joinDetails.onleftkey())->as<FieldAccessExpressionNode>();
    auto rightKeyAccessExpression =
        ExpressionSerializationUtil::deserializeExpression(joinDetails.onrightkey())->as<FieldAccessExpressionNode>();
    auto joinDefinition = Join::LogicalJoinDefinition::create(leftKeyAccessExpression,
                                                              rightKeyAccessExpression,
                                                              window,
                                                              distChar,
                                                              trigger,
                                                              action,
                                                              joinDetails.numberofinputedgesleft(),
                                                              joinDetails.numberofinputedgesright(),
                                                              joinType);
    auto joinOperator = LogicalOperatorFactory::createJoinOperator(joinDefinition, operatorId)->as<JoinLogicalOperatorNode>();
    joinOperator->setWindowStartEndKeyFieldName(joinDetails.windowstartfieldname(),
                                                joinDetails.windowendfieldname(),
                                                joinDetails.windowkeyfieldname());
    joinOperator->setOriginId(joinDetails.origin());
    return joinOperator;

    //TODO: enable distrChar for distributed joins
    //    if (distrChar.distr() == SerializableOperator_JoinDetails_DistributionCharacteristic_Distribution_Complete) {
    //        return LogicalOperatorFactory::createCentralWindowSpecializedOperator(windowDef, operatorId)->as<CentralWindowOperator>();
    //    } else if (distrChar.distr() == SerializableOperator_JoinDetails_DistributionCharacteristic_Distribution_Combining) {
    //        return LogicalOperatorFactory::createWindowComputationSpecializedOperator(windowDef, operatorId)->as<WindowComputationOperator>();
    //    } else if (distrChar.distr() == SerializableOperator_JoinDetails_DistributionCharacteristic_Distribution_Slicing) {
    //        return LogicalOperatorFactory::createSliceCreationSpecializedOperator(windowDef, operatorId)->as<SliceCreationOperator>();
    //    } else {
    //        x_NOT_IMPLEMENTED();
    //    }
}

void OperatorSerializationUtil::serializeBatchJoinOperator(const Experimental::BatchJoinLogicalOperatorNode& joinOperator,
                                                           SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to BatchJoinLogicalOperatorNode");

    auto joinDetails = SerializableOperator_BatchJoinDetails();
    auto joinDefinition = joinOperator.getBatchJoinDefinition();

    ExpressionSerializationUtil::serializeExpression(joinDefinition->getBuildJoinKey(), joinDetails.mutable_onbuildkey());
    ExpressionSerializationUtil::serializeExpression(joinDefinition->getProbeJoinKey(), joinDetails.mutable_onprobekey());

    joinDetails.set_numberofinputedgesbuild(joinDefinition->getNumberOfInputEdgesBuild());
    joinDetails.set_numberofinputedgesprobe(joinDefinition->getNumberOfInputEdgesProbe());

    serializedOperator.mutable_details()->PackFrom(joinDetails);
}

Experimental::BatchJoinLogicalOperatorNodePtr
OperatorSerializationUtil::deserializeBatchJoinOperator(const SerializableOperator_BatchJoinDetails& joinDetails,
                                                        OperatorId operatorId) {
    auto buildKeyAccessExpression =
        ExpressionSerializationUtil::deserializeExpression(joinDetails.onbuildkey())->as<FieldAccessExpressionNode>();
    auto probeKeyAccessExpression =
        ExpressionSerializationUtil::deserializeExpression(joinDetails.onprobekey())->as<FieldAccessExpressionNode>();
    auto joinDefinition = Join::Experimental::LogicalBatchJoinDefinition::create(buildKeyAccessExpression,
                                                                                 probeKeyAccessExpression,
                                                                                 joinDetails.numberofinputedgesprobe(),
                                                                                 joinDetails.numberofinputedgesbuild());
    auto retValue = LogicalOperatorFactory::createBatchJoinOperator(joinDefinition, operatorId)
                        ->as<Experimental::BatchJoinLogicalOperatorNode>();
    return retValue;
}

void OperatorSerializationUtil::serializeSourceDescriptor(const SourceDescriptor& sourceDescriptor,
                                                          SerializableOperator_SourceDetails& sourceDetails,
                                                          bool isClientOriginated) {
    x_TRACE("OperatorSerializationUtil:: serialize to SourceDescriptor");
    x_DEBUG("OperatorSerializationUtil:: serialize to SourceDescriptor with ={}", sourceDescriptor.toString());
    if (sourceDescriptor.instanceOf<const ZmqSourceDescriptor>()) {
        // serialize zmq source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableZMQSourceDescriptor");
        auto zmqSourceDescriptor = sourceDescriptor.as<const ZmqSourceDescriptor>();
        auto zmqSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableZMQSourceDescriptor();
        zmqSerializedSourceDescriptor.set_host(zmqSourceDescriptor->getHost());
        zmqSerializedSourceDescriptor.set_port(zmqSourceDescriptor->getPort());
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(zmqSourceDescriptor->getSchema(),
                                                 zmqSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(zmqSerializedSourceDescriptor);
    }
#ifdef ENABLE_MQTT_BUILD
    else if (sourceDescriptor.instanceOf<const MQTTSourceDescriptor>()) {
        // serialize MQTT source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableMQTTSourceDescriptor");
        auto mqttSourceDescriptor = sourceDescriptor.as<const MQTTSourceDescriptor>();
        //init serializable source config
        auto serializedPhysicalSourceType = new SerializablePhysicalSourceType();
        serializedPhysicalSourceType->set_sourcetype(mqttSourceDescriptor->getSourceConfigPtr()->getSourceTypeAsString());
        //init serializable mqtt source config
        auto mqttSerializedSourceConfig = SerializablePhysicalSourceType_SerializableMQTTSourceType();
        mqttSerializedSourceConfig.set_clientid(mqttSourceDescriptor->getSourceConfigPtr()->getClientId()->getValue());
        mqttSerializedSourceConfig.set_url(mqttSourceDescriptor->getSourceConfigPtr()->getUrl()->getValue());
        mqttSerializedSourceConfig.set_username(mqttSourceDescriptor->getSourceConfigPtr()->getUserName()->getValue());
        mqttSerializedSourceConfig.set_topic(mqttSourceDescriptor->getSourceConfigPtr()->getTopic()->getValue());
        mqttSerializedSourceConfig.set_qos(mqttSourceDescriptor->getSourceConfigPtr()->getQos()->getValue());
        mqttSerializedSourceConfig.set_cleansession(mqttSourceDescriptor->getSourceConfigPtr()->getCleanSession()->getValue());
        mqttSerializedSourceConfig.set_flushintervalms(
            mqttSourceDescriptor->getSourceConfigPtr()->getFlushIntervalMS()->getValue());
        switch (mqttSourceDescriptor->getSourceConfigPtr()->getInputFormat()->getValue()) {
            case Configurations::InputFormat::JSON:
                mqttSerializedSourceConfig.set_inputformat(SerializablePhysicalSourceType_InputFormat_JSON);
                break;
            case Configurations::InputFormat::CSV:
                mqttSerializedSourceConfig.set_inputformat(SerializablePhysicalSourceType_InputFormat_CSV);
                break;
        }
        serializedPhysicalSourceType->mutable_specificphysicalsourcetype()->PackFrom(mqttSerializedSourceConfig);
        //init serializable mqtt source descriptor
        auto mqttSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableMQTTSourceDescriptor();
        mqttSerializedSourceDescriptor.set_allocated_physicalsourcetype(serializedPhysicalSourceType);
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(mqttSourceDescriptor->getSchema(),
                                                 mqttSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(mqttSerializedSourceDescriptor);
    }
#endif
#ifdef ENABLE_OPC_BUILD
    else if (sourceDescriptor->instanceOf<OPCSourceDescriptor>()) {
        // serialize opc source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableOPCSourceDescriptor");
        auto opcSourceDescriptor = sourceDescriptor->as<OPCSourceDescriptor>();
        auto opcSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableOPCSourceDescriptor();
        char* ident = (char*) UA_malloc(sizeof(char) * opcSourceDescriptor->getNodeId().identifier.string.length + 1);
        memcpy(ident,
               opcSourceDescriptor->getNodeId().identifier.string.data,
               opcSourceDescriptor->getNodeId().identifier.string.length);
        ident[opcSourceDescriptor->getNodeId().identifier.string.length] = '\0';
        opcSerializedSourceDescriptor.set_identifier(ident);
        opcSerializedSourceDescriptor.set_url(opcSourceDescriptor->getUrl());
        opcSerializedSourceDescriptor.set_namespaceindex(opcSourceDescriptor->getNodeId().namespaceIndex);
        opcSerializedSourceDescriptor.set_identifiertype(opcSourceDescriptor->getNodeId().identifierType);
        opcSerializedSourceDescriptor.set_user(opcSourceDescriptor->getUser());
        opcSerializedSourceDescriptor.set_password(opcSourceDescriptor->getPassword());
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(opcSourceDescriptor->getSchema(),
                                                 opcSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails->mutable_sourcedescriptor()->PackFrom(opcSerializedSourceDescriptor);
    }
#endif
#ifdef ENABLE_ARROW_BUILD
    else if (sourceDescriptor.instanceOf<ArrowSourceDescriptor>()) {
        // serialize arrow source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableArrowSourceDescriptor");
        auto arrowSourceDescriptor = sourceDescriptor.as<const ArrowSourceDescriptor>();
        // init serializable source config
        auto serializedSourceConfig = new SerializablePhysicalSourceType();
        serializedSourceConfig->set_sourcetype(arrowSourceDescriptor->getSourceConfig()->getSourceTypeAsString());
        // init serializable arrow source config
        auto arrowSerializedSourceConfig = SerializablePhysicalSourceType_SerializableArrowSourceType();
        arrowSerializedSourceConfig.set_numberofbufferstoproduce(
            arrowSourceDescriptor->getSourceConfig()->getNumberOfBuffersToProduce()->getValue());
        arrowSerializedSourceConfig.set_numberoftuplestoproduceperbuffer(
            arrowSourceDescriptor->getSourceConfig()->getNumberOfTuplesToProducePerBuffer()->getValue());
        arrowSerializedSourceConfig.set_sourcegatheringinterval(
            arrowSourceDescriptor->getSourceConfig()->getGatheringInterval()->getValue());
        arrowSerializedSourceConfig.set_filepath(arrowSourceDescriptor->getSourceConfig()->getFilePath()->getValue());
        serializedSourceConfig->mutable_specificphysicalsourcetype()->PackFrom(arrowSerializedSourceConfig);
        // init serializable arrow source descriptor
        auto arrowSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableArrowSourceDescriptor();
        arrowSerializedSourceDescriptor.set_allocated_physicalsourcetype(serializedSourceConfig);
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(arrowSourceDescriptor->getSchema(),
                                                 arrowSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(arrowSerializedSourceDescriptor);
    }
#endif
    else if (sourceDescriptor.instanceOf<const TCPSourceDescriptor>()) {
        // serialize TCP source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableTCPSourceDescriptor");
        auto tcpSourceDescriptor = sourceDescriptor.as<const TCPSourceDescriptor>();
        //init serializable source config
        auto serializedPhysicalSourceType = new SerializablePhysicalSourceType();
        serializedPhysicalSourceType->set_sourcetype(tcpSourceDescriptor->getSourceConfig()->getSourceTypeAsString());
        //init serializable tcp source config
        auto tcpSerializedSourceConfig = SerializablePhysicalSourceType_SerializableTCPSourceType();
        tcpSerializedSourceConfig.set_sockethost(tcpSourceDescriptor->getSourceConfig()->getSocketHost()->getValue());
        tcpSerializedSourceConfig.set_socketport(tcpSourceDescriptor->getSourceConfig()->getSocketPort()->getValue());
        tcpSerializedSourceConfig.set_socketdomain(tcpSourceDescriptor->getSourceConfig()->getSocketDomain()->getValue());
        tcpSerializedSourceConfig.set_sockettype(tcpSourceDescriptor->getSourceConfig()->getSocketType()->getValue());
        std::string tupleSeparator;
        tupleSeparator = tcpSourceDescriptor->getSourceConfig()->getTupleSeparator()->getValue();
        tcpSerializedSourceConfig.set_tupleseparator(tupleSeparator);
        tcpSerializedSourceConfig.set_flushintervalms(tcpSourceDescriptor->getSourceConfig()->getFlushIntervalMS()->getValue());
        switch (tcpSourceDescriptor->getSourceConfig()->getInputFormat()->getValue()) {
            case Configurations::InputFormat::JSON:
                tcpSerializedSourceConfig.set_inputformat(SerializablePhysicalSourceType_InputFormat_JSON);
                break;
            case Configurations::InputFormat::CSV:
                tcpSerializedSourceConfig.set_inputformat(SerializablePhysicalSourceType_InputFormat_CSV);
                break;
        }
        switch (tcpSourceDescriptor->getSourceConfig()->getDecideMessageSize()->getValue()) {
            case Configurations::TCPDecideMessageSize::TUPLE_SEPARATOR:
                tcpSerializedSourceConfig.set_tcpdecidemessagesize(
                    SerializablePhysicalSourceType_TCPDecideMessageSize_TUPLE_SEPARATOR);
                break;
            case Configurations::TCPDecideMessageSize::USER_SPECIFIED_BUFFER_SIZE:
                tcpSerializedSourceConfig.set_tcpdecidemessagesize(
                    SerializablePhysicalSourceType_TCPDecideMessageSize_USER_SPECIFIED_BUFFER_SIZE);
                break;
            case Configurations::TCPDecideMessageSize::BUFFER_SIZE_FROM_SOCKET:
                tcpSerializedSourceConfig.set_tcpdecidemessagesize(
                    SerializablePhysicalSourceType_TCPDecideMessageSize_BUFFER_SIZE_FROM_SOCKET);
                break;
        }
        tcpSerializedSourceConfig.set_socketbuffersize(tcpSourceDescriptor->getSourceConfig()->getSocketBufferSize()->getValue());
        tcpSerializedSourceConfig.set_bytesusedforsocketbuffersizetransfer(
            tcpSourceDescriptor->getSourceConfig()->getBytesUsedForSocketBufferSizeTransfer()->getValue());
        serializedPhysicalSourceType->mutable_specificphysicalsourcetype()->PackFrom(tcpSerializedSourceConfig);
        //init serializable tcp source descriptor
        auto tcpSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableTCPSourceDescriptor();
        tcpSerializedSourceDescriptor.set_allocated_physicalsourcetype(serializedPhysicalSourceType);

        // serialize source schema
        SchemaSerializationUtil::serializeSchema(tcpSourceDescriptor->getSchema(),
                                                 tcpSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(tcpSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const MonitoringSourceDescriptor>()) {
        // serialize network source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableNetworkSourceDescriptor");
        auto monitoringSourceDescriptor = sourceDescriptor.as<const MonitoringSourceDescriptor>();
        auto monitoringSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableMonitoringSourceDescriptor();
        auto metricCollectorType = monitoringSourceDescriptor->getMetricCollectorType();
        auto waitTime = monitoringSourceDescriptor->getWaitTime();
        // serialize source schema
        monitoringSerializedSourceDescriptor.set_metriccollectortype(magic_enum::enum_integer(metricCollectorType));
        monitoringSerializedSourceDescriptor.set_waittime(waitTime.count());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(monitoringSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const Network::NetworkSourceDescriptor>()) {
        // serialize network source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableNetworkSourceDescriptor");
        auto networkSourceDescriptor = sourceDescriptor.as<const Network::NetworkSourceDescriptor>();
        auto networkSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableNetworkSourceDescriptor();
        const auto nodeLocation = networkSourceDescriptor->getNodeLocation();
        const auto xPartition = networkSourceDescriptor->getxPartition();
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(networkSourceDescriptor->getSchema(),
                                                 networkSerializedSourceDescriptor.mutable_sourceschema());
        networkSerializedSourceDescriptor.mutable_xpartition()->set_operatorid(xPartition.getOperatorId());
        networkSerializedSourceDescriptor.mutable_xpartition()->set_partitionid(xPartition.getPartitionId());
        networkSerializedSourceDescriptor.mutable_xpartition()->set_queryid(xPartition.getQueryId());
        networkSerializedSourceDescriptor.mutable_xpartition()->set_subpartitionid(xPartition.getSubpartitionId());
        networkSerializedSourceDescriptor.mutable_nodelocation()->set_port(nodeLocation.getPort());
        networkSerializedSourceDescriptor.mutable_nodelocation()->set_hostname(nodeLocation.getHostname());
        networkSerializedSourceDescriptor.mutable_nodelocation()->set_nodeid(nodeLocation.getNodeId());
        auto s = std::chrono::duration_cast<std::chrono::milliseconds>(networkSourceDescriptor->getWaitTime());
        networkSerializedSourceDescriptor.set_waittime(s.count());
        networkSerializedSourceDescriptor.set_retrytimes(networkSourceDescriptor->getRetryTimes());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(networkSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const DefaultSourceDescriptor>()) {
        // serialize default source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableDefaultSourceDescriptor");
        auto defaultSourceDescriptor = sourceDescriptor.as<const DefaultSourceDescriptor>();
        auto defaultSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableDefaultSourceDescriptor();
        defaultSerializedSourceDescriptor.set_sourcegatheringinterval(defaultSourceDescriptor->getSourceGatheringIntervalCount());
        defaultSerializedSourceDescriptor.set_numberofbufferstoproduce(defaultSourceDescriptor->getNumbersOfBufferToProduce());
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(defaultSourceDescriptor->getSchema(),
                                                 defaultSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(defaultSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const BinarySourceDescriptor>()) {
        // serialize binary source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableBinarySourceDescriptor");
        auto binarySourceDescriptor = sourceDescriptor.as<const BinarySourceDescriptor>();
        auto binarySerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableBinarySourceDescriptor();
        binarySerializedSourceDescriptor.set_filepath(binarySourceDescriptor->getFilePath());
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(binarySourceDescriptor->getSchema(),
                                                 binarySerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(binarySerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const CsvSourceDescriptor>()) {
        // serialize csv source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableCsvSourceDescriptor");
        auto csvSourceDescriptor = sourceDescriptor.as<const CsvSourceDescriptor>();
        // init serializable source config
        auto serializedSourceConfig = new SerializablePhysicalSourceType();
        serializedSourceConfig->set_sourcetype(csvSourceDescriptor->getSourceConfig()->getSourceTypeAsString());
        // init serializable csv source config
        auto csvSerializedSourceConfig = SerializablePhysicalSourceType_SerializableCSVSourceType();
        csvSerializedSourceConfig.set_numberofbufferstoproduce(
            csvSourceDescriptor->getSourceConfig()->getNumberOfBuffersToProduce()->getValue());
        csvSerializedSourceConfig.set_numberoftuplestoproduceperbuffer(
            csvSourceDescriptor->getSourceConfig()->getNumberOfTuplesToProducePerBuffer()->getValue());
        csvSerializedSourceConfig.set_sourcegatheringinterval(
            csvSourceDescriptor->getSourceConfig()->getGatheringInterval()->getValue());
        csvSerializedSourceConfig.set_filepath(csvSourceDescriptor->getSourceConfig()->getFilePath()->getValue());
        csvSerializedSourceConfig.set_skipheader(csvSourceDescriptor->getSourceConfig()->getSkipHeader()->getValue());
        csvSerializedSourceConfig.set_delimiter(csvSourceDescriptor->getSourceConfig()->getDelimiter()->getValue());
        serializedSourceConfig->mutable_specificphysicalsourcetype()->PackFrom(csvSerializedSourceConfig);
        // init serializable csv source descriptor
        auto csvSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableCsvSourceDescriptor();
        csvSerializedSourceDescriptor.set_allocated_physicalsourcetype(serializedSourceConfig);
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(csvSourceDescriptor->getSchema(),
                                                 csvSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(csvSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const SenseSourceDescriptor>()) {
        // serialize sense source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableSenseSourceDescriptor");
        auto senseSourceDescriptor = sourceDescriptor.as<const SenseSourceDescriptor>();
        auto senseSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableSenseSourceDescriptor();
        senseSerializedSourceDescriptor.set_udfs(senseSourceDescriptor->getUdfs());
        // serialize source schema
        SchemaSerializationUtil::serializeSchema(senseSourceDescriptor->getSchema(),
                                                 senseSerializedSourceDescriptor.mutable_sourceschema());
        sourceDetails.mutable_sourcedescriptor()->PackFrom(senseSerializedSourceDescriptor);
    } else if (sourceDescriptor.instanceOf<const LogicalSourceDescriptor>()) {
        // serialize logical source descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableLogicalSourceDescriptor");
        auto logicalSourceDescriptor = sourceDescriptor.as<const LogicalSourceDescriptor>();
        auto logicalSourceSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableLogicalSourceDescriptor();
        logicalSourceSerializedSourceDescriptor.set_logicalsourcename(logicalSourceDescriptor->getLogicalSourceName());
        logicalSourceSerializedSourceDescriptor.set_physicalsourcename(logicalSourceDescriptor->getPhysicalSourceName());

        if (!isClientOriginated) {
            // serialize source schema
            SchemaSerializationUtil::serializeSchema(logicalSourceDescriptor->getSchema(),
                                                     logicalSourceSerializedSourceDescriptor.mutable_sourceschema());
        }
        sourceDetails.mutable_sourcedescriptor()->PackFrom(logicalSourceSerializedSourceDescriptor);
    } else {
        x_ERROR("OperatorSerializationUtil: Unknown Source Descriptor Type {}", sourceDescriptor.toString());
        throw std::invalid_argument("Unknown Source Descriptor Type");
    }
}

SourceDescriptorPtr
OperatorSerializationUtil::deserializeSourceDescriptor(const SerializableOperator_SourceDetails& sourceDetails) {
    x_TRACE("OperatorSerializationUtil:: de-serialized SourceDescriptor id={}", sourceDetails.DebugString());
    const auto& serializedSourceDescriptor = sourceDetails.sourcedescriptor();

    if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableZMQSourceDescriptor>()) {
        // de-serialize zmq source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as ZmqSourceDescriptor");
        auto zmqSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableZMQSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&zmqSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(zmqSerializedSourceDescriptor.sourceschema());
        auto ret =
            ZmqSourceDescriptor::create(schema, zmqSerializedSourceDescriptor.host(), zmqSerializedSourceDescriptor.port());
        return ret;
    }
#ifdef ENABLE_MQTT_BUILD
    if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableMQTTSourceDescriptor>()) {
        // de-serialize mqtt source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as MQTTSourceDescriptor");
        auto* mqttSerializedSourceDescriptor = new SerializableOperator_SourceDetails_SerializableMQTTSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(mqttSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(mqttSerializedSourceDescriptor->sourceschema());
        auto sourceConfig = MQTTSourceType::create();
        auto mqttSourceConfig = new SerializablePhysicalSourceType_SerializableMQTTSourceType();
        mqttSerializedSourceDescriptor->physicalsourcetype().specificphysicalsourcetype().UnpackTo(mqttSourceConfig);
        sourceConfig->setUrl(mqttSourceConfig->url());
        sourceConfig->setClientId(mqttSourceConfig->clientid());
        sourceConfig->setUserName(mqttSourceConfig->username());
        sourceConfig->setTopic(mqttSourceConfig->topic());
        sourceConfig->setQos(mqttSourceConfig->qos());
        sourceConfig->setCleanSession(mqttSourceConfig->cleansession());
        sourceConfig->setFlushIntervalMS(mqttSourceConfig->flushintervalms());
        sourceConfig->setInputFormat(static_cast<Configurations::InputFormat>(mqttSourceConfig->inputformat()));
        auto ret = MQTTSourceDescriptor::create(schema, sourceConfig);
        return ret;
    }

#endif
#ifdef ENABLE_OPC_BUILD
    else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableOPCSourceDescriptor>()) {
        // de-serialize opc source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as OPCSourceDescriptor");
        auto opcSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableOPCSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&opcSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(opcSerializedSourceDescriptor.sourceschema());
        char* ident = (char*) UA_malloc(sizeof(char) * opcSerializedSourceDescriptor.identifier().length() + 1);
        memcpy(ident, opcSerializedSourceDescriptor.identifier().data(), opcSerializedSourceDescriptor.identifier().length());
        ident[opcSerializedSourceDescriptor.identifier().length()] = '\0';
        UA_NodeId nodeId = UA_NODEID_STRING(opcSerializedSourceDescriptor.namespaceindex(), ident);
        auto ret = OPCSourceDescriptor::create(schema,
                                               opcSerializedSourceDescriptor.url(),
                                               nodeId,
                                               opcSerializedSourceDescriptor.user(),
                                               opcSerializedSourceDescriptor.password());
        return ret;
    }
#endif
    else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableTCPSourceDescriptor>()) {
        // de-serialize tcp source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as TCPSourceDescriptor");
        auto* tcpSerializedSourceDescriptor = new SerializableOperator_SourceDetails_SerializableTCPSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(tcpSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(tcpSerializedSourceDescriptor->sourceschema());
        auto sourceConfig = TCPSourceType::create();
        auto tcpSourceConfig = new SerializablePhysicalSourceType_SerializableTCPSourceType();
        tcpSerializedSourceDescriptor->physicalsourcetype().specificphysicalsourcetype().UnpackTo(tcpSourceConfig);
        sourceConfig->setSocketHost(tcpSourceConfig->sockethost());
        sourceConfig->setSocketPort(tcpSourceConfig->socketport());
        sourceConfig->setSocketDomain(tcpSourceConfig->socketdomain());
        sourceConfig->setSocketType(tcpSourceConfig->sockettype());
        sourceConfig->setFlushIntervalMS(tcpSourceConfig->flushintervalms());
        sourceConfig->setInputFormat(static_cast<Configurations::InputFormat>(tcpSourceConfig->inputformat()));
        sourceConfig->setDecideMessageSize(
            static_cast<Configurations::TCPDecideMessageSize>(tcpSourceConfig->tcpdecidemessagesize()));
        sourceConfig->setTupleSeparator(tcpSourceConfig->tupleseparator().at(0));
        sourceConfig->setSocketBufferSize(tcpSourceConfig->socketbuffersize());
        sourceConfig->setBytesUsedForSocketBufferSizeTransfer(tcpSourceConfig->bytesusedforsocketbuffersizetransfer());
        auto ret = TCPSourceDescriptor::create(schema, sourceConfig);
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableMonitoringSourceDescriptor>()) {
        // de-serialize zmq source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as monitoringSourceDescriptor");
        auto monitoringSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableMonitoringSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&monitoringSerializedSourceDescriptor);
        // de-serialize source schema
        auto waitTime = std::chrono::milliseconds(monitoringSerializedSourceDescriptor.waittime());
        auto metricCollectorType = monitoringSerializedSourceDescriptor.metriccollectortype();
        auto ret = MonitoringSourceDescriptor::create(waitTime, Monitoring::MetricCollectorType(metricCollectorType));
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableNetworkSourceDescriptor>()) {
        // de-serialize zmq source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as NetworkSourceDescriptor");
        auto networkSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableNetworkSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&networkSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(networkSerializedSourceDescriptor.sourceschema());
        Network::xPartition xPartition{networkSerializedSourceDescriptor.xpartition().queryid(),
                                           networkSerializedSourceDescriptor.xpartition().operatorid(),
                                           networkSerializedSourceDescriptor.xpartition().partitionid(),
                                           networkSerializedSourceDescriptor.xpartition().subpartitionid()};
        x::Network::NodeLocation nodeLocation(networkSerializedSourceDescriptor.nodelocation().nodeid(),
                                                networkSerializedSourceDescriptor.nodelocation().hostname(),
                                                networkSerializedSourceDescriptor.nodelocation().port());
        auto waitTime = std::chrono::milliseconds(networkSerializedSourceDescriptor.waittime());
        auto ret = Network::NetworkSourceDescriptor::create(schema,
                                                            xPartition,
                                                            nodeLocation,
                                                            waitTime,
                                                            networkSerializedSourceDescriptor.retrytimes());
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableDefaultSourceDescriptor>()) {
        // de-serialize default source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as DefaultSourceDescriptor");
        auto defaultSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableDefaultSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&defaultSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(defaultSerializedSourceDescriptor.sourceschema());
        auto ret = DefaultSourceDescriptor::create(schema,
                                                   defaultSerializedSourceDescriptor.numberofbufferstoproduce(),
                                                   defaultSerializedSourceDescriptor.sourcegatheringinterval());
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableBinarySourceDescriptor>()) {
        // de-serialize binary source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as BinarySourceDescriptor");
        auto binarySerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableBinarySourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&binarySerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(binarySerializedSourceDescriptor.sourceschema());
        auto ret = BinarySourceDescriptor::create(schema, binarySerializedSourceDescriptor.filepath());
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableCsvSourceDescriptor>()) {
        // de-serialize csv source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as CsvSourceDescriptor");
        auto csvSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableCsvSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&csvSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(csvSerializedSourceDescriptor.sourceschema());
        auto sourceConfig = CSVSourceType::create();
        auto csvSourceConfig = new SerializablePhysicalSourceType_SerializableCSVSourceType();
        csvSerializedSourceDescriptor.physicalsourcetype().specificphysicalsourcetype().UnpackTo(csvSourceConfig);
        sourceConfig->setFilePath(csvSourceConfig->filepath());
        sourceConfig->setSkipHeader(csvSourceConfig->skipheader());
        sourceConfig->setDelimiter(csvSourceConfig->delimiter());
        sourceConfig->setGatheringInterval(csvSourceConfig->sourcegatheringinterval());
        sourceConfig->setGatheringMode(static_cast<GatheringMode>(csvSourceConfig->sourcegatheringmode()));
        sourceConfig->setNumberOfBuffersToProduce(csvSourceConfig->numberofbufferstoproduce());
        sourceConfig->setNumberOfTuplesToProducePerBuffer(csvSourceConfig->numberoftuplestoproduceperbuffer());
        auto ret = CsvSourceDescriptor::create(schema, sourceConfig);
        return ret;
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableSenseSourceDescriptor>()) {
        // de-serialize sense source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as SenseSourceDescriptor");
        auto senseSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableSenseSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&senseSerializedSourceDescriptor);
        // de-serialize source schema
        auto schema = SchemaSerializationUtil::deserializeSchema(senseSerializedSourceDescriptor.sourceschema());
        return SenseSourceDescriptor::create(schema, senseSerializedSourceDescriptor.udfs());
    } else if (serializedSourceDescriptor.Is<SerializableOperator_SourceDetails_SerializableLogicalSourceDescriptor>()) {
        // de-serialize logical source descriptor
        x_DEBUG("OperatorSerializationUtil:: de-serialized SourceDescriptor as LogicalSourceDescriptor");
        auto logicalSourceSerializedSourceDescriptor = SerializableOperator_SourceDetails_SerializableLogicalSourceDescriptor();
        serializedSourceDescriptor.UnpackTo(&logicalSourceSerializedSourceDescriptor);

        // de-serialize source schema
        SourceDescriptorPtr logicalSourceDescriptor =
            LogicalSourceDescriptor::create(logicalSourceSerializedSourceDescriptor.logicalsourcename());
        logicalSourceDescriptor->setPhysicalSourceName(logicalSourceSerializedSourceDescriptor.physicalsourcename());
        // check if the schema is set
        if (logicalSourceSerializedSourceDescriptor.has_sourceschema()) {
            auto schema = SchemaSerializationUtil::deserializeSchema(logicalSourceSerializedSourceDescriptor.sourceschema());
            logicalSourceDescriptor->setSchema(schema);
        }

        return logicalSourceDescriptor;
    } else {
        x_ERROR("OperatorSerializationUtil: Unknown Source Descriptor Type {}", serializedSourceDescriptor.type_url());
        throw std::invalid_argument("Unknown Source Descriptor Type");
    }
}

void OperatorSerializationUtil::serializeSinkDescriptor(const SinkDescriptor& sinkDescriptor,
                                                        SerializableOperator_SinkDetails& sinkDetails,
                                                        uint64_t numberOfOrigins) {
    // serialize a sink descriptor and all its properties depending on its type
    x_DEBUG("OperatorSerializationUtil:: serialized SinkDescriptor ");
    if (sinkDescriptor.instanceOf<const PrintSinkDescriptor>()) {
        // serialize print sink descriptor
        auto printSinkDescriptor = sinkDescriptor.as<const PrintSinkDescriptor>();
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializablePrintSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializablePrintSinkDescriptor();
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(printSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const NullOutputSinkDescriptor>()) {
        auto nullSinkDescriptor = sinkDescriptor.as<const NullOutputSinkDescriptor>();
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableNullOutputSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableNullOutputSinkDescriptor();
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(nullSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const ZmqSinkDescriptor>()) {
        // serialize zmq sink descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableZMQSinkDescriptor");
        auto zmqSinkDescriptor = sinkDescriptor.as<const ZmqSinkDescriptor>();
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableZMQSinkDescriptor();
        serializedSinkDescriptor.set_port(zmqSinkDescriptor->getPort());
        serializedSinkDescriptor.set_isinternal(zmqSinkDescriptor->isInternal());
        serializedSinkDescriptor.set_host(zmqSinkDescriptor->getHost());
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(zmqSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const MonitoringSinkDescriptor>()) {
        // serialize Monitoring sink descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableMonitoringSinkDescriptor");
        auto monitoringSinkDescriptor = sinkDescriptor.as<const MonitoringSinkDescriptor>();
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMonitoringSinkDescriptor();
        serializedSinkDescriptor.set_collectortype(magic_enum::enum_integer(monitoringSinkDescriptor->getCollectorType()));
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(monitoringSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    }

#ifdef ENABLE_OPC_BUILD
    else if (sinkDescriptor.instanceOf<const OPCSinkDescriptor>()) {
        // serialize opc sink descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableOPCSinkDescriptor");
        auto opcSinkDescriptor = sinkDescriptor->as<OPCSinkDescriptor>();
        auto opcSerializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableOPCSinkDescriptor();
        char* ident = (char*) UA_malloc(sizeof(char) * opcSinkDescriptor->getNodeId().identifier.string.length + 1);
        memcpy(ident,
               opcSinkDescriptor->getNodeId().identifier.string.data,
               opcSinkDescriptor->getNodeId().identifier.string.length);
        ident[opcSinkDescriptor->getNodeId().identifier.string.length] = '\0';
        opcSerializedSinkDescriptor.set_identifier(ident);
        free(ident);
        opcSerializedSinkDescriptor.set_url(opcSinkDescriptor->getUrl());
        opcSerializedSinkDescriptor.set_namespaceindex(opcSinkDescriptor->getNodeId().namespaceIndex);
        opcSerializedSinkDescriptor.set_identifiertype(opcSinkDescriptor->getNodeId().identifierType);
        opcSerializedSinkDescriptor.set_user(opcSinkDescriptor->getUser());
        opcSerializedSinkDescriptor.set_password(opcSinkDescriptor->getPassword());
        sinkDetails->mutable_sinkdescriptor()->PackFrom(opcSerializedSinkDescriptor);
    }
#endif
    else if (sinkDescriptor.instanceOf<const MQTTSinkDescriptor>()) {
        // serialize MQTT sink descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SourceDescriptor as "
                  "SerializableOperator_SourceDetails_SerializableMQTTSourceDescriptor");
        auto mqttSinkDescriptor = sinkDescriptor.as<const MQTTSinkDescriptor>();
        auto mqttSerializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMQTTSinkDescriptor();
        mqttSerializedSinkDescriptor.set_address(mqttSinkDescriptor->getAddress());
        mqttSerializedSinkDescriptor.set_clientid(mqttSinkDescriptor->getClientId());
        mqttSerializedSinkDescriptor.set_topic(mqttSinkDescriptor->getTopic());
        mqttSerializedSinkDescriptor.set_user(mqttSinkDescriptor->getUser());
        mqttSerializedSinkDescriptor.set_maxbufferedmsgs(mqttSinkDescriptor->getMaxBufferedMSGs());
        mqttSerializedSinkDescriptor.set_timeunit(
            (SerializableOperator_SinkDetails_SerializableMQTTSinkDescriptor_TimeUnits) mqttSinkDescriptor->getTimeUnit());
        mqttSerializedSinkDescriptor.set_msgdelay(mqttSinkDescriptor->getMsgDelay());
        mqttSerializedSinkDescriptor.set_asynchronousclient(mqttSinkDescriptor->getAsynchronousClient());

        sinkDetails.mutable_sinkdescriptor()->PackFrom(mqttSerializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(mqttSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const Network::NetworkSinkDescriptor>()) {
        // serialize zmq sink descriptor
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableNetworkSinkDescriptor");
        auto networkSinkDescriptor = sinkDescriptor.as<const Network::NetworkSinkDescriptor>();
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableNetworkSinkDescriptor();
        //set details of xPartition
        auto* serializedxPartition = serializedSinkDescriptor.mutable_xpartition();
        auto xPartition = networkSinkDescriptor->getxPartition();
        serializedxPartition->set_queryid(xPartition.getQueryId());
        serializedxPartition->set_operatorid(xPartition.getOperatorId());
        serializedxPartition->set_partitionid(xPartition.getPartitionId());
        serializedxPartition->set_subpartitionid(xPartition.getSubpartitionId());
        //set details of NodeLocation
        auto* serializedNodeLocation = serializedSinkDescriptor.mutable_nodelocation();
        auto nodeLocation = networkSinkDescriptor->getNodeLocation();
        serializedNodeLocation->set_nodeid(nodeLocation.getNodeId());
        serializedNodeLocation->set_hostname(nodeLocation.getHostname());
        serializedNodeLocation->set_port(nodeLocation.getPort());
        // set reconnection details
        auto s = std::chrono::duration_cast<std::chrono::milliseconds>(networkSinkDescriptor->getWaitTime());
        serializedSinkDescriptor.set_waittime(s.count());
        serializedSinkDescriptor.set_retrytimes(networkSinkDescriptor->getRetryTimes());
        //set unique network sink partition id. Take care to ue this value during deserialization as well!
        serializedSinkDescriptor.set_uniquenetworksinkdescriptorid(networkSinkDescriptor->getUniqueNetworkSinkDescriptorId());
        //pack to output
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(networkSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const FileSinkDescriptor>()) {
        // serialize file sink descriptor. The file sink has different types which have to be set correctly
        x_TRACE("OperatorSerializationUtil:: serialized SinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableFileSinkDescriptor");
        auto fileSinkDescriptor = sinkDescriptor.as<const FileSinkDescriptor>();
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableFileSinkDescriptor();

        serializedSinkDescriptor.set_filepath(fileSinkDescriptor->getFileName());
        serializedSinkDescriptor.set_append(fileSinkDescriptor->getAppend());
        serializedSinkDescriptor.set_addtimestamp(fileSinkDescriptor->getAddTimestamp());

        auto format = fileSinkDescriptor->getSinkFormatAsString();
        if (format == "JSON_FORMAT") {
            serializedSinkDescriptor.set_sinkformat("JSON_FORMAT");
        } else if (format == "CSV_FORMAT") {
            serializedSinkDescriptor.set_sinkformat("CSV_FORMAT");
        } else if (format == "x_FORMAT") {
            serializedSinkDescriptor.set_sinkformat("x_FORMAT");
        } else if (format == "TEXT_FORMAT") {
            serializedSinkDescriptor.set_sinkformat("TEXT_FORMAT");
        } else {
            x_ERROR("serializeSinkDescriptor: format not supported");
        }
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(fileSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else if (sinkDescriptor.instanceOf<const Experimental::MaterializedView::MaterializedViewSinkDescriptor>()) {
        x_TRACE("OperatorSerializationUtil:: serialized MaterializedViewSinkDescriptor as "
                  "SerializableOperator_SinkDetails_SerializableMaterializedViewSinkDescriptor");
        auto materializedViewSinkDescriptor =
            sinkDescriptor.as<const Experimental::MaterializedView::MaterializedViewSinkDescriptor>();
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMaterializedViewSinkDescriptor();
        serializedSinkDescriptor.set_viewid(materializedViewSinkDescriptor->getViewId());
        sinkDetails.mutable_sinkdescriptor()->PackFrom(serializedSinkDescriptor);
        sinkDetails.set_faulttolerancemode(static_cast<uint64_t>(materializedViewSinkDescriptor->getFaultToleranceType()));
        sinkDetails.set_numberoforiginids(numberOfOrigins);
    } else {

        x_ERROR("OperatorSerializationUtil: Unknown Sink Descriptor Type - {}", sinkDescriptor.toString());
        throw std::invalid_argument("Unknown Sink Descriptor Type");
    }
}

SinkDescriptorPtr OperatorSerializationUtil::deserializeSinkDescriptor(const SerializableOperator_SinkDetails& sinkDetails) {
    // de-serialize a sink descriptor and all its properties to a SinkDescriptor.
    x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor {}", sinkDetails.DebugString());
    const auto& deserializedSinkDescriptor = sinkDetails.sinkdescriptor();
    const auto deserializedFaultTolerance = sinkDetails.faulttolerancemode();
    const auto deserializedNumberOfOrigins = sinkDetails.numberoforiginids();
    if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializablePrintSinkDescriptor>()) {
        // de-serialize print sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as PrintSinkDescriptor");
        return PrintSinkDescriptor::create(FaultToleranceType(deserializedFaultTolerance), deserializedNumberOfOrigins);
    }
    if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableNullOutputSinkDescriptor>()) {
        // de-serialize print sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as PrintSinkDescriptor");
        return NullOutputSinkDescriptor::create(FaultToleranceType(deserializedFaultTolerance), deserializedNumberOfOrigins);
    } else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableZMQSinkDescriptor>()) {
        // de-serialize zmq sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as ZmqSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableZMQSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        return ZmqSinkDescriptor::create(serializedSinkDescriptor.host(),
                                         serializedSinkDescriptor.port(),
                                         serializedSinkDescriptor.isinternal(),
                                         FaultToleranceType(deserializedFaultTolerance),
                                         deserializedNumberOfOrigins);
    } else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableMonitoringSinkDescriptor>()) {
        // de-serialize zmq sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as MonitoringSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMonitoringSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        return MonitoringSinkDescriptor::create(Monitoring::MetricCollectorType(serializedSinkDescriptor.collectortype()),
                                                FaultToleranceType(deserializedFaultTolerance),
                                                deserializedNumberOfOrigins);
    }
#ifdef ENABLE_OPC_BUILD
    else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableOPCSinkDescriptor>()) {
        // de-serialize opc sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as OPCSinkDescriptor");
        auto opcSerializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableOPCSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&opcSerializedSinkDescriptor);
        char* ident = (char*) UA_malloc(sizeof(char) * opcSerializedSinkDescriptor.identifier().length() + 1);
        memcpy(ident, opcSerializedSinkDescriptor.identifier().data(), opcSerializedSinkDescriptor.identifier().length());
        ident[opcSerializedSinkDescriptor.identifier().length()] = '\0';
        UA_NodeId nodeId = UA_NODEID_STRING(opcSerializedSinkDescriptor.namespaceindex(), ident);
        return OPCSinkDescriptor::create(opcSerializedSinkDescriptor.url(),
                                         nodeId,
                                         opcSerializedSinkDescriptor.user(),
                                         opcSerializedSinkDescriptor.password());
    }
#endif

    else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableMQTTSinkDescriptor>()) {
        // de-serialize MQTT sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as MQTTSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMQTTSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        return MQTTSinkDescriptor::create(std::string{serializedSinkDescriptor.address()},
                                          std::string{serializedSinkDescriptor.topic()},
                                          std::string{serializedSinkDescriptor.user()},
                                          serializedSinkDescriptor.maxbufferedmsgs(),
                                          (MQTTSinkDescriptor::TimeUnits) serializedSinkDescriptor.timeunit(),
                                          serializedSinkDescriptor.msgdelay(),
                                          (MQTTSinkDescriptor::ServiceQualities) serializedSinkDescriptor.qualityofservice(),
                                          serializedSinkDescriptor.asynchronousclient(),
                                          std::string{serializedSinkDescriptor.clientid()},
                                          FaultToleranceType(deserializedFaultTolerance),
                                          deserializedNumberOfOrigins);
    } else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableNetworkSinkDescriptor>()) {
        // de-serialize zmq sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as NetworkSinkDescriptor");
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableNetworkSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        Network::xPartition xPartition{serializedSinkDescriptor.xpartition().queryid(),
                                           serializedSinkDescriptor.xpartition().operatorid(),
                                           serializedSinkDescriptor.xpartition().partitionid(),
                                           serializedSinkDescriptor.xpartition().subpartitionid()};
        Network::NodeLocation nodeLocation{serializedSinkDescriptor.nodelocation().nodeid(),
                                           serializedSinkDescriptor.nodelocation().hostname(),
                                           serializedSinkDescriptor.nodelocation().port()};
        auto waitTime = std::chrono::milliseconds(serializedSinkDescriptor.waittime());
        return Network::NetworkSinkDescriptor::create(nodeLocation,
                                                      xPartition,
                                                      waitTime,
                                                      serializedSinkDescriptor.retrytimes(),
                                                      FaultToleranceType(deserializedFaultTolerance),
                                                      deserializedNumberOfOrigins,
                                                      serializedSinkDescriptor.uniquenetworksinkdescriptorid());
    } else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableFileSinkDescriptor>()) {
        // de-serialize file sink descriptor
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableFileSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as FileSinkDescriptor");
        return FileSinkDescriptor::create(serializedSinkDescriptor.filepath(),
                                          serializedSinkDescriptor.sinkformat(),
                                          serializedSinkDescriptor.append() ? "APPEND" : "OVERWRITE",
                                          serializedSinkDescriptor.addtimestamp(),
                                          FaultToleranceType(deserializedFaultTolerance),
                                          deserializedNumberOfOrigins);
    } else if (deserializedSinkDescriptor.Is<SerializableOperator_SinkDetails_SerializableMaterializedViewSinkDescriptor>()) {
        // de-serialize materialized view sink descriptor
        auto serializedSinkDescriptor = SerializableOperator_SinkDetails_SerializableMaterializedViewSinkDescriptor();
        deserializedSinkDescriptor.UnpackTo(&serializedSinkDescriptor);
        x_TRACE("OperatorSerializationUtil:: de-serialized SinkDescriptor as MaterializedViewSinkDescriptor");
        return Experimental::MaterializedView::MaterializedViewSinkDescriptor::create(
            serializedSinkDescriptor.viewid(),
            FaultToleranceType(deserializedFaultTolerance),
            deserializedNumberOfOrigins);
    } else {
        x_ERROR("OperatorSerializationUtil: Unknown sink Descriptor Type {}", sinkDetails.DebugString());
        throw std::invalid_argument("Unknown Sink Descriptor Type");
    }
}

void OperatorSerializationUtil::serializeLimitOperator(const LimitLogicalOperatorNode& limitOperator,
                                                       SerializableOperator& serializedOperator) {

    x_TRACE("OperatorSerializationUtil:: serialize limit operator ");

    auto limitDetails = SerializableOperator_LimitDetails();
    limitDetails.set_limit(limitOperator.getLimit());
    serializedOperator.mutable_details()->PackFrom(limitDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeLimitOperator(const SerializableOperator_LimitDetails& limitDetails) {
    return LogicalOperatorFactory::createLimitOperator(limitDetails.limit(), Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeWatermarkAssignerOperator(
    const WatermarkAssignerLogicalOperatorNode& watermarkAssignerOperator,
    SerializableOperator& serializedOperator) {

    x_TRACE("OperatorSerializationUtil:: serialize watermark assigner operator ");

    auto watermarkStrategyDetails = SerializableOperator_WatermarkStrategyDetails();
    auto watermarkStrategyDescriptor = watermarkAssignerOperator.getWatermarkStrategyDescriptor();
    serializeWatermarkStrategyDescriptor(*watermarkStrategyDescriptor, watermarkStrategyDetails);
    serializedOperator.mutable_details()->PackFrom(watermarkStrategyDetails);
}

LogicalUnaryOperatorNodePtr OperatorSerializationUtil::deserializeWatermarkAssignerOperator(
    const SerializableOperator_WatermarkStrategyDetails& watermarkStrategyDetails) {

    auto watermarkStrategyDescriptor = deserializeWatermarkStrategyDescriptor(watermarkStrategyDetails);
    return LogicalOperatorFactory::createWatermarkAssignerOperator(watermarkStrategyDescriptor, Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeWatermarkStrategyDescriptor(
    const Windowing::WatermarkStrategyDescriptor& watermarkStrategyDescriptor,
    SerializableOperator_WatermarkStrategyDetails& watermarkStrategyDetails) {
    x_TRACE("OperatorSerializationUtil:: serialize watermark strategy ");

    if (watermarkStrategyDescriptor.instanceOf<const Windowing::EventTimeWatermarkStrategyDescriptor>()) {
        auto eventTimeWatermarkStrategyDescriptor =
            watermarkStrategyDescriptor.as<const Windowing::EventTimeWatermarkStrategyDescriptor>();
        auto serializedWatermarkStrategyDescriptor =
            SerializableOperator_WatermarkStrategyDetails_SerializableEventTimeWatermarkStrategyDescriptor();
        ExpressionSerializationUtil::serializeExpression(eventTimeWatermarkStrategyDescriptor->getOnField(),
                                                         serializedWatermarkStrategyDescriptor.mutable_onfield());
        serializedWatermarkStrategyDescriptor.set_allowedlatexs(
            eventTimeWatermarkStrategyDescriptor->getAllowedLatexs().getTime());
        serializedWatermarkStrategyDescriptor.set_multiplier(eventTimeWatermarkStrategyDescriptor->getTimeUnit().getMultiplier());
        watermarkStrategyDetails.mutable_strategy()->PackFrom(serializedWatermarkStrategyDescriptor);
    } else if (watermarkStrategyDescriptor.instanceOf<const Windowing::IngestionTimeWatermarkStrategyDescriptor>()) {
        auto serializedWatermarkStrategyDescriptor =
            SerializableOperator_WatermarkStrategyDetails_SerializableIngestionTimeWatermarkStrategyDescriptor();
        watermarkStrategyDetails.mutable_strategy()->PackFrom(serializedWatermarkStrategyDescriptor);
    } else {
        x_ERROR("OperatorSerializationUtil: Unknown Watermark Strategy Descriptor Type");
        throw std::invalid_argument("Unknown Watermark Strategy Descriptor Type");
    }
}

Windowing::WatermarkStrategyDescriptorPtr OperatorSerializationUtil::deserializeWatermarkStrategyDescriptor(
    const SerializableOperator_WatermarkStrategyDetails& watermarkStrategyDetails) {

    x_TRACE("OperatorSerializationUtil:: de-serialize watermark strategy ");
    const auto& deserializedWatermarkStrategyDescriptor = watermarkStrategyDetails.strategy();
    if (deserializedWatermarkStrategyDescriptor
            .Is<SerializableOperator_WatermarkStrategyDetails_SerializableEventTimeWatermarkStrategyDescriptor>()) {
        // de-serialize print sink descriptor
        x_TRACE("OperatorSerializationUtil:: de-serialized WatermarkStrategy as EventTimeWatermarkStrategyDescriptor");
        auto serializedEventTimeWatermarkStrategyDescriptor =
            SerializableOperator_WatermarkStrategyDetails_SerializableEventTimeWatermarkStrategyDescriptor();
        deserializedWatermarkStrategyDescriptor.UnpackTo(&serializedEventTimeWatermarkStrategyDescriptor);

        auto onField =
            ExpressionSerializationUtil::deserializeExpression(serializedEventTimeWatermarkStrategyDescriptor.onfield())
                ->as<FieldAccessExpressionNode>();
        x_DEBUG("OperatorSerializationUtil:: deserialized field name {}", onField->getFieldName());
        auto eventTimeWatermarkStrategyDescriptor = Windowing::EventTimeWatermarkStrategyDescriptor::create(
            Attribute(onField->getFieldName()),
            Windowing::TimeMeasure(serializedEventTimeWatermarkStrategyDescriptor.allowedlatexs()),
            Windowing::TimeUnit(serializedEventTimeWatermarkStrategyDescriptor.multiplier()));
        return eventTimeWatermarkStrategyDescriptor;
    }
    if (deserializedWatermarkStrategyDescriptor
            .Is<SerializableOperator_WatermarkStrategyDetails_SerializableIngestionTimeWatermarkStrategyDescriptor>()) {
        return Windowing::IngestionTimeWatermarkStrategyDescriptor::create();
    } else {
        x_ERROR("OperatorSerializationUtil: Unknown Serialized Watermark Strategy Descriptor Type");
        throw std::invalid_argument("Unknown Serialized Watermark Strategy Descriptor Type");
    }
}

void OperatorSerializationUtil::serializeInputSchema(const OperatorNodePtr& operatorNode,
                                                     SerializableOperator& serializedOperator) {

    x_TRACE("OperatorSerializationUtil:: serialize input schema");
    if (!operatorNode->isBinaryOperator()) {
        if (operatorNode->isExchangeOperator()) {
            SchemaSerializationUtil::serializeSchema(operatorNode->as<ExchangeOperatorNode>()->getInputSchema(),
                                                     serializedOperator.mutable_inputschema());
        } else {
            SchemaSerializationUtil::serializeSchema(operatorNode->as<UnaryOperatorNode>()->getInputSchema(),
                                                     serializedOperator.mutable_inputschema());
        }
    } else {
        SchemaSerializationUtil::serializeSchema(operatorNode->as<BinaryOperatorNode>()->getLeftInputSchema(),
                                                 serializedOperator.mutable_leftinputschema());
        SchemaSerializationUtil::serializeSchema(operatorNode->as<BinaryOperatorNode>()->getRightInputSchema(),
                                                 serializedOperator.mutable_rightinputschema());
    }
}

void OperatorSerializationUtil::deserializeInputSchema(LogicalOperatorNodePtr operatorNode,
                                                       SerializableOperator& serializedOperator) {
    // de-serialize operator input schema
    if (!operatorNode->isBinaryOperator()) {
        if (operatorNode->isExchangeOperator()) {
            operatorNode->as<ExchangeOperatorNode>()->setInputSchema(
                SchemaSerializationUtil::deserializeSchema(serializedOperator.inputschema()));
        } else {
            operatorNode->as<UnaryOperatorNode>()->setInputSchema(
                SchemaSerializationUtil::deserializeSchema(serializedOperator.inputschema()));
        }
    } else {
        operatorNode->as<BinaryOperatorNode>()->setLeftInputSchema(
            SchemaSerializationUtil::deserializeSchema(serializedOperator.leftinputschema()));
        operatorNode->as<BinaryOperatorNode>()->setRightInputSchema(
            SchemaSerializationUtil::deserializeSchema(serializedOperator.rightinputschema()));
    }
}

void OperatorSerializationUtil::serializeInferModelOperator(const InferModel::InferModelLogicalOperatorNode& inferModelOperator,
                                                            SerializableOperator& serializedOperator) {

    // serialize infer model operator
    x_TRACE("OperatorSerializationUtil:: serialize to InferModelLogicalOperatorNode");
    auto inferModelDetails = SerializableOperator_InferModelDetails();

    for (auto& exp : inferModelOperator.getInputFields()) {
        auto* mutableInputFields = inferModelDetails.mutable_inputfields()->Add();
        ExpressionSerializationUtil::serializeExpression(exp->getExpressionNode(), mutableInputFields);
    }
    for (auto& exp : inferModelOperator.getOutputFields()) {
        auto* mutableOutputFields = inferModelDetails.mutable_outputfields()->Add();
        ExpressionSerializationUtil::serializeExpression(exp->getExpressionNode(), mutableOutputFields);
    }

    inferModelDetails.set_mlfilename(inferModelOperator.getDeployedModelPath());
    std::ifstream input(inferModelOperator.getModel(), std::ios::binary);

    std::string bytes((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
    input.close();
    inferModelDetails.set_mlfilecontent(bytes);

    serializedOperator.mutable_details()->PackFrom(inferModelDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeInferModelOperator(const SerializableOperator_InferModelDetails& inferModelDetails) {
    std::vector<ExpressionItemPtr> inputFields;
    std::vector<ExpressionItemPtr> outputFields;

    for (auto serializedInputField : inferModelDetails.inputfields()) {
        auto inputField = ExpressionSerializationUtil::deserializeExpression(serializedInputField);
        inputFields.push_back(std::make_shared<ExpressionItem>(inputField));
    }
    for (auto serializedOutputField : inferModelDetails.outputfields()) {
        auto outputField = ExpressionSerializationUtil::deserializeExpression(serializedOutputField);
        outputFields.push_back(std::make_shared<ExpressionItem>(outputField));
    }

    auto content = inferModelDetails.mlfilecontent();
    std::ofstream output(inferModelDetails.mlfilename(), std::ios::binary);
    output << content;
    output.close();

    return LogicalOperatorFactory::createInferModelOperator(inferModelDetails.mlfilename(),
                                                            inputFields,
                                                            outputFields,
                                                            Util::getNextOperatorId());
}

void OperatorSerializationUtil::serializeCEPIterationOperator(const IterationLogicalOperatorNode& iterationOperator,
                                                              SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to IterationLogicalOperatorNode");
    auto iterationDetails = SerializableOperator_CEPIterationDetails();
    iterationDetails.set_miniteration(iterationOperator.getMinIterations());
    iterationDetails.set_maxiteration(iterationOperator.getMaxIterations());

    serializedOperator.mutable_details()->PackFrom(iterationDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeCEPIterationOperator(const SerializableOperator_CEPIterationDetails& cepIterationDetails) {
    auto maxIteration = cepIterationDetails.maxiteration();
    auto minIteration = cepIterationDetails.miniteration();
    return LogicalOperatorFactory::createCEPIterationOperator(minIteration, maxIteration, Util::getNextOperatorId());
}

template<typename T, typename D>
void OperatorSerializationUtil::serializeJavaUDFOperator(const T& javaUdfOperatorNode, SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize {}", javaUdfOperatorNode.toString());
    auto javaUdfDetails = D();
    UDFSerializationUtil::serializeJavaUDFDescriptor(javaUdfOperatorNode.getUDFDescriptor(),
                                                     *javaUdfDetails.mutable_javaudfdescriptor());
    serializedOperator.mutable_details()->PackFrom(javaUdfDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeMapJavaUDFOperator(const SerializableOperator_MapJavaUdfDetails& mapJavaUdfDetails) {
    auto javaUDFDescriptor = UDFSerializationUtil::deserializeJavaUDFDescriptor(mapJavaUdfDetails.javaudfdescriptor());
    return LogicalOperatorFactory::createMapUDFLogicalOperator(javaUDFDescriptor);
}

LogicalUnaryOperatorNodePtr OperatorSerializationUtil::deserializeFlatMapJavaUDFOperator(
    const SerializableOperator_FlatMapJavaUdfDetails& flatMapJavaUdfDetails) {
    auto javaUDFDescriptor = UDFSerializationUtil::deserializeJavaUDFDescriptor(flatMapJavaUdfDetails.javaudfdescriptor());
    return LogicalOperatorFactory::createFlatMapUDFLogicalOperator(javaUDFDescriptor);
}

void OperatorSerializationUtil::serializeOpenCLOperator(const OpenCLLogicalOperatorNode& openCLLogicalOperatorNode,
                                                        SerializableOperator& serializedOperator) {
    x_TRACE("OperatorSerializationUtil:: serialize to OpenCLLogicalOperatorNode");
    auto openCLDetails = SerializableOperator_OpenCLOperatorDetails();
    UDFSerializationUtil::serializeJavaUDFDescriptor(openCLLogicalOperatorNode.getJavaUDFDescriptor(),
                                                     *openCLDetails.mutable_javaudfdescriptor());
    openCLDetails.set_deviceid(openCLLogicalOperatorNode.getDeviceId());
    openCLDetails.set_openclcode(openCLLogicalOperatorNode.getOpenClCode());
    serializedOperator.mutable_details()->PackFrom(openCLDetails);
}

LogicalUnaryOperatorNodePtr
OperatorSerializationUtil::deserializeOpenCLOperator(const SerializableOperator_OpenCLOperatorDetails& openCLDetails) {
    auto javaUDFDescriptor = UDFSerializationUtil::deserializeJavaUDFDescriptor(openCLDetails.javaudfdescriptor());
    auto openCLOperator = LogicalOperatorFactory::createOpenCLLogicalOperator(javaUDFDescriptor)->as<OpenCLLogicalOperatorNode>();
    openCLOperator->setDeviceId(openCLDetails.deviceid());
    openCLOperator->setOpenClCode(openCLDetails.openclcode());
    return openCLOperator;
}

}// namespace x
