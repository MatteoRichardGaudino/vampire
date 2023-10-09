//
// Created by Matte on 01/10/2023.
//

#ifndef VAMPIRE_BINDINGCLASSIFIER_H
#define VAMPIRE_BINDINGCLASSIFIER_H

#include "Kernel/Unit.hpp"
#include "Kernel/Formula.hpp"

namespace BindingFragments {

using namespace Kernel;


enum Fragment{
  UNIVERSAL_ONE_BINDING = 0,
  ONE_BINDING = 1,
  CONJUNCTIVE_BINDING = 2,
  DISJUNCTIVE_BINDING = 3,
  NONE = 4
};

class BindingClassification{
public:
  Fragment fragment;
  TermList* term;

  BindingClassification();
  BindingClassification(Fragment fragment, TermList *term);

  inline bool is(Fragment fragment1) const{
    return fragment == fragment1 || (fragment1 == ONE_BINDING && fragment == UNIVERSAL_ONE_BINDING);
  }

  static Fragment compare(const Fragment &first, const Fragment &second);
  static BindingClassification compare(const BindingClassification &first, const BindingClassification &second, Connective connective);
private:
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

  static BindingClassification _classifyHelper(Formula* formula);
};

}
#endif // VAMPIRE_BINDINGCLASSIFIER_H
