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

#ifndef x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxS_HPP_
#define x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxS_HPP_

#include <API/Query.hpp>
#include <Catalogs/Source/PhysicalSourceTypes/CSVSourceType.hpp>
#include <Operators/LogicalOperators/Sinks/FileSinkDescriptor.hpp>
#include <Operators/LogicalOperators/Sinks/SinkLogicalOperatorNode.hpp>
#include <Services/QueryService.hpp>
#include <Util/TestHarxs/TestHarxsWorkerConfiguration.hpp>
#include <Util/TestUtils.hpp>
#include <filesystem>
#include <type_traits>
#include <utility>

/**
 * @brief This test harxs wrap query deployment test in our test framework.
 */
namespace x {

class CSVSourceType;
using CSVSourceTypePtr = std::shared_ptr<CSVSourceType>;

/// Create compile-time tests that allow checking a specific function's type for a specific function by calling
///     [function-name]CompilesFromType<Return Type, Types of arguments, ...>
/// or  [function-name]Compiles<Return Type, argument 1, argument 2>.
///
/// Note that the non-type compile time checks for the non-type template arguments are limited to consteval-
/// constructible and non-floating point types.
/// Another limitation is that as of now, type and non type template argument tests cannot be mixed.
#define SETUP_COMPILE_TIME_TESTS(name, f)                                                                                        \
    SETUP_COMPILE_TIME_TEST(name, f);                                                                                            \
    SETUP_COMPILE_TIME_TEST_ARGS(name, f)

/// Check if function #func compiles from constexpr arguments and produce the expected return type that is provided
/// as the check's first template argument.
#define SETUP_COMPILE_TIME_TEST_ARGS(name, func)                                                                                 \
    namespace detail {                                                                                                           \
    template<typename, auto...>                                                                                                  \
    struct name##FromArgs : std::false_type {};                                                                                  \
    template<auto... args>                                                                                                       \
    struct name##FromArgs<decltype(func(args...)), args...> : std::true_type {};                                                 \
    }                                                                                                                            \
    template<typename R, auto... a>                                                                                              \
    using name##Compiles = detail::name##FromArgs<R, a...>

/// Check if function #func compiles from argument of given types produce the expected return type that is provided as
/// the check's first template argument.
#define SETUP_COMPILE_TIME_TEST(name, func)                                                                                      \
    namespace detail {                                                                                                           \
    template<typename, typename...>                                                                                              \
    struct name##FromType : std::false_type {};                                                                                  \
    template<typename... Ts>                                                                                                     \
    struct name##FromType<decltype(func(std::declval<Ts>()...)), Ts...> : std::true_type {};                                     \
    }                                                                                                                            \
    template<typename... Args>                                                                                                   \
    using name##CompilesFromType = detail::name##FromType<Args...>

class TestHarxs {
  public:
    /**
     * @brief The constructor of TestHarxs
     * @param numWorkers number of worker (each for one physical source) to be used in the test
     * @param queryWithoutSink query string to test (without the sink operator)
     * @param restPort port for the rest service
     * @param rpcPort for for the grpc
     */
    explicit TestHarxs(std::string queryWithoutSink,
                         uint16_t restPort,
                         uint16_t rpcPort,
                         std::filesystem::path testHarxsResourcePath,
                         uint64_t memSrcFrequency = 0,
                         uint64_t memSrcNumBuffToProcess = 1);

    /**
     * @brief The constructor of TestHarxs
     * @param numWorkers number of worker (each for one physical source) to be used in the test
     * @param queryWithoutSink query object to test (without the sink operator)
     * @param restPort port for the rest service
     * @param rpcPort for for the grpc
     */
    explicit TestHarxs(Query queryWithoutSink,
                         uint16_t restPort,
                         uint16_t rpcPort,
                         std::filesystem::path testHarxsResourcePath,
                         uint64_t memSrcFrequency = 0,
                         uint64_t memSrcNumBuffToProcess = 1);

    /**
     * @brief Enable using nautilus compiler
     * @return self
     */
    TestHarxs& enableNautilus();

    /**
     * @brief Enables the distributed window optimization
     */
    TestHarxs& enableDistributedWindowOptimization();

    /**
     * @brief Enable new request executor
     * @return self
     */
    TestHarxs& enableNewRequestExecutor();

    /**
     * @brief Sets the join strategy
     * @param joinStrategy
     * @return Self
     */
    TestHarxs& setJoinStrategy(QueryCompilation::StreamJoinStrategy& newJoinStrategy);

    /**
         * @brief push a single element/tuple to specific source
         * @param element element of Record to push
         * @param workerId id of the worker whose source will produce the pushed element
         */
    template<typename T>
    TestHarxs& pushElement(T element, uint64_t workerId) {
        if (workerId > topologyId) {
            x_THROW_RUNTIME_ERROR("TestHarxs: workerId " + std::to_string(workerId) + " does not exists");
        }

        bool found = false;
        for (const auto& harxsWorkerConfig : testHarxsWorkerConfigurations) {
            if (harxsWorkerConfig->getWorkerId() == workerId) {
                found = true;
                if (!std::is_class<T>::value) {
                    x_THROW_RUNTIME_ERROR("TestHarxs: tuples must be instances of struct");
                }

                if (harxsWorkerConfig->getSourceType()
                    != TestHarxsWorkerConfiguration::TestHarxsWorkerSourceType::MemorySource) {
                    x_THROW_RUNTIME_ERROR("TestHarxs: Record can be pushed only for source of Memory type.");
                }

                SchemaPtr schema;
                for (const auto& logicalSource : logicalSources) {
                    if (logicalSource->getLogicalSourceName() == harxsWorkerConfig->getLogicalSourceName()) {
                        schema = logicalSource->getSchema();
                        break;
                    }
                }

                if (!schema) {
                    x_THROW_RUNTIME_ERROR("TestHarxs: Unable to find schema for logical source "
                                            + harxsWorkerConfig->getLogicalSourceName()
                                            + ". Make sure you have defined a logical source with this name in test harxs");
                }

                if (sizeof(T) != schema->getSchemaSizeInBytes()) {
                    x_THROW_RUNTIME_ERROR("TestHarxs: tuple size and schema size does not match");
                }

                auto* memArea = reinterpret_cast<uint8_t*>(malloc(sizeof(T)));
                memcpy(memArea, reinterpret_cast<uint8_t*>(&element), sizeof(T));
                harxsWorkerConfig->addRecord(memArea);
                break;
            }
        }

        if (!found) {
            x_THROW_RUNTIME_ERROR("TestHarxs: Unable to locate worker with id " + std::to_string(workerId));
        }

        return *this;
    }

    TestHarxs& addLogicalSource(const std::string& logicalSourceName, const SchemaPtr& schema);

    /**
     * @brief check the schema size of the logical source and if it already exists
     * @param logical source name
     * @param schema schema of the source
     * @param physical source name
     */
    void checkAndAddLogicalSources();

    /**
     * @brief add a memory source to be used in the test and connect to parent with specific parent id
     * @param logical source name
     * @param schema schema of the source
     * @param physical source name
     * @param parentId id of the parent to connect
     */
    TestHarxs&
    attachWorkerWithMemorySourceToWorkerWithId(const std::string& logicalSourceName,
                                               uint32_t parentId,
                                               WorkerConfigurationPtr workerConfiguration = WorkerConfiguration::create());

    /**
     * @brief add a memory source to be used in the test
     * @param logical source name
     * @param schema schema of the source
     * @param physical source name
     */
    TestHarxs& attachWorkerWithMemorySourceToCoordinator(const std::string& logicalSourceName);

    /**
     * @brief add a memory source to be used in the test
     * @param logical source name
     * @param schema schema of the source
     * @param physical source name
     */
    TestHarxs& attachWorkerWithLambdaSourceToCoordinator(const std::string& logicalSourceName,
                                                           PhysicalSourceTypePtr physicalSource,
                                                           WorkerConfigurationPtr workerConfiguration);

    /**
     * @brief add a csv source to be used in the test and connect to parent with specific parent id
     * @param logicalSourceName logical source name
     * @param csvSourceType csv source type
     * @param parentId id of the parent to connect
     */
    TestHarxs& attachWorkerWithCSVSourceToWorkerWithId(const std::string& logicalSourceName,
                                                         CSVSourceTypePtr csvSourceType,
                                                         uint64_t parentId);

    /**
      * @brief add a csv source to be used in the test
      * @param logicalSourceName logical source name
      * @param csvSourceType csv source type
      */
    TestHarxs& attachWorkerWithCSVSourceToCoordinator(const std::string& logicalSourceName,
                                                        const CSVSourceTypePtr& csvSourceType);

    /**
     * @brief add worker and connect to parent with specific parent id
     * @param parentId id of the Test Harxs worker to connect
     * Note: The parent id can not be greater than the current testharxs worker id
     */
    TestHarxs& attachWorkerToWorkerWithId(uint32_t parentId);

    /**
     * @brief add non source worker
     */
    TestHarxs& attachWorkerToCoordinator();

    uint64_t getWorkerCount();

    TestHarxs& validate();

    PhysicalSourcePtr createPhysicalSourceOfLambdaType(TestHarxsWorkerConfigurationPtr workerConf);

    PhysicalSourcePtr createPhysicalSourceOfMemoryType(TestHarxsWorkerConfigurationPtr workerConf);
    /**
     * @brief Method to setup the topology
     * @param crdConfigFunctor A function pointer to specify the config changes of the CoordinatorConfiguration
     * @return the TestHarxs
     */
    TestHarxs& setupTopology(std::function<void(CoordinatorConfigurationPtr)> crdConfigFunctor =
                                   [](CoordinatorConfigurationPtr) {
                                   });

    /**
     * @brief execute the test based on the given operator, pushed elements, and number of workers,
     * then return the result of the query execution
     * @param placementStrategyName: placement strategy name
     * @param faultTolerance: chosen fault tolerance guarantee
     * @param lineage: chosen lineage type
     * @param numberOfContentToExpect: total number of tuple expected in the query result
     * @return output string
     */
    template<typename T>
    std::vector<T> getOutput(uint64_t numberOfContentToExpect,
                             std::string placementStrategyName = "BottomUp",
                             std::string faultTolerance = "NONE",
                             std::string lineage = "IN_MEMORY",
                             uint64_t testTimeout = 60) {

        if (!topologySetupDone || !validationDone) {
            throw Exceptions::RuntimeException(
                "Make sure to call first validate() and then setupTopology() to the test harxs before checking the output");
        }

        QueryServicePtr queryService = xCoordinator->getQueryService();
        QueryCatalogServicePtr queryCatalogService = xCoordinator->getQueryCatalogService();

        // local fs
        std::string filePath = testHarxsResourcePath / "testHarxs.out";
        remove(filePath.c_str());

        //register query
        auto placementStrategy = magic_enum::enum_cast<Optimizer::PlacementStrategy>(placementStrategyName).value();
        auto faultToleranceMode = magic_enum::enum_cast<FaultToleranceType>(faultTolerance).value();
        auto lineageMode = magic_enum::enum_cast<LineageType>(lineage).value();
        QueryId queryId = INVALID_QUERY_ID;

        if (!queryWithoutSinkStr.empty()) {
            // we can remove this, once we just use Query
            std::string queryString =
                queryWithoutSinkStr + R"(.sink(FileSinkDescriptor::create(")" + filePath + R"(" , "x_FORMAT", "APPEND"));)";
            queryId =
                queryService->validateAndQueueAddQueryRequest(queryString, placementStrategy, faultToleranceMode, lineageMode);
        } else if (queryWithoutSink.get() != nullptr) {
            auto query = queryWithoutSink->sink(FileSinkDescriptor::create(filePath, "x_FORMAT", "APPEND"));
            queryId = queryService->addQueryRequest(query.getQueryPlan()->toString(),
                                                    query.getQueryPlan(),
                                                    placementStrategy,
                                                    faultToleranceMode,
                                                    lineageMode);
        } else {
            x_THROW_RUNTIME_ERROR("TestHarxs expects that either the query is given as a string or as a query object!");
        }

        if (!TestUtils::waitForQueryToStart(queryId, queryCatalogService)) {
            x_THROW_RUNTIME_ERROR("TestHarxs: waitForQueryToStart returns false");
        }

        // Check if the size of output struct match with the size of output schema
        // Output struct might be padded, in this case the size is not equal to the total size of its field
        // Currently, we need to produce a result with the schema that does not cause the associated struct to be padded
        // (e.g., the size is multiple of 8)
        uint64_t outputSchemaSizeInBytes = queryCatalogService->getEntryForQuery(queryId)
                                               ->getExecutedQueryPlan()
                                               ->getSinkOperators()[0]
                                               ->getOutputSchema()
                                               ->getSchemaSizeInBytes();
        x_DEBUG("TestHarxs: outputSchema: {}",
                  queryCatalogService->getEntryForQuery(queryId)
                      ->getInputQueryPlan()
                      ->getSinkOperators()[0]
                      ->getOutputSchema()
                      ->toString());
        x_ASSERT(outputSchemaSizeInBytes == sizeof(T),
                   "The size of output struct does not match output schema."
                   " Output struct:"
                       << std::to_string(sizeof(T)) << " Schema:" << std::to_string(outputSchemaSizeInBytes));

        if (!TestUtils::checkBinaryOutputContentLengthOrTimeout<T>(queryId,
                                                                   queryCatalogService,
                                                                   numberOfContentToExpect,
                                                                   filePath,
                                                                   testTimeout)) {
            x_THROW_RUNTIME_ERROR("TestHarxs: checkBinaryOutputContentLengthOrTimeout returns false, "
                                    "number of buffers to expect="
                                    << std::to_string(numberOfContentToExpect));
        }

        if (!TestUtils::checkStoppedOrTimeout(queryId, queryCatalogService)) {
            x_THROW_RUNTIME_ERROR("TestHarxs: checkStoppedOrTimeout returns false for query with id= " << queryId);
        }

        std::ifstream ifs(filePath.c_str());
        if (!ifs.good()) {
            x_WARNING("TestHarxs:ifs.good() returns false for query with id {} file path= {}", queryId, filePath);
        }

        // check the length of the output file
        ifs.seekg(0, std::ifstream::end);
        auto length = ifs.tellg();
        ifs.seekg(0, std::ifstream::beg);

        // read the binary output as a vector of T
        std::vector<T> outputVector;
        outputVector.resize(length / sizeof(T));
        auto* buff = reinterpret_cast<char*>(outputVector.data());
        ifs.read(buff, length);

        x_DEBUG("TestHarxs: ExecutedQueryPlan: {}",
                  queryCatalogService->getEntryForQuery(queryId)->getExecutedQueryPlan()->toString());
        queryPlan = queryCatalogService->getEntryForQuery(queryId)->getExecutedQueryPlan();

        for (const auto& worker : testHarxsWorkerConfigurations) {
            worker->getxWorker()->stop(false);
        }
        xCoordinator->stopCoordinator(false);

        return outputVector;
    }

    TopologyPtr getTopology();
    const QueryPlanPtr& getQueryPlan() const;

  private:
    std::string getNextPhysicalSourceName();
    uint32_t getNextTopologyId();

    const std::chrono::seconds SETUP_TIMEOUT_IN_SEC = std::chrono::seconds(2);
    const std::string queryWithoutSinkStr;
    const QueryPtr queryWithoutSink;
    std::string coordinatorIPAddress;
    uint16_t restPort;
    uint16_t rpcPort;
    bool useNautilus;
    bool performDistributedWindowOptimization;
    bool useNewRequestExecutor;
    uint64_t memSrcFrequency;
    uint64_t memSrcNumBuffToProcess;
    uint64_t bufferSize;
    xCoordinatorPtr xCoordinator;
    std::vector<LogicalSourcePtr> logicalSources;
    std::vector<TestHarxsWorkerConfigurationPtr> testHarxsWorkerConfigurations;
    uint32_t physicalSourceCount;
    uint32_t topologyId;
    QueryCompilation::StreamJoinStrategy joinStrategy;
    bool validationDone;
    bool topologySetupDone;
    std::filesystem::path testHarxsResourcePath;
    QueryPlanPtr queryPlan;
};
}// namespace x

#endif// x_CORE_INCLUDE_UTIL_TESTHARxS_TESTHARxS_HPP_
