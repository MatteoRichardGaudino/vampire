//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Kernel/Clause.hpp"
#include "Indexing/Index.hpp"
#include "Indexing/LiteralSubstitutionTree.hpp"
#include "BindingClassifier.h"
#include "Indexing/TermSubstitutionTree.hpp"
#include "OneBindingSat.h"

#include <iostream>

using namespace std;
using namespace Kernel;
using namespace Indexing;



void literalIS(UnitList* list){
  cout<< "---- Literal Indexing structure -----" << endl;
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

  uit.reset(list);
  auto clause = uit.next()->asClause();
  auto l1 = clause->literals()[0];

  cout<< multiline(is.getTree(l1, false));

  cout<< "L1: "<< l1->toString() << endl;
  auto it = is.getUnifications(l1, false, true);
  while (it.hasNext()){
    auto res = it.next();
    cout<< "\t" << res.literal->toString() << endl;
    res.substitution->output(cout);
    cout << endl;
  }

  cout<< "------------------------------------" << endl;
}

void termIS(UnitList* units)
{
  cout << "---- Term Indexing structure -----" << endl;
  TermSubstitutionTree is;

  UnitList::Iterator uit(units);
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (!uit.hasNext())
      break;
    cout << "Unit: " << unit->toString() << endl;

    auto clause = unit->asClause();
    auto literals = clause->literals();
    for (unsigned int j = 0; j < clause->length(); j++) {
      cout << "GetUnifications on: " << literals[j]->toString() << endl;
      auto resultIt = is.getUnifications(literals[j], false, true);
      auto clause = unit->asClause();
      auto literals = clause->literals();
      for (int j = 0; j < clause->length(); j++) {
        auto lit = literals[j];
        for (int k = 0; k < lit->arity(); k++) {
          if (lit->termArg(k).isTerm())
            is.insert(TypedTermList(lit->termArg(k).term()), lit, clause);
        }
      }

      RobSubstitution rSub;

      int r = 0;
      while (resultIt.hasNext()) {
        auto result = resultIt.next();
        cout << "\t\tRes " << ++r << ":" << result.literal->toString() << " Clause: " << result.clause->number() << endl;
        auto s = result.substitution;
        cout << "\t";
        s->output(cout);
        cout << endl;
        cout << "Albero: " << endl;
        cout << multiline(is) << endl;
        cout << "------------------------------------" << endl;

        return;
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

      //  uit.reset(units);
      //  auto clause = uit.next()->asClause();
      //  auto l1 = clause->literals()[0];
      //  cout<< "Literal: "<< l1->toString() << endl;
      //  for(int k = 0; k < l1->arity(); k++){
      //        if(l1->termArg(k).isTerm()){
      //          auto term = l1->termArg(k).term();
      //          auto it = is.getUnifications(term, true, false);
      //          while (it.hasNext()){
      //                auto res = it.next();
      //                cout<< "\t" << res << endl;
      //          }
      //        }
      //    }
      //  }

      cout << "------------------------------------" << endl;
    }
  }
}

void substTree(UnitList* list){
  cout<< "---- Substitution tree -----" << endl;
  SubstitutionTree tree(false, false);

  UnitList::Iterator uit(list);
  while (uit.hasNext()){
    auto unit = uit.next();
    if(!uit.hasNext())
      break;
    cout<< "Unit: "<< unit->toString() << endl;

    auto clause = unit->asClause();
    auto literals = clause->literals();
    for(unsigned int j = 0; j < clause->length(); j++){
      auto lit = literals[j];
      cout<< "[" << lit <<  "] " << lit->toString() << endl;
      tree.handle(lit, SubstitutionTree::LeafData(clause, lit), true);
    }
  }

  uit.reset(list);
  auto clause = uit.next()->asClause();
  auto l1 = clause->literals()[0];

  cout<< multiline(tree) << endl;
  cout<< "L1: [" << l1 << "] " << l1->toString() << endl;
  auto it = tree.iterator<SubstitutionTree::UnificationsIterator>(l1, true, false);
  while (it.hasNext()){
    auto res = it.next();
    cout<< "\t"<< '[' << res.data->literal <<  "] " << res.data->literal->toString() << endl;
    auto r = res.subst->applyToResult(res.data->literal);
    auto q = res.subst->applyToQuery(l1);
    cout<< "\t\t" << "Subst: r[" << r << "] " << r->toString() << " =? " << q->toString() << " q[" << q << "]" << endl;
    //res.subst->output(cout);
    cout << endl;
  }

  cout<< "------------------------------------" << endl;
}


void BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt, int classification){
  ASS_L(classification, DISJUNCTIVE_BINDING)
  if(classification == CONJUNCTIVE_BINDING) toUniversalOneBinding(prb);

//  substTree(prb.units());
//
//  return;
  OneBindingSat sat(prb, opt);
  sat.solve();
}
void BindingFragments::ProvingHelper::toUniversalOneBinding(Problem &prb){
  // TODO
}
