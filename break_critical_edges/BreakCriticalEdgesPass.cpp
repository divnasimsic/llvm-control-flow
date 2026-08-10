#include "llvm/Pass.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "BreakCriticalEdgesPass"

namespace {

  struct BreakCriticalEdgesPass : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    BreakCriticalEdgesPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      
      bool changed = false;
      for(BasicBlock &A: F){
        //conditions:
        Instruction *Term = A.getTerminator();
        if(Term->getNumSuccessors()<=1)continue;

        for(unsigned i=0; i<Term->getNumSuccessors();i++){
          //next B
          BasicBlock *B = Term->getSuccessor(i);
          if(pred_size(B)<=1)continue;
          //now we have critical edge A->B

          changed=true;
          //1. new root
          
          BasicBlock *SplitB = BasicBlock::Create(F.getContext(),"split_b",&F);
          //unconditional inst SplitB->B
          BranchInst::Create(B,SplitB);
          Term->replaceSuccessorWith(B,SplitB);

          //2. updating basicblock B
          for(PHINode &PN: B->phis()){
            int ind = PN.getBasicBlockIndex(&A);
            if(ind!=-1)PN.setIncomingBlock(ind,SplitB);
          }
          //other way Inst -> dyn_cast ...
          break;//inf loop
        }
      }
      return changed;
    }
  };
}

char BreakCriticalEdgesPass::ID = 0;
static RegisterPass<BreakCriticalEdgesPass> X("our-break-critical-edges", "Our Break Critical Edges Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);