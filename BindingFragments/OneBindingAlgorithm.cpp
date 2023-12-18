//
// Created by Matteo Richard on 12/12/23.
//

#include "OneBindingAlgorithm.h"

#include "OneBindingSat.h"

#include <SAT/MinisatInterfacing.hpp>

namespace BindingFragments {



OneBindingAlgorithm::OneBindingAlgorithm(PreprocessV2 &prp) : _prp(prp)
{
  _showProof = env.options->proof() == Options::Proof::ON;
  if (!_prp.emptyClauseFound)
    _setupSolver();
}
bool OneBindingAlgorithm::solve(){
  if(_showProof) env.beginOutput();
  TIME_TRACE(TimeTrace::ONE_BINDING_ALGORITHM)

  if(_prp.emptyClauseFound) {
    if(_showProof)
      env.out() << "Empty clause found" << std::endl;
    return false;
  }
  if(_prp.isGround()) {
    if(_showProof)
      env.out() << "Problem is ground" << std::endl;
    return _solver->solve() == SATSolver::SATISFIABLE;
  }


  if (_showProof) env.out() << "-------------------- Solve --------------------------" << std::endl;
  while (_solver->solve() == SATSolver::SATISFIABLE){
    // if(_showProof) printAssignment(); // TODO

    bool RESULT = true;

    Array<Literal*> implicants;
    unsigned l = _implicants(implicants);

    std::sort(implicants.begin(), implicants.begin()+l, [](Literal* a, Literal* b) {
      const auto a1 = a->arity(), b1 = b->arity();
      const auto g1 = a->ground(), g2 = b->ground();

      if(a1 < b1) return true;
      if(a1 > b1) return false;
      if(g1 && !g2) return true;
      if(g2 && !g1) return false;
      return a->functor() < b->functor();

      // if(a1 == b1) {
      //   if(g1) return true;
      //   return false < true;
      // } else return a1 < b1;
    });

    if (_showProof) {
      env.out() << "Implicants ordered by arity: " << std::endl;
      for(int i = 0; i < l; i++){
        env.out() << implicants[i]->toString() << " ";
      }
      env.out() << std::endl;
    }


    ArityGroupIterator groupIt(implicants, l);

    while (RESULT && groupIt.hasNext()){
      auto group = groupIt.next();
      MaximalUnifiableSubsets mus(group, [&](LiteralStack solution){
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET_SAT_SOLVING)
        auto solver = _newSatSolver();
        solver->ensureVarCount(_prp.maxBindingVarNo());

        if(_showProof) env.out() << "MUS: ";
        LiteralStack::Iterator solIt(solution);
        while (solIt.hasNext()) {
          auto lit = solIt.next();
          if(_showProof) env.out() << lit->toString() << " ";
          SATClauseStack* stk = _prp.getSatClauses(lit);
          SATClauseStack::Iterator it(*stk);
          while (it.hasNext()) solver->addClause(it.next());
        }
        auto status = solver->solve();
        if(_showProof) env.out() << "::::: " << status << std::endl;
        delete solver;
        return status == SATSolver::SATISFIABLE;
      });

      while (group.hasNext()){
        auto lit = group.next();
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET)
        if(_showProof) {
          env.out() << "---------- mus for " << lit->toString() << " ------------" << std::endl;
        }
        bool res = mus.musV2(lit);
        if(!res){
          RESULT = false;
          break;
        }
      }
    }

    if(RESULT){
      if(_showProof) {
        env.out() <<  "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& SAT &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
        env.endOutput();
      }
      return true;
    }

    if(_showProof) env.out() << "||||||||||||||||||||||||||||||||| blocking model |||||||||||||||||||||||||||||||||" << std::endl;
    blockModel(implicants, l);
  }

  if(_showProof) env.out() <<  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ UNSAT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;

  if(_showProof) env.endOutput();
  return false;

}


// *** SAT solver ***
PrimitiveProofRecordingSATSolver *BindingFragments::OneBindingAlgorithm::_newSatSolver(){
  switch (env.options->satSolver())
  {
    case Options::SatSolver::MINISAT:
      return new MinisatInterfacing(*env.options, true);
    break;
#if VZ3 // TODO
    case Options::SatSolver::Z3: {
      BYPASSING_ALLOCATOR
      _solverIsSMT = true;
      _solver = new Z3Interfacing(_parent.getOptions(), _parent.satNaming(), /* unsat core */ false, _parent.getOptions().exportAvatarProblem());
      if (_parent.getOptions().satFallbackForSMT()) {
        // TODO make fallback minimizing?
        SATSolver *fallback = new MinisatInterfacing(_parent.getOptions(), true);
        return = new FallbackSolverWrapper(_solver.release(), fallback);
      }
    } break;
#endif
    default:
      ASSERTION_VIOLATION_REP(env.options->satSolver());
  }
}

void OneBindingAlgorithm::_setupSolver()
{
  _solver = _newSatSolver();

  _solver->ensureVarCount(_prp.maxSatVar());

  SATClauseStack::Iterator it(*_prp.satClauses());
  while (it.hasNext()) {
    auto clause = it.next();
    _solver->addClause(clause);
  }
}
unsigned OneBindingAlgorithm::_implicants(Array<Literal*> &implicants){
  int l = 0;

  LiteralList::Iterator lIt(_prp.literals());
  while (lIt.hasNext()) {
    Literal *lit = lIt.next();
    auto satLit = _prp.toSAT(lit);
    if (_solver->trueInAssignment(satLit)) {
      if(_prp.isBooleanBinding(lit)) {
        for(
          LiteralList::Iterator lbIt(_prp.getLiteralBindings(lit));
          lbIt.hasNext();
          implicants[l++] = lbIt.next()
        );
      } else {
        implicants[l++] = lit;
      }
    }
  }
  return l;
}

void OneBindingAlgorithm::blockModel(Array<Literal*>& implicants, unsigned size){
  SATLiteralStack blockingClause;

  for(int i = 0; i < size; ++i) {
    auto impl = implicants[i];
    auto satImpl = _prp.toSAT(_prp.isBindingLiteral(impl)? _prp.getBooleanBinding(impl) : impl);
    blockingClause.push(satImpl.opposite());
  }

  auto clause = SATClause::fromStack(blockingClause);
  if (_showProof)
    std::cout << "Blocking Clause: " << clause->toString() << std::endl;
  _solver->addClause(clause);
}


} // BindingFragments