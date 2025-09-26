// See https://github.com/liquid-rocketry-illinois/LRI-RCI-CPP/blob/master/docs/testing.md

#include "RCP_Target/procedures.h"

#include "RCP_Target/RCP_Target.h"

namespace Test {

    void Procedure::initialize() {}

    void Procedure::execute() {}

    void Procedure::end([[maybe_unused]] bool interrupted) {}

    bool Procedure::isFinished() { return true; }

    EStopSetterWrapper::EStopSetterWrapper(Procedure* proc, Procedure* seqestop, Procedure* endestop) :
        proc(proc), seqestop(seqestop), endestop(endestop) {}

    void EStopSetterWrapper::initialize() {
        proc->initialize();
        RCP::ESTOP_PROC = seqestop;
    }

    void EStopSetterWrapper::execute() { proc->execute(); }

    bool EStopSetterWrapper::isFinished() { return proc->isFinished(); }

    void EStopSetterWrapper::end(bool interrupted) {
        proc->end(interrupted);
        RCP::ESTOP_PROC = endestop;
    }

    EStopSetterWrapper::~EStopSetterWrapper() { delete proc; }

    OneShot::OneShot(Runnable run) : run(run) {}

    void OneShot::initialize() { run(); }

    WaitProcedure::WaitProcedure(const unsigned long& waitTime) : waitTime(waitTime) {}

    void WaitProcedure::initialize() { startTime = RCP::millis(); }

    bool WaitProcedure::isFinished() { return RCP::millis() - startTime > waitTime; }

    BoolWaiter::BoolWaiter(BoolSupplier supplier) : supplier(supplier) {}

    bool BoolWaiter::isFinished() { return supplier(); }

    SequentialProcedure::~SequentialProcedure() {
        for(int i = 0; i < numProcedures; i++) {
            delete procedures[i];
        }

        delete[] procedures;
    }

    void SequentialProcedure::initialize() {
        current = 0;
        if(current >= numProcedures) return;
        procedures[current]->initialize();
    }

    void SequentialProcedure::execute() {
        if(current >= numProcedures) return;

        Procedure* proc = procedures[current];
        proc->execute();
        if(proc->isFinished()) {
            proc->end(false);
            current++;
            if(current < numProcedures) procedures[current]->initialize();
        }
    }

    void SequentialProcedure::end(bool interrupted) {
        if(interrupted && current < numProcedures) procedures[current]->end(interrupted);
    }

    bool SequentialProcedure::isFinished() { return current >= numProcedures; }

    ParallelProcedure::~ParallelProcedure() {
        for(unsigned int i = 0; i < numProcedures; i++) {
            delete procedures[i];
        }

        delete[] procedures;
    }

    void ParallelProcedure::initialize() {
        for(unsigned int i = 0; i < numProcedures; i++) {
            procedures[i]->initialize();
            running[i] = true;
        }
    }

    void ParallelProcedure::execute() {
        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) continue;
            procedures[i]->execute();

            if(procedures[i]->isFinished()) {
                procedures[i]->end(false);
                running[i] = false;
            }
        }
    }

    void ParallelProcedure::end(bool interrupted) {
        if(!interrupted) return;
        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) continue;
            procedures[i]->end(true);
            running[i] = false;
        }
    }

    bool ParallelProcedure::isFinished() {
        for(unsigned int i = 0; i < numProcedures; i++)
            if(running[i]) return false;
        return true;
    }

    void ParallelRaceProcedure::end([[maybe_unused]] bool interrupted) {
        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) continue;
            procedures[i]->end(true);
            running[i] = false;
        }
    }

    bool ParallelRaceProcedure::isFinished() {
        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) return true;
        }
        return false;
    }

    ParallelDeadlineProcedure::~ParallelDeadlineProcedure() {
        delete deadline;

        for(unsigned int i = 0; i < numProcedures; i++) {
            delete procedures[i];
        }

        delete[] procedures;
    }

    void ParallelDeadlineProcedure::initialize() {
        deadline->initialize();
        for(unsigned int i = 0; i < numProcedures; i++) {
            procedures[i]->initialize();
        }
    }

    void ParallelDeadlineProcedure::execute() {
        if(deadlineRunning) {
            deadline->execute();
            if(deadline->isFinished()) {
                deadline->end(false);
                deadlineRunning = true;
            }
        }

        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) continue;
            procedures[i]->execute();

            if(procedures[i]->isFinished()) {
                procedures[i]->end(false);
                running[i] = false;
            }
        }
    }

    void ParallelDeadlineProcedure::end([[maybe_unused]] bool interrupted) {
        if(deadlineRunning) deadline->end(true);

        for(unsigned int i = 0; i < numProcedures; i++) {
            if(!running[i]) continue;
            procedures[i]->end(true);
        }
    }

    bool ParallelDeadlineProcedure::isFinished() { return !deadlineRunning; }

    SelectorProcedure::SelectorProcedure(Procedure* yes, Procedure* no, BoolSupplier chooser) :
        yes(yes), no(no), chooser(chooser) {}

    SelectorProcedure::~SelectorProcedure() {
        delete yes;
        delete no;
    }

    void SelectorProcedure::initialize() {
        choice = chooser();
        (choice ? yes : no)->initialize();
    }

    void SelectorProcedure::execute() { (choice ? yes : no)->execute(); }

    bool SelectorProcedure::isFinished() { return (choice ? yes : no)->isFinished(); }

    void SelectorProcedure::end(bool interrupted) { (choice ? yes : no)->end(interrupted); }

} // namespace Test
