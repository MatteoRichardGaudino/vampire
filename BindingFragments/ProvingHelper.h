//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_PROVINGHELPER_H
#define VAMPIRE_PROVINGHELPER_H

#include "Kernel/Formula.hpp"

namespace BindingFragments{
using namespace Kernel;
using namespace Shell;

class ProvingHelper {
public:
  static bool run1BSatAlgorithm(Problem& prb, const Options& opt);
};
}

// https://theory.stanford.edu/~nikolaj/programmingz3.html#sec-blocking-evaluations

#endif // VAMPIRE_PROVINGHELPER_H
