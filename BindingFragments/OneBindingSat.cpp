//
// Created by Matte on 25/09/2023.
//

#include "OneBindingSat.h"
#include <iostream>

#include "SAT/MinisatInterfacing.hpp"

using namespace std;


BindingFragments::OneBindingSat::OneBindingSat(Problem &prb, const Options &opt) : _problem(prb), _options(opt) {
  switch(opt.satSolver()){
    case Options::SatSolver::MINISAT:
      _solver = new MinisatInterfacing(opt, true);
      break;
#if VZ3 // TODO
    case Options::SatSolver::Z3:
    { BYPASSING_ALLOCATOR
      _solverIsSMT = true;
      _solver = new Z3Interfacing(_parent.getOptions(),_parent.satNaming(), /* unsat core */ false, _parent.getOptions().exportAvatarProblem());
      if(_parent.getOptions().satFallbackForSMT()){
        // TODO make fallback minimizing?
        SATSolver* fallback = new MinisatInterfacing(_parent.getOptions(),true);
        _solver = new FallbackSolverWrapper(_solver.release(),fallback);
      }
    }
    break;
#endif
    default:
      ASSERTION_VIOLATION_REP(opt.satSolver());
  }


}

bool BindingFragments::OneBindingSat::solve(){
  auto it = _problem.clauseIterator();

  for (int i = 0; i < 4; ++i) {
    _solver->newVar();
  }
  while (it.hasNext()){
    auto clause = it.next();
    cout<< "Clause" << clause->toString() << endl;
    auto satClause = _sat2fo.toSAT(clause);
    cout<< "SatClause: "<<  satClause->toString() << endl << endl;
    _solver->addClause(satClause);
  }

  auto status = _solver->solve();
  cout<< "Status: " << status << endl;
  return status == SATSolver::Status::SATISFIABLE;
}
