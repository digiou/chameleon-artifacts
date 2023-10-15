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
#ifndef x_CORE_INCLUDE_SERVICES_QUERYPARSINGSERVICE_HPP_
#define x_CORE_INCLUDE_SERVICES_QUERYPARSINGSERVICE_HPP_
#include <API/QueryAPI.hpp>
#include <memory>

namespace x {
class Query;
using QueryPtr = std::shared_ptr<Query>;

class Schema;
using SchemaPtr = std::shared_ptr<Schema>;
}// namespace x

namespace x::Compiler {
class JITCompiler;
}
namespace x {

class QueryParsingService {
  public:
    explicit QueryParsingService(std::shared_ptr<Compiler::JITCompiler> jitCompiler);
    static std::shared_ptr<QueryParsingService> create(std::shared_ptr<Compiler::JITCompiler>);

    /**
    *  @brief this function **executes** the C++ code provided by the user and returns an Query Object
    *  @caution: this function will replace the source name "xyz" provided by the user with the source generated by the system and fetched from the catalog
    *  @param query as a string
    *  @return Smart pointer to InputQuery object of the query
    */
    QueryPlanPtr createQueryFromCodeString(const std::string& queryCodeSnippet);

    /**
    *  @brief this function parses the declarative pattern string provided by the user and returns an Query Object
    *  @caution: this function will replace the source name "xyz" provided by the user with the source generated by the system and fetched from the catalog
    *  @param query as a string
    *  @return Smart pointer to InputQuery object of the query
    */
    QueryPlanPtr createPatternFromCodeString(const std::string& queryCodeSnippet);

    /**
    *  @brief this function **executes** the code provided by the user and returns an schema Object
    *  @param query as a string
    *  @return Smart pointer to InputQuery object of the query
    *
    */
    SchemaPtr createSchemaFromCode(const std::string& schemaCodeSnippet);

  private:
    std::shared_ptr<Compiler::JITCompiler> jitCompiler;
};

}// namespace x

#endif// x_CORE_INCLUDE_SERVICES_QUERYPARSINGSERVICE_HPP_