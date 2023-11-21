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

inline vstring fragmentToString(Fragment fragment){
    switch (fragment) {
    case UNIVERSAL_ONE_BINDING:
    return "Universal One Binding";
    case ONE_BINDING:
    return "One Binding";
    case CONJUNCTIVE_BINDING:
    return "Conjunctive Binding";
    case DISJUNCTIVE_BINDING:
    return "Disjunctive Binding";
    case NONE:
    return "None";
    }
}

class BindingClassification{
public:
  Fragment fragment;
  Literal* mostLeftLit;

  BindingClassification();
  BindingClassification(Fragment fragment, Literal *mostLeftLit);

  bool is(Fragment fragment1) const{
    return fragment == fragment1 || (fragment1 == ONE_BINDING && fragment == UNIVERSAL_ONE_BINDING);
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
  static BindingClassification _classifyHelper(Formula* formula);
};

}
#endif // VAMPIRE_BINDINGCLASSIFIER_H
