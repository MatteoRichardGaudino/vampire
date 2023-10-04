//
// Created by Matte on 01/10/2023.
//

#include "BindingClassifier.h"
#include "Kernel/Formula.hpp"

#include <iostream>

using namespace std;
using namespace Kernel;
using namespace BindingFragments;

Fragment BindingClassifier::classify(UnitList* units){
  return Fragment::NONE;
}
Fragment BindingClassifier::classify(Formula* formula){
  return Fragment::NONE;
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
        if (!isOneBinding(it.next()))
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
      Connective connective;
      do {
        subformula = subformula->qarg();
        connective = subformula->connective();
        if(fragment == UNIVERSAL_ONE_BINDING && connective == Connective::EXISTS) return false;
      } while (connective == Connective::FORALL || connective == Connective::EXISTS);
      auto term = _findATerm(subformula);
      if(term == nullptr) return false;
      switch (fragment) {
        case ONE_BINDING:
          return _isOneBindingHelper(subformula, term);
        case UNIVERSAL_ONE_BINDING:
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
    case IMP:
    case IFF:
    case XOR:
        return _isOneBindingHelper(formula->left(), term) && _isOneBindingHelper(formula->right(), term);
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
    formula = (it.hasNext())? formula : formula->uarg(); // Se è la congettura rimuove la negazione // TODO da fare in altro modo

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

TermList* BindingClassifier::_findATerm(Formula* formula){
  switch (formula->connective()){
    case LITERAL:
      return formula->literal()->termArgs();
    case AND:
    case OR:
      return _findATerm(formula->args()->head());
    case IFF:
    case XOR:
    case IMP:
      return _findATerm(formula->left());
    case NOT:
      return _findATerm(formula->uarg());
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
    case IMP:
    case IFF:
    case XOR:
      return _isOneBindingHelper(formula->left(), term) && _isOneBindingHelper(formula->right(), term);
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
    case IMP:
    case IFF:
    case XOR:
      return _isOneBindingHelper(formula->left(), term) && _isOneBindingHelper(formula->right(), term);
    case NOT:
      return _isOneBindingHelper(formula->uarg(), term);
    default:
      return false;
  }
}
