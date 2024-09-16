
#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/Constants.h>

using namespace llvm;

bool runOnBasicBlock(BasicBlock &B)
{
  outs() << "Basic Block: " << B.getName() << "\n";
  bool Transformed = false;
  std::vector<Instruction *> ToErase;
  for(auto Iter = B.begin(); Iter != B.end(); ++Iter)
  {
    outs() << "Instruction: " << *Iter << "\n";
    if(BinaryOperator *BO = dyn_cast<BinaryOperator>(&*Iter))
    {
      if(BO->getOpcode() == Instruction::Add)
      {
        if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(0)))
        {
          if(C->isZero())
          {
            outs() << "Applying algebraic identity for 0\n";
            BO->replaceAllUsesWith(BO->getOperand(1));
            ToErase.push_back(BO);
            Transformed = true;
          }
        }
        else if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1)))
        {
          if(C->isZero())
          {
            outs() << "Applying algebraic identity for 0\n";
            BO->replaceAllUsesWith(BO->getOperand(0));
            ToErase.push_back(BO);
            Transformed = true;
          }
        }
      }
      else if(BO->getOpcode() == Instruction::Mul)
      {
        if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1)))
        {
          if(C->isOne())
          {
            outs() << "Applying algebraic identity for 1\n";
            BO->replaceAllUsesWith(BO->getOperand(0));
            ToErase.push_back(BO);
            Transformed = true;
          }
          else if(C->getValue().getLimitedValue() == 15)
          {
            outs() << "Applying strength reduction\n";
            Instruction *NewShiftInst = BinaryOperator::Create
            (
              Instruction::Shl, BO->getOperand(0),
              ConstantInt::get(BO->getType(), 4)
            );
            Instruction *NewSubInst = BinaryOperator::Create
            (
              Instruction::Sub,
              NewShiftInst,
              BO->getOperand(0)
            );
            NewShiftInst->insertAfter(BO);
            NewSubInst->insertAfter(NewShiftInst);
            BO->replaceAllUsesWith(NewSubInst);
            ToErase.push_back(BO);
            Transformed = true;
          }
        }
        else if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(0)))
        {
          if(C->isOne())
          {
            outs() << "Applying algebraic identity for 1\n";
            BO->replaceAllUsesWith(BO->getOperand(1));
            ToErase.push_back(BO);
            Transformed = true;
          }
          else if(C->getValue().getLimitedValue() == 15)
          {
            outs() << "Applying strength reduction\n";
            Instruction *NewShiftInst = BinaryOperator::Create
            (
              Instruction::Shl, BO->getOperand(1),
              ConstantInt::get(BO->getType(), 4)
            );
            Instruction *NewSubInst = BinaryOperator::Create
            (
              Instruction::Sub,
              NewShiftInst,
              BO->getOperand(1)
            );
            NewShiftInst->insertAfter(BO);
            NewSubInst->insertAfter(NewShiftInst);
            BO->replaceAllUsesWith(NewSubInst);
            ToErase.push_back(BO);
            Transformed = true;
          }
        }
      }
      else if(BO->getOpcode() == Instruction::SDiv)
      {
        if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1)))
        {
          if(C->getValue().getLimitedValue() == 8)
          {
            outs() << "Applying strength reduction\n";
            Instruction *NewInst = BinaryOperator::Create
            (
              Instruction::AShr, BO->getOperand(0),
              ConstantInt::get(BO->getType(), 3)
            );
            NewInst->insertAfter(BO);
            BO->replaceAllUsesWith(NewInst);
            ToErase.push_back(BO);
            Transformed = true;
          }
        }
      }
    }
  }

  for(auto I : ToErase)
  {
    I->eraseFromParent();
  }
  ToErase.clear();

  outs() << "Multi Instruction Optimization:\n";
  for(auto Iter = B.begin(); Iter != B.end(); ++Iter)
  {
    outs() << "Instruction: " << *Iter << "\n";
    if(BinaryOperator *BO = dyn_cast<BinaryOperator>(&*Iter))
    {
      outs() << "Is a binary operator\n";
      if(BO->getOpcode() == Instruction::Add)
      {
        outs() << "Is an add instruction\n";
        if(ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1)))
        {
          outs() << "The second operand is a constant\n";
          if(C->isOne())
          {
            outs() << "The constant is 1\n";
            outs() << "Scanning next instructions:\n";
            for(auto Iter2 = std::next(Iter); Iter2 != B.end(); ++Iter2)
            {
              outs() << "Scanning Instruction: " << *Iter2 << "\n";
              if(BinaryOperator *BO2 = dyn_cast<BinaryOperator>(&*Iter2))
              {
                outs() << "Is a binary operator\n";
                if(BO2->getOpcode() == Instruction::Sub)
                {
                  outs() << "Is a sub instruction\n";
                  if(BO2->getOperand(0) == BO)
                  {
                    outs() << "The first operand is the add instruction\n";
                    if(ConstantInt *C2 = dyn_cast<ConstantInt>(BO2->getOperand(1)))
                    {
                      if(C2->isOne())
                      {
                        outs() << "Applying multi instruction optimization\n";
                        BO2->replaceAllUsesWith(BO->getOperand(0));
                        ToErase.push_back(BO2);
                        Transformed = true;
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for(auto I : ToErase)
  {
    I->eraseFromParent();
  }

  return Transformed;
}

bool runOnFunction(Function &F)
{
  outs() << "Function: " << F.getName() << "\n";
  bool Transformed = false;
  for(auto Iter = F.begin(); Iter != F.end(); ++Iter)
  {
    if(runOnBasicBlock(*Iter))
    {
      Transformed = true;
    }
  }
  return Transformed;
}

PreservedAnalyses LocalOpts::run(Module &M, ModuleAnalysisManager &AM)
{
  for(auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
  {
    if(runOnFunction(*Fiter))
    {
      return PreservedAnalyses::none();
    }
  }
  return PreservedAnalyses::all();
}