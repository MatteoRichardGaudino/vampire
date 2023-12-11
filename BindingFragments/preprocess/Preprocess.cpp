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
#include "SAT/MinisatInterfacing.hpp"
#include "BindingClassifier.h"


using namespace std;
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

// TODO

void printStack(SATClauseStack* stk){
  SATClauseStack::Iterator it(*stk);
  while (it.hasNext()){
    std::cout<< it.next()->toString() << std::endl;
  }
}
void printStack(ClauseStack* stk){
  ClauseStack::Iterator it(*stk);
  while (it.hasNext()){
    std::cout<< it.next()->toString() << std::endl;
  }
}

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
  if (dIt.hasNext()) {
    dIt.next();
    auto newProb = new FormulaUnit(
        (FormulaList::length(axAndConj) != 1) ? new JunctionFormula(OR, axAndConj) : axAndConj->head(),
        FromInput(UnitInputType::CONJECTURE));
    dIt.replace(newProb);

    if (env.options->showPreprocessing()) {
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

// ------------------ PreprocessV2 ------------------
BindingFragments::PreprocessV2::PreprocessV2(Problem &prb, Fragment fragment) : prb(prb), fragment(fragment){
  Kernel::Preprocess(*env.options).preprocess1(prb);
}

void BindingFragments::PreprocessV2::ennf()
{
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (unit->isClause())
      continue;

    FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
    FormulaUnit *fu0 = fu;

    // *** ENNF ***
    fu = NNF::ennf(fu);
    fu = Flattening::flatten(fu);

    if (fu != fu0) {
      uit.replace(fu);
    }
  }
}
void BindingFragments::PreprocessV2::topBooleanFormulaAndBindings(){
  ASS_L(fragment, NONE)
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();

    Formula* topFormula = _topBooleanFormula(unit->getFormula());
    uit.replace(
      new FormulaUnit(
        topFormula,
        Inference(FormulaTransformation(InferenceRule::DEFINITION_FOLDING, unit))
      )
    );
  }
}
void BindingFragments::PreprocessV2::naming(){
  Naming naming(env.options->naming(),false, false);
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (unit->isClause())
      continue;

    FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
    FormulaUnit *fu0 = fu;

    // *** Naming ***

    UnitList* defs;
    fu0 = naming.apply(fu,defs);
    if (fu0 != fu) {
      ASS(defs);
      uit.insert(defs);
      uit.replace(fu0);
    }
  }
}
void BindingFragments::PreprocessV2::nnf(){
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (unit->isClause())
      continue;

    FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
    FormulaUnit *fu0 = fu;

    // *** NNF ***
    fu = NNF::nnf(fu);
    fu = Flattening::flatten(fu);

    if(fu != fu0){
      uit.replace(fu);
    }
  }
}
void BindingFragments::PreprocessV2::skolemize(){
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (unit->isClause())
      continue;

    FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
    FormulaUnit *fu0 = fu;

    // *** Skolemisation ***
    fu = Skolem::skolemise(fu, false);
    if(fu != fu0){
      uit.replace(fu);
    }

    fu = Flattening::flatten(fu);
    if(fu != fu0){
      uit.replace(fu);
    }
  }
}
void BindingFragments::PreprocessV2::distributeForall()
{
  UnitList::DelIterator uit(prb.units());
  while (uit.hasNext()) {
    auto unit = uit.next();
    if (unit->isClause())
      continue;

    FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
    FormulaUnit *fu0 = fu;

    // *** Distribute Forall over And ***
    fu = ForallAnd::forallAnd(fu);
    if (fu != fu0) {
      uit.replace(fu);
    }
  }
}
void BindingFragments::PreprocessV2::printBindings(){
  auto it = _bindings.domain();
  while (it.hasNext()) {
    auto b = it.next();
    std::cout<< "BindingLiteral: " << b->toString() << std::endl;
    auto value = _bindings.get(b);
    auto mll = value.first;
    std::cout << "\tMostLeftLiteral: " << mll->toString() << std::endl;
    auto stk = value.second;
    std::cout << "\tSATClauseStack: " << std::endl;
    printStack(stk);
  }
  std::cout << std::endl;
}

Formula* BindingFragments::PreprocessV2::_newBinding(Literal *literal){
  if (_bindings.find(literal))
    return new AtomicFormula(literal);
  auto clause = new (1) SATClause(1);
  (*clause)[0] = SATLiteral(literal->functor(), literal->polarity());
  auto stk = new SATClauseStack(1);
  stk->push(clause);

  _bindings.insert(literal, {literal, stk});
  if (literal->functor() > _maxBindingVarNo)
    _maxBindingVarNo = literal->functor();
  return new AtomicFormula(literal);
}


Formula* BindingFragments::PreprocessV2::_newBinding(Formula *formula){
  // Preprocess formula
  FormulaUnit unit(formula, Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));
  FormulaUnit *fu = &unit;

  // *** NNF, Skolemization and Clausification *** // TODO nuove unita' intermedie non vanno cancellate?
  fu = NNF::nnf(fu);
  fu = Flattening::flatten(fu);
  fu = Skolem::skolemise(fu, false);
  fu = Flattening::flatten(fu);

  // if fragment is ConjunctiveBinding, check if this formula is ConjunctiveBinding
  auto classification = (fragment == CONJUNCTIVE_BINDING)? BindingClassifier::classify(fu->getFormula()) : fragment;
  if(classification == CONJUNCTIVE_BINDING) {
    // TODO una alternativa sarebbe cercare, con la stessa tecnica usata per classificare, le subformula one binding e distribuire il forall solo su quelle.
    // esempio ![X, Y]:(p(X) & q(X) & r(X, Y)) viene trasformata in ![X,Y]:p(X) & ![X,Y]:q(X) & ![X,Y]:r(X,Y)
    // ma la trasformazione ottimale sarebbe ![X]: (p(X) & q(X)) & ![X,Y]: r(X,Y)
    // Ovviamente dipende anche da come sono disposti i letterali
    // esempio: ![X, Y]:(p(X) & r(X, Y) & q(X)) non e' banale da trasformare in ![X,Y]:(p(X) & q(X)) & ![X,Y]:r(X,Y)
    fu = ForallAnd::forallAnd(fu);
    ASS_EQ(fu->formula()->connective(), AND)

    FormulaList::Iterator it(fu->formula()->args());
    FormulaList* bindingConj = FormulaList::empty();
    while (it.hasNext()) {
      FormulaUnit subUnit(it.next(), Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));

      auto mll = BindingClassifier::mostLeftLiteral(subUnit.formula());
      auto stk = _clausifyBindingFormula(&subUnit);
      auto binding = _newBindingLiteral();

      _bindings.insert(binding, {mll, stk});
      FormulaList::push(new AtomicFormula(binding), bindingConj);
    }
    return new JunctionFormula(AND, bindingConj);
  }
  else /* if(classification == ONE_BINDING)*/ {
    auto mll = BindingClassifier::mostLeftLiteral(fu->formula());
    auto stk = _clausifyBindingFormula(fu);
    auto binding = _newBindingLiteral();

    _bindings.insert(binding, {mll, stk});
    return new AtomicFormula(binding);
  }
}


SATClauseStack *BindingFragments::PreprocessV2::_clausifyBindingFormula(FormulaUnit* unit){
  ClauseStack stk;
  CNF cnf;
  cnf.clausify(unit, stk);
  // cout<< "Clausify: "; printStack(&stk); cout << endl;

  // *** SAT Clausification ***
  auto satStk = new SATClauseStack(stk.size());
  ClauseStack::Iterator it(stk);
  while (it.hasNext()) {
    auto clause = it.next();
    unsigned int size = clause->size();
    auto satClause = new (clause->size()) SATClause(clause->size());

    for (int i = 0; i < size; i++) {
      const auto lit = (*clause)[i];
      (*satClause)[i] = SATLiteral(lit->functor(), lit->polarity());
      _maxBindingVarNo = std::max(lit->functor(), _maxBindingVarNo);
    }
    satStk->push(satClause);
    clause->destroy(); // TODO potrebbe esplodere qualcosa
  }
  //cout<< "SAT Clausify: "; printStack(satStk); cout << endl;
  return satStk;
}

Literal *BindingFragments::PreprocessV2::_newBindingLiteral(){
  _bindingCount++;
  return Literal::create(
      env.signature->addFreshPredicate(0, "$b"),
      0, true, false, nullptr);
}
Formula *BindingFragments::PreprocessV2::_topBooleanFormula(Formula *formula){
  switch (formula->connective()) {
    case LITERAL: {
      return _newBinding(formula->literal());
    }
    case AND:
    case OR: {
      FormulaList *args = FormulaList::empty();
      FormulaList::Iterator it(formula->args());
      while(it.hasNext()){
        FormulaList::push(_topBooleanFormula(it.next()), args);
      }
      return new JunctionFormula(formula->connective(), args);
    }
    case NOT: {
      return new NegatedFormula(_topBooleanFormula(formula->uarg()));
    }
    case FORALL:
    case EXISTS: {
      return _newBinding(formula);
    }
    case IFF:
    case IMP:
    case XOR:
      return new BinaryFormula(
        formula->connective(),
        _topBooleanFormula(formula->left()),
        _topBooleanFormula(formula->right())
      );
    default:
      std::cout <<  "Connective: " << formula->connective() << std::endl;
      std::cout << "Formula: " << formula->toString() << std::endl;
      ASSERTION_VIOLATION; // TODO
  }
}


