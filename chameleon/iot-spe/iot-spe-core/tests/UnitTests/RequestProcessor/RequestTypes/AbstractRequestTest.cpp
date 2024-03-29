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
#include <RequestProcessor/RequestTypes/AbstractRequest.hpp>
#include <RequestProcessor/StorageHandles/StorageHandler.hpp>
#include <gtest/gtest.h>

namespace x::RequestProcessor::Experimental {

class DummyResponse : public AbstractRequestResponse {
  public:
    explicit DummyResponse(uint32_t number) : number(number){};
    uint32_t number;
};

class DummyRequest : public AbstractRequest {
  public:
    DummyRequest(const std::vector<ResourceType>& requiredResources, uint8_t maxRetries, uint32_t responseValue)
        : AbstractRequest(requiredResources, maxRetries), responseValue(responseValue){};

    std::vector<AbstractRequestPtr> executeRequestLogic(const StorageHandlerPtr&) override {
        responsePromise.set_value(std::make_shared<DummyResponse>(responseValue));
        return {};
    }

    std::vector<AbstractRequestPtr> rollBack(RequestExecutionException&, const StorageHandlerPtr&) override { return {}; }

  protected:
    void preRollbackHandle(const RequestExecutionException&, const StorageHandlerPtr&) override {}
    void postRollbackHandle(const RequestExecutionException&, const StorageHandlerPtr&) override {}
    void postExecution(const StorageHandlerPtr&) override {}

  private:
    uint32_t responseValue;
};

class DummyStorageHandler : public StorageHandler {
  public:
    explicit DummyStorageHandler() = default;
    GlobalExecutionPlanHandle getGlobalExecutionPlanHandle(RequestId) override { return nullptr; };

    TopologyHandle getTopologyHandle(RequestId) override { return nullptr; };

    QueryCatalogServiceHandle getQueryCatalogServiceHandle(RequestId) override { return nullptr; };

    GlobalQueryPlanHandle getGlobalQueryPlanHandle(RequestId) override { return nullptr; };

    SourceCatalogHandle getSourceCatalogHandle(RequestId) override { return nullptr; };

    UDFCatalogHandle getUDFCatalogHandle(RequestId) override { return nullptr; };

    CoordinatorConfigurationHandle getCoordinatorConfiguration(RequestId) override { return nullptr; }
};

class AbstractRequestTest : public Testing::BaseUnitTest {
  public:
    static void SetUpTestCase() { x::Logger::setupLogging("AbstractRequestTest.log", x::LogLevel::LOG_DEBUG); }
};

TEST_F(AbstractRequestTest, testPromise) {
    constexpr uint32_t responseValue = 20;
    RequestId requestId = 1;
    std::vector<ResourceType> requiredResources;
    uint8_t maxRetries = 1;
    DummyRequest request(requiredResources, maxRetries, responseValue);
    request.setId(requestId);
    auto future = request.getFuture();
    auto thread = std::make_shared<std::thread>([&request]() {
        std::shared_ptr<DummyStorageHandler> storageHandler;
        request.executeRequestLogic(storageHandler);
    });
    thread->join();
    EXPECT_EQ(std::static_pointer_cast<DummyResponse>(future.get())->number, responseValue);
}
}// namespace x::RequestProcessor::Experimental
