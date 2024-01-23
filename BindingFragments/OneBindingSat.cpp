//
// Created by Matte on 25/09/2023.


// ./Problems/SYO/SYO580+1.p
// ./Problems/SYO/SYO579+1.p
//

#include "OneBindingSat.h"
#include "preprocess/BindingClassifier.h"
#include "Shell/CNF.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Indexing/TermSharing.hpp"
#include <iostream>

#include "SAT/MinisatInterfacing.hpp"

#include "Indexing/TermSubstitutionTree.hpp"

using namespace std;
using namespace Kernel;
using namespace Indexing;


BindingFragments::OneBindingSat::OneBindingSat(Problem &prb, const Options &opt) : _options(opt) {
  _showProof = _options.proof() == Options::Proof::ON;
  if(_showProof) env.beginOutput();
  TIME_TRACE(TimeTrace::ONE_BINDING_ALGORITHM_CONFIG)
  _satClauses = new ClauseStack(16); // TODO allocazione statica
  _satLiterals = LiteralList::empty();

  auto units = prb.units();
  UnitList::Iterator it(units);
  while(it.hasNext()) generateSATClauses(it.next());

  ClauseStack::Iterator cIt(*_satClauses);
  while (cIt.hasNext()){
    auto lIt = cIt.next()->getLiteralIterator();
    while (lIt.hasNext()) {
      LiteralList::push(lIt.next(), _satLiterals);
    }
  }

  if(_showProof) {
    cout<< "------------------ FO SAT Clauses ----------------------------" << endl;
    cout << "Clauses size: " << _satClauses->size() << endl;

    ClauseStack::Iterator cIt1(*_satClauses);
    int i = 0;
    while(cIt1.hasNext()){
      cout << "Clause " << ++i << ": "<< cIt1.next()->toString() << endl;
    }
    cout<< "------------------------ Bindings ------------------------------" << endl;
    _printBindings();

    cout<< "------------------ Setup Sat solver ----------------------------" << endl;
  }

  setupSolver();
}

bool BindingFragments::OneBindingSat::solve(){
  TIME_TRACE(TimeTrace::ONE_BINDING_ALGORITHM)
  if (_showProof) cout<< "-------------------- Solve --------------------------" << endl;
  while (_solver->solve() == SAT::SATSolver::SATISFIABLE){
    if(_showProof) printAssignment();
    if(_bindingCount == 0) { // problem is groud
      if(_showProof) {
        cout<< "Problem is Ground" << endl;
        cout<<  "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& SAT &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << endl;
      }
      return true;
    }

    bool RESULT = true;



    Array<Literal*> implicants;
    int l = 0;

    LiteralList::Iterator lIt(_satLiterals);
    while (lIt.hasNext()) {
      Literal *lit = lIt.next();
      auto satLit = _sat2fo.toSAT(lit);
      if (_solver->trueInAssignment(satLit)) {
        implicants[l++] = lit;
      }
    }

    sort(implicants.begin(), implicants.begin()+l, [](Literal* a, Literal* b) {
      const auto a1 = a->arity(), b1 = b->arity();
      const auto g1 = a->ground(), g2 = b->ground();

      if(a1 < b1) return true;
      if(a1 > b1) return false;
      if(g1 && !g2) return true;
      if(g2 && !g1) return false;
      return a->functor() < b->functor();

      // if(a1 == b1) {
      //   if(g1) return true;
      //   return false < true;
      // } else return a1 < b1;
    });

    if (_showProof) {
      cout<< "Implicants ordered by arity: " << endl;
      for(int i = 0; i < l; i++){
        cout<< implicants[i]->toString() << " ";
      }
      cout<< endl;
    }


    ArityGroupIterator groupIt(implicants, l);

    while (RESULT && groupIt.hasNext()){
      auto group = groupIt.next();
      MaximalUnifiableSubsets mus(group, [&](LiteralStack solution){
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET_SAT_SOLVING)
        auto solver = _newSatSolver();
        solver->ensureVarCount(_maxBindingVarNo);
        if(_showProof) cout<< "MUS: ";
        LiteralStack::Iterator solIt(solution);
        while (solIt.hasNext()) {
          auto lit = solIt.next();
          if(_showProof) cout<< lit->toString() << " ";
          SATClauseStack* stk = _bindings[lit];
          SATClauseStack::Iterator it(*stk);
          while (it.hasNext()) solver->addClause(it.next());
        }
        auto status = solver->solve();
        if(_showProof) cout<< "::::: " << status << endl;
        delete solver;
        return status == SATSolver::SATISFIABLE;
      });

      while (group.hasNext()){
        auto lit = group.next();
        TIME_TRACE(TimeTrace::MAXIMAL_UNIFIABLE_SUBSET)
        if(_showProof) {
          cout<< "---------- mus for " << lit->toString() << " ------------" << endl;
        }
        bool res = mus.musV2(lit);
        if(!res){
          RESULT = false;
          break;
        }
      }
    }

    if(RESULT){
      if(_showProof) cout<<  "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& SAT &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << endl;
      return true;
    }

    if(_showProof) cout<< "||||||||||||||||||||||||||||||||| blocking model |||||||||||||||||||||||||||||||||" << endl;
    blockModel(implicants, l);
  }

  if(_showProof) cout<<  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ UNSAT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
  return false;
}


void BindingFragments::OneBindingSat::generateSATClauses(Unit* unit){
  auto satFormula = generateSatFormula(unit->getFormula());

  Unit* satUnit = new FormulaUnit(satFormula, unit->inference());

  if(_showProof) {
    cout<< "Unit: " << unit->toString() << endl;
    cout<< "SAT Unit: " << satUnit->toString() << endl << endl;
  }
  CNF cnf;
  cnf.clausify(satUnit, *_satClauses);
}

Formula *BindingFragments::OneBindingSat::generateSatFormula(Formula *formula){
  switch (formula->connective()) {
    case LITERAL: {
      _addBinding(formula->literal());
      return new AtomicFormula(formula->literal());
    }
    case AND:
    case OR: {
      FormulaList *args = FormulaList::empty();
      FormulaList::Iterator it(formula->args());
      while(it.hasNext()){
        FormulaList::push(generateSatFormula(it.next()), args);
      }
      return new JunctionFormula(formula->connective(), args);
    }
    case NOT: {
      return new NegatedFormula(generateSatFormula(formula->uarg()));
    }
    case FORALL:
    case EXISTS: {
      while(formula->connective() == FORALL || formula->connective() == EXISTS){
        formula = formula->qarg();
      }
      auto mll = BindingClassifier::mostLeftLiteral(formula);
      auto arg = mll->args();
      auto arity = mll->arity();
      DArray<TermList> term(arity);
      for(unsigned int i = 0; i < arity; i++){
        term[i] = arg[0];
        arg = arg->next();
      }

      unsigned freshPredicate = env.signature->addFreshPredicate(arity, "$b");
      auto newLit = Literal::create(freshPredicate, arity, true, false, term.array());

      _bindingCount++;
      _addBinding(newLit, formula);

      return new AtomicFormula(newLit);
    }
    default:
      cout <<  "Connective: " << formula->connective() << endl;
      cout << "Formula: " << formula->toString() << endl;
      ASSERTION_VIOLATION; // TODO
  }
}

void BindingFragments::OneBindingSat::setupSolver(){
  _solver = _newSatSolver();
  //ClauseStack::Iterator cIt(*_satClauses);
  SATClauseStack clauses;
  int i = 0;
  while (!_satClauses->isEmpty()){
    auto clause = _satClauses->pop();
    auto satClause = _sat2fo.toSAT(clause); // TODO e la clausola vuota?
    if(satClause == nullptr){
      if(_showProof) cout<< "Skipping: " << clause->toString() << endl;
      continue;
    }
//    cout<< "Clause: " << clause->toString() << endl;
    if(_showProof) cout<< "SATClause " << ++i << ": " << satClause->toString() << endl;
    clauses.push(satClause);
    clause->destroy();
  }

  _solver->ensureVarCount(_sat2fo.maxSATVar());

  SATClauseStack::Iterator it(clauses);
        while (it.hasNext()) {
    auto clause = it.next();
    _solver->addClause(clause);
  }
}

void BindingFragments::OneBindingSat::printAssignment(){
  cout<< "Assignment[(satVar, literal, value)]: ";
  unsigned int max = _sat2fo.maxSATVar();
  for(unsigned int j = 1; j <= max; j++){
    cout << "("
         << j << ", "
         << _sat2fo.toFO(SATLiteral(j, true))->toString() <<  ", "
         << (_solver->getAssignment(j) == SATSolver::TRUE ? "true" : "false")
         << ")" << endl;
  }
  cout << endl;
}
// void BindingFragments::OneBindingSat::blockModel()
// {
//   SATLiteralStack blockingClause;
//   unsigned int max = _sat2fo.maxSATVar();
//   for (unsigned int j = 1; j <= max; j++) {
//     if (_solver->getAssignment(j) == SAT::SATSolver::TRUE) {
//       blockingClause.push(SATLiteral(j, false));
//     }
//   }
//   auto clause = SATClause::fromStack(blockingClause);
//   if (_showProof)
//     cout << "Blocking Clause: " << clause->toString() << endl;
//   _solver->addClause(clause);
// }

void BindingFragments::OneBindingSat::blockModel(Array<Literal*>& implicants, int size){
  SATLiteralStack blockingClause;

  for(int i = 0; i < size; ++i) {
    auto impl = implicants[i];
    auto satImpl = _sat2fo.toSAT(impl);
    blockingClause.push(satImpl.opposite());
  }

  auto clause = SATClause::fromStack(blockingClause);
  if (_showProof)
    cout << "Blocking Clause: " << clause->toString() << endl;
  _solver->addClause(clause);
}

BindingFragments::ArityGroupIterator::GroupIterator BindingFragments::ArityGroupIterator::next(){
  _start = _end;
  _arity = _literals[_end]->arity();
  while ((_end < _literalSize) && (_literals[_end]->arity() == _arity)){
    _end++;
  }
  return GroupIterator(_literals, _start, _end);
}



/// --------------------------------- Maximal Unifiable subsets ---------------------------------


BindingFragments::MaximalUnifiableSubsets::MaximalUnifiableSubsets(
    ArityGroupIterator::GroupIterator group, std::function<bool(LiteralStack)> fun) : _group(group), _tree(false, false), _fun(fun), _solution(16)
{

  _buildTree();

  while (_group.hasNext()) {
    auto lit = _group.next();
    _s[lit] = 0;
  }
  _group.reset();
}


bool BindingFragments::MaximalUnifiableSubsets::_groundLiteralMus(Literal *literal){
  if(_s[literal] != 0) return true;
  auto uIt = _tree.iterator<SubstitutionTree::UnificationsIterator>(literal, false, false);
  while (uIt.hasNext()) {
    auto res = uIt.next();
    auto u = res.data->literal;
    if(_s[u] == 0) {
      if(u->ground()) _s[u] = -1;
      _solution.push(u);
    }
  }
  bool res = _fun(_solution);
  if(res) _solution.reset();
  return res;
}
bool BindingFragments::MaximalUnifiableSubsets::mus(Literal *literal){
  if(_s[literal] != 0) return true;
  if (literal->ground()) {
    return _groundLiteralMus(literal);
  }

  _s[literal] = 1;
  _solution.push(literal);
  bool res = _mus(literal, 2);
  _solution.pop();
  _s[literal] = -1;
  return res;
}
bool BindingFragments::MaximalUnifiableSubsets::_mus(Literal *literal, int depth){
  bool isMax = true;
  auto uIt = _tree.iterator<SubstitutionTree::UnificationsIterator>(literal, true, false);

  while (uIt.hasNext()){
      auto res = uIt.next();
      auto u = res.data->literal;
      if(_s[u] == 0){
        isMax = false;
        _s[u] = depth;
        //cout<< "push: " << u->toString() << endl;
        _solution.push(u);
        unsigned int pop = 1;

        auto sub = res.subst->applyToQuery(literal);
        while (sub == literal && uIt.hasNext()) {
          res = uIt.next();
          u = res.data->literal;
          if(_s[u] == 0){
            _s[u] = depth;
            _solution.push(u);
            pop++;
            sub = res.subst->applyToQuery(literal);
          }
        }

        if(!_mus(sub, depth+1)) return false;

        //while (pop-- > 0) _s[_solution.pop()] = depth;
        while (pop-- > 0) _solution.pop();

        //cout<< "pop&block: " << u->toString() << endl;
      }
  }

  if(isMax){ // && (depth == 1 => _sol.top() != binding)
      if(!_fun(_solution)){
        return false;
      }
  }

  for(auto & _ : _s){
      if(_.second == depth){
        _s[_.first] = 0;
  //      cout<< "Unblock: " << _.first->toString() << endl;
      }
  }
  return true;
}

bool BindingFragments::MaximalUnifiableSubsets::musV2(Literal* literal){
  if(_s[literal] != 0)
    return true;
  if (literal->ground()) {
    return _groundLiteralMus(literal);
  }

  _s[literal] = 1;
  LiteralList* l = LiteralList::empty();
  bool res = _musV2(literal, l);
  _s[literal] = -1;
  delete l;
  return res;
}
bool BindingFragments::MaximalUnifiableSubsets::_musV2(Literal* literal, LiteralList*& fToFree){
  bool isMax = true;
  auto uIt = _tree.iterator<SubstitutionTree::UnificationsIterator>(literal, true, false);
  auto* toFree = LiteralList::empty();

  while (uIt.hasNext()){
    auto res = uIt.next();
    auto u = res.data->literal;

    if(_s[u] == 0){
      _s[u] = 1;

      auto l1 = res.subst->applyToQuery(literal);
      if(l1 == literal) {
        auto u1 = res.subst->applyToResult(u);
        LiteralList::push(u, (u1 == u) ? fToFree : toFree);
      } else {
        isMax = false;
        if(!_musV2(l1, toFree)) return false;
        _s[u] = -1;
        LiteralList::push(u, toFree);
      }
    }
  }

  if(isMax){
    // Build solution
    for(auto _ : _s){
      if(_.second > 0)
        _solution.push(_.first);
    }
    // ------
    if(!_fun(_solution)){
      delete toFree;
      return false;
    }
    _solution.reset();
  }

  while (LiteralList::isNonEmpty(toFree)) _s[LiteralList::pop(toFree)] = 0;

  delete toFree;
  return true;
}


void BindingFragments::MaximalUnifiableSubsets::_buildSolution()
{
  for (auto &_ : _s) {
    if (_.second > 0)
      _solution.push(_.first);
  }
}
void BindingFragments::MaximalUnifiableSubsets::_sReplace(int a, int b)
{
  for (auto &_ : _s) {
    if (_.second == a) {
      _s[_.first] = b;
    }
  }
}

bool BindingFragments::GroundArityAndTermComparator(Literal *&l1, Literal *&l2)
{
  ASS(l1->shared());
  ASS(l2->shared());

  if(l1 == l2) return false;

  if (l1->arity() != l2->arity())
    return l1->arity() < l2->arity();
  if (l1->ground() && !l2->ground())
    return true;
  if (!l1->ground() && l2->ground())
    return false;

  // ASS_EQ(l1->arity(), l2->arity())
  // ASS((l1->ground() && l2->ground()) != (!l1->ground() && !l2->ground()))

  SubtermIterator sit1(l1);
  SubtermIterator sit2(l2);
  while (sit1.hasNext()) {
    ALWAYS(sit2.hasNext());
    TermList st1 = sit1.next();
    TermList st2 = sit2.next();
    if (st1.isTerm()) {
      if (st2.isTerm()) {
        unsigned f1 = st1.term()->functor();
        unsigned f2 = st2.term()->functor();
        if (f1 != f2) {
          return f1 < f2;
        }
      }
      else {
        return true;
      }
    }
    else {
      if (st2.isTerm()) {
        return true;
      }
      else {
        if (st1.var() != st2.var()) {
          return st1.var() < st2.var();
        }
      }
    }
  }

  return false;
}

bool BindingFragments::GroundAndArityComparator(Literal *&l1, Literal *&l2){
  ASS(l1->shared());
  ASS(l2->shared());

  if(l1 == l2) return false;

  if (l1->arity() != l2->arity())
    return l1->arity() < l2->arity();
  if (l1->ground() && !l2->ground())
    return true;
  if (!l1->ground() && l2->ground())
    return false;

  return l1->functor() < l2->functor();
}


void BindingFragments::MaximalUnifiableSubsets::_buildTree(){
  while (_group.hasNext()){
    auto lit = _group.next();
    _tree.handle(lit, SubstitutionTree::LeafData(nullptr, lit), true);
  }
  _group.reset();
}





void BindingFragments::OneBindingSat::_addBinding(Literal *literal){
  auto clause = new (1) SATClause(1);
  (*clause)[0] = SATLiteral(literal->functor(), literal->polarity());
  auto stk = new SATClauseStack(1);
  stk->push(clause);

  _bindings[literal] = stk;
  if(literal->functor() > _maxBindingVarNo) _maxBindingVarNo = literal->functor();
}

void BindingFragments::OneBindingSat::_addBinding(Literal* literal, Formula *formula){
  // TODO magari andrebbe messa una regola non scelta a caso
  FormulaUnit unit(formula, Inference(FromInput(UnitInputType::AXIOM)));
  ClauseStack stk;
  CNF cnf;
  cnf.clausify(&unit, stk);
  auto satStk = new SATClauseStack(stk.size());

  ClauseStack::Iterator it(stk);
  while (it.hasNext()){
    auto clause = it.next();
    unsigned int size = clause->size();
    auto satClause = new(clause->size()) SATClause(clause->size());

    for(int i = 0; i < size; i++){
      const auto lit = (*clause)[i];
      (*satClause)[i] = SATLiteral(lit->functor(), lit->polarity());
      if(lit->functor() > _maxBindingVarNo) _maxBindingVarNo = lit->functor();
    }
    satStk->push(satClause);
  }
  _bindings[literal] = satStk;
}


PrimitiveProofRecordingSATSolver *BindingFragments::OneBindingSat::_newSatSolver(){
  switch(_options.satSolver()){
    case Options::SatSolver::MINISAT:
      return new MinisatInterfacing(_options, true);
      break;
#if VZ3 // TODO
    case Options::SatSolver::Z3:
    { BYPASSING_ALLOCATOR
      _solverIsSMT = true;
      _solver = new Z3Interfacing(_parent.getOptions(),_parent.satNaming(), /* unsat core */ false, _parent.getOptions().exportAvatarProblem());
      if(_parent.getOptions().satFallbackForSMT()){
        // TODO make fallback minimizing?
        SATSolver* fallback = new MinisatInterfacing(_parent.getOptions(),true);
        return = new FallbackSolverWrapper(_solver.release(),fallback);
      }
    }
    break;
#endif
    default:
      ASSERTION_VIOLATION_REP(_options.satSolver());
  }

}
void BindingFragments::OneBindingSat::_printBindings(){
    for(auto & _ : _bindings){
      cout<< _.first->toString() << ": " << endl;
      SATClauseStack::Iterator it(*_.second);
      while (it.hasNext()){
        cout<< "\t" << it.next()->toString() << endl;
      }
    }
}
