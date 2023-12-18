//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_PROVINGHELPER_H
#define VAMPIRE_PROVINGHELPER_H

#include "Kernel/Formula.hpp"
#include "preprocess/Preprocess.h"

namespace BindingFragments{
using namespace Kernel;
using namespace Shell;

class ProvingHelper {
public:
  static bool run1BSatAlgorithm(PreprocessV2& prp);
};
}

// https://theory.stanford.edu/~nikolaj/programmingz3.html#sec-blocking-evaluations

#endif // VAMPIRE_PROVINGHELPER_H
