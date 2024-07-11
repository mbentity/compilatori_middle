#include "llvm/IR/Dominators.h
#include <llvm/Analysis/LoopPass.h>

using namespace llvm;

namespace {

class LicmCustom final : public LoopPass {
public:
  static char ID;

  LicmCustom() : LoopPass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.setPreservesCFG();
  }

  virtual bool runOnLoop(Loop *L, LPPassManager &LPM) override {
    outs() << "\nLOOPPASS INIZIATO...\n";
    bool Changed = false;
    // loop invariant code motion
    LoopStandardAnalysisResults AR = {
      getAnalysis<DominatorTreeWrapperPass>().getDomTree(),
      getAnalysis<LoopInfoWrapperPass>().getLoopInfo(),
      getAnalysis<ScalarEvolutionWrapperPass>().getSE(),
      getAnalysis<BlockFrequencyInfoWrapperPass>().getBFI(),
      getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI(),
      getAnalysis<AssumptionCacheTracker>().getAssumptionCache(*L->getHeader()->getParent()),
      &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI(*L->getHeader()->getParent())
    };
    DominatorTree &DT = AR.DT;
    LoopInfo &LI = AR.LI;
    SmallPtrSet<BasicBlock *, 8> LoopBlocks(L->block_begin(), L->block_end());
    SmallVector<Instruction *, 8> InvariantInsts;
    for (auto *BB : L->blocks()) {
      for (auto &I : *BB) {
        if (L->isLoopInvariant(&I)) {
          InvariantInsts.push_back(&I);
        }
      }
    }
    SmallVector<BasicBlock *, 8> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);
    for (Instruction *I : InvariantInsts) {
      bool CanMove = true;
      for (BasicBlock *ExitBlock : ExitingBlocks) {
        if (!DT.dominates(I->getParent(), ExitBlock)) {
          CanMove = false;
          break;
        }
      }
      for (Use &U : I->operands()) {
        Instruction *OperandInst = dyn_cast<Instruction>(U);
        if (OperandInst && LoopBlocks.count(OperandInst->getParent())) {
          if (std::find(InvariantInsts.begin(), InvariantInsts.end(), OperandInst) == InvariantInsts.end()) {
            CanMove = false;
            break;
          }
        }
      }
      if (CanMove) {
        BasicBlock *Preheader = L->getLoopPreheader();
        if (Preheader) {
          I->moveBefore(Preheader->getTerminator());
          Changed = true;
        }
      }
    }
    return Changed;
  }
};

PreservedAnalyses LicmCustom::run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  DominatorTree &DT = AR.DT;
  LoopInfo &LI = AR.LI;
  ScalarEvolution &SE = AR.SE;
  TargetLibraryInfo &TLI = AR.TLI;

  SmallPtrSet<BasicBlock *, 8> LoopBlocks(L.block_begin(), L.block_end());
  SmallVector<Instruction *, 8> InvariantInsts;

  for (auto *BB : L.blocks()) {
    for (auto &I : *BB) {
      if (L.isLoopInvariant(&I)) {
        InvariantInsts.push_back(&I);
      }
    }
  }

  SmallVector<BasicBlock *, 8> ExitingBlocks;
  L.getExitingBlocks(ExitingBlocks);

  bool Changed = false;
  for (Instruction *I : InvariantInsts) {
    bool CanMove = true;
    for (BasicBlock *ExitBlock : ExitingBlocks) {
      if (!DT.dominates(I->getParent(), ExitBlock)) {
        CanMove = false;
        break;
      }
    }
    for (Use &U : I->operands()) {
      Instruction *OperandInst = dyn_cast<Instruction>(U);
      if (OperandInst && LoopBlocks.count(OperandInst->getParent())) {
        if (std::find(InvariantInsts.begin(), InvariantInsts.end(), OperandInst) == InvariantInsts.end()) {
          CanMove = false;
          break;
        }
      }
    }
    if (CanMove) {
      BasicBlock *Preheader = L.getLoopPreheader();
      if (Preheader) {
        I->moveBefore(Preheader->getTerminator());
        Changed = true;
      }
    }
  }

  if (Changed) {
    return PreservedAnalyses::none();
  } else {
    return PreservedAnalyses::all();
  }
}

char LicmCustom::ID = 0;
RegisterPass<LicmCustom> X("loop-walk",
                             "Loop Walk");

} // anonymous namespace