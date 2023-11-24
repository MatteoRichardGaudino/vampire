//
// Created by Matte on 09/10/2023.
//

#ifndef VAMPIRE_PREPROCESS_H
#define VAMPIRE_PREPROCESS_H

#include "../../Kernel/Unit.hpp"

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
}

#endif // VAMPIRE_PREPROCESS_H
