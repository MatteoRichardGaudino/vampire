//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Indexing/Index.hpp"
#include "Indexing/LiteralSubstitutionTree.hpp"
#include "BindingClassifier.h"
#include "Kernel/RobSubstitution.hpp"
#include "OneBindingSat.h"

#include <iostream>

using namespace std;
using namespace Kernel;
using namespace Indexing;

void BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt){
  OneBindingSat sat(prb, opt);
  sat.solve();

  return;
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
    for(unsigned int j = 0; j < clause->length(); j++){
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
    for(unsigned int j = 0; j < clause->length(); j++){
      cout<< "GetUnifications on: " << literals[j]->toString() << endl;
      auto resultIt = is.getUnifications(literals[j], false, true);

      RobSubstitution rSub;

      int r = 0;
      while(resultIt.hasNext()){
        auto result = resultIt.next();
        cout<< "\t\tRes " << ++r<< ":" << result.literal->toString() << " Clause: " << result.clause->number() << endl;
        auto s = result.substitution;
        cout << "\t"; s->output(cout); cout << endl;

//        cout<< "\t\t[00] " << s->applyTo(literals[j], 0)->toString() << " =? " << s->applyTo(result.literal, 0)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 0, *result.literal->termArgs(), 0) << endl;
//        cout<< "\t\t[01] " << s->applyTo(literals[j], 0)->toString() << " =? " << s->applyTo(result.literal, 1)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 0, *result.literal->termArgs(), 1) << endl;
//        cout<< "\t\t[10] " << s->applyTo(literals[j], 1)->toString() << " =? " << s->applyTo(result.literal, 0)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 1, *result.literal->termArgs(), 0) << endl;
//        cout<< "\t\t[11] " << s->applyTo(literals[j], 1)->toString() << " =? " << s->applyTo(result.literal, 1)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 1, *result.literal->termArgs(), 1) << endl;
//        cout<< "\t\t[22] " << s->applyTo(literals[j], 2)->toString() << " =? " << s->applyTo(result.literal, 2)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 2, *result.literal->termArgs(), 2) << endl;
//        cout<< "\t\t[33] " << s->applyTo(literals[j], 3)->toString() << " =? " << s->applyTo(result.literal, 3)->toString()
//             << " unify? " << rSub.unify(*literals[j]->termArgs(), 3, *result.literal->termArgs(), 3) << endl;
      }

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
