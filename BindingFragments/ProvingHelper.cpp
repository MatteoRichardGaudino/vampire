//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Kernel/Clause.hpp"
#include "Indexing/Index.hpp"
#include "Indexing/LiteralSubstitutionTree.hpp"
#include "BindingClassifier.h"

#include <iostream>

using namespace std;
using namespace Kernel;
using namespace Indexing;

void BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt){
  auto list = prb.units();

  LiteralSubstitutionTree is;

  UnitList::Iterator uit(list);
  while (uit.hasNext()){
    auto unit = uit.next();
    if(!uit.hasNext())
      break;
    cout<< "Unit: "<< unit->toString() << endl;

    auto clause = unit->asClause();
    auto literals = clause->literals();
    for(int j = 0; j < clause->length(); j++){
      is.insert(literals[j], clause);
    }
  }

  uit.reset(prb.units());
  while (uit.hasNext()){
    auto unit = uit.next();
    if(!uit.hasNext())
      break;

    auto clause = unit->asClause();
    auto literals = clause->literals();
    for(int j = 0; j < clause->length(); j++){
      cout<< "Query on: " << literals[j]->toString() << endl;
      cout<< "\tgetUnifications: " << endl;
      auto resultIt = is.getUnifications(literals[j], false, true);

      int r = 0;
      while(resultIt.hasNext()){
        auto result = resultIt.next();
        cout<< "\t\tRes " << ++r<< ":" << result.literal->toString() << " Clause: " << result.clause->number() << endl;
        auto s = result.substitution;

        auto l = s->applyTo(result.literal, 1);
//        arr[arr_i++] = *l;
        s->output(cout);
        cout << endl;
        cout<< "\t\t Sostituzione1: " << l->toString() << endl;
        auto l2 = s->applyTo(literals[j], 0);
        cout<< "\t\t Sostituzione2: " << l2->toString() << endl;
      }

      cout<< "\tgetGeneralizations: " << endl;
      resultIt = is.getGeneralizations(literals[j], false, false);
      r = 0;
      while(resultIt.hasNext()){
        auto result = resultIt.next();
        cout<< "\t\tRes " << ++r<< ":" << result.literal->toString() << " Clause: " << result.clause->number() << endl;
      }
      cout<< endl;
    }
  }

  cout<< "FINITO" << endl;

  return;

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
