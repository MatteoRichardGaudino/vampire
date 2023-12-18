//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"

#include "OneBindingAlgorithm.h"
#include "Kernel/Clause.hpp"
#include "OneBindingSat.h"


bool BindingFragments::ProvingHelper::run1BSatAlgorithm(PreprocessV2& prp){
  // TODO
  OneBindingAlgorithm sat(prp);
  return sat.solve();
}

