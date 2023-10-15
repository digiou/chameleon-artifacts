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
#include <Exceptions/CoordinatesOutOfRangeException.hpp>
#include <Exceptions/InvalidCoordinateFormatException.hpp>
#include <Util/Logger/Logger.hpp>
#include <Util/TestUtils.hpp>
#include <gtest/gtest.h>

namespace x {

class LocationUnitTest : public Testing::BaseUnitTest {
  public:
    static void SetUpTestCase() {
        x::Logger::setupLogging("GeoLoc.log", x::LogLevel::LOG_DEBUG);
        x_INFO("Setup Location test class.");
    }

    static void TearDownTestCase() { x_INFO("Tear down GeographilcalLocationUnitTest test class."); }
};

TEST_F(LocationUnitTest, testExceptionHandling) {
    ASSERT_THROW(x::Spatial::DataTypes::Experimental::GeoLocation(200, 0),
                 x::Spatial::Exception::CoordinatesOutOfRangeException);
    ASSERT_THROW(x::Spatial::DataTypes::Experimental::GeoLocation(200, 200),
                 x::Spatial::Exception::CoordinatesOutOfRangeException);
    ASSERT_THROW(x::Spatial::DataTypes::Experimental::GeoLocation::fromString("200, 0"),
                 x::Spatial::Exception::CoordinatesOutOfRangeException);
    ASSERT_THROW(x::Spatial::DataTypes::Experimental::GeoLocation::fromString("200. 0"),
                 x::Spatial::Exception::CoordinatesOutOfRangeException);
    ASSERT_THROW(x::Spatial::DataTypes::Experimental::GeoLocation::fromString("12ee2, 122sff"),
                 x::Spatial::Exception::CoordinatesOutOfRangeException);

    auto geoLoc = x::Spatial::DataTypes::Experimental::GeoLocation::fromString("23, 110");
    EXPECT_EQ(geoLoc.getLatitude(), 23);
    EXPECT_EQ(geoLoc.getLongitude(), 110);
    ASSERT_TRUE(geoLoc.isValid());
    auto invalidGeoLoc1 = x::Spatial::DataTypes::Experimental::GeoLocation();
    auto invalidGeoLoc2 = x::Spatial::DataTypes::Experimental::GeoLocation();
    EXPECT_FALSE(invalidGeoLoc1.isValid());
    EXPECT_TRUE(std::isnan(invalidGeoLoc1.getLatitude()));
    ASSERT_TRUE(std::isnan(invalidGeoLoc1.getLongitude()));

    EXPECT_EQ(invalidGeoLoc1, invalidGeoLoc2);
    ASSERT_NE(geoLoc, invalidGeoLoc1);
}
}// namespace x
