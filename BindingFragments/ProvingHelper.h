//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_PROVINGHELPER_H
#define VAMPIRE_PROVINGHELPER_H

#include "Kernel/Problem.hpp"
#include "Kernel/Formula.hpp"

namespace BindingFragments{
using namespace Kernel;
using namespace Shell;

class ProvingHelper {
public:
  static void run1BSatAlgorithm(Problem& prb, const Options& opt);
};
}


#endif // VAMPIRE_PROVINGHELPER_H
