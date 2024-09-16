#include "llvm/Transforms/Utils/LoopFusion.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/IR/Constants.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

bool areLoopsAdjacent(Loop *L1, Loop *L2)
{
  SmallVector<BasicBlock *, 4> ExitBlock;
  L1->getUniqueNonLatchExitBlocks(ExitBlock);
  for(BasicBlock *BB : ExitBlock)
  {
    if(L2->isGuarded() && BB!=dyn_cast<BasicBlock>(L2->getLoopGuardBranch()))
    {
      outs() << "Second loop guard exists, but it is not first loop exit block\n";
      return false;
    }
    if(BB != L2->getLoopPreheader())
    {
      outs() << "Second loop guard does not exist, but second loop preheader is not first loop exit block\n";
      return false;
    }
  }
  outs() << "Loops are adjacent\n";
  return true;
}

bool haveSameTripCount(Loop *L1, Loop *L2, ScalarEvolution &SE)
{
  const SCEV *S1 = SE.getBackedgeTakenCount(L1);
  const SCEV *S2 = SE.getBackedgeTakenCount(L2);
  if(S1 == S2)
  {
    outs() << "Loops have same trip count\n";
    return true;
  }
  else
  {
    outs() << "Loops do not have same trip count\n";
    return false;
  }
}

bool areLoopsControlFlowEquivalent(Loop *L1, Loop *L2, DominatorTree &DT, PostDominatorTree &PDT)
{
  SmallVector<BasicBlock *> L1ExitBlocks;
  L1->getExitBlocks(L1ExitBlocks);

  for(BasicBlock *exitBlock : L1ExitBlocks)
  {
    BasicBlock *nextBB = exitBlock->getTerminator()->getSuccessor(0);
    int successorsCount = exitBlock->getTerminator()->getNumSuccessors();
    if(successorsCount > 0 && !DT.dominates(exitBlock, nextBB) && !PDT.dominates(nextBB, exitBlock))
    {
      outs() << "Loops are not control flow equivalent\n";
      return false;
    }
  }
  outs() << "Loops are control flow equivalent\n";
  return true;
}

bool areLoopsNegativeDependant(Loop *L1, Loop *L2, DependenceInfo &DI)
{
  for(auto *BB1 : L1->getBlocks())
  {
    for(auto *BB2 : L2->getBlocks())
    {
      for(auto &I1 : *BB1)
      {
        for(auto &I2 : *BB2)
        {
          auto D = DI.depends(&I1, &I2, true);
          if(D)
          {
            outs() << "Found dependance between instructions:\n";
            I1.print(outs());
            I2.print(outs());
            if(D->isAnti())
            {
              outs() << "Dependance is negative\n";
              return true;
            }
          }
        }
      }
    }
  }
  outs() << "No negative dependance found\n";
  return false;
}

void replaceInductionVariables(Loop *L1, Loop *L2)
{
  PHINode *indVar1 = L1->getCanonicalInductionVariable();
  PHINode *indVar2 = L2->getCanonicalInductionVariable();

  if(!indVar1 || !indVar2)
  {
    outs() << "Warning: at least one loop does not have a canonical induction variable\n";
    return;
  }

  std::vector<Instruction *> Users;
  for(auto &Use : indVar2->uses())
  {
    Instruction *User = cast<Instruction>(Use.getUser());
    if(L2->contains(User))
    {
      Users.push_back(User);
    }
  }
  for(auto *User : Users)
  {
    User->replaceUsesOfWith(indVar2, indVar1);
  }

  bool replacementSuccess = true;
  for(auto &Use : indVar2->uses())
  {
    Instruction *User = cast<Instruction>(Use.getUser());
    if(L2->contains(User) && User->getOperand(0) == indVar2)
    {
      replacementSuccess = false;
      break;
    }
  }

  if(replacementSuccess)
  {
    outs() << "Induction variables replaced successfully\n";
  }
  else
  {
    outs() << "Induction variables replacement failed\n";
  }
}

bool fuseLoops(Loop *L1, Loop *L2)
{
  outs() << "Starting loop fusion...\n";
  replaceInductionVariables(L1, L2);

  BasicBlock *header1 = L1->getHeader();
  BasicBlock *preheader1 = L1->getLoopPreheader();
  BasicBlock *latch1 = L1->getLoopLatch();
  BasicBlock *body1 = latch1->getSinglePredecessor();
  BasicBlock *exit1 = L1->getExitBlock();

  BasicBlock *header2 = L2->getHeader();
  BasicBlock *preheader2 = L2->getLoopPreheader();
  BasicBlock *latch2 = L2->getLoopLatch();
  BasicBlock *body2 = latch2->getSinglePredecessor();
  BasicBlock *exit2 = L2->getExitBlock();

  BasicBlock *body2entry;

  if(L2->contains(header2->getTerminator()->getSuccessor(0)))
  {
    body2entry = header2->getTerminator()->getSuccessor(0);
  }
  else
  {
    body2entry = header2->getTerminator()->getSuccessor(1);
  }

  if(preheader2 != exit1)
  {
    outs() << "Second loop preheader is not first loop exit block\n";
    return false;
  }

  header1->getTerminator()->replaceSuccessorWith(preheader2, exit2);
  body1->getTerminator()->replaceSuccessorWith(latch1, body2entry);
  ReplaceInstWithInst(header2->getTerminator(), BranchInst::Create(latch2));
  body2->getTerminator()->replaceSuccessorWith(latch2, latch1);

  outs() << "Loop fusion completed\n";
  return true;
}

PreservedAnalyses LoopFusion::run(Function &F, FunctionAnalysisManager &AM)
{
  outs() << "Starting loop fusion optimization...\n";
  bool Transformed = false;

  LoopInfo &LI = AM.getResult<LoopAnalysis>(F);
  ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
  DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
  DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

  Loop *L1 = nullptr;

  for(auto lit = LI.rbegin(); lit != LI.rend(); ++lit)
  {
    Loop *L2 = *lit;
    if(L1)
    {
      if(
          areLoopsAdjacent(L1, L2) &&
          haveSameTripCount(L1, L2, SE) &&
          areLoopsControlFlowEquivalent(L1, L2, DT, PDT) &&
          !areLoopsNegativeDependant(L1, L2, DI) &&
          L1->isLoopSimplifyForm() &&
          L2->isLoopSimplifyForm()
        )
      {
        outs() << "Found candidate loops for fusion\n";
        fuseLoops(L1, L2);
        Transformed = true;
        L2 = *lit;
        L1 = L2;
        continue;
      }
    }
    L1 = L2;
  }

  outs() << "Loop fusion optimization completed\n";

  if(Transformed)
  {
    return PreservedAnalyses::none();
  }
  else
  {
    return PreservedAnalyses::all();
  }
}