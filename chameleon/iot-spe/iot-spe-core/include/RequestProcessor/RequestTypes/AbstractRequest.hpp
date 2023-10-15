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
#ifndef x_CORE_INCLUDE_WORKQUEUES_REQUESTTYPES_ABSTRACTREQUEST_HPP_
#define x_CORE_INCLUDE_WORKQUEUES_REQUESTTYPES_ABSTRACTREQUEST_HPP_

#include <future>
#include <memory>
#include <vector>

namespace x {
namespace Exceptions {
class RequestExecutionException;
}
using Exceptions::RequestExecutionException;

namespace Configurations {
class OptimizerConfiguration;
}

class WorkerRPCClient;
using WorkerRPCClientPtr = std::shared_ptr<WorkerRPCClient>;

namespace RequestProcessor::Experimental {

using RequestId = uint64_t;
enum class ResourceType : uint8_t;

//the base class for the responses to be given to the creator of the request
struct AbstractRequestResponse {};
using AbstractRequestResponsePtr = std::shared_ptr<AbstractRequestResponse>;

class StorageHandler;
using StorageHandlerPtr = std::shared_ptr<StorageHandler>;

/**
 * @brief is the abstract base class for any kind of coordinator side request to deploy or undeploy queries, change the topology or perform
 * other actions. Specific request types are implemented as subclasses of this request.
 */
class AbstractRequest;
using AbstractRequestPtr = std::shared_ptr<AbstractRequest>;

class AbstractRequest : public std::enable_shared_from_this<AbstractRequest> {
  public:
    /**
     * @brief constructor
     * @param requiredResources: as list of resource types which indicates which resources will be accessed t oexecute the request
     * @param maxRetries: amount of retries to execute the request after execution failed due to errors
     */
    explicit AbstractRequest(const std::vector<ResourceType>& requiredResources, uint8_t maxRetries);

    /**
     * @brief Acquires locks on the needed resources and executes the request logic
     * @param storageHandle: a handle to access the coordinators data structures which might be needed for executing the
     * request
     * @return a list of follow up requests to be executed (can be empty if no further actions are required)
     */
    std::vector<AbstractRequestPtr> execute(const StorageHandlerPtr& storageHandle);

    /**
     * @brief Roll back any changes made by a request that did not complete due to errors.
     * @param ex: The exception thrown during request execution.
     * @param storageHandle: The storage access handle that was used by the request to modify the system state.
     * @return a list of follow up requests to be executed (can be empty if no further actions are required)
     */
    virtual std::vector<AbstractRequestPtr> rollBack(RequestExecutionException& ex, const StorageHandlerPtr& storageHandle) = 0;

    /**
     * @brief Calls rollBack and executes additional error handling based on the exception if necessary
     * @param ex: The exception thrown during request execution.
     * @param storageHandle: The storage access handle that was used by the request to modify the system state.
     * @return a list of follow up requests to be executed (can be empty if no further actions are required)
     */
    std::vector<AbstractRequestPtr> handleError(RequestExecutionException& ex, const StorageHandlerPtr& storageHandle);

    /**
     * @brief Check if the request has already reached the maximum allowed retry attempts or if it can be retried again. If the
     * maximum is not reached yet, increase the amount of recorded actual retries.
     * @return true if the actual retries are less than the allowed maximum
     */
    bool retry();

    /**
     * @brief creates a future which will contain the response supplied by this request
     * @return a future containing a pointer to the response object
     */
    std::future<AbstractRequestResponsePtr> getFuture();

    /**
     * @brief set the id of this request. This has to be done before the request is executed.
     * @param requestId
     */
    void setId(RequestId requestId);

    /**
     * @brief destructor
     */
    virtual ~AbstractRequest() = default;

    /**
     * @brief Checks if this object is of type AbstractRequest
     * @tparam RequestType: a subclass ob AbstractRequest
     * @return bool true if object is of type AbstractRequest
     */
    template<class RequestType>
    bool instanceOf() {
        if (dynamic_cast<RequestType*>(this)) {
            return true;
        }
        return false;
    };

    /**
    * @brief Dynamically casts the exception to the given type
    * @tparam RequestType: a subclass ob AbstractRequest
    * @return returns a shared pointer of the given type
    */
    template<class RequestType>
    std::shared_ptr<RequestType> as() {
        if (instanceOf<RequestType>()) {
            return std::dynamic_pointer_cast<RequestType>(this->shared_from_this());
        }
        throw std::logic_error("Exception:: we performed an invalid cast of exception");
    }

  protected:
    /**
     * @brief Performs request specific error handling to be done before changes to the storage are rolled back
     * @param ex: The exception encountered
     * @param storageHandle: The storage access handle used by the request
     */
    virtual void preRollbackHandle(const RequestExecutionException& ex, const StorageHandlerPtr& storageHandle) = 0;

    /**
     * @brief Performs request specific error handling to be done after changes to the storage are rolled back
     * @param ex: The exception encountered
     * @param storageHandle: The storage access handle used by the request
     */
    virtual void postRollbackHandle(const RequestExecutionException& ex, const StorageHandlerPtr& storageHandle) = 0;

    /**
     * @brief Performs steps to be done before execution of the request logic, e.g. locking the required data structures
     * @param storageHandle: The storage access handle used by the request
     */
    virtual void preExecution(const StorageHandlerPtr& storageHandle);

    /**
     * @brief Performs steps to be done after execution of the request logic, e.g. unlocking the required data structures
     * @param storageHandle: The storage access handle used by the request
     */
    virtual void postExecution(const StorageHandlerPtr& storageHandle) = 0;

    /**
     * @brief Executes the request logic.
     * @param storageHandle: a handle to access the coordinators data structures which might be needed for executing the
     * request
     * @return a list of follow up requests to be executed (can be empty if no further actions are required)
     */
    virtual std::vector<AbstractRequestPtr> executeRequestLogic(const StorageHandlerPtr& storageHandle) = 0;

  protected:
    RequestId requestId;
    std::promise<AbstractRequestResponsePtr> responsePromise;

  private:
    uint8_t maxRetries;
    uint8_t actualRetries;
    std::vector<ResourceType> requiredResources;
};
}// namespace RequestProcessor::Experimental
}// namespace x
#endif// x_CORE_INCLUDE_WORKQUEUES_REQUESTTYPES_ABSTRACTREQUEST_HPP_