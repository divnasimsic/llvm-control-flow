#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>
#include "llvm/IR/CFG.h"

using namespace llvm;

namespace {
    struct SimplifyCFGPass : public FunctionPass {
        static char ID;

        SimplifyCFGPass() : FunctionPass(ID) {}

        std::vector<BasicBlock *> BlocksToRemove;

        bool isEmptyBlock(BasicBlock &BB) {

            if (&BB == &BB.getParent()->getEntryBlock()) {
                return false;
            }

            if(BB.size() != 1) {
                return false;
            }

            BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator());
            if (BI == nullptr) {
                return false;
            }

            if (BI -> isConditional()) {
                return false;
            }

            if (std::distance(pred_begin(&BB), pred_end(&BB)) != 1) {
                return false;
            }

            return true;
        }

        bool isUnreachable(BasicBlock &BB) {
            if (&BB == &BB.getParent()->getEntryBlock()) {
                return false;
            }

            if (pred_empty(&BB)) {
                return true;
            }

            return false;
        }

        bool runOnFunction(Function &F) override {
            bool Changed = false;


            // 1. UNREACHABLE BLOCK ELIMINATION

            BlocksToRemove.clear();

            for (BasicBlock &BB : F) {
                if (isUnreachable(BB)) {
                    errs() << "Unreachable block found:\n";
                    BB.print(errs());
                    errs() << "\n";

                    BlocksToRemove.push_back(&BB);
                    Changed = true;
                }
            }
            
            for (BasicBlock *BB : BlocksToRemove) {
                BB->eraseFromParent();
            }

            // 2. EMPTY BLOCK ELIMINATION

            BlocksToRemove.clear();

            for (BasicBlock &BB : F) {
                if (isEmptyBlock(BB)) {

                    BranchInst *BI = cast<BranchInst>(BB.getTerminator());

                    BasicBlock *Pred = *pred_begin(&BB);
                    BasicBlock *Succ = BI->getSuccessor(0);

                    BranchInst *PredBI = dyn_cast<BranchInst>(Pred->getTerminator());

                    if (PredBI == nullptr) {
                        continue;
                    }
                    for (unsigned i = 0; i < PredBI->getNumSuccessors(); i++) {
                        if (PredBI->getSuccessor(i) == &BB) {
                            PredBI->setSuccessor(i, Succ);
                            BlocksToRemove.push_back(&BB);
                            Changed = true;
                            break; //da ne bismo isti blok dodali vise puta
                        }
                    }

                    errs() << "Empty block found: :\n";
                    BB.print(errs());
                    errs() << "\n";
                }
            }


            for (BasicBlock *BB : BlocksToRemove) {
                BB->eraseFromParent();
            }

            return Changed;
        }

    };
}

char SimplifyCFGPass::ID = 0;

static RegisterPass<SimplifyCFGPass> X(
    "our-simplifycfg",
    "Our SimplifyCFG optimization pass",
    false,
    false
);