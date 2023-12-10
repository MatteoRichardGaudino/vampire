//
// Created by Matte on 09/10/2023.
//

#include "../../Forwards.hpp"
#include "Preprocess.h"
#include "../../Kernel/Formula.hpp"
#include "../../Kernel/Clause.hpp"
#include "../../Kernel/Problem.hpp"
#include "../../Kernel/FormulaUnit.hpp"
#include "../../Shell/NNF.hpp"
#include "../../Shell/Skolem.hpp"
#include "../../Shell/Flattening.hpp"
#include "../../Shell/CNF.hpp"
#include "../../Shell/Statistics.hpp"
#include "ForallAnd.h"
#include "../../Shell/Preprocess.hpp"
#include "../../Shell/Naming.hpp"

// void BindingFragments::Preprocess::preprocess(Problem &prb){
//   Kernel::Preprocess(*env.options).preprocess1(prb);
//
//   UnitList::DelIterator uit(prb.units());
//   while (uit.hasNext()){
//     auto unit = uit.next();
//     if(unit->isClause()) continue;
//
//     // *** NNF ***
//     FormulaUnit* fu = static_cast<FormulaUnit*>(unit);
//     FormulaUnit* fu0 = fu;
//     fu = NNF::nnf(fu);
//     fu = Flattening::flatten(fu);
//
//     if(fu != fu0){
//         uit.replace(fu);
//     }
//
//     // *** Skolemisation ***
//     if(_skolemize || _clausify) {
//         fu0 = fu;
//         fu = Skolem::skolemise(fu, false);
//         if(fu != fu0){
//           uit.replace(fu);
//         }
//
//       fu = Flattening::flatten(fu);
//       if(fu != fu0){
//         uit.replace(fu);
//       }
//     }
//
//     // *** Distribute Forall over And ***
//     if(_distributeForall){
//       fu0 = fu;
//       fu = ForallAnd::forallAnd(fu);
//       if(fu != fu0){
//             uit.replace(fu);
//       }
//     }
//
//   }
//
//   if(_clausify) {
//     env.statistics->phase = Statistics::CLAUSIFICATION;
//     clausify(prb);
//   }
// }


void BindingFragments::Preprocess::preprocess(Problem &prb){
  Kernel::Preprocess(*env.options).preprocess1(prb);
  Naming naming(env.options->naming(),false, prb.isHigherOrder()); // For now just force eprPreservingNaming to be false, should update Naming

  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()){
    auto unit = uit.next();
    if(unit->isClause()) continue;


    FormulaUnit* fu = static_cast<FormulaUnit*>(unit);
    FormulaUnit* fu0 = fu;

    // // *** ENNF ***
    // fu = NNF::ennf(fu);
    // fu = Flattening::flatten(fu);
    //
    // if(fu != fu0){
    //   uit.replace(fu);
    // }
    //
    // // *** Naming ***
    //
    // UnitList* defs;
    // fu0 = naming.apply(fu,defs);
    // if (fu0 != fu) {
    //   ASS(defs);
    //   uit.insert(defs);
    //   uit.replace(fu0);
    // }

    // *** NNF ***
    fu = static_cast<FormulaUnit*>(unit);
    fu0 = fu;
    fu = NNF::nnf(fu);
    fu = Flattening::flatten(fu);

    if(fu != fu0){
      uit.replace(fu);
    }

    // *** Skolemisation ***
    if(_skolemize || _clausify) {
      fu0 = fu;
      fu = Skolem::skolemise(fu, false);
      if(fu != fu0){
        uit.replace(fu);
      }

      fu = Flattening::flatten(fu);
      if(fu != fu0){
        uit.replace(fu);
      }
    }

    // *** Distribute Forall over And ***
    if(_distributeForall){
      fu0 = fu;
      fu = ForallAnd::forallAnd(fu);
      if(fu != fu0){
        uit.replace(fu);
      }
    }

  }

  if(_clausify) {
    env.statistics->phase = Statistics::CLAUSIFICATION;
    clausify(prb);
  }
}


void BindingFragments::Preprocess::clausify(Problem &prb)
{
  env.statistics->phase = Statistics::CLAUSIFICATION;
  // we check if we haven't discovered an empty clause during preprocessing
  Unit *emptyClause = 0;

  bool modified = false;

  UnitList::DelIterator us(prb.units());
  CNF cnf;
  Stack<Clause *> clauses(32);
  while (us.hasNext()) {
    Unit *u = us.next();
    if (env.options->showPreprocessing()) {
      env.beginOutput();
      env.out() << "[PP] clausify: " << u->toString() << std::endl;
      env.endOutput();
    }
    if (u->isClause()) {
      if (static_cast<Clause *>(u)->isEmpty()) {
        emptyClause = u;
        break;
      }
      continue;
    }
    modified = true;
    cnf.clausify(u, clauses);
    while (!clauses.isEmpty()) {
      Unit *u = clauses.pop();
      if (static_cast<Clause *>(u)->isEmpty()) {
        emptyClause = u;
        goto fin;
      }
      us.insert(u);
    }
    us.del();
  }
fin:
  if (emptyClause) {
    UnitList::destroy(prb.units());
    prb.units() = 0;
    UnitList::push(emptyClause, prb.units());
  }
  if (modified) {
    prb.invalidateProperty();
  }
  prb.reportFormulasEliminated();
}

void BindingFragments::Preprocess::negatedProblem(Problem &prb)
{
  FormulaList *axAndConj = FormulaList::empty();
  auto units = prb.units();
  UnitList::Iterator uit(units);
  while (uit.hasNext()) {
    Unit *u = uit.next();
    FormulaList::push(new NegatedFormula(u->getFormula()), axAndConj);
  }

  UnitList::DelIterator dIt(units);
  if(dIt.hasNext()) {
    dIt.next();
    auto newProb = new FormulaUnit(
                       (FormulaList::length(axAndConj) != 1)?
                          new JunctionFormula(OR, axAndConj) :
                          axAndConj->head(),
                       FromInput(UnitInputType::CONJECTURE));
    dIt.replace(newProb);

    if(env.options->showPreprocessing()) {
      env.beginOutput();
      env.out() << "[pp] Negated Problem: " << newProb->toString() << std::endl;
      env.endOutput();
    }
  }
  while (dIt.hasNext()) {
    dIt.next();
    dIt.del();
  }
}