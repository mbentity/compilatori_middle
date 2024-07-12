#include <llvm/Passes/PassBuilder.h>

using namespace llvm;

class LocalOpts final : public PassInfoMixin<LocalOpts> {

  // if the basic block is modified, return true, otherwise return false
  bool runOnBasicBlock(BasicBlock &B) {

    // algebraic identity
    // x + 0 = x
    // 0 + x = x
    // x * 1 = x
    // 1 * x = x

    // strength reduction
    // 15 * x = (x << 4) - x
    // x * 15 = (x << 4) - x
    // x/8 = x >> 3

    for (auto Iter = B.begin(); Iter != B.end(); ++Iter) {
      if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&*Iter)) {
        if (BO->getOpcode() == Instruction::Add || BO->getOpcode() == Instruction::Mul) {
          if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(0))) {
            if (C->isZero()) {
              BO->replaceAllUsesWith(BO->getOperand(1));
              BO->eraseFromParent();
              return true;
            }
          }
          else if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
            if (C->isZero()) {
              BO->replaceAllUsesWith(BO->getOperand(0));
              BO->eraseFromParent();
              return true;
            }
          }
        }
        else if (BO->getOpcode() == Instruction::Mul) {
          if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
            if (C->isOne()) {
              BO->replaceAllUsesWith(BO->getOperand(0));
              BO->eraseFromParent();
              return true;
            }
            else if (C->getValue().getLimitedValue() == 15) {
              Instruction *NewInst = BinaryOperator::Create(
                Instruction::Shl, BO->getOperand(0),
                ConstantInt::get(BO->getType(), 4));
              NewInst->insertAfter(BO);
              BO->replaceAllUsesWith(NewInst);
              BO->eraseFromParent();
              return true;
            }
          }
          else if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(0))) {
            if (C->isOne()) {
              BO->replaceAllUsesWith(BO->getOperand(1));
              BO->eraseFromParent();
              return true;
            }
            else if (C->getValue().getLimitedValue() == 15) {
              Instruction *NewInst = BinaryOperator::Create(
                Instruction::Shl, BO->getOperand(1),
                ConstantInt::get(BO->getType(), 4));
              NewInst->insertAfter(BO);
              BO->replaceAllUsesWith(NewInst);
              BO->eraseFromParent();
              return true;
            }
          }
        }
        else if (BO->getOpcode() == Instruction::SDiv) {
          if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
            if (C->getValue().getLimitedValue() == 8) {
              Instruction *NewInst = BinaryOperator::Create(
                Instruction::AShr, BO->getOperand(0),
                ConstantInt::get(BO->getType(), 3));
              NewInst->insertAfter(BO);
              BO->replaceAllUsesWith(NewInst);
              BO->eraseFromParent();
              return true;
            }
          }
        }
      }
    }

    // multi instruction optimization
    // if a = b + 1
    // and c = a - 1
    // then c = b

    // for each instruction in the basic block
    for (auto Iter = B.begin(); Iter != B.end(); ++Iter) {
      // if the instruction is a binary operator
      if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&*Iter)) {
        // if the instruction is an add instruction
        if (BO->getOpcode() == Instruction::Add) {
          // if the second operand of the add instruction is a constant
          if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
            // if the constant is 1
            if (C->getValue().getLimitedValue() == 1) {
              // for each instruction in the basic block after the add instruction
              for (auto Iter2 = std::next(Iter); Iter2 != B.end(); ++Iter2) {
                // if the instruction is a binary operator
                if (BinaryOperator *BO2 = dyn_cast<BinaryOperator>(&*Iter2)) {
                  // if the instruction is a sub instruction
                  if (BO2->getOpcode() == Instruction::Sub) {
                    // if the first operand of the sub instruction is the add instruction
                    if (BO2->getOperand(0) == BO) {
                      // if the second operand of the sub instruction is a constant
                      if (ConstantInt *C2 = dyn_cast<ConstantInt>(BO2->getOperand(1))) {
                        // if the constant is 1
                        if (C2->getValue().getLimitedValue() == 1) {
                          // replace the sub instruction with the first operand of the add instruction
                          BO2->replaceAllUsesWith(BO->getOperand(0));
                          // erase the sub instruction
                          BO2->eraseFromParent();
                          return true;
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

    return false;
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