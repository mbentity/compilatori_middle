#ifndef LLVM_TRANSFORMS_LICMCUSTOM_H
#define LLVM_TRANSFORMS_LICMCUSTOM_H
#include "llvm/IR/PassManager.h"
#include <llvm/Analysis/LoopPass.h>

namespace llvm {
class LicmCustom : public PassInfoMixin<LicmCustom> {
public:
PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR, LPMUpdater &U);
};
} // namespace llvm
#endif // LLVM_TRANSFORMS_LICMCUSTOM _H