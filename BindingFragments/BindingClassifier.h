//
// Created by Matte on 01/10/2023.
//

#ifndef VAMPIRE_BINDINGCLASSIFIER_H
#define VAMPIRE_BINDINGCLASSIFIER_H

#include "Kernel/Unit.hpp"

namespace BindingFragments {

using namespace Kernel;


enum Fragment{
  ONE_BINDING,
  UNIVERSAL_ONE_BINDING,
  CONJUNCTIVE_BINDING,
  DISJUNCTIVE_BINDING,
  NONE
};

class BindingClassifier {
public:
  inline static bool isOneBinding(UnitList* units){
    return _isFragment(units, ONE_BINDING);
  }
  inline static bool isOneBinding(Formula* formula){
      return _isFragment(formula, ONE_BINDING);
  }

  inline static bool isConjunctiveBinding(UnitList* units){
      return _isFragment(units, CONJUNCTIVE_BINDING);
  }
  inline static bool isConjunctiveBinding(Formula* formula){
      return _isFragment(formula, CONJUNCTIVE_BINDING);
  }

  inline static bool isUniversalOneBinding(UnitList* units){
      return _isFragment(units, UNIVERSAL_ONE_BINDING);
  }
  inline static bool isUniversalOneBinding(Formula* formula){
      return _isFragment(formula, UNIVERSAL_ONE_BINDING);
  }

  inline static bool isDisjunctiveBinding(UnitList* units){
      return _isFragment(units, DISJUNCTIVE_BINDING);
  }
  inline static bool isDisjunctiveBinding(Formula* formula){
      return _isFragment(formula, DISJUNCTIVE_BINDING);
  }

  static Fragment classify(UnitList* units);
  static Fragment classify(Formula* formula);
private:
  static bool _isFragment(UnitList* units, Fragment fragment);
  static bool _isFragment(Formula* formula, Fragment fragment);

  static bool _isOneBindingHelper(Formula* formula, TermList *term);
  static bool _isConjunctiveBindingHelper(Formula* formula, TermList *term);
  static bool _isDisjunctiveBindingHelper(Formula* formula, TermList *term);
  static TermList* _findATerm(Formula* formula);
};

}
#endif // VAMPIRE_BINDINGCLASSIFIER_H
