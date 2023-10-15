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

#ifndef x_RUNTIME_INCLUDE_NAUTILUS_BACKENDS_MLIR_MLIRPASSMANAGER_HPP_
#define x_RUNTIME_INCLUDE_NAUTILUS_BACKENDS_MLIR_MLIRPASSMANAGER_HPP_

#include <mlir/IR/BuiltinOps.h>
#include <mlir/Pass/Pass.h>
#include <vector>

namespace x::Nautilus::Backends::MLIR {

// The MLIRPassManager takes a generated MLIR module,
// and applies configured lowering & optimization passes to it.
class MLIRPassManager {
  public:
    enum class LoweringPass : uint8_t { SCF, LLVM };
    enum class OptimizationPass : uint8_t { Inline };

    MLIRPassManager(); // Disable default constructor
    ~MLIRPassManager();// Disable default destructor

    static int lowerAndOptimizeMLIRModule(mlir::OwningOpRef<mlir::ModuleOp>& module,
                                          std::vector<LoweringPass> loweringPasses,
                                          std::vector<OptimizationPass> optimizationPasses);
};
}// namespace x::Nautilus::Backends::MLIR
#endif// x_RUNTIME_INCLUDE_NAUTILUS_BACKENDS_MLIR_MLIRPASSMANAGER_HPP_
