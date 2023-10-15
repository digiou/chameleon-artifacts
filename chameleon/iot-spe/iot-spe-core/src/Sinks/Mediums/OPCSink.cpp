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

#ifdef ENABLE_OPC_BUILD
#include <Sinks/Mediums/OPCSink.hpp>
#include <Sinks/Mediums/SinkMedium.hpp>
#include <cstring>
#include <sstream>
#include <string>

#include <Runtime/QueryManager.hpp>
#include <Util/Logger/Logger.hpp>
#include <utility>

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>

#include <iostream>
#include <stdlib.h>

namespace x {

OPCSink::OPCSink(SinkFormatPtr format,
                 Runtime::QueryManagerPtr queryManager,
                 const std::string& url,
                 UA_NodeId nodeId,
                 std::string user,
                 std::string password,
                 QueryId queryId,
                 QuerySubPlanId querySubPlanId)
    : SinkMedium(std::move(format), queryManager, queryId, querySubPlanId), connected(false), url(url), nodeId(nodeId),
      user(std::move(std::move(user))), password(std::move(password)), retval(UA_STATUSCODE_GOOD), client(UA_Client_new()) {
    x_DEBUG("OPCSINK   {} : Init OPC Sink to  {}  . {}", this, url);
}

OPCSink::~OPCSink() {
    x_DEBUG("OPCSink::~OPCSink: destructor called");
    bool success = disconnect();
    if (success) {
        x_DEBUG("OPCSink   {} : Destroy OPC Sink", this);
    } else {
        x_ASSERT2_FMT(false, "OPCSink  " << this << ": Destroy OPC Sink failed because it could not be disconnected");
    }
    x_DEBUG("OPCSink   {} : Destroy OPC Sink", this);
}

bool OPCSink::writeData(Runtime::TupleBuffer& inputBuffer, Runtime::WorkerContext&) {
    std::unique_lock lock(writeMutex);
    x_DEBUG("OPCSINK::writeData()   {}", this);
    x_DEBUG("OPCSINK::writeData url:  {} .", url);
    if (connect()) {

        /* Read current value of attribute, also necessary to get the type information of the node */
        auto* val = new UA_Variant;
        retval = UA_Client_readValueAttribute(client, nodeId, val);
        if (retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val)) {
            x_DEBUG("OPCSINK::writeData: Node exists, successfully obtained information about current node. ");
            /* Write node attribute with scalar value*/
            retval = UA_Variant_setScalarCopy(val, inputBuffer.getBuffer(), val->type);
        } else if (retval == UA_STATUSCODE_GOOD && UA_Variant_hasArrayType(val, val->type)) {
            x_DEBUG("OPCSINK::writeData: Node exists, successfully obtained information about current node. ");
            /* Write node attribute with array value*/
            retval = UA_Variant_setArrayCopy(val, inputBuffer.getBuffer(), inputBuffer.getBufferSize(), val->type);
        } else {
            x_ERROR("OPCSINK::writeData: Node does not exist or is not a scalar.");
            return false;
        }

        if (retval != UA_STATUSCODE_GOOD) {
            x_ERROR("OPCSINK::writeData: Format of value in input buffer and format of nodeid do not match.");
            return false;
        }

        retval = UA_Client_writeValueAttribute(client, nodeId, val);

        if (retval == UA_STATUSCODE_GOOD) {
            x_DEBUG("OPCSINK::writeData: Value has been successfully updated ");
        } else {
            x_ERROR("OPCSINK::writeData: Updating value was not possible.");
            return false;
        }

        UA_delete(val, val->type);

        auto* var = new UA_Variant;
        UA_Client_readValueAttribute(client, nodeId, var);
        if (retval == UA_STATUSCODE_GOOD && UA_Variant_isScalar(val)) {
            x_DEBUG("OPCSINK::writeData: New value is:  {}", *(UA_Int32*) var->data);
        } else {
            x_ERROR("OPCSINK::writeData: Node does not exist or is not a scalar.");
            return false;
        }
        UA_delete(var, var->type);

        x_DEBUG("OPCSOURCE::receiveData()   {} : got buffer ", this);
        x_DEBUG("OPCSINK::writeData() UA_StatusCode is:  {}", std::hex << retval);

    } else {
        x_ERROR("OPCSOURCE::receiveData(): Not connected!");
        return false;
    }
    return true;
}

void OPCSink::setup() { x_DEBUG("OPCSINK::setup"); }

void OPCSink::shutdown() { x_DEBUG("OPCSINK::shutdown"); }

std::string OPCSink::toString() const {
    char* ident = (char*) UA_malloc(sizeof(char) * nodeId.identifier.string.length + 1);
    memcpy(ident, nodeId.identifier.string.data, nodeId.identifier.string.length);
    ident[nodeId.identifier.string.length] = '\0';
    std::stringstream ss;
    ss << "OPC_SINK(";
    ss << "SCHEMA(" << sinkFormat->getSchemaPtr()->toString() << "), ";
    ss << "URL= " << this->getUrl() << ", ";
    ss << "NODE_INDEX= " << nodeId.namespaceIndex << ", ";
    ss << "NODE_IDENTIFIER= " << ident << ". ";

    return ss.str();
}

bool OPCSink::connect() {
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    if (!connected) {

        x_DEBUG("OPCSINK::connect(): was !conncect now connect  {} : connected", this);
        retval = UA_Client_connect(client, url.c_str());
        x_DEBUG("OPCSINK::connect(): connected without user or password");
        x_DEBUG("OPCSINK::connect(): use address  {}", url);

        if (retval != UA_STATUSCODE_GOOD) {

            UA_Client_delete(client);
            connected = false;
            x_ERROR("OPCSINK::connect(): ERROR with Status Code: {} OPCSINK {}: set connected false", retval, this);
        } else {

            connected = true;
        }
    }

    if (connected) {
        x_DEBUG("OPCSINK::connect():   {} : connected", this);
    } else {
        x_DEBUG("Exception: OPCSINK::connect():   {} : NOT connected", this);
    }
    return connected;
}

bool OPCSink::disconnect() {
    x_DEBUG("OPCSink::disconnect() connected= {}", connected);
    if (connected) {

        x_DEBUG("OPCSINK::disconnect() disconnect client");
        UA_Client_disconnect(client);
        x_DEBUG("OPCSINK::disconnect() delete client");
        UA_Client_delete(client);
        connected = false;
    }
    if (!connected) {
        x_DEBUG("OPCSINK::disconnect()   {} : disconnected", this);
    } else {
        x_DEBUG("OPCSINK::disconnect()   {} : NOT disconnected", this);
    }
    return !connected;
}

std::string OPCSink::getUrl() const { return url; }

UA_NodeId OPCSink::getNodeId() const { return nodeId; }

std::string OPCSink::getUser() const { return user; }

std::string OPCSink::getPassword() const { return password; }

SinkMediumTypes OPCSink::getSinkMediumType() { return SinkMediumTypes::OPC_SINK; }

UA_StatusCode OPCSink::getRetval() const { return retval; }

}// namespace x
#endif