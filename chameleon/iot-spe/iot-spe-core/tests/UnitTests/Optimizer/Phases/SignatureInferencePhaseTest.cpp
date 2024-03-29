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

#include <API/Expressions/ArithmeticalExpressions.hpp>
#include <API/Expressions/Expressions.hpp>
#include <API/Query.hpp>
#include <BaseIntegrationTest.hpp>
#include <Catalogs/Source/LogicalSource.hpp>
#include <Catalogs/Source/PhysicalSource.hpp>
#include <Catalogs/Source/SourceCatalog.hpp>
#include <Catalogs/Source/SourceCatalogEntry.hpp>
#include <Catalogs/UDF/UDFCatalog.hpp>
#include <Configurations/WorkerConfigurationKeys.hpp>
#include <Configurations/WorkerPropertyKeys.hpp>
#include <Operators/LogicalOperators/MapLogicalOperatorNode.hpp>
#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sources/SourceLogicalOperatorNode.hpp>
#include <Optimizer/Phases/SignatureInferencePhase.hpp>
#include <Optimizer/Phases/TypeInferencePhase.hpp>
#include <Optimizer/QuerySignatures/QuerySignature.hpp>
#include <Optimizer/QuerySignatures/SignatureEqualityUtil.hpp>
#include <Plans/Query/QueryPlan.hpp>
#include <Topology/TopologyNode.hpp>
#include <Util/Experimental/SpatialType.hpp>
#include <Util/Logger/Logger.hpp>
#include <gtest/gtest.h>
#include <z3++.h>

namespace x::Optimizer {

class SignatureInferencePhaseTest : public Testing::BaseUnitTest {
  public:
    std::shared_ptr<Catalogs::UDF::UDFCatalog> udfCatalog;
    /* Will be called before any test in this class are executed. */
    static void SetUpTestCase() {
        x::Logger::setupLogging("SignatureInferencePhaseTest.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup SignatureInferencePhaseTest test case.");
    }

    void SetUp() override {
        Testing::BaseUnitTest::SetUp();
        udfCatalog = Catalogs::UDF::UDFCatalog::create();
    }
};

/**
 * @brief In this test we execute query merger phase on a single invalid query plan.
 */
TEST_F(SignatureInferencePhaseTest, executeQueryMergerPhaseForSingleInvalidQueryPlan) {

    //Prepare
    x_INFO("SignatureInferencePhaseTest: Create a new query without assigning it a query id.");

    std::map<std::string, std::any> properties;
    properties[x::Worker::Properties::MAINTENANCE] = false;
    properties[x::Worker::Configuration::SPATIAL_SUPPORT] = x::Spatial::Experimental::SpatialType::NO_LOCATION;

    TopologyNodePtr physicalNode = TopologyNode::create(1, "localhost", 4000, 4002, 4, properties);

    PhysicalSourcePtr physicalSource = PhysicalSource::create("default_logical", "test1");
    auto logicalSource = LogicalSource::create("default_logical", Schema::create());
    Catalogs::Source::SourceCatalogPtr sourceCatalog =
        std::make_shared<Catalogs::Source::SourceCatalog>(QueryParsingServicePtr());
    Catalogs::Source::SourceCatalogEntryPtr sce =
        std::make_shared<Catalogs::Source::SourceCatalogEntry>(physicalSource, logicalSource, physicalNode);
    sourceCatalog->addPhysicalSource("default_logical", sce);

    auto typeInferencePhase = TypeInferencePhase::create(sourceCatalog, udfCatalog);
    z3::ContextPtr context = std::make_shared<z3::context>();
    auto signatureInferencePhase =
        SignatureInferencePhase::create(context, QueryMergerRule::Z3SignatureBasedCompleteQueryMergerRule);

    auto query1 = Query::from("default_logical").map(Attribute("f3") = Attribute("id")++).sink(FileSinkDescriptor::create(""));
    auto plan1 = query1.getQueryPlan();

    typeInferencePhase->execute(plan1);
    signatureInferencePhase->execute(plan1);

    auto query2 = Query::from("default_logical").map(Attribute("f3") = Attribute("id")++).sink(FileSinkDescriptor::create(""));
    auto plan2 = query2.getQueryPlan();

    typeInferencePhase->execute(plan2);
    signatureInferencePhase->execute(plan2);

    auto mapOperators1 = plan1->getOperatorByType<MapLogicalOperatorNode>();
    auto mapOperators2 = plan2->getOperatorByType<MapLogicalOperatorNode>();

    ASSERT_EQ(mapOperators1.size(), 1u);
    ASSERT_EQ(mapOperators2.size(), 1u);

    auto signatureEqualityChecker = SignatureEqualityUtil::create(context);

    EXPECT_TRUE(signatureEqualityChecker->checkEquality(mapOperators1[0]->getZ3Signature(), mapOperators2[0]->getZ3Signature()));

    auto srcOperators1 = plan1->getOperatorByType<SourceLogicalOperatorNode>();
    auto srcOperators2 = plan2->getOperatorByType<SourceLogicalOperatorNode>();

    ASSERT_EQ(srcOperators1.size(), 1u);
    ASSERT_EQ(srcOperators2.size(), 1u);

    EXPECT_TRUE(signatureEqualityChecker->checkEquality(srcOperators1[0]->getZ3Signature(), srcOperators2[0]->getZ3Signature()));
}
}// namespace x::Optimizer
