#ifndef LLVM_TRANSFORMS_LICMCUSTOM_H
#define LLVM_TRANSFORMS_LICMCUSTOM_H

#include <llvm/IR/PassManager.h>

class LicmCustom final
    : public llvm::PassInfoMixin<LicmCustom> {
public:
  llvm::PreservedAnalyses run(llvm::Loop &,
                              llvm::LoopAnalysisManager &,
                              llvm::LoopStandardAnalysisResults &,
                              llvm::LPMUpdater &);
};

#endif