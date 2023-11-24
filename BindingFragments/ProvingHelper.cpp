//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Kernel/Clause.hpp"
#include "OneBindingSat.h"


bool BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt){
  // TODO
  OneBindingSat sat(prb, opt);
  return sat.solve();
}

