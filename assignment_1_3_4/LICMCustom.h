#ifndef LLVM_TRANSFORMS_LICMCUSTOM_H
#define LLVM_TRANSFORMS_LICMCUSTOM_H

#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"

namespace llvm {
    class LICMCustom : public PassInfoMixin<LICMCustom> {
        public:
            PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR, LPMUpdater &U);
    };
}

#endif