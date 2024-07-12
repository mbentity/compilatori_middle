#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>

using namespace llvm;

class LocalOpts final : public PassInfoMixin<LocalOpts> {

  // check if loops are fusible
  bool areLoopsFusible(Loop *L1, Loop *L2, DominatorTree &DT, PostDominatorTree &PDT, ScalarEvolution &SE) {
    // if loops are guarded
    if (L1->getLoopPreheader() != L2->getLoopPreheader()) {
      // for L1, get loop guard
      BranchInst *L1Guard = dyn_cast<BranchInst>(L1->getLoopPreheader()->getTerminator());
      // if L1 guard successor is not L2 entry block, return false
      if (L1Guard->getSuccessor(0) != L2->getHeader()) {
        return false;
      }
    }
    else {
      // if L1 exit block is not L2 preheader, return false
      if (L1->getExitBlock() != L2->getLoopPreheader()) {
        return false;
      }
    }
    // if L1 does not dominate L2, return false
    if (!DT.dominates(L1->getHeader(), L2->getHeader())) {
      return false;
    }
    // if L2 does not post-dominate L1, return false
    if (!PDT.dominates(L2->getHeader(), L1->getHeader())) {
      return false;
    }
    // if L1 and L2 have different trip counts, return false
    if (SE.getBackedgeTakenCount(L1) != SE.getBackedgeTakenCount(L2)) {
      return false;
    }
    // check if second loop at iteration n uses value from first loop at iteration n+m
    for (BasicBlock *BB : L2->blocks()) {
      for (Instruction &I : *BB) {
        for (unsigned OpIdx = 0, EndOp = I.getNumOperands(); OpIdx != EndOp; ++OpIdx) {
          Value *Op = I.getOperand(OpIdx);
          if (Instruction *OpInst = dyn_cast<Instruction>(Op)) {
            if (L1->contains(OpInst)) {
              return false;
            }
          }
        }
      }
    }
    return true;
  }
  
  // try to fuse loops
  bool tryToFuseLoops(Loop *L1, Loop *L2, LoopInfo &LI) {
    // replace L2 induction variable uses with L1 induction variable
    PHINode *L1IndVar = L1->getCanonicalInductionVariable();
    PHINode *L2IndVar = L2->getCanonicalInductionVariable();
    L2IndVar->replaceAllUsesWith(L1IndVar);
    // get exit block of the first loop
    BasicBlock *L1Exit = L1->getExitBlock();
    // for all blocks in the second loop
    for (BasicBlock *BB : L2->blocks()) {
        // move block to the first loop
        BB->moveBefore(L1Exit);
    }
    // remove the second loop from the loop info
    LI.erase(L2);
    // return true if fusion was successful
    return true;
  }

public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    bool Transformed = false;
    DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
    PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
    ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
    // get loop info
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
    // init worklist with all loops
    SmallVector<Loop *, 8> Worklist(LI.begin(), LI.end());
    // for each loop in the worklist
    for (auto It = Worklist.begin(); It != Worklist.end(); ++It) {
      // get current loop
      Loop *CurrentLoop = *It;
      // check if there is a next loop
      if ((It + 1) != Worklist.end()) {
        // get next loop
        Loop *NextLoop = *(It + 1);
        // check if loops are fusible
        if (areLoopsFusible(CurrentLoop, NextLoop, DT, PDT, SE)) {
          // try to fuse loops
          if (tryToFuseLoops(CurrentLoop, NextLoop, LI)) {
            // remove next loop from the worklist if fusion was successful
            ++It;
            Transformed = true;
          }
        }
      }
    }
    if (Transformed) {
      return PreservedAnalyses::none();
    }
    return PreservedAnalyses::all();
  }

};