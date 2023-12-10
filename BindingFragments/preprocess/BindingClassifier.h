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
  ONE_BINDING = 1,
  CONJUNCTIVE_BINDING = 2,
  DISJUNCTIVE_BINDING = 3,
  NONE = 4
};

inline vstring fragmentToString(Fragment fragment){
    switch (fragment) {
      case ONE_BINDING:
        return "One Binding";
      case CONJUNCTIVE_BINDING:
        return "Conjunctive Binding";
      case DISJUNCTIVE_BINDING:
        return "Disjunctive Binding";
      default:
        return "None";
    }
}

inline Fragment complementaryFragment(Fragment fragment){
  switch (fragment) {
    case ONE_BINDING:
      return ONE_BINDING;
    case CONJUNCTIVE_BINDING:
      return DISJUNCTIVE_BINDING;
    case DISJUNCTIVE_BINDING:
      return CONJUNCTIVE_BINDING;
    case NONE:
      return NONE;
  }
}

inline Fragment operator~(Fragment fragment){
  return complementaryFragment(fragment);
}

class BindingClassification{
public:
  Fragment fragment;
  Literal* mostLeftLit;

  BindingClassification();
  BindingClassification(Fragment fragment, Literal *mostLeftLit);

  bool is(Fragment fragment1) const{
    return fragment == fragment1;
  }

  BindingClassification complementary() const {
    return {complementaryFragment(fragment), mostLeftLit};
  }

  static Fragment compare(const Fragment &first, const Fragment &second);
  static BindingClassification compare(const BindingClassification &first, const BindingClassification &second, Connective connective);

  static bool compareLiteralTerms(const Literal* first, const Literal* second);
};

class BindingClassifier {
public:
  static Fragment classify(UnitList* units);
  static Fragment classify(Formula* formula);

  static Literal* mostLeftLiteral(Formula* formula);
private:
  static BindingClassification _innerClassify(Formula* formula);
};

}
#endif // VAMPIRE_BINDINGCLASSIFIER_H
