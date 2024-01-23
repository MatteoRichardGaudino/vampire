//
// Created by Matteo Richard on 12/12/23.
//

#include "OneBindingAlgorithm.h"

#include "OneBindingSat.h"
#include "Debug/RuntimeStatistics.hpp"
#include "Lib/Timer.hpp"
#include "Shell/Statistics.hpp"

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
    if(_showProof) {
      env.out() << "Empty clause found" << std::endl;
      env.endOutput();
    }
    return false;
  }
  if(_prp.isGround()) {
    if(_showProof) {
      env.out() << "Problem is ground" << std::endl;
      env.endOutput();
    }
    auto res = _solver->solve() == SATSolver::SATISFIABLE;
    if(res) RSTAT_CTR_INC("boolean models");
    return res;
  }


  if (_showProof) env.out() << "-------------------- Solve --------------------------" << std::endl;
  unsigned avgTimePerModel = 0;
  unsigned modelCount = 0;
  while (_solver->solve() == SATSolver::SATISFIABLE){
    RSTAT_CTR_INC("boolean models");
    modelCount++;
    auto start = env.timer->elapsedMilliseconds();

    bool RESULT = true;

    Array<Literal*> implicants;
    unsigned l = _implicants(implicants);

    // std::cout << "implicants: " << std::endl;
    // for(int i = 0; i < l; ++i) {
    //   std::cout<< implicants[i]->toString() << ", ";
    // }
    // std::cout<< std::endl;
    // exit(0);

    {
      TIME_TRACE(TimeTrace::IMPLICANT_SORTING)
      std::sort(implicants.begin(), implicants.begin()+l, GroundArityAndTermComparator); // 'subterm_sort'
      //std::sort(implicants.begin(), implicants.begin()+l, GroundAndArityComparator); // 'no_subterm_sort'
    }

    if (_showProof) {
      env.out() << "Implicants ordered by arity: " << std::endl;
      for(int i = 0; i < l; i++){
        env.out() << implicants[i]->toString() << " ";
      }
      env.out() << std::endl;
    }


    // check if implicants contains a literal binding. if not, then the problem is ground and return true
    bool hasBinding = false;
    for(int i = l-1; i >= 0; --i) {
      if(_prp.isBindingLiteral(implicants[i])) {
        hasBinding = true;
        break;
      }
    }
    if(!hasBinding) {
      if(_showProof) {
        env.out() << "No binding found in implicants, Problem is ground" << std::endl;
        env.endOutput();
      }
      return true;
    }



    ArityGroupIterator groupIt(implicants, l);

    while (RESULT && groupIt.hasNext()){
      auto group = groupIt.next();
      MaximalUnifiableSubsets mus(group, [&](LiteralStack solution){
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET_SAT_SOLVING)
        if(solution.length() == 1) {
          if(!_prp.isBindingLiteral(solution.top())) return true; // a single ground literal is always satisfiable

          if(_prp.getSatClauses(solution.top())->length() == 1) return true;
        }

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

        // vstring tmp;
        // std::cin >> tmp;

        return status == SATSolver::SATISFIABLE;
      });

      while (group.hasNext()){
        auto lit = group.next();
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET)
        // if(_showProof) {
        //   env.out() << "---------- mus for " << lit->toString() << " ------------" << std::endl;
        // }
        bool res = mus.musV2(lit);
        if(!res){
          RESULT = false;
          blockModel(mus.getSolution());
          break;
        }
      }
    }

    auto end = env.timer->elapsedMilliseconds();

    if(env.statistics->maxTimePerModel == 0 || end - start > env.statistics->maxTimePerModel) {
      env.statistics->maxTimePerModel = end - start;
    }
    if(env.statistics->minTimePerModel == 0 || end - start < env.statistics->minTimePerModel) {
      env.statistics->minTimePerModel = end - start;
    }

    avgTimePerModel += (end - start);

    if(RESULT){
      env.statistics->avgTimePerModel = avgTimePerModel / modelCount;
      if(_showProof) {
        env.out() <<  "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& SAT &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
        env.endOutput();
      }
      return true;
    }

    //blockModel(implicants, l);
  }
  env.statistics->avgTimePerModel = avgTimePerModel / modelCount;
  if(_showProof) {
    env.out() <<  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ UNSAT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
    env.endOutput();
  }

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

void OneBindingAlgorithm::blockModel(Array<Literal *> &implicants, unsigned size)
{
  SATLiteralStack blockingClause;

  for (int i = 0; i < size; ++i) {
    auto impl = implicants[i];
    auto satImpl = _prp.toSAT(_prp.isBindingLiteral(impl) ? _prp.getBooleanBinding(impl) : impl);
    blockingClause.push(satImpl.opposite());
  }

  auto clause = SATClause::fromStack(blockingClause);
  if (_showProof) env.out() << "Blocking Clause: " << clause->toString() << std::endl;

  _solver->addClause(clause);
}

void OneBindingAlgorithm::blockModel(LiteralStack* implicants){
  if(_showProof) env.out() << "||||||||||||||||||||||||||||||||| blocking model |||||||||||||||||||||||||||||||||" << std::endl;

  SATLiteralStack blockingClause;

  LiteralStack::Iterator it(*implicants);
  while (it.hasNext()) {
    auto impl = it.next();
    auto satImpl = _prp.toSAT(_prp.isBindingLiteral(impl) ? _prp.getBooleanBinding(impl) : impl);
    blockingClause.push(satImpl.opposite());
  }

  auto clause = SATClause::fromStack(blockingClause);
  if (_showProof)
    env.out() << "Blocking Clause: " << clause->toString() << std::endl;
  _solver->addClause(clause);
}


} // BindingFragments