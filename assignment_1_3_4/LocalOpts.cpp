//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/Constants.h>

using namespace llvm;

bool runOnBasicBlock(BasicBlock &B) {
    
    // Preleviamo le prime due istruzioni del BB
    Instruction &Inst1st = *B.begin(), &Inst2nd = *(++B.begin());

    // L'indirizzo della prima istruzione deve essere uguale a quello del 
    // primo operando della seconda istruzione (per costruzione dell'esempio)
    assert(&Inst1st == Inst2nd.getOperand(0));

    // Stampa la prima istruzione
    outs() << "PRIMA ISTRUZIONE: " << Inst1st << "\n";
    // Stampa la prima istruzione come operando
    outs() << "COME OPERANDO: ";
    Inst1st.printAsOperand(outs(), false);
    outs() << "\n";

    // User-->Use-->Value
    outs() << "I MIEI OPERANDI SONO:\n";
    for (auto *Iter = Inst1st.op_begin(); Iter != Inst1st.op_end(); ++Iter) {
      Value *Operand = *Iter;

      if (Argument *Arg = dyn_cast<Argument>(Operand)) {
        outs() << "\t" << *Arg << ": SONO L'ARGOMENTO N. " << Arg->getArgNo() 
	       <<" DELLA FUNZIONE " << Arg->getParent()->getName()
               << "\n";
      }
      if (ConstantInt *C = dyn_cast<ConstantInt>(Operand)) {
        outs() << "\t" << *C << ": SONO UNA COSTANTE INTERA DI VALORE " << C->getValue()
               << "\n";
      }
    }

    outs() << "LA LISTA DEI MIEI USERS:\n";
    for (auto Iter = Inst1st.user_begin(); Iter != Inst1st.user_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(*Iter)) << "\n";
    }

    outs() << "E DEI MIEI USI (CHE E' LA STESSA):\n";
    for (auto Iter = Inst1st.use_begin(); Iter != Inst1st.use_end(); ++Iter) {
      outs() << "\t" << *(dyn_cast<Instruction>(Iter->getUser())) << "\n";
    }

    // Manipolazione delle istruzioni

    // Identità Algebrica e Riduzione di Forza

    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(&Inst1st)) {
        if (BO->getOpcode() == Instruction::Add) {
            if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(1))) {
                if (C->isZero()) {
                    BO->replaceAllUsesWith(BO->getOperand(0));
                    BO->eraseFromParent();
                    return true;
                }
            }
            else if (ConstantInt *C = dyn_cast<ConstantInt>(BO->getOperand(0))) {
                if (C->isZero()) {
                    BO->replaceAllUsesWith(BO->getOperand(1));
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

    // Ottimizzazione Multi-Istruzione

    if (Instruction *Inst2nd = dyn_cast<Instruction>(Inst1st.getNextNode())) {
        if (Inst1st.getNumOperands() == 2 && Inst2nd->getNumOperands() == 2) {
            if (BinaryOperator *BO1 = dyn_cast<BinaryOperator>(&Inst1st)) {
                if (BO1->getOpcode() == Instruction::Add) {
                    if (ConstantInt *C1 = dyn_cast<ConstantInt>(BO1->getOperand(1))) {
                        if (C1->getValue().getLimitedValue() == 1) {
                            if (BinaryOperator *BO2 = dyn_cast<BinaryOperator>(Inst2nd)) {
                                if (BO2->getOpcode() == Instruction::Sub) {
                                    if (BO2->getOperand(0) == &Inst1st) {
                                        if (ConstantInt *C2 = dyn_cast<ConstantInt>(BO2->getOperand(1))) {
                                            if (C2->getValue().getLimitedValue() == 1) {
                                                Inst1st.replaceAllUsesWith(BO1->getOperand(0));
                                                Inst2nd->replaceAllUsesWith(BO1->getOperand(0));
                                                Inst1st.eraseFromParent();
                                                Inst2nd->eraseFromParent();
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


bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


PreservedAnalyses LocalOpts::run(Module &M,
                                      ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();
  
  return PreservedAnalyses::all();
}

