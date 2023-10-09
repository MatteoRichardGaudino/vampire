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
  UnitList::Iterator uit(list);
  int i = 0;
  while (uit.hasNext()){
    cout<< "Unit "<< i << uit.next()->toString() << endl;
  }

  cout<< endl << endl;
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
  cout<< endl;
  auto classification = BindingClassifier::classify(list);
  cout<< "Classification fragment: "<< classification << endl;
  cout<< endl;
}
