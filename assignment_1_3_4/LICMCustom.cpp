#include "llvm/Transforms/Utils/LICMCustom.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;

bool isLoopInvariant(Instruction &I, Loop &L, LoopStandardAnalysisResults &LSAR)
{
  for(unsigned OpIdx = 0, EndOp = I.getNumOperands(); OpIdx != EndOp; ++OpIdx)
  {
    Value *Op = I.getOperand(OpIdx);
    if(Instruction *OpInst = dyn_cast<Instruction>(Op))
    {
      if(L.contains(OpInst))
      {
        outs() << "Instruction is not loop invariant\n";
        return false;
      }
    }
  }
  outs() << "Instruction is loop invariant\n";
  return true;
}

bool canBeHoisted(Instruction &I, Loop &L, DominatorTree &DT, SmallVectorImpl<BasicBlock *> &ExitBlocks)
{
  for(BasicBlock *ExitBlock : ExitBlocks)
  {
    if(!DT.dominates(I.getParent(), ExitBlock)) 
    {
      outs() << "Instruction can't be hoisted\n";
      return false;
    }
  }
  outs() << "Instruction can be hoisted\n";
  return true;
}

PreservedAnalyses LICMCustom::run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR, LPMUpdater &U)
{
  bool Transformed = false;
  outs() << "Running LICMCustom\n";
  if(!L.isLoopSimplifyForm())
  {
    outs() << "Loop is not in simplified form\n";
    return PreservedAnalyses::all();
  }
  auto *Preheader = L.getLoopPreheader();
  if(!Preheader) 
  {
    outs() << "Loop doesn't have a preheader\n";
    return PreservedAnalyses::all();
  }
  DominatorTree &DT = AR.DT;
  SmallVector<BasicBlock *, 8> ExitBlocks;
  L.getUniqueExitBlocks(ExitBlocks);
  outs() << "Iterating over all blocks in the loop\n";
  for(auto *Block : L.blocks())
  {
    outs() << "Entering New Block\n";
    for(auto &Inst : llvm::make_early_inc_range(*Block))
    {
      outs() << "Instruction: " << Inst << "\n";
      if(isLoopInvariant(Inst, L, AR) && canBeHoisted(Inst, L, DT, ExitBlocks))
      {
        outs() << "Hoisting instruction: " << Inst << "\n";
        Inst.moveBefore(Preheader->getTerminator());
        Transformed = true;
      }
    }
  }

  if(Transformed)
  {
    return PreservedAnalyses::none();
  }
  else
  {
    return PreservedAnalyses::all();
  }
}