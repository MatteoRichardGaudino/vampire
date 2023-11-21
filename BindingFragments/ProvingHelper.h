//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_PROVINGHELPER_H
#define VAMPIRE_PROVINGHELPER_H

#include "Kernel/Problem.hpp"
#include "Kernel/Formula.hpp"
#include "preprocess/BindingClassifier.h"

namespace BindingFragments{
using namespace Kernel;
using namespace Shell;

class ProvingHelper {
public:
  static void run1BSatAlgorithm(Problem& prb, const Options& opt, int classification);

private:
  static void toUniversalOneBinding(Problem& prb);
};
}

// https://theory.stanford.edu/~nikolaj/programmingz3.html#sec-blocking-evaluations

#endif // VAMPIRE_PROVINGHELPER_H
