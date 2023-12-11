//
// Created by Matte on 09/10/2023.
//

#ifndef VAMPIRE_PREPROCESS_H
#define VAMPIRE_PREPROCESS_H

#include "BindingClassifier.h"
#include "../../Kernel/Unit.hpp"
#include "../../Lib/DHMap.hpp"

using namespace Kernel;

namespace BindingFragments {
class Preprocess {
public:
  explicit Preprocess(
    bool skolemize = true,
    bool clausify = false,
    bool distributeForall = true
  ):_skolemize(skolemize), _clausify(clausify), _distributeForall(distributeForall) {};
  void preprocess(Problem& prb);
  void clausify(Problem& prb);

  /**
   * Convert the problem from default mode i.e. A1 & A2 & ... & An & ~C
   * to (A1 & A2 & ... & An) => C === ~A1 | ~A2 | ... | ~An | C
   */
  static void negatedProblem(Problem& prb);

  void setSkolemize(bool skolemize) { _skolemize = skolemize; }
  void setClausify(bool clausify) { _clausify = clausify; }
  void setDistributeForall(bool distribute_forall) { _distributeForall = distribute_forall; }


private:
  bool _skolemize;
  bool _clausify;
  bool _distributeForall;
};

typedef DHMap<Literal*, std::pair<Literal*, SAT::SATClauseStack*>> BindingMap;

class PreprocessV2 {
public:
  Problem &prb;
  Fragment fragment;

  explicit PreprocessV2(Problem& prb, Fragment fragment);

  void ennf();
  void topBooleanFormulaAndBindings();
  void naming();
  void nnf();
  void skolemize();
  void distributeForall();

  std::pair<Literal*, SAT::SATClauseStack*> getBinding(Literal* literal);
  void printBindings();

private:
  BindingMap _bindings;
  unsigned _maxBindingVarNo = 0;
  int _bindingCount;

  Formula* _newBinding(Literal* literal);
  Formula* _newBinding(Formula* formula);

  SAT::SATClauseStack* _clausifyBindingFormula(FormulaUnit* formula);

  Literal* _newBindingLiteral();

  Formula* _topBooleanFormula(Formula* formula);
};
}

#endif // VAMPIRE_PREPROCESS_H
