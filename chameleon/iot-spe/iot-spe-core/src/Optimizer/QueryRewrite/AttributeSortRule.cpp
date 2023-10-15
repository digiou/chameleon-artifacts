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

#include <API/Schema.hpp>
#include <Common/ValueTypes/ArrayValue.hpp>
#include <Common/ValueTypes/BasicValue.hpp>
#include <Nodes/Expressions/ArithmeticalExpressions/AddExpressionNode.hpp>
#include <Nodes/Expressions/ArithmeticalExpressions/DivExpressionNode.hpp>
#include <Nodes/Expressions/ArithmeticalExpressions/MulExpressionNode.hpp>
#include <Nodes/Expressions/ArithmeticalExpressions/SubExpressionNode.hpp>
#include <Nodes/Expressions/ConstantValueExpressionNode.hpp>
#include <Nodes/Expressions/FieldAccessExpressionNode.hpp>
#include <Nodes/Expressions/FieldAssignmentExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/AndExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/EqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/GreaterEqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/GreaterExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/LessEqualsExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/LessExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/NegateExpressionNode.hpp>
#include <Nodes/Expressions/LogicalExpressions/OrExpressionNode.hpp>
#include <Operators/LogicalOperators/FilterLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/MapLogicalOperatorNode.hpp>
#include <Optimizer/QueryRewrite/AttributeSortRule.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Util/Logger/Logger.hpp>
#include <numeric>
#include <utility>

namespace x::Optimizer {

AttributeSortRulePtr AttributeSortRule::create() { return std::make_shared<AttributeSortRule>(); }

QueryPlanPtr AttributeSortRule::apply(x::QueryPlanPtr queryPlan) {

    auto filterOperators = queryPlan->getOperatorByType<FilterLogicalOperatorNode>();
    for (auto const& filterOperator : filterOperators) {
        auto predicate = filterOperator->getPredicate();
        auto updatedPredicate = sortAttributesInExpression(predicate);
        auto updatedFilter = LogicalOperatorFactory::createFilterOperator(updatedPredicate);
        updatedFilter->setInputSchema(filterOperator->getInputSchema()->copy());
        updatedFilter->as_if<LogicalOperatorNode>()->setOutputSchema(
            filterOperator->as_if<LogicalOperatorNode>()->getOutputSchema()->copy());
        filterOperator->replace(updatedFilter);
    }

    auto mapOperators = queryPlan->getOperatorByType<MapLogicalOperatorNode>();
    for (auto const& mapOperator : mapOperators) {
        auto mapExpression = mapOperator->getMapExpression();
        auto updatedMapExpression = sortAttributesInExpression(mapExpression)->as<FieldAssignmentExpressionNode>();
        auto updatedMap = LogicalOperatorFactory::createMapOperator(updatedMapExpression);
        updatedMap->setInputSchema(mapOperator->getInputSchema()->copy());
        updatedMap->as_if<LogicalOperatorNode>()->setOutputSchema(
            mapOperator->as_if<LogicalOperatorNode>()->getOutputSchema()->copy());
        mapOperator->replace(updatedMap);
    }
    return queryPlan;
}

x::ExpressionNodePtr AttributeSortRule::sortAttributesInExpression(x::ExpressionNodePtr expression) {
    x_DEBUG("Sorting attributed for input expression {}", expression->toString());
    if (expression->instanceOf<x::LogicalExpressionNode>()) {
        return sortAttributesInLogicalExpressions(expression);
    }
    if (expression->instanceOf<x::ArithmeticalExpressionNode>()) {
        return sortAttributesInArithmeticalExpressions(expression);
    } else if (expression->instanceOf<x::FieldAssignmentExpressionNode>()) {
        auto fieldAssignmentExpressionNode = expression->as<x::FieldAssignmentExpressionNode>();
        auto assignment = fieldAssignmentExpressionNode->getAssignment();
        auto updatedAssignment = sortAttributesInExpression(assignment);
        auto field = fieldAssignmentExpressionNode->getField();
        return x::FieldAssignmentExpressionNode::create(field, updatedAssignment);
    } else if (expression->instanceOf<x::ConstantValueExpressionNode>()
               || expression->instanceOf<x::FieldAccessExpressionNode>()) {
        return expression;
    }
    x_THROW_RUNTIME_ERROR("No conversion to Z3 expression implemented for the expression: " + expression->toString());
    return nullptr;
}

ExpressionNodePtr AttributeSortRule::sortAttributesInArithmeticalExpressions(ExpressionNodePtr expression) {
    x_DEBUG("Create Z3 expression for arithmetical expression {}", expression->toString());
    if (expression->instanceOf<x::AddExpressionNode>()) {
        auto addExpressionNode = expression->as<x::AddExpressionNode>();

        auto sortedLeft = sortAttributesInExpression(addExpressionNode->getLeft());
        auto sortedRight = sortAttributesInExpression(addExpressionNode->getRight());

        auto leftCommutativeFields = fetchCommutativeFields<x::AddExpressionNode>(sortedLeft);
        auto rightCommutativeFields = fetchCommutativeFields<x::AddExpressionNode>(sortedRight);

        std::vector<ExpressionNodePtr> allCommutativeFields;
        allCommutativeFields.insert(allCommutativeFields.end(), leftCommutativeFields.begin(), leftCommutativeFields.end());
        allCommutativeFields.insert(allCommutativeFields.end(), rightCommutativeFields.begin(), rightCommutativeFields.end());

        std::vector<ExpressionNodePtr> sortedCommutativeFields;
        sortedCommutativeFields.reserve(allCommutativeFields.size());
        for (const auto& commutativeField : allCommutativeFields) {
            sortedCommutativeFields.push_back(commutativeField->copy());
        }

        std::sort(sortedCommutativeFields.begin(),
                  sortedCommutativeFields.end(),
                  [](const x::ExpressionNodePtr& lhsField, const x::ExpressionNodePtr& rhsField) {
                      std::string leftValue;
                      std::string rightValue;

                      if (lhsField->instanceOf<x::ConstantValueExpressionNode>()) {
                          auto constantValue = lhsField->as<x::ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          leftValue = basicValueType->value;
                      } else {
                          leftValue = lhsField->as<x::FieldAccessExpressionNode>()->getFieldName();
                      }

                      if (rhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = rhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          rightValue = basicValueType->value;
                      } else {
                          rightValue = rhsField->as<x::FieldAccessExpressionNode>()->getFieldName();
                      }
                      return leftValue.compare(rightValue) < 0;
                  });

        for (unsigned long i = 0; i < sortedCommutativeFields.size(); i++) {
            auto originalField = allCommutativeFields[i];
            auto updatedField = sortedCommutativeFields[i];

            if (sortedLeft.get() == originalField.get()) {
                sortedLeft = updatedField;
            } else if (!(sortedLeft->instanceOf<FieldAccessExpressionNode>()
                         || sortedLeft->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedLeft, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }

            if (sortedRight.get() == originalField.get()) {
                sortedRight = updatedField;
            } else if (!(sortedRight->instanceOf<FieldAccessExpressionNode>()
                         || sortedRight->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedRight, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }
        }

        if (!sortedLeft->instanceOf<AddExpressionNode>() || !sortedRight->instanceOf<AddExpressionNode>()) {
            auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
            auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
            int compared = leftSortedFieldName.compare(rightSortedFieldName);
            if (compared > 0) {
                return AddExpressionNode::create(sortedRight, sortedLeft);
            }
        }

        return AddExpressionNode::create(sortedLeft, sortedRight);
    }
    if (expression->instanceOf<SubExpressionNode>()) {
        auto subExpressionNode = expression->as<SubExpressionNode>();
        auto left = subExpressionNode->getLeft();
        auto right = subExpressionNode->getRight();
        sortAttributesInExpression(left);
        sortAttributesInExpression(right);
        return expression;
    } else if (expression->instanceOf<MulExpressionNode>()) {
        auto mulExpressionNode = expression->as<MulExpressionNode>();
        auto left = mulExpressionNode->getLeft();
        auto right = mulExpressionNode->getRight();

        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftCommutativeFields = fetchCommutativeFields<MulExpressionNode>(sortedLeft);
        auto rightCommutativeFields = fetchCommutativeFields<MulExpressionNode>(sortedRight);

        std::vector<ExpressionNodePtr> allCommutativeFields;
        allCommutativeFields.insert(allCommutativeFields.end(), leftCommutativeFields.begin(), leftCommutativeFields.end());
        allCommutativeFields.insert(allCommutativeFields.end(), rightCommutativeFields.begin(), rightCommutativeFields.end());

        std::vector<ExpressionNodePtr> sortedCommutativeFields;
        sortedCommutativeFields.reserve(allCommutativeFields.size());
        for (const auto& commutativeField : allCommutativeFields) {
            sortedCommutativeFields.push_back(commutativeField->copy());
        }

        std::sort(sortedCommutativeFields.begin(),
                  sortedCommutativeFields.end(),
                  [](const ExpressionNodePtr& lhsField, const ExpressionNodePtr& rhsField) {
                      std::string leftValue;
                      std::string rightValue;

                      if (lhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = lhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          leftValue = basicValueType->value;
                      } else {
                          leftValue = lhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }

                      if (rhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = rhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          rightValue = basicValueType->value;
                      } else {
                          rightValue = rhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }
                      return leftValue.compare(rightValue) < 0;
                  });

        for (unsigned long i = 0; i < sortedCommutativeFields.size(); i++) {
            auto originalField = allCommutativeFields[i];
            auto updatedField = sortedCommutativeFields[i];

            if (sortedLeft.get() == originalField.get()) {
                sortedLeft = updatedField;
            } else if (!(sortedLeft->instanceOf<FieldAccessExpressionNode>()
                         || sortedLeft->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedLeft, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }

            if (sortedRight.get() == originalField.get()) {
                sortedRight = updatedField;
            } else if (!(sortedRight->instanceOf<FieldAccessExpressionNode>()
                         || sortedRight->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedRight, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }
        }

        if (!sortedLeft->instanceOf<MulExpressionNode>() || !sortedRight->instanceOf<MulExpressionNode>()) {
            auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
            auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
            int compared = leftSortedFieldName.compare(rightSortedFieldName);
            if (compared > 0) {
                return MulExpressionNode::create(sortedRight, sortedLeft);
            }
        }

        return MulExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<DivExpressionNode>()) {
        auto divExpressionNode = expression->as<DivExpressionNode>();
        auto left = divExpressionNode->getLeft();
        auto right = divExpressionNode->getRight();
        sortAttributesInExpression(left);
        sortAttributesInExpression(right);
        return expression;
    }
    x_THROW_RUNTIME_ERROR("No conversion to Z3 expression implemented for the arithmetical expression node: "
                            + expression->toString());
    return nullptr;
}

ExpressionNodePtr AttributeSortRule::sortAttributesInLogicalExpressions(const ExpressionNodePtr& expression) {
    x_DEBUG("Create Z3 expression node for logical expression {}", expression->toString());
    if (expression->instanceOf<AndExpressionNode>()) {
        auto andExpressionNode = expression->as<AndExpressionNode>();
        auto left = andExpressionNode->getLeft();
        auto right = andExpressionNode->getRight();
        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftCommutativeFields = fetchCommutativeFields<AndExpressionNode>(sortedLeft);
        auto rightCommutativeFields = fetchCommutativeFields<AndExpressionNode>(sortedRight);

        std::vector<ExpressionNodePtr> allCommutativeFields;
        allCommutativeFields.insert(allCommutativeFields.end(), leftCommutativeFields.begin(), leftCommutativeFields.end());
        allCommutativeFields.insert(allCommutativeFields.end(), rightCommutativeFields.begin(), rightCommutativeFields.end());

        std::vector<ExpressionNodePtr> sortedCommutativeFields;
        sortedCommutativeFields.reserve(allCommutativeFields.size());
        for (const auto& commutativeField : allCommutativeFields) {
            sortedCommutativeFields.push_back(commutativeField->copy());
        }

        std::sort(sortedCommutativeFields.begin(),
                  sortedCommutativeFields.end(),
                  [](const ExpressionNodePtr& lhsField, const ExpressionNodePtr& rhsField) {
                      std::string leftValue;
                      std::string rightValue;

                      if (lhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = lhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          leftValue = basicValueType->value;
                      } else {
                          leftValue = lhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }

                      if (rhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = rhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          rightValue = basicValueType->value;
                      } else {
                          rightValue = rhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }
                      return leftValue.compare(rightValue) < 0;
                  });

        for (unsigned long i = 0; i < sortedCommutativeFields.size(); i++) {
            auto originalField = allCommutativeFields[i];
            auto updatedField = sortedCommutativeFields[i];

            if (sortedLeft.get() == originalField.get()) {
                sortedLeft = updatedField;
            } else if (!(sortedLeft->instanceOf<FieldAccessExpressionNode>()
                         || sortedLeft->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedLeft, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }

            if (sortedRight.get() == originalField.get()) {
                sortedRight = updatedField;
            } else if (!(sortedRight->instanceOf<FieldAccessExpressionNode>()
                         || sortedRight->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedRight, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }
        }

        if (!sortedLeft->instanceOf<AndExpressionNode>() || !sortedRight->instanceOf<AndExpressionNode>()) {
            auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
            auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
            int compared = leftSortedFieldName.compare(rightSortedFieldName);
            if (compared > 0) {
                return AndExpressionNode::create(sortedRight, sortedLeft);
            }
        }
        return AndExpressionNode::create(sortedLeft, sortedRight);
    }
    if (expression->instanceOf<OrExpressionNode>()) {
        auto orExpressionNode = expression->as<OrExpressionNode>();
        auto left = orExpressionNode->getLeft();
        auto right = orExpressionNode->getRight();
        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftCommutativeFields = fetchCommutativeFields<OrExpressionNode>(sortedLeft);
        auto rightCommutativeFields = fetchCommutativeFields<OrExpressionNode>(sortedRight);

        std::vector<ExpressionNodePtr> allCommutativeFields;
        allCommutativeFields.insert(allCommutativeFields.end(), leftCommutativeFields.begin(), leftCommutativeFields.end());
        allCommutativeFields.insert(allCommutativeFields.end(), rightCommutativeFields.begin(), rightCommutativeFields.end());

        std::vector<ExpressionNodePtr> sortedCommutativeFields;
        sortedCommutativeFields.reserve(allCommutativeFields.size());
        for (const auto& commutativeField : allCommutativeFields) {
            sortedCommutativeFields.push_back(commutativeField->copy());
        }

        std::sort(sortedCommutativeFields.begin(),
                  sortedCommutativeFields.end(),
                  [](const ExpressionNodePtr& lhsField, const ExpressionNodePtr& rhsField) {
                      std::string leftValue;
                      std::string rightValue;

                      if (lhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = lhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          leftValue = basicValueType->value;
                      } else {
                          leftValue = lhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }

                      if (rhsField->instanceOf<ConstantValueExpressionNode>()) {
                          auto constantValue = rhsField->as<ConstantValueExpressionNode>()->getConstantValue();
                          auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue);
                          rightValue = basicValueType->value;
                      } else {
                          rightValue = rhsField->as<FieldAccessExpressionNode>()->getFieldName();
                      }
                      return leftValue.compare(rightValue) < 0;
                  });

        for (unsigned long i = 0; i < sortedCommutativeFields.size(); i++) {
            auto originalField = allCommutativeFields[i];
            auto updatedField = sortedCommutativeFields[i];

            if (sortedLeft.get() == originalField.get()) {
                sortedLeft = updatedField;
            } else if (!(sortedLeft->instanceOf<FieldAccessExpressionNode>()
                         || sortedLeft->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedLeft, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }

            if (sortedRight.get() == originalField.get()) {
                sortedRight = updatedField;
            } else if (!(sortedRight->instanceOf<FieldAccessExpressionNode>()
                         || sortedRight->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(sortedRight, originalField, updatedField);
                if (replaced) {
                    continue;
                }
            }
        }

        if (!sortedLeft->instanceOf<OrExpressionNode>() || !sortedRight->instanceOf<OrExpressionNode>()) {
            auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
            auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
            int compared = leftSortedFieldName.compare(rightSortedFieldName);
            if (compared > 0) {
                return OrExpressionNode::create(sortedRight, sortedLeft);
            }
        }
        return OrExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<LessExpressionNode>()) {

        auto lessExpressionNode = expression->as<LessExpressionNode>();
        auto left = lessExpressionNode->getLeft();
        auto right = lessExpressionNode->getRight();

        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
        auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
        int compared = leftSortedFieldName.compare(rightSortedFieldName);
        if (compared > 0) {
            return GreaterExpressionNode::create(sortedRight, sortedLeft);
        }
        return LessExpressionNode::create(sortedLeft, sortedRight);

    } else if (expression->instanceOf<LessEqualsExpressionNode>()) {
        auto lessEqualsExpressionNode = expression->as<LessEqualsExpressionNode>();
        auto left = lessEqualsExpressionNode->getLeft();
        auto right = lessEqualsExpressionNode->getRight();
        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
        auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
        int compared = leftSortedFieldName.compare(rightSortedFieldName);
        if (compared > 0) {
            return GreaterEqualsExpressionNode::create(sortedRight, sortedLeft);
        }
        return LessEqualsExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<GreaterExpressionNode>()) {
        auto greaterExpressionNode = expression->as<GreaterExpressionNode>();
        auto left = greaterExpressionNode->getLeft();
        auto right = greaterExpressionNode->getRight();

        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
        auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
        int compared = leftSortedFieldName.compare(rightSortedFieldName);
        if (compared > 0) {
            return LessExpressionNode::create(sortedRight, sortedLeft);
        }
        return GreaterExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<GreaterEqualsExpressionNode>()) {
        auto greaterEqualsExpressionNode = expression->as<GreaterEqualsExpressionNode>();
        auto left = greaterEqualsExpressionNode->getLeft();
        auto right = greaterEqualsExpressionNode->getRight();

        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
        auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
        int compared = leftSortedFieldName.compare(rightSortedFieldName);
        if (compared > 0) {
            return LessEqualsExpressionNode::create(sortedRight, sortedLeft);
        }
        return GreaterEqualsExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<EqualsExpressionNode>()) {
        auto equalsExpressionNode = expression->as<EqualsExpressionNode>();
        auto left = equalsExpressionNode->getLeft();
        auto right = equalsExpressionNode->getRight();
        auto sortedLeft = sortAttributesInExpression(left);
        auto sortedRight = sortAttributesInExpression(right);

        auto leftSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedLeft);
        auto rightSortedFieldName = fetchLeftMostConstantValueOrFieldName(sortedRight);
        int compared = leftSortedFieldName.compare(rightSortedFieldName);
        if (compared > 0) {
            return EqualsExpressionNode::create(sortedRight, sortedLeft);
        }
        return EqualsExpressionNode::create(sortedLeft, sortedRight);
    } else if (expression->instanceOf<NegateExpressionNode>()) {
        auto negateExpressionNode = expression->as<NegateExpressionNode>();
        auto childExpression = negateExpressionNode->child();
        auto updatedChildExpression = sortAttributesInExpression(childExpression);
        return NegateExpressionNode::create(updatedChildExpression);
    }
    x_THROW_RUNTIME_ERROR("No conversion to Z3 expression possible for the logical expression node: " + expression->toString());
    return nullptr;
}

bool AttributeSortRule::replaceCommutativeExpressions(const ExpressionNodePtr& parentExpression,
                                                      const ExpressionNodePtr& originalExpression,
                                                      const ExpressionNodePtr& updatedExpression) {

    auto binaryExpression = parentExpression->as<BinaryExpressionNode>();

    const ExpressionNodePtr& leftChild = binaryExpression->getLeft();
    const ExpressionNodePtr& rightChild = binaryExpression->getRight();
    if (leftChild.get() == originalExpression.get()) {
        binaryExpression->removeChildren();
        binaryExpression->setChildren(updatedExpression, rightChild);
        return true;
    }
    if (rightChild.get() == originalExpression.get()) {
        binaryExpression->removeChildren();
        binaryExpression->setChildren(leftChild, updatedExpression);
        return true;
    } else {
        auto children = parentExpression->getChildren();
        for (const auto& child : children) {
            if (!(child->instanceOf<FieldAccessExpressionNode>() || child->instanceOf<ConstantValueExpressionNode>())) {
                bool replaced = replaceCommutativeExpressions(child->as<ExpressionNode>(), originalExpression, updatedExpression);
                if (replaced) {
                    return true;
                }
            }
        }
    }
    return false;
}

std::string AttributeSortRule::fetchLeftMostConstantValueOrFieldName(ExpressionNodePtr expression) {
    ExpressionNodePtr startPoint = std::move(expression);
    while (!(startPoint->instanceOf<FieldAccessExpressionNode>() || startPoint->instanceOf<ConstantValueExpressionNode>())) {
        startPoint = startPoint->getChildren()[0]->as<ExpressionNode>();
    }

    if (startPoint->instanceOf<FieldAccessExpressionNode>()) {
        return startPoint->template as<FieldAccessExpressionNode>()->getFieldName();
    }
    const ValueTypePtr& constantValue = startPoint->as<ConstantValueExpressionNode>()->getConstantValue();
    if (auto basicValueType = std::dynamic_pointer_cast<BasicValue>(constantValue); basicValueType) {
        return basicValueType->value;
    }

    if (auto arrayValueType = std::dynamic_pointer_cast<ArrayValue>(constantValue); arrayValueType) {
        return std::accumulate(arrayValueType->values.begin(), arrayValueType->values.end(), std::string());
    }

    x_THROW_RUNTIME_ERROR("AttributeSortRule not equipped for handling value type!");
}

}// namespace x::Optimizer