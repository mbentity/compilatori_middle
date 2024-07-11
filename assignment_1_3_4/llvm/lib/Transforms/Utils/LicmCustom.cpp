#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

class LicmCustom final : public PassInfoMixin<LicmCustom> {

public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LSAR, LPMUpdater &Updater) {
    // insert here transformations
    return PreservedAnalyses::none();
  }

};