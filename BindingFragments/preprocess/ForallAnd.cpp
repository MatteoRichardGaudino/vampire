//
// Created by Matteo Richard on 20/11/23.
//

#include "ForallAnd.h"
#include "../../Debug/RuntimeStatistics.hpp"

#include "../../Kernel/Inference.hpp"
#include "../../Kernel/FormulaUnit.hpp"
#include "../../Kernel/Problem.hpp"
#include "../../Kernel/Unit.hpp"

#include "../../Lib/Environment.hpp"
#include "../../Shell/Options.hpp"
#include "../../Shell/Statistics.hpp"

namespace Shell
{


FormulaUnit* ForallAnd::forallAnd(FormulaUnit* unit)
{
  ASS(! unit->isClause());

  Formula* f = unit->formula();
  Formula* g = forallAnd(f);
  if (f == g) { // not changed
    return unit;
  }

  FormulaUnit* res = new FormulaUnit(g,
      FormulaTransformation(InferenceRule::FORALL_AND,unit));
  if (env.options->showPreprocessing()) {
    env.beginOutput();
    env.out() << "[PP] forallAnd in: " << unit->toString() << std::endl;
    env.out() << "[PP] forallAnd out: " << res->toString() << std::endl;
    env.endOutput();
  }
  return res;
}


Formula* ForallAnd::innerForallAnd (Formula* f)
{
  Connective con = f->connective();
  switch (con) {
  case TRUE:
  case FALSE:
  case LITERAL:
  case NOT:
  case BOOL_TERM:
    return f;
  case AND:
  case OR:
    {
      FormulaList* args = forallAnd(f->args(),con);
      if (args == f->args()) {
	return f;
      }
      return new JunctionFormula(con,args);
    }
  case FORALL:
    {
      Formula* arg = f->qarg();
      if(arg->connective() != AND) return f;
      if(FormulaList::length(arg->args()) == 1) return f;

      // distribute forall over and
      FormulaList::Iterator it(arg->args());
      FormulaList* qAndArgs = FormulaList::empty();

      while (it.hasNext()) {
        FormulaList::push(new QuantifiedFormula(f->connective(), f->vars(), f->sorts(), it.next()), qAndArgs);
      }

      return new JunctionFormula(AND, qAndArgs);
    }
  default:
    ASSERTION_VIOLATION;
  }
}

FormulaList* ForallAnd::forallAnd (FormulaList* fs,
				  Connective con)
{
  ASS(con == OR || con == AND);
  if(!fs) {
    return 0;
  }

  FormulaList* fs0 = fs;

  bool modified = false;
  Stack<Formula*> res(8);
  Stack<FormulaList*> toDo(8);
  for(;;) {
    if(fs->head()->connective()==con) {
      modified = true;
      if(fs->tail()) {
	toDo.push(fs->tail());
      }
      fs = fs->head()->args();
      continue;
    }
    Formula* hdRes = forallAnd(fs->head());
    if(hdRes!=fs->head()) {
      modified = true;
    }
    res.push(hdRes);
    fs = fs->tail();
    if(!fs) {
      if(toDo.isEmpty()) {
	break;
      }
      fs = toDo.pop();
    }
  }
  if(!modified) {
    return fs0;
  }
  FormulaList* resLst = 0;
  FormulaList::pushFromIterator(Stack<Formula*>::TopFirstIterator(res), resLst);
  return resLst;

}


}
