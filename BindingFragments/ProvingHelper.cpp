//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Kernel/Clause.hpp"

#include "BindingClassifier.h"

#include <iostream>

using namespace std;
using namespace Kernel;

void BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt){
  auto list = prb.units();
//  UnitList::Iterator it(list);
//  cout<< "Units: "<< list->toString() << endl;

//  while (it.hasNext()){
//    auto unit = it.next();
//    auto formula = unit->getFormula();
//    cout<< "Formula toString: " << formula->toString() << endl;
//    cout<< "Formula connective: " << formula->connective() << endl;
//
//    if(formula->connective() == Connective::AND || formula->connective() == Connective::OR) {
//      FormulaList::Iterator it2(formula->args());
//      while (it2.hasNext()) {
//        auto formula2 = it2.next();
//        cout << "\tFormula2 toString: " << formula2->toString() << endl;
//        cout << "\tFormula2 connective: " << formula2->connective() << endl;
//      }
//    }
//    if(formula->connective() == Connective::FORALL){
//      auto f = formula->qarg();
//      cout<< "\tFormula2 toString: " << f->toString() << endl;
//    }
//  }

  cout<< endl;
  auto isOneBinding = BindingClassifier::isOneBinding(list);
  cout<< "Is one Binding? "<< isOneBinding << endl;
  cout<< endl;
  auto isCB = BindingClassifier::isConjunctiveBinding(list);
  cout<< "Is Conjunctive Binding? "<< isCB << endl;
  cout<< endl;
  auto isU1B = BindingClassifier::isUniversalOneBinding(list);
  cout<< "Is Universal one Binding? "<< isU1B << endl;
  cout<< endl;
  auto isDB = BindingClassifier::isDisjunctiveBinding(list);
  cout<< "Is Disjunctive Binding? "<< isDB << endl;
  cout<< endl;
}
