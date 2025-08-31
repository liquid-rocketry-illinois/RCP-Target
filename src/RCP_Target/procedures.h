#ifndef TESTS_H
#define TESTS_H

#include <cstdint>

namespace Test {
class Procedure;

  struct Tests {
    Procedure* tests[16];

    Procedure* operator[](const size_t index) const {
      return tests[index];
    }
  };

  Tests& getTests();

typedef void (*Runnable)();
typedef bool (*BoolSupplier)();

class OneShot;
class WaitProcedure;
class SequentialProcedure;
class ParallelProcedure;
class ParallelRaceProcedure;
class ParallelDeadlineProcedure;

class Procedure {
 public:
  Procedure() = default;

  virtual void initialize();
  virtual void execute();
  virtual void end(bool interrupted);
  virtual bool isFinished();

  virtual ~Procedure() = default;
};

class EStopSetterWrapper : public Procedure {
  Procedure* const proc;
  Procedure* const seqestop;
  Procedure* const endestop;

 public:
  EStopSetterWrapper(Procedure* proc, Procedure* seqestop, Procedure* endestop);

  void initialize() override;
  void execute() override;
  void end(bool interrupted) override;
  bool isFinished() override;

  ~EStopSetterWrapper() override;
};

class OneShot : public Procedure {
  Runnable run;

 public:
  explicit OneShot(Runnable run);

  void initialize() override;
};

class WaitProcedure : public Procedure {
  const unsigned long waitTime;
  unsigned long startTime;

 public:
  explicit WaitProcedure(const unsigned long& waitTime);

  void initialize() override;
  bool isFinished() override;
};

class BoolWaiter : public Procedure {
  BoolSupplier supplier;

 public:
  explicit BoolWaiter(BoolSupplier supplier);

  bool isFinished() override;
};

class SequentialProcedure : public Procedure {
  Procedure** const procedures;
  const int numProcedures;
  int current;

 public:
  template<typename... Procs> explicit SequentialProcedure(Procs... procs);
  ~SequentialProcedure() override;

  void initialize() override;
  void execute() override;
  void end(bool interrupted) override;
  bool isFinished() override;
};

class ParallelProcedure : public Procedure {
 protected:
  Procedure** const procedures;
  const unsigned int numProcedures;
  bool* const running;

 public:
  template<typename... Procs> explicit ParallelProcedure(Procs... procs);
  ~ParallelProcedure() override;

  void initialize() override;
  void execute() override;
  void end(bool interrupted) override;
  bool isFinished() override;
};

class ParallelRaceProcedure : public ParallelProcedure {
 public:
  template<typename... Procs> explicit ParallelRaceProcedure(Procs... procs);

  void end(bool interrupted) override;
  bool isFinished() override;
};

class ParallelDeadlineProcedure : public Procedure {
  Procedure** const procedures;
  const unsigned int numProcedures;
  bool* const running;
  Procedure* const deadline;
  bool deadlineRunning;

 public:
  template<typename... Procs> explicit ParallelDeadlineProcedure(Procedure* deadline, Procs... procs);
  ~ParallelDeadlineProcedure() override;

  void initialize() override;
  void execute() override;
  void end(bool interrupted) override;
  bool isFinished() override;
};

class SelectorProcedure : public Procedure {
  Procedure* const yes;
  Procedure* const no;
  const BoolSupplier chooser;
  bool choice;

 public:
  SelectorProcedure(Procedure* yes, Procedure* no, BoolSupplier chooser);
  ~SelectorProcedure() override;

  void initialize() override;
  void execute() override;
  void end(bool interrupted) override;
  bool isFinished() override;
};

} // namespace Test

#endif