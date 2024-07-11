#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

class LocalOpts final : public PassInfoMixin<LocalOpts> {

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    // insert here transformations
    return PreservedAnalyses::none();
  }

};