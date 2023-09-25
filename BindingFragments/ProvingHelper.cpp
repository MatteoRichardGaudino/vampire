//
// Created by Matte on 25/09/2023.
//

#include "ProvingHelper.h"
#include "Kernel/Clause.hpp"

#include <iostream>

using namespace std;
using namespace Kernel;

void BindingFragments::ProvingHelper::run1BSatAlgorithm(Problem &prb, const Options &opt){
  int i = 0;
  cout<< "---- Clause Iterator ----" << endl;
  auto iterator = prb.clauseIterator();
  while (iterator.hasNext()){
    auto c = iterator.next();
    cout << "Clausola " << ++i << ": " <<  c->literalsOnlyToString() << endl;
    cout<< "\t Length: " << c->length() << endl;
    cout<< "\t is Ground: " << c->isGround() << endl;
    cout<< "\t isPropositional: " << c->isPropositional() << endl;
    auto litIterator = c->getLiteralIterator();
    while(litIterator.hasNext()){
      auto lit = litIterator.next();
      cout<< "\t\t Literal: " << lit->toString() << endl;
      cout<< "\t\t Arity: " << lit->arity() << endl;
      auto terms = lit->termArgs();
      while (!terms->isEmpty()){
        if(terms->isTerm()){
          auto term = terms->term();
          cout<< "\t\t\t Term: " << term->toString() << endl;
          cout<< "\t\t\t functionName: " << term->functionName() << endl;
          cout<< "\t\t\t Arity: " << term->arity() << endl;

        } else if(terms->isVar()) {
          auto var = terms->var();
          cout<< "\t\t\t Var: " << Term::variableToString(var) << endl;
        }
        terms = terms->next();
      }
      cout<< "\t\t -------------- " << endl;
    }

    cout<< endl;
  }
}
