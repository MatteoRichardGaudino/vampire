//
// Created by Matteo Richard on 12/12/23.
//

#ifndef ONEBINDINGALGORITHM_H
#define ONEBINDINGALGORITHM_H

#include "Kernel/Formula.hpp"
#include "SAT/SATSolver.hpp"
#include "Indexing/SubstitutionTree.hpp"
#include "preprocess/Preprocess.h"

namespace BindingFragments {

class OneBindingAlgorithm {
public:
  explicit OneBindingAlgorithm(PreprocessV2& prp);

  bool solve();
private:
  PreprocessV2& _prp;
  PrimitiveProofRecordingSATSolver* _solver;

  bool _showProof;


  static PrimitiveProofRecordingSATSolver* _newSatSolver();
  void _setupSolver();
  //void printAssignment();
  unsigned _implicants(Array<Literal*>& implicants);
  void blockModel(Array<Literal*>& implicants, unsigned size);

  void blockModel(LiteralStack* implicants);

  bool _internalSat(LiteralStack* solution);


  PrimitiveProofRecordingSATSolver* _gmus_solver = nullptr;
  void _onStartGroundMus(){
    std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&& onStatrtGmus &&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
    _gmus_solver = _newSatSolver();
    _gmus_solver->ensureVarCount(_prp.maxBindingVarNo());
  }
  bool _onAddedToGroundMus(Literal* lit){
    TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET_SAT_SOLVING)
    // std::cout << "onAddedToGroundMus" << lit->toString();

    SATClauseStack* stk = _prp.getSatClauses(lit);
    SATClauseStack::Iterator it(*stk);
    while (it.hasNext()) _gmus_solver->addClause(it.next());

    auto status = _gmus_solver->solve() == SATSolver::SATISFIABLE;

    // std::cout << ":::" << (status? "SAT" : "UNSAT") << std::endl;
    return status;
  }
  void _onEndGroundMus(){
    std::cout << "%%%%%%%%%%%%%%%%%%%%% onEndGroundMus %%%%%%%%%%%%%%%%%%%%%%%%%%%" << std::endl;
    delete _gmus_solver;
    _gmus_solver = nullptr;
  }

};

} // BindingFragments

#endif //ONEBINDINGALGORITHM_H
