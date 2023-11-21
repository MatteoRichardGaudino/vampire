//
// Created by Matte on 25/09/2023.
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


BindingFragments::OneBindingSat::OneBindingSat(Problem &prb, const Options &opt) : _problem(prb), _options(opt) {

  _satClauses = new ClauseStack(16);
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
  setupSolver();
}

bool BindingFragments::OneBindingSat::solve(){
  cout<< "-------------------- Solve --------------------------" << endl;
  while (_solver->solve() == SAT::SATSolver::SATISFIABLE){
    printAssignment();

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
      return a->arity() < b->arity();
    });


    cout<< "Implicants ordered by arity: " << endl;
    for(int i = 0; i < l; i++){
      cout<< implicants[i]->toString() << " ";
    }
    cout<< endl;

//    cout<< "------ Building substitution trees by arity groups ------------------" << endl;
    ArityGroupIterator groupIt(implicants, l);

    while (RESULT && groupIt.hasNext()){
      auto group = groupIt.next();
      MaximalUnifiableSubsets mus(group, [&](auto map){
        auto solver = _newSatSolver();
        solver->ensureVarCount(_maxBindingVarNo);
        cout<< "MUS: ";
        for(auto & _ : map){
          if(_.second > 0){
            cout<< _.first->toString() << " ";
            SATClauseStack* stk = _bindings[_.first];
            SATClauseStack::Iterator it(*stk);
            while (it.hasNext()) solver->addClause(it.next());
          }
        }
        auto status = solver->solve();
        cout<< "::::: " << status << endl;
        delete solver;
        return status == SATSolver::SATISFIABLE;
      });

      while (group.hasNext()){
        auto lit = group.next();
        //cout<< " -------------------- Maximal unifiable subsets for " << lit->toString() << " ------------------" << endl;
        bool res = mus.mus(lit);
        if(!res){
          RESULT = false;
          break;
        }
      }
    }

    if(RESULT){
      cout<<  "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& SAT &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << endl;
      return true;
    }

    cout<< "||||||||||||||||||||||||||||||||| blocking model |||||||||||||||||||||||||||||||||" << endl;
    blockModel();
  }

  cout<<  "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ UNSAT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
  return false;
}


void BindingFragments::OneBindingSat::generateSATClauses(Unit* unit){
  auto satFormula = generateSatFormula(unit->getFormula());
  Unit* satUnit = new FormulaUnit(satFormula, unit->inference());

//  cout<< "Unit: " << unit->toString() << endl;
//  cout<< "SAT Unit: " << satUnit->toString() << endl << endl;
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

      _addBinding(newLit, formula);

      return new AtomicFormula(newLit);
    }
    default:
      ASSERTION_VIOLATION; // TODO
  }
}
void BindingFragments::OneBindingSat::setupSolver(){
  _solver = _newSatSolver();
  ClauseStack::Iterator cIt(*_satClauses);
  SATClauseStack clauses;
  int i = 0;
  while (cIt.hasNext()){
    auto clause = cIt.next();
    auto satClause = _sat2fo.toSAT(clause);
    if(satClause == nullptr){
      cout<< "Skipping: " << clause->toString() << endl;
      continue;
    }
//    cout<< "Clause: " << clause->toString() << endl;
    cout<< "SATClause " << ++i << ": " << satClause->toString() << endl;
    clauses.push(satClause);
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
         << ")" << " ";
  }
  cout << endl;
}
void BindingFragments::OneBindingSat::blockModel(){ // TODO non va bene cosi
  SATLiteralStack blockingClause;
  unsigned int max = _sat2fo.maxSATVar();
  for(unsigned int j = 1; j <= max; j++){
    if(_solver->getAssignment(j) == SAT::SATSolver::TRUE){
      blockingClause.push(SATLiteral(j, false));
    }
  }
  auto clause = SATClause::fromStack(blockingClause);
  cout<< "Blocking Clause: " << clause->toString() << endl;
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
    ArityGroupIterator::GroupIterator group, std::function<bool(std::map<Literal*, int>)> fun):
          _group(group), _tree(false, false), _fun(fun){

  _buildTree();

  while (_group.hasNext()){
    auto lit = _group.next();
    _s[lit] = 0;
  }
  _group.reset();
}
bool BindingFragments::MaximalUnifiableSubsets::mus(Literal *literal){
  _s[literal] = 1;
  bool res = _mus(literal, 2);
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
        _s[u] = depth;
        //cout<< "push: " << u->toString() << endl;
        isMax = false;
        if(!_mus(res.subst->applyToQuery(literal), depth+1)) return false;
        _s[u] = -depth;
        //cout<< "pop&block: " << u->toString() << endl;
      }
  }

  if(isMax && depth > 2){ // TODO depth > 2. Se depth è 1 significa che c'è un solo elemento nella soluzione ed è per forza soddisfacibile
      if(!_fun(_s)){
        return false;
      }
  }

  for(auto & _ : _s){
      if(_.second == -depth){
        _s[_.first] = 0;
  //      cout<< "Unblock: " << _.first->toString() << endl;
      }
  }
  return true;
}
void BindingFragments::MaximalUnifiableSubsets::_buildTree(){
  while (_group.hasNext()){
    auto lit = _group.next();
    _tree.handle(lit, SubstitutionTree::LeafData(nullptr, lit), true);
  }
  _group.reset();
}


void BindingFragments::OneBindingSat::_addBinding(Literal *literal){
  if (_bindings.find(literal) != _bindings.end()) return;

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

    for(unsigned int i = 0; i < size; i++){
      auto lit = (*clause)[i];
      (*satClause)[i] = SATLiteral(lit->functor(), lit->polarity());
      if(literal->functor() > _maxBindingVarNo) _maxBindingVarNo = literal->functor();
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
