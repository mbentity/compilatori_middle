#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/Dominators.h>

using namespace llvm;

class LicmCustom final : public PassInfoMixin<LicmCustom> {

  bool isLoopInvariant(Instruction &I, Loop &L, LoopStandardAnalysisResults &LSAR) {
    for (unsigned OpIdx = 0, EndOp = I.getNumOperands(); OpIdx != EndOp; ++OpIdx) {
      Value *Op = I.getOperand(OpIdx);
      if (Instruction *OpInst = dyn_cast<Instruction>(Op)) {
        if (L.contains(OpInst)) {
          return false;
        }
      }
    }
    return true;
  }

  bool canBeHoisted(Instruction &I, Loop &L, DominatorTree &DT, SmallVectorImpl<BasicBlock *> &ExitBlocks) {
    for (BasicBlock *ExitBlock : ExitBlocks) {
      if (!DT.dominates(I.getParent(), ExitBlock)) {
        return false;
      }
    }
    return true;
  }

public:
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LSAR, LPMUpdater &Updater) {
    bool Transformed = false;
    // check if loop is in simplified form
    if (!L.isLoopSimplifyForm()) {
      return PreservedAnalyses::all();
    }
    // get preheader
    auto *Preheader = L.getLoopPreheader();
    if (!Preheader) {
      return PreservedAnalyses::all();
    }
    // calculate dominator tree
    DominatorTree &DT = LSAR.DT;
    SmallVector<BasicBlock *, 8> ExitBlocks;
    L.getUniqueExitBlocks(ExitBlocks);
    // iterate over all blocks in the loop
    for (auto *Block : L.blocks()) {
      for (auto &Inst : llvm::make_early_inc_range(*Block)) {
        if (isLoopInvariant(Inst, L, LSAR) && canBeHoisted(Inst, L, DT, ExitBlocks)) {
          Inst.moveBefore(Preheader->getTerminator());
          Transformed = true;
        }
      }
    }

    if (Transformed) {
      return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
  }

};