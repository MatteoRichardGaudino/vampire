//
// Created by Matte on 09/10/2023.
//

#ifndef VAMPIRE_PREPROCESS_H
#define VAMPIRE_PREPROCESS_H

#include "Kernel/Unit.hpp"

using namespace Kernel;

namespace BindingFragments {
class Preprocess {
public:
  explicit Preprocess(bool skolemize = true, bool clausify = false):_skolemize(skolemize), _clausify(clausify) {};
  void preprocess(Problem& prb);
  void clausify(Problem& prb);
private:
  bool _skolemize;
  bool _clausify;
};
}

#endif // VAMPIRE_PREPROCESS_H
