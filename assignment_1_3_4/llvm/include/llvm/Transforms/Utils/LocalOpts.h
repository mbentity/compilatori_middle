#ifndef LLVM_TRANSFORMS_UTILS_LOCALOPTS_H
#define LLVM_TRANSFORMS_UTILS_LOCALOPTS_H

#include <llvm/IR/PassManager.h>

class LocalOpts final
    : public llvm::PassInfoMixin<LocalOpts> {
public:
  llvm::PreservedAnalyses run(llvm::Module &,
                              llvm::ModuleAnalysisManager &);
};

#endif