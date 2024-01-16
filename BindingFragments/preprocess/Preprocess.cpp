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
#include "Debug/RuntimeStatistics.hpp"
#include "Kernel/SubformulaIterator.hpp"

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


void BindingFragments::Preprocess::clausify(Problem &prb){
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

  // Clausifica i letterali singoli (che non sono boolean binding) e associa i binding negati alla formula negata
  uit.reset();
  while (uit.hasNext()) {
    SubformulaIterator sit(uit.next()->getFormula());

    while(sit.hasNext()) {
      auto f = sit.next();
      if(f->connective() != LITERAL) continue;

      auto l = f->literal();

      if(isBooleanBinding(l)) {
        if(l->polarity()) continue;
        auto positiveLit = Literal::positiveLiteral(l);
        auto positiveFormula = _bindingFormulas.get(positiveLit);

        _bindingFormulas.insert(l, new NegatedFormula(positiveFormula));
      } else {
        if(_bindingClauses.find(l)) continue;
        _bindingClauses.insert(l, _getSingleLiteralClause(l));
      }
    }
  }

  // clausifica le formule associate ai boolean binding
  while (!_bindingFormulas.isEmpty()) {
    auto booleanBinding = _bindingFormulas.getOneKey();
    Formula * formula;
    _bindingFormulas.pop(booleanBinding, formula);

    // Preprocess formula
    FormulaUnit unit(formula, Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));
    FormulaUnit *fu = &unit;

    // *** NNF, Skolemization and Clausification *** // TODO le nuove unita' intermedie non vanno cancellate?
    fu = NNF::nnf(fu);
    fu = Flattening::flatten(fu);
    fu = Skolem::skolemise(fu, false);
    fu = Flattening::flatten(fu);

    // if fragment is ConjunctiveBinding, check if this formula is ConjunctiveBinding
    Formula* qSubFormula = fu->formula();
    while (qSubFormula->connective() == FORALL) qSubFormula = qSubFormula->qarg();
    // cout<< "Inner classify on: " << qSubFormula->toString() << endl;
    auto classification = (fragment == CONJUNCTIVE_BINDING)? BindingClassifier::innerClassify(qSubFormula).fragment : fragment;
    // cout<< "Classification: " << fragmentToString(classification) << endl;

    auto toDo = FormulaList::empty();

    if(classification == CONJUNCTIVE_BINDING) {
      fu = ForallAnd::forallAnd(fu);
      ASS_EQ(fu->formula()->connective(), AND)

      toDo = fu->formula()->args();
    } else {
      ASS_EQ(classification, ONE_BINDING)
      FormulaList::push(fu->formula(), toDo);
    }

    auto literalBindings = LiteralList::empty();
    while (FormulaList::isNonEmpty(toDo)) {
      auto subFormula = FormulaList::pop(toDo);
      auto mll = BindingClassifier::mostLeftLiteral(subFormula);
      auto newBindingLiteral = _newBindingLiteral(mll);

      FormulaUnit subUnit(subFormula, Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));
      auto stk = _clausifyBindingFormula(&subUnit);

      _bindingClauses.insert(newBindingLiteral, stk);
      _literalToBooleanBindings.insert(newBindingLiteral, booleanBinding);
      LiteralList::push(newBindingLiteral, literalBindings);
    }
    _booleanToLiteralBindings.insert(booleanBinding, literalBindings);
  }
  _bindingFormulas.reset();
}
// void BindingFragments::PreprocessV2::skolemize(){
//   UnitList::DelIterator uit(prb.units());
//   while (uit.hasNext()) {
//     auto unit = uit.next();
//     if (unit->isClause())
//       continue;
//
//     FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
//     FormulaUnit *fu0 = fu;
//
//     // *** Skolemisation ***
//     fu = Skolem::skolemise(fu, false);
//     if(fu != fu0){
//       uit.replace(fu);
//     }
//
//     fu = Flattening::flatten(fu);
//     if(fu != fu0){
//       uit.replace(fu);
//     }
//   }
// }
// void BindingFragments::PreprocessV2::distributeForall()
// {
//   UnitList::DelIterator uit(prb.units());
//   while (uit.hasNext()) {
//     auto unit = uit.next();
//     if (unit->isClause())
//       continue;
//
//     FormulaUnit *fu = static_cast<FormulaUnit *>(unit);
//     FormulaUnit *fu0 = fu;
//
//     // *** Distribute Forall over And ***
//     fu = ForallAnd::forallAnd(fu);
//     if (fu != fu0) {
//       uit.replace(fu);
//     }
//   }
// }

// void BindingFragments::PreprocessV2::clausify(){
//   env.statistics->phase = Statistics::CLAUSIFICATION;
//   // we check if we haven't discovered an empty clause during preprocessing
//   Unit *emptyClause = 0;
//
//   bool modified = false;
//
//   UnitList::DelIterator us(prb.units());
//   CNF cnf;
//   Stack<Clause *> clauses(32);
//   while (us.hasNext()) {
//     Unit *u = us.next();
//     if (env.options->showPreprocessing()) {
//       env.beginOutput();
//       env.out() << "[PP] clausify: " << u->toString() << std::endl;
//       env.endOutput();
//     }
//     if (u->isClause()) {
//       if (static_cast<Clause *>(u)->isEmpty()) {
//         emptyClause = u;
//         break;
//       }
//       continue;
//     }
//     modified = true;
//     cnf.clausify(u, clauses);
//     while (!clauses.isEmpty()) {
//       Unit *u = clauses.pop();
//       if (static_cast<Clause *>(u)->isEmpty()) {
//         emptyClause = u;
//         goto fin;
//       }
//       us.insert(u);
//     }
//     us.del();
//   }
// fin:
//   if (emptyClause) {
//     UnitList::destroy(prb.units());
//     prb.units() = 0;
//     UnitList::push(emptyClause, prb.units());
//   }
//   if (modified) {
//     prb.invalidateProperty();
//   }
//   prb.reportFormulasEliminated();
// }
void BindingFragments::PreprocessV2::satClausify(){
  env.statistics->phase = Statistics::CLAUSIFICATION;
  // we check if we haven't discovered an empty clause during preprocessing
  Unit *emptyClause = 0;

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
      clauses.push(u->asClause());
    } else {
      cnf.clausify(u, clauses);
    }

    while (!clauses.isEmpty()) {
      auto clause = clauses.pop();
      if (clause->isEmpty()) {
        emptyClause = u;
        goto fin;
      }
      _addLiterals(clause);
      auto satClause = _sat2fo.toSAT(clause);
      if(satClause == nullptr) continue;
      _clauses.push(satClause);
      clause->destroy();
    }
    us.del();
  }
  fin:
    UnitList::destroy(prb.units());
    prb.units() = 0;
    if (emptyClause) {
      UnitList::push(emptyClause, prb.units());
      emptyClauseFound = true;
      _clauses.reset();
    }


}

void BindingFragments::PreprocessV2::printSatClauses()
{
  if(!env.options->showPreprocessing()) return;

  env.beginOutput();
  SATClauseStack::Iterator it(_clauses);
  while (it.hasNext()) {
    env.out() << it.next()->toString() << endl;
  }
  env.endOutput();
}

// SATClauseStack *BindingFragments::PreprocessV2::getSatClauses(Literal *literal)
// {
//   if (!_isBinding(literal)) {
//     auto stk = *(_literalMap.findPtr(literal));
//     if (stk != nullptr)
//       return stk;
//
//     stk = _getSingleLiteralClause(literal);
//     _literalMap.insert(literal, stk);
//     return stk;
//   }
//   else {
//     return _bindings.get(literal->functor() - 1).bindingClauses;
//   }
// }
Literal *BindingFragments::PreprocessV2::_newBooleanBinding()
{
  _bindingCount++;
  RSTAT_CTR_INC("bindings");
  unsigned booleanBindingFunctor = env.signature->addFreshPredicate(0, "$b");

  if (_minBooleanBindingFunctor == -1)
    _minBooleanBindingFunctor = booleanBindingFunctor;
  if (booleanBindingFunctor > _maxBooleanBindingFunctor)
    _maxBooleanBindingFunctor = booleanBindingFunctor;

  return Literal::create(booleanBindingFunctor, 0, true, false, nullptr);
}
Literal *BindingFragments::PreprocessV2::_newBindingLiteral(Literal *literal)
{
  _bindingCount++;
  const auto arity = literal->arity();

  unsigned bindingFunctor = env.signature->addFreshPredicate(arity, "$b");

  if (_minLiteralBindingFunctor == -1)
    _minLiteralBindingFunctor = bindingFunctor;
  if (bindingFunctor > _maxLiteralBindingFunctor)
    _maxLiteralBindingFunctor = bindingFunctor;

  auto arg = literal->args();
  DArray<TermList> term(arity);
  for (unsigned int i = 0; i < arity; i++) {
    term[i] = arg[0];
    arg = arg->next();
  }

  return Literal::create(bindingFunctor, arity, true, false, term.array());
}
Formula *BindingFragments::PreprocessV2::_addBindingFormula(Formula *formula)
{
  auto booleanBinding = _newBooleanBinding();
  _bindingFormulas.insert(booleanBinding, formula);
  return new AtomicFormula(booleanBinding);
}

void BindingFragments::PreprocessV2::_addLiterals(Clause *clause){
  auto it = clause->getLiteralIterator();
  while (it.hasNext()) {
    auto lit = it.next();
    LiteralList::push(lit, _literals);
    _maxBindingVarNo = std::max(lit->functor(), _maxBindingVarNo);
  }
}


SATClauseStack* BindingFragments::PreprocessV2::_getSingleLiteralClause(Literal* literal){
  auto clause = new (1) SATClause(1);
  (*clause)[0] = SATLiteral(literal->functor(), literal->polarity());
  auto stk = new SATClauseStack(1);
  stk->push(clause);
  return stk;
}

// Formula * BindingFragments::PreprocessV2::_newBinding(Literal *literal){
//   ASS(literal->ground());
//
//   if (_literalMap.find(literal))
//     return new AtomicFormula(literal);
//
//   auto stk = _getSingleLiteralClause(literal);
//
//   _literalMap.insert(literal, stk);
//   if (literal->functor() > _maxBindingVarNo)
//     _maxBindingVarNo = literal->functor();
//   return new AtomicFormula(literal);
// }


// Formula* BindingFragments::PreprocessV2::_newBinding(Formula *formula){
//   // Preprocess formula
//   FormulaUnit unit(formula, Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));
//   FormulaUnit *fu = &unit;
//
//   // *** NNF, Skolemization and Clausification *** // TODO le nuove unita' intermedie non vanno cancellate?
//   fu = NNF::nnf(fu);
//   fu = Flattening::flatten(fu);
//   fu = Skolem::skolemise(fu, false);
//   fu = Flattening::flatten(fu);
//
//   // if fragment is ConjunctiveBinding, check if this formula is ConjunctiveBinding
//   auto classification = (fragment == CONJUNCTIVE_BINDING)? BindingClassifier::classify(fu->getFormula()) : fragment;
//   if(classification == CONJUNCTIVE_BINDING) {
//     // TODO una alternativa (a forallAnd) sarebbe cercare, con la stessa tecnica usata per classificare, le subformula one binding e distribuire il forall solo su quelle.
//     // esempio ![X, Y]:(p(X) & q(X) & r(X, Y)) viene trasformata in ![X,Y]:p(X) & ![X,Y]:q(X) & ![X,Y]:r(X,Y)
//     // ma la trasformazione ottimale sarebbe ![X]: (p(X) & q(X)) & ![X,Y]: r(X,Y)
//     // Ovviamente dipende anche da come sono disposti i letterali
//     // esempio: ![X, Y]:(p(X) & r(X, Y) & q(X)) non e' banale da trasformare in ![X,Y]:(p(X) & q(X)) & ![X,Y]:r(X,Y)
//     fu = ForallAnd::forallAnd(fu);
//     ASS_EQ(fu->formula()->connective(), AND)
//
//     FormulaList::Iterator it(fu->formula()->args());
//     FormulaList* bindingConj = FormulaList::empty();
//     while (it.hasNext()) {
//       FormulaUnit subUnit(it.next(), Inference(NonspecificInference0(UnitInputType::AXIOM, InferenceRule::INPUT)));
//
//       auto mll = BindingClassifier::mostLeftLiteral(subUnit.formula());
//       auto stk = _clausifyBindingFormula(&subUnit);
//       auto [booleanBinding, binding] = _newBindingLiteral(mll);
//
//       _bindings.insert(booleanBinding->functor(), {booleanBinding, binding, stk});
//       FormulaList::push(new AtomicFormula(booleanBinding), bindingConj);
//     }
//     return new JunctionFormula(AND, bindingConj);
//   }
//   else /* if(classification == ONE_BINDING)*/ {
//     auto mll = BindingClassifier::mostLeftLiteral(fu->formula());
//     auto stk = _clausifyBindingFormula(fu);
//     auto [booleanBinding, binding] = _newBindingLiteral(mll);
//
//     _bindings.insert(booleanBinding->functor(), {booleanBinding,binding, stk});
//     return new AtomicFormula(booleanBinding);
//   }
// }
//

SATClauseStack *BindingFragments::PreprocessV2::_clausifyBindingFormula(FormulaUnit* unit){
  ClauseStack stk;
  CNF cnf;
  cnf.clausify(unit, stk);
  // cout<< "Clausify: "; printStack(&stk); cout << endl;
  // cout<< "Original formula: " << unit->toString() << endl;
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
      //_maxBindingVarNo = std::max(lit->functor(), _maxBindingVarNo);
    }
    satClause = SATClause::removeDuplicateLiterals(satClause);
    if(satClause != nullptr) // null === $true
      satStk->push(satClause);
    clause->destroy(); // TODO potrebbe esplodere qualcosa
  }
  //cout<< "SAT Clausify: "; printStack(satStk); cout << endl;
  return satStk;
}

// pair<Literal*, Literal*> BindingFragments::PreprocessV2::_newBindingLiteral(Literal* mll){
//   _bindingCount++;
//   const auto arity = mll->arity();
//
//   unsigned booleanBindingFunctor = env.signature->addFreshPredicate(0, "$b");
//   unsigned bindingFunctor = env.signature->addFreshPredicate(arity, "$b");
//
//   if(_minBindingFunctor == -1) _minBindingFunctor = booleanBindingFunctor;
//   else if(booleanBindingFunctor < _minBindingFunctor) _minBindingFunctor = booleanBindingFunctor;
//   if(bindingFunctor > _maxBindingFunctor) _maxBindingFunctor = bindingFunctor;
//
//   ASS_EQ((booleanBindingFunctor+1), bindingFunctor);
//
//   auto arg = mll->args();
//   DArray<TermList> term(arity);
//   for(unsigned int i = 0; i < arity; i++){
//     term[i] = arg[0];
//     arg = arg->next();
//   }
//
//   auto booleanBindingLiteral = Literal::create(booleanBindingFunctor, 0, true, false, nullptr);
//   auto bindingLiteral = Literal::create(bindingFunctor, arity, true, false, term.array());
//
//   return {booleanBindingLiteral, bindingLiteral};
// }


Formula *BindingFragments::PreprocessV2::_topBooleanFormula(Formula *formula){
  switch (formula->connective()) {
    case LITERAL: {
      return new AtomicFormula(formula->literal());
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
      return _addBindingFormula(formula);
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


