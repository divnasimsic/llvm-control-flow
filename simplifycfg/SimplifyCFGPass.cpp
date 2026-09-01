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

        //vektor blokova za brisanje
        std::vector<BasicBlock *> BlocksToRemove;

        //Empty block elimiation
        //-trazimo blok koji samo prosledjuje kontrolu dalje
        bool isEmptyBlock(BasicBlock &BB) {

            //takav blok ne sme biti entry blok
            //getParent vraca funkciju kojoj pripada BB
            if (&BB == &BB.getParent()->getEntryBlock()) {
                return false;
            }

            //takav blok mora imati tacno jednu instrukciju
            if(BB.size() != 1) {
                return false;
            }

            //ta instrukcija mora biti branch instrukcija
            //posto znamo da potoji jedna instrukcija, mozemo uzeti terminator
            BranchInst *BI = dyn_cast<BranchInst>(BB.getTerminator());
            if (BI == nullptr) {
                return false;
            }

            //takva Branch instrukcija mora biti uncoditional
            if (BI -> isConditional()) {
                return false;
            }

            //takav blok mora imati samo jednog predecessor-a
            //pred_begin(&BB) daje iterator na prvog predecessor-a bloka BB.
            //pred_end(&BB) označava kraj liste predecessor-a.
            if (std::distance(pred_begin(&BB), pred_end(&BB)) != 1) {
                return false;
            }

            return true;
        }

        // Unreachable block elimination
        // - blok je unreachable ako nije entry i nema predecessor-a
        bool isUnreachable(BasicBlock &BB) {
            //entry block ne uklanjamo
            if (&BB == &BB.getParent()->getEntryBlock()) {
                return false;
            }

            //ako nema predecessora do njega se ne moze doci
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
            // prvo uklanjamo unreachable blokove
            for (BasicBlock *BB : BlocksToRemove) {
                BB->eraseFromParent();
            }

            // 2. EMPTY BLOCK ELIMINATION

            BlocksToRemove.clear();

            for (BasicBlock &BB : F) {
                if (isEmptyBlock(BB)) {

                    BranchInst *BI = cast<BranchInst>(BB.getTerminator());
                    //koristimo cast umesto dym_cast jer znamo da je terminator BranchInst

                    BasicBlock *Pred = *pred_begin(&BB);
                    BasicBlock *Succ = BI->getSuccessor(0);

                    //UKLANJANJE EMPTY BB
                    //nalazimo terminator od predccessor-a
                    BranchInst *PredBI = dyn_cast<BranchInst>(Pred->getTerminator());

                    if (PredBI == nullptr) {
                        continue;
                    }
                    //pronalazimo onaj sukcesor pridicesora koji pokazuje na empty bb
                    //i zamenjujemo ga sukcesorom empty basic bloka
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


            //brisanje blokova
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