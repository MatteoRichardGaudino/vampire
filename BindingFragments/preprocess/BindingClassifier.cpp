//
// Created by Matte on 01/10/2023.
//

#include "BindingClassifier.h"
#include "Kernel/Formula.hpp"
#include "Kernel/Clause.hpp"

#include <iostream>

using namespace std;
using namespace Kernel;
using namespace BindingFragments;

// ----------------------- BindingClassifier -----------------------


Fragment BindingClassifier::classify(UnitList* units){
  TIME_TRACE(TimeTrace::CLASSIFICATION)
  UnitList::Iterator it(units);
  Fragment classification;

  bool out = env.options->showPreprocessing();
  if(out) env.beginOutput();

  if(it.hasNext()){
    auto formula = it.next()->getFormula();
    classification = classify(formula);
    if(out) env.out() << "[classify] Formula: " << formula->toString() << " fragment: "<< fragmentToString(classification)<< endl;
    if(!it.hasNext() || classification == NONE) return classification;
  }
  while (it.hasNext()){
    auto formula = it.next()->getFormula();
    auto classification2 = classify(formula);
    if(out) env.out() << "[classify] Formula: " << formula->toString() << " fragment: "<< fragmentToString(classification2) << endl;
    classification = BindingClassification::compare(classification, classification2);
    if(classification == NONE) return classification;
  }

  if(out) env.endOutput();
  return classification;
}
Fragment BindingClassifier::classify(Formula* formula){
  switch (formula->connective()) {

    case LITERAL:
      return UNIVERSAL_ONE_BINDING;
    case AND:
    case OR: {
      FormulaList::Iterator it(formula->args());
      Fragment classification = NONE;
      if (it.hasNext()) {
        auto subformula = it.next();
        classification = classify(subformula);
        if (!it.hasNext() || classification == NONE)
          return classification;
      }
      while (it.hasNext()) {
        auto subformula = it.next();
        classification = BindingClassification::compare(classification, classify(subformula));
        if (classification == NONE)
          return classification;
      }
      return classification;
    }
    case NOT: {
      auto subformula = formula->uarg();
      if (subformula->connective() == LITERAL)
        return UNIVERSAL_ONE_BINDING;
      else
        return NONE;
    }
    case FORALL:
    case EXISTS: {
      Formula *subformula = formula;
      Connective connective;
      bool canBeUniversal = (subformula->connective() == Connective::FORALL);
      do {
        subformula = subformula->qarg();
        connective = subformula->connective();
        if(connective == EXISTS) canBeUniversal = false;
      } while (connective == FORALL || connective == EXISTS);

      auto classification = _classifyHelper(subformula).fragment;
      if(!canBeUniversal && classification == UNIVERSAL_ONE_BINDING)
        classification = ONE_BINDING;
      return classification;
    }
    case BOOL_TERM:
    case FALSE:
    case TRUE:
      return UNIVERSAL_ONE_BINDING;
    default:
      return NONE;
  }
}

BindingClassification BindingClassifier::_classifyHelper(Formula* formula){
  switch (formula->connective()) {
    case LITERAL:
      return {UNIVERSAL_ONE_BINDING, formula->literal()};
    case AND:
    case OR:{
      FormulaList::Iterator it(formula->args());
      BindingClassification classification;
      if (it.hasNext()) {
        auto subformula = it.next();
        classification = _classifyHelper(subformula);
        if (!it.hasNext())
          return classification;
      }
      while (it.hasNext()) {
        auto subformula = it.next();
        classification = BindingClassification::compare(classification, _classifyHelper(subformula), formula->connective());
        if (classification.is(NONE))
          return classification;
      }
      return classification;
    }
    case NOT: {
      auto subformula = formula->uarg();
      if (subformula->connective() == LITERAL) {
        return {UNIVERSAL_ONE_BINDING, subformula->literal()};
      }
      else
        return {NONE, nullptr};
    }
    case BOOL_TERM:
    case FALSE:
    case TRUE:
      return {UNIVERSAL_ONE_BINDING, nullptr};
    default:
      return {NONE, nullptr};
  }
}


Literal* BindingClassifier::mostLeftLiteral(Formula* formula){
  switch (formula->connective()){
    case LITERAL:
      return formula->literal();
    case AND:
    case OR:
      return mostLeftLiteral(formula->args()->head());
    case NOT:
      return mostLeftLiteral(formula->uarg());
    default:
      return nullptr;
  }
}


// ----------------------- BindingClassification -----------------------

BindingClassification::BindingClassification(): fragment(NONE), mostLeftLit(nullptr){}
BindingClassification::BindingClassification(Fragment fragment, Literal *mostLeftLit) : fragment(fragment), mostLeftLit(mostLeftLit) {}

Fragment BindingClassification::compare(const Fragment &first, const Fragment &second){
  if(first == CONJUNCTIVE_BINDING && second == DISJUNCTIVE_BINDING) return NONE;
  if(first == DISJUNCTIVE_BINDING && second == CONJUNCTIVE_BINDING) return NONE;

  if(first > second) return first;
  else return second;
}
BindingClassification BindingClassification::compare(const BindingClassification &first, const BindingClassification &second, Connective connective){
  if(first.is(ONE_BINDING) && second.is(ONE_BINDING)){
    if(compareLiteralTerms(first.mostLeftLit, second.mostLeftLit)) {
      if(first.fragment <= second.fragment) return second; // If one is UNIVERSAL_ONE_BINDING
      return first;
    }
    if(connective == AND)
      return {CONJUNCTIVE_BINDING, nullptr};
    if(connective == OR)
      return {DISJUNCTIVE_BINDING, nullptr};
  } // return ONE_BINDING or UNIVERSAL_ONE_BINDING
  if(first.is(ONE_BINDING) && second.is(CONJUNCTIVE_BINDING) && connective == AND) return second; // return CONJUNCTIVE
  if(first.is(ONE_BINDING) && second.is(DISJUNCTIVE_BINDING) && connective == OR) return second; // return DISJUNCTIVE
  if(second.is(ONE_BINDING) && first.is(CONJUNCTIVE_BINDING) && connective == AND) return first; // return CONJUNCTIVE
  if(second.is(ONE_BINDING) && first.is(DISJUNCTIVE_BINDING) && connective == OR) return first; // return DISJUNCTIVE

  if(first.is(CONJUNCTIVE_BINDING) && second.is(CONJUNCTIVE_BINDING) && connective == AND) return first;
  if(first.is(DISJUNCTIVE_BINDING) && second.is(DISJUNCTIVE_BINDING) && connective == OR) return first;

  return {NONE, nullptr};
}

bool BindingClassification::compareLiteralTerms(const Literal* first, const Literal* second){
  if(first == nullptr && second == nullptr) return true;
  if(first == nullptr || second == nullptr) return false;

  // cout << "Compare: " << first->toString() << ", " << second->toString() << endl;
  const auto a1 = first->arity();
  const auto a2 = second->arity();
  if(a1 != a2) return false;
  if(a1 == 0 && a2 == 0) return true;

  auto t1 = first->args();
  auto t2 = second->args();

  for(unsigned i = 0; i < a1; ++i) {
    // cout<< "\tCompare: " << t1->toString() << ", " << t2->toString() << endl;
    if(!TermList::equals(*t1, *t2)) {
      // cout<< "FALSE" << endl;
      return false;
    }

    t1 = t1->next();
    t2 = t2->next();
  }
  // cout<< "TRUE" << endl;
  return true;
}
