//
// Created by Matteo Richard on 20/11/23.
//

#ifndef FORALLAND_H
#define FORALLAND_H

#include "../../Forwards.hpp"
#include "../../Kernel/Formula.hpp"

using namespace Kernel;

// TODO move to shell folder after approval

namespace Shell {
class ForallAnd {
public:
  static FormulaUnit* forallAnd (FormulaUnit*);
  static Formula* forallAnd (Formula* f){
    Formula* res  = innerForallAnd(f);
    res->label(f->getLabel());
    return res;
  }
  static FormulaList* forallAnd (FormulaList*,Connective con);
private:
  static Formula* innerForallAnd(Formula*);
};
}





#endif //FORALLAND_H
