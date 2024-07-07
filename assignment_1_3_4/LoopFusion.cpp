#include "llvm/IR/Dominators.h
#include <llvm/Analysis/LoopPass.h>

using namespace llvm;

namespace {
class LoopFusionPass : public FunctionPass {
public:
  static char ID;
  LoopFusionPass() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    SmallVector<Loop *, 8> Worklist;

    for (Loop *TopLevelLoop : LI)
      for (Loop *L : depth_first(TopLevelLoop))
        if (L->isInnermost())
          Worklist.push_back(L);

    for (size_t i = 0; i + 1 < Worklist.size(); ++i) {
      Loop *Lj = Worklist[i];
      Loop *Lk = Worklist[i + 1];

      if (!areAdjacent(Lj, Lk))
        continue;

      if (!iterateSameNumberOfTimes(Lj, Lk))
        continue;

      if (!areControlFlowEquivalent(Lj, Lk))
        continue;

      if (hasNegativeDistanceDependencies(Lj, Lk))
        continue;

      fuseLoops(Lj, Lk);
    }

    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
  }

private:
  bool areAdjacent(Loop *Lj, Loop *Lk) {
    auto *LjExitBlock = Lj->getExitBlock();
    auto *LkHeader = Lk->getHeader();

    if (!LjExitBlock || !LkHeader)
      return false;

    auto *LjSucc = LjExitBlock->getSingleSuccessor();
    return LjSucc == LkHeader;
  }

  bool iterateSameNumberOfTimes(Loop *Lj, Loop *Lk) {
    auto *LjCount = Lj->getTripCount();
    auto *LkCount = Lk->getTripCount();

    if (!LjCount || !LkCount)
      return false;

    return LjCount == LkCount;
  }

  bool areControlFlowEquivalent(Loop *Lj, Loop *Lk) {
    return (Lj->getLoopPreheader() == Lk->getLoopPreheader()) &&
           (Lj->getExitBlock() == Lk->getExitBlock());
  }

  bool hasNegativeDistanceDependencies(Loop *Lj, Loop *Lk) {
    return false;
  }

  void fuseLoops(Loop *Lj, Loop *Lk) {
    auto *LjBody = Lj->getLoopLatch();
    auto *LkBody = Lk->getLoopLatch();
    BasicBlock *LkPreheader = Lk->getLoopPreheader();
    BasicBlock *LkHeader = Lk->getHeader();
    BasicBlock *LkLatch = Lk->getLoopLatch();

    LjBody->getTerminator()->setSuccessor(0, LkHeader);
    LkLatch->getTerminator()->setSuccessor(0, LjBody);

    LkPreheader->replaceAllUsesWith(LjBody);
    LkPreheader->eraseFromParent();
  }
};

char LoopFusionPass::ID = 0;
static RegisterPass<LoopFusionPass> X("loop-fusion", "Loop Fusion Pass", false, false);
} // namespace