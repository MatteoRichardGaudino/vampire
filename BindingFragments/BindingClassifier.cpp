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


Fragment BindingClassifier::classify(UnitList* units){
  UnitList::Iterator it(units);
  Fragment classification;

  if(it.hasNext()){
    auto formula = it.next()->getFormula();
    classification = classify(formula);
    cout<< "[classify] Formula: " << formula->toString() << " fragment: "<< classification << endl;
    if(!it.hasNext() || classification == NONE) return classification;
  }
  while (it.hasNext()){
    auto formula = it.next()->getFormula();
    auto classification2 = classify(formula);
    cout<< "[classify] Formula: " << formula->toString() << " fragment: "<< classification2 << endl;
    classification = BindingClassification::compare(classification, classification2);
    cout<< "\t[classify-Compare] fragment: "<< classification << endl;
    if(classification == NONE) return classification;
  }
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
        if(connective == Connective::EXISTS) canBeUniversal = false;
      } while (connective == Connective::FORALL || connective == Connective::EXISTS);

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
      return {UNIVERSAL_ONE_BINDING, formula->literal()->termArgs()};
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
      if (subformula->connective() == LITERAL)
        return {UNIVERSAL_ONE_BINDING, subformula->literal()->termArgs()};
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


bool BindingClassifier::_isFragment(Kernel::Formula *formula, BindingFragments::Fragment fragment){
  switch (formula->connective()) {
    case FALSE:
    case TRUE:
    case LITERAL:
      return true;
    case AND:
    case OR: {
      FormulaList::Iterator it(formula->args());
      while (it.hasNext()) {
        if (!_isFragment(it.next(), fragment))
          return false;
      }
      return true;
    }
    case NOT:
      if (formula->uarg()->connective() == Connective::LITERAL)
        return true;
      else
        return false;
    case FORALL:
    case EXISTS: {
      Formula *subformula = formula;
      Connective connective = formula->connective();
      if(fragment == UNIVERSAL_ONE_BINDING && connective == Connective::EXISTS) return false;
      do {
        subformula = subformula->qarg();
        connective = subformula->connective();
        if(fragment == UNIVERSAL_ONE_BINDING && connective == Connective::EXISTS) return false;
      } while (connective == Connective::FORALL || connective == Connective::EXISTS);
      auto term = getBindingTerm(subformula);
      if(term == nullptr) return false;
      switch (fragment) {
        case UNIVERSAL_ONE_BINDING:
        case ONE_BINDING:
          return _isOneBindingHelper(subformula, term);
        case CONJUNCTIVE_BINDING:
          return _isConjunctiveBindingHelper(subformula, term);
        case DISJUNCTIVE_BINDING:
          return _isDisjunctiveBindingHelper(subformula, term);
        default:
          return false;
      }
    }
    default:
      return false;
  }
}
bool BindingClassifier::_isOneBindingHelper(Formula *formula, TermList *term){
  switch (formula->connective()) {
    case LITERAL:
      return Kernel::TermList::equals(*term, *formula->literal()->termArgs());
    case AND:
    case OR:{
        FormulaList::Iterator it(formula->args());
        while (it.hasNext()) {
          if (!_isOneBindingHelper(it.next(), term))
            return false;
        }
        return true;
    }
    case NOT:
      return _isOneBindingHelper(formula->uarg(), term);
    default:
      return false;
  }
}

bool BindingClassifier::_isFragment(UnitList* units, Fragment fragment){
  UnitList::Iterator it(units);
  while (it.hasNext()){
    auto formula = it.next()->getFormula();

    bool res;
    switch(fragment){
      case ONE_BINDING:
          res = isOneBinding(formula);
          break;
      case CONJUNCTIVE_BINDING:
          res = isConjunctiveBinding(formula);
          break;
      case UNIVERSAL_ONE_BINDING:
          res = isUniversalOneBinding(formula);
          break;
      case DISJUNCTIVE_BINDING:
          res = isDisjunctiveBinding(formula);
          break;
      default:
          res = false;
    }
    cout<< "[_isFragment(_, " << fragment << ")] " << ((res)? "TRUE ": "FALSE ") << " formula: " << formula->toString() << endl;
    if(!res) return false;
  }
  return true;
}

TermList* BindingClassifier::getBindingTerm(Formula* formula){
  return mostLeftLiteral(formula)->termArgs();
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

bool BindingClassifier::_isConjunctiveBindingHelper(Formula *formula, TermList *term){
  switch (formula->connective()) {
    case LITERAL:
      return true;
    case AND: {
      FormulaList::Iterator it(formula->args());
      while (it.hasNext()) {
          if (!_isConjunctiveBindingHelper(it.next(), term))
            return false;
      }
      return true;
    }
    case OR: {
      FormulaList::Iterator it(formula->args());
      while (it.hasNext()) {
          if (!_isOneBindingHelper(it.next(), term))
            return false;
      }
      return true;
    }
    case NOT:
      return _isOneBindingHelper(formula->uarg(), term);
    default:
      return false;
  }
}
bool BindingClassifier::_isDisjunctiveBindingHelper(Formula *formula, TermList *term){
  switch (formula->connective()) {
    case LITERAL:
      return true;
    case OR: {
      FormulaList::Iterator it(formula->args());
      while (it.hasNext()) {
          if (!_isDisjunctiveBindingHelper(it.next(), term))
            return false;
      }
      return true;
    }
    case AND: {
      FormulaList::Iterator it(formula->args());
      while (it.hasNext()) {
          if (!_isOneBindingHelper(it.next(), term))
            return false;
      }
      return true;
    }
    case NOT:
      return _isOneBindingHelper(formula->uarg(), term);
    default:
      return false;
  }
}

BindingClassification::BindingClassification(): fragment(NONE), term(nullptr){}
BindingClassification::BindingClassification(Fragment fragment, TermList *term) : fragment(fragment), term(term) {}


Fragment BindingClassification::compare(const Fragment &first, const Fragment &second){
  if(first == CONJUNCTIVE_BINDING && second == DISJUNCTIVE_BINDING) return NONE;
  if(first == DISJUNCTIVE_BINDING && second == CONJUNCTIVE_BINDING) return NONE;

  if(first > second) return first;
  else return second;
}
BindingClassification BindingClassification::compare(const BindingClassification &first, const BindingClassification &second, Connective connective){
  if(first.is(ONE_BINDING) && second.is(ONE_BINDING)){
    if(first.term == nullptr || second.term == nullptr) return {(first.fragment <= second.fragment)? second.fragment: first.fragment, nullptr};
    if(TermList::equals(*first.term, *second.term))
      if(first.fragment <= second.fragment) return second; // If one is UNIVERSAL_ONE_BINDING
      else return first;
    else{
      if(connective == Connective::AND)
        return {Fragment::CONJUNCTIVE_BINDING, nullptr};
      else if(connective == Connective::OR)
        return {Fragment::DISJUNCTIVE_BINDING, nullptr};
    }
  } // return ONE_BINDING or UNIVERSAL_ONE_BINDING
  else if(first.fragment == second.fragment) return first; // return DISJUNCTIVE or CONJUNCTIVE
  else if(first.is(ONE_BINDING) && second.is(CONJUNCTIVE_BINDING) && connective == AND) return second; // return CONJUNCTIVE
  else if(first.is(ONE_BINDING) && second.is(DISJUNCTIVE_BINDING) && connective == OR) return second; // return DISJUNCTIVE
  else if(second.is(ONE_BINDING) && first.is(CONJUNCTIVE_BINDING) && connective == AND) return first; // return CONJUNCTIVE
  else if(second.is(ONE_BINDING) && first.is(DISJUNCTIVE_BINDING) && connective == OR) return first; // return DISJUNCTIVE

  return {NONE, nullptr};
}