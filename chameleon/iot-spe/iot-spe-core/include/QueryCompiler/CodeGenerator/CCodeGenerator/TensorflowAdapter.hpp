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

#ifndef x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_TENSORFLOWADAPTER_HPP_
#define x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_TENSORFLOWADAPTER_HPP_

#include <Common/PhysicalTypes/BasicPhysicalType.hpp>
#include <memory>
#include <vector>

class TfLiteInterpreter;

namespace x {

class TensorflowAdapter;
typedef std::shared_ptr<TensorflowAdapter> TensorflowAdapterPtr;

class TensorflowAdapter {
#ifdef TFDEF
  public:
    static TensorflowAdapterPtr create();
    TensorflowAdapter();

    /**
     * @brief runs the tensorflow model of a single tuple
     * @param dataType enum of type BasicPhysicalType::NativeType; only available options currently are: INT_64, DOUBLE, and BOOLEAN
     * @param n size of the first dimensional input tensor (vector)
     * @param ... values for the first dimensional input tensor (vector)
     */
    void infer(BasicPhysicalType::NativeType dataType, int n, ...);

    /**
     * @brief accesses the ith field of the output
     * @param i
     * @return
     */
    float getResultAt(int i);
    void initializeModel(std::string model);
    void pass() {}

  private:
    TfLiteInterpreter* interpreter;
    // TODO https://github.com/IoTSPE/IoTSPE/issues/3424
    // Right now we only support 32-bit floats as output.
    float* output{};
#endif// TFDEF
};

}// namespace x

#endif// x_CORE_INCLUDE_QUERYCOMPILER_CODEGENERATOR_CCODEGENERATOR_TENSORFLOWADAPTER_HPP_
