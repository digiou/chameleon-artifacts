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
#include <QueryCompiler/CodeGenerator/CCodeGenerator/TensorflowAdapter.hpp>
#include <Util/Logger/Logger.hpp>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>

#ifdef TFDEF
#include <Common/PhysicalTypes/BasicPhysicalType.hpp>
#include <Util/magicenum/magic_enum.hpp>
#include <tensorflow/lite/c/c_api.h>
#include <tensorflow/lite/c/common.h>

#endif// TFDEF

#ifdef TFDEF
x::TensorflowAdapter::TensorflowAdapter() {}

void x::TensorflowAdapter::initializeModel(std::string model) {
    x_DEBUG("INITIALIZING MODEL:  {}", model);

    std::ifstream input(model, std::ios::in | std::ios::binary);

    std::string bytes((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));

    input.close();
    x_INFO("MODEL SIZE: {}", std::to_string(bytes.size()));
    TfLiteInterpreterOptions* options = TfLiteInterpreterOptionsCreate();
    interpreter = TfLiteInterpreterCreate(TfLiteModelCreateFromFile(model.c_str()), options);
    TfLiteInterpreterAllocateTensors(interpreter);
}

x::TensorflowAdapterPtr x::TensorflowAdapter::create() { return std::make_shared<TensorflowAdapter>(); }

float x::TensorflowAdapter::getResultAt(int i) { return output[i]; }

void x::TensorflowAdapter::infer(BasicPhysicalType::NativeType dataType, int n, ...) {

    va_list vl;
    va_start(vl, n);

    //create input for tensor
    TfLiteTensor* inputTensor = TfLiteInterpreterGetInputTensor(interpreter, 0);
    int inputSize = (int) (TfLiteTensorByteSize(inputTensor));

    //Prepare input parameters based on data type
    if (dataType == BasicPhysicalType::NativeType::INT_64) {

        int* inputData = (int*) malloc(inputSize);
        for (int i = 0; i < n; ++i) {
            inputData[i] = (int) va_arg(vl, int);
        }

        //Copy input tensor
        TfLiteTensorCopyFromBuffer(inputTensor, inputData, inputSize);
        //Invoke tensor model and perform inference
        TfLiteInterpreterInvoke(interpreter);

        //Release allocated memory for input
        free(inputData);

        //Clear allocated memory to output
        if (output != nullptr) {
            free(output);
        }

        //Prepare output tensor
        const TfLiteTensor* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        int outputSize = (int) (TfLiteTensorByteSize(outputTensor));
        output = (float*) malloc(outputSize);

        //Copy value to the output
        TfLiteTensorCopyToBuffer(outputTensor, output, outputSize);

    } else if (dataType == BasicPhysicalType::NativeType::FLOAT) {
        //create input for tensor
        float* inputData = (float*) malloc(inputSize);
        for (int i = 0; i < n; ++i) {
            inputData[i] = (float) va_arg(vl, double);
        }

        //Copy input tensor
        TfLiteTensorCopyFromBuffer(inputTensor, inputData, inputSize);
        //Invoke tensor model and perform inference
        TfLiteInterpreterInvoke(interpreter);

        //Release allocated memory for input
        free(inputData);

        //Clear allocated memory to output
        if (output != nullptr) {
            free(output);
        }

        //Prepare output tensor
        const TfLiteTensor* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        int outputSize = (int) (TfLiteTensorByteSize(outputTensor));
        output = (float*) malloc(outputSize);

        //Copy value to the output
        TfLiteTensorCopyToBuffer(outputTensor, output, outputSize);

    } else if (dataType == BasicPhysicalType::NativeType::DOUBLE) {
        //create input for tensor
        double* inputData = (double*) malloc(inputSize);
        for (int i = 0; i < n; ++i) {
            inputData[i] = (double) va_arg(vl, double);
        }

        //Copy input tensor
        TfLiteTensorCopyFromBuffer(inputTensor, inputData, inputSize);
        //Invoke tensor model and perform inference
        TfLiteInterpreterInvoke(interpreter);

        //Release allocated memory for input
        free(inputData);

        //Clear allocated memory to output
        if (output != nullptr) {
            free(output);
        }

        //Prepare output tensor
        const TfLiteTensor* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        int outputSize = (int) (TfLiteTensorByteSize(outputTensor));
        output = (float*) malloc(outputSize);

        //Copy value to the output
        TfLiteTensorCopyToBuffer(outputTensor, output, outputSize);

    } else if (dataType == BasicPhysicalType::NativeType::BOOLEAN) {
        //create input for tensor
        bool* inputData = (bool*) malloc(inputSize);
        for (int i = 0; i < n; ++i) {
            inputData[i] = (bool) va_arg(vl, int);
        }

        //Copy input tensor
        TfLiteTensorCopyFromBuffer(inputTensor, inputData, inputSize);
        //Invoke tensor model and perform inference
        TfLiteInterpreterInvoke(interpreter);

        //Release allocated memory for input
        free(inputData);

        //Clear allocated memory to output
        if (output != nullptr) {
            free(output);
        }

        //Prepare output tensor
        const TfLiteTensor* outputTensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        int outputSize = (int) (TfLiteTensorByteSize(outputTensor));
        output = (float*) malloc(outputSize);

        //Copy value to the output
        TfLiteTensorCopyToBuffer(outputTensor, output, outputSize);
    }

    va_end(vl);
}

#endif// TFDEF