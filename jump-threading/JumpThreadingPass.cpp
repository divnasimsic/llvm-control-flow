#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define DEBUG_TYPE "JumpThreadingPass"

namespace {

  struct JumpThreadingPass : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    JumpThreadingPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      //code:

      bool changed=false;

      for(BasicBlock &BB: F){

        //if terminator is conditional Branch:

        Instruction *Ter = BB.getTerminator();
        BranchInst *BI = dyn_cast_or_null<BranchInst>(Ter);
        if(!BI || !BI->isConditional())continue;

        Value *Cond = BI->getCondition();
        if (ICmpInst *Cmp = dyn_cast<ICmpInst>(Cond)) {
          Cond = Cmp->getOperand(0);
        }
        //taking PHI Node if exist
        PHINode *PN = dyn_cast<PHINode>(Cond);
        if (!PN || PN->getParent() != &BB) continue;

        //if conditional get successors:
        BasicBlock* ThenDest = BI->getSuccessor(0);
        BasicBlock* ElseDest = BI->getSuccessor(1);

        //now find and connect pn 0 ---> ThenDest , pn 1 ---> ElseDest
        for(int i=(int)PN->getNumIncomingValues()-1; i>=0; i--){

          //... [2,%3],[1,%4],[0,%5]
          Value *IncomingVal = PN->getIncomingValue(i); 
          BasicBlock *IncomingBB = PN->getIncomingBlock(i);//from witch we came

          ConstantInt *CI = dyn_cast<ConstantInt>(IncomingVal);
          if(!CI)continue; //some val in runtime we don't know jet

          Instruction *PredTer = IncomingBB->getTerminator();
          BranchInst *PredBI = dyn_cast_or_null<BranchInst>(PredTer);
          if(!PredBI || !PredBI->isUnconditional())continue; //we need unconditional simple PredBI

          BasicBlock* NewDest = nullptr;

          if(CI->isOne())NewDest = ThenDest; 
          else if(CI->isZero())NewDest = ElseDest;

          if(NewDest){
            //redirecting IncomingBB to NewDest
            PredBI->setSuccessor(0,NewDest); //always have ind 0
          
            //updating PHI Nodes (from NewDest)
            for(Instruction &Inst: *NewDest){
              
              //if some phi node show BB 
              if(PHINode *DestPN = dyn_cast<PHINode>(&Inst)){
                int ind = DestPN->getBasicBlockIndex(&BB);
                if(ind!=-1){
                  Value *ValFromBB = DestPN->getIncomingValue(ind);
                  DestPN->addIncoming(ValFromBB,IncomingBB);
                }
              }
              else break;
            }
            
            //for all PHI Nodes in BB
            for(Instruction &Inst: BB){
              if(PHINode *CurrentPN = dyn_cast<PHINode>(&Inst)){
                int idx = CurrentPN->getBasicBlockIndex(IncomingBB);
                if(idx!=-1){
                  CurrentPN->removeIncomingValue(idx,false);

                }
              }
              else break;
            }

            changed=true;
            break;
            //if(PN->getNumIncomingValues()==0)break;
          }
        } 
      }
      return changed;
    }
  };
}

char JumpThreadingPass::ID = 0;
static RegisterPass<JumpThreadingPass> X("our-jump-threading", "Our Jump Threading Pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);