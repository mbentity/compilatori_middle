#ifndef LLVM_TRANSFORMS_UTILS_LOOPFUSION_H
#define LLVM_TRANSFORMS_UTILS_LOOPFUSION_H

#include <llvm/IR/PassManager.h>

class LoopFusion final
    : public llvm::PassInfoMixin<LoopFusion> {
public:
  llvm::PreservedAnalyses run(llvm::Function &,
                              llvm::FunctionAnalysisManager &);
};

#endif