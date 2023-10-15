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
#include <BaseIntegrationTest.hpp>
#include <Topology/Prediction/Edge.hpp>
#include <Topology/Prediction/TopologyDelta.hpp>
#include <gtest/gtest.h>
namespace x {
using Experimental::TopologyPrediction::Edge;
using Experimental::TopologyPrediction::TopologyDelta;

class TopologyDeltaTest : public Testing::BaseIntegrationTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("TopologyDeltaTest.log", x::LogLevel::LOG_DEBUG);
        x_DEBUG("Set up TopologyDelta test class");
    }
};

TEST_F(TopologyDeltaTest, testToString) {
    TopologyDelta delta({{1, 2}, {1, 3}, {5, 3}}, {{2, 3}, {1, 5}});
    EXPECT_EQ(delta.toString(), "added: {1->2, 1->3, 5->3}, removed: {2->3, 1->5}");
    TopologyDelta emptyDelta({}, {});
    EXPECT_EQ(emptyDelta.toString(), "added: {}, removed: {}");
}

TEST_F(TopologyDeltaTest, testGetEdges) {
    std::vector<Edge> added = {{1, 2}, {1, 3}, {5, 3}};
    std::vector<Edge> removed = {{2, 3}, {1, 5}};
    TopologyDelta delta(added, removed);
    EXPECT_EQ(delta.getAdded(), added);
    EXPECT_EQ(delta.getRemoved(), removed);

    TopologyDelta emptyDelta({}, {});
    EXPECT_TRUE(emptyDelta.getAdded().empty());
    EXPECT_TRUE(emptyDelta.getRemoved().empty());
}
}// namespace x
