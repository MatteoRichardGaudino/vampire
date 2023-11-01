//
// Created by Matte on 25/09/2023.
//

#include "OneBindingSat.h"
#include "Shell/CNF.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Indexing/TermSharing.hpp"
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


  _satClauses = new ClauseStack(16);

  auto units = prb.units();
  UnitList::Iterator it(units);
  while(it.hasNext()) generateSATClauses(it.next());

  cout << "Clauses size: " << _satClauses->size() << endl;
  ClauseStack::Iterator cIt(*_satClauses);
  int i = 0;
  while(cIt.hasNext()){
    cout << "Clause " << ++i << ": "<< cIt.next()->toString() << endl;
  }

  cout<< "------------------ Setup Sat solver ----------------------------" << endl;
  setupSolver();
  cout<< "-------------------- Solve --------------------------" << endl;

  while (_solver->solve() == SAT::SATSolver::SATISFIABLE){
    printAssignment();
    blockModel();
  }
  cout<< "----------------------------------------------" << endl;
}

bool BindingFragments::OneBindingSat::solve(){
  return false;
}


void BindingFragments::OneBindingSat::generateSATClauses(Unit* unit){
  auto satFormula = generateSatFormula(unit->getFormula());
  Unit* satUnit = new FormulaUnit(satFormula, unit->inference());

  cout<< "Unit: " << unit->toString() << endl;
  cout<< "SAT Unit: " << satUnit->toString() << endl << endl;
  CNF cnf;
  cnf.clausify(satUnit, *_satClauses);
}

Formula *BindingFragments::OneBindingSat::generateSatFormula(Formula *formula){
  switch (formula->connective()) {
    case LITERAL:
      _satVarCount++;
      return new AtomicFormula(formula->literal());
    case AND:
    case OR: {
      FormulaList *args = FormulaList::empty();
      FormulaList::Iterator it(formula->args());
      while(it.hasNext()){
        FormulaList::push(generateSatFormula(it.next()), args);
      }
      return new JunctionFormula(formula->connective(), args);
    }
    case NOT: {
      return new NegatedFormula(generateSatFormula(formula->uarg()));
    }
    case FORALL:
    case EXISTS: {
      _satVarCount++;
      _bindingCount++;

      auto vList = formula->vars();
      auto size = Lib::List<unsigned int>::length(vList);
      auto* tList = new TermList[size];

      VList::Iterator it(vList);
      unsigned j = 0;
      while (it.hasNext()){
        auto i = it.next();
        tList[j++] = TermList(i, false);
      }

      unsigned freshPredicate = env.signature->addFreshPredicate(size, "$b");
      //vstring name =  reinterpret_cast<basic_string<char, std::char_traits<char>, Lib::STLAllocator<char>> &&>(to_string(_bindingCount++).append("_Binding"));
      //return new NamedFormula(name);
      //Literal lit(freshFunctor, size, false, false);
      auto newLit = Literal::create(freshPredicate, size, true, false, tList);
      return new AtomicFormula(newLit);
    }
    default:
      ASSERTION_VIOLATION; // TODO
  }
}
void BindingFragments::OneBindingSat::setupSolver(){
  ClauseStack::Iterator cIt(*_satClauses);
  SATClauseStack clauses;
  while (cIt.hasNext()){
    auto clause = cIt.next();
    auto satClause = _sat2fo.toSAT(clause);
    cout<< "Clause: " << clause->toString() << endl;
    cout<< "SAT Clause: " << satClause->toString() << endl << endl;
    clauses.push(satClause);
  }

  _solver->ensureVarCount(_sat2fo.maxSATVar());

  SATClauseStack::Iterator it(clauses);
        while (it.hasNext()) {
    auto clause = it.next();
    _solver->addClause(clause);
  }
}
void BindingFragments::OneBindingSat::printAssignment(){
  cout<< "Assignment[(satVar, literal, value)]: ";
  unsigned int max = _sat2fo.maxSATVar();
  for(unsigned int j = 1; j <= max; j++){
    cout << "("
         << j << ", "
         << _sat2fo.toFO(SATLiteral(j, true))->toString() <<  ", "
         <<(_solver->getAssignment(j) == SATSolver::TRUE ? "true" : "false")
         << ")" << " ";
  }
  cout << endl;
}
void BindingFragments::OneBindingSat::blockModel(){
  SATLiteralStack blockingClause;
  unsigned int max = _sat2fo.maxSATVar();
  for(unsigned int j = 1; j <= max; j++){
    if(_solver->getAssignment(j) == SAT::SATSolver::TRUE){
      blockingClause.push(SATLiteral(j, false));
    }
  }
  auto clause = SATClause::fromStack(blockingClause);
  cout<< "Blocking Clause: " << clause->toString() << endl;
  _solver->addClause(clause);
}
