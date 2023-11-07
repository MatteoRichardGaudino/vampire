//
// Created by Matte on 09/10/2023.
//

#include "Forwards.hpp"
#include "Preprocess.h"
#include "Kernel/Formula.hpp"
#include "Kernel/Clause.hpp"
#include "Kernel/Problem.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Shell/NNF.hpp"
#include "Shell/Skolem.hpp"
#include "Shell/Flattening.hpp"
#include "Shell/CNF.hpp"
#include "Shell/Statistics.hpp"


void BindingFragments::Preprocess::preprocess(Problem &prb){
  env.statistics->phase = Statistics::NORMALIZATION;
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()){
    auto unit = uit.next();
    if(unit->isClause()) continue;
//    auto inf = unit->inference();
//
//    if(inf.rule() == InferenceRule::NEGATED_CONJECTURE){ // if is conjecture remove negationI
//      auto conjecture = new FormulaUnit(unit->getFormula()->uarg(), FromInput(UnitInputType::CONJECTURE));
//      uit.replace(conjecture);
//      unit = conjecture;
//    }

    FormulaUnit* fu = static_cast<FormulaUnit*>(unit);
    FormulaUnit* fu0 = fu;
    fu = Shell::NNF::nnf(fu);
    fu = Flattening::flatten(fu);

    if(fu != fu0){
        uit.replace(fu);
    }

    if(_skolemize || _clausify) {
        fu0 = fu;
        fu = Skolem::skolemise(fu, false);
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

void BindingFragments::Preprocess::clausify(Problem& prb){
  env.statistics->phase=Statistics::CLAUSIFICATION;
  //we check if we haven't discovered an empty clause during preprocessing
  Unit* emptyClause = 0;

  bool modified = false;

  UnitList::DelIterator us(prb.units());
  CNF cnf;
  Stack<Clause*> clauses(32);
  while (us.hasNext()) {
    Unit* u = us.next();
    if (env.options->showPreprocessing()) {
        env.beginOutput();
        env.out() << "[PP] clausify: " << u->toString() << std::endl;
        env.endOutput();
    }
    if (u->isClause()) {
        if (static_cast<Clause*>(u)->isEmpty()) {
          emptyClause = u;
          break;
        }
        continue;
    }
    modified = true;
    cnf.clausify(u,clauses);
    while (! clauses.isEmpty()) {
        Unit* u = clauses.pop();
        if (static_cast<Clause*>(u)->isEmpty()) {
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
