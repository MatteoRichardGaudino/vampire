//
// Created by Matte on 09/10/2023.
//

#include "Preprocess.h"
#include "Kernel/Formula.hpp"
#include "Kernel/Problem.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Shell/NNF.hpp"
#include "Shell/Flattening.hpp"


void BindingFragments::Preprocess::preprocess(Problem &prb){
  // ---- Remove the negation from the conjecture and NNF ----
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()){
    auto unit = uit.next();
    ASS(!unit->isClause())
    if(!uit.hasNext()){ // if is conjecture remove negation
      auto conjecture = new FormulaUnit(unit->getFormula()->uarg(), FromInput(UnitInputType::CONJECTURE));
      uit.replace(conjecture);
      unit = conjecture;
    }

    FormulaUnit* fu = static_cast<FormulaUnit*>(unit);
    FormulaUnit* fu0 = fu;
    fu = Shell::NNF::nnf(fu);
    fu = Flattening::flatten(fu);

    if(fu != fu0){
        uit.replace(fu);
    }
  }
}
