#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

class LocalOpts final : public PassInfoMixin<LocalOpts> {

  // if the basic block is modified, return true, otherwise return false
  bool runOnBasicBlock(BasicBlock &B) {
    bool Transformed = false;
    // insert here transformations, and if something changes,
    // set Transformed to true
    return Transformed;
  }

  // if the function is modified, return true, otherwise return false
  bool runOnFunction(Function &F) {
    bool Transformed = false;
    for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
      if (runOnBasicBlock(*Iter)) {
        Transformed = true;
      }
    }
    return Transformed;
  }

public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    for (auto Iter = M.begin(); Iter != M.end(); ++Iter) {
      if (runOnFunction(*Iter)) {
        return PreservedAnalyses::none();
      }
    }

  return PreservedAnalyses::none();
  }

};