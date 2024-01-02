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
};

} // BindingFragments

#endif //ONEBINDINGALGORITHM_H
