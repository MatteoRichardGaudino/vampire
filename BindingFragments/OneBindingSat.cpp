//
// Created by Matte on 25/09/2023.
//

#include "OneBindingSat.h"
#include "BindingClassifier.h"
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
  switch(opt.satSolver()){
    case Options::SatSolver::MINISAT:
      _solver = new MinisatInterfacing(opt, true);
      break;
#if VZ3 // TODO
    case Options::SatSolver::Z3:
    { BYPASSING_ALLOCATOR
      _solverIsSMT = true;
      _solver = new Z3Interfacing(_parent.getOptions(),_parent.satNaming(), /* unsat core */ false, _parent.getOptions().exportAvatarProblem());
      if(_parent.getOptions().satFallbackForSMT()){
        // TODO make fallback minimizing?
        SATSolver* fallback = new MinisatInterfacing(_parent.getOptions(),true);
        _solver = new FallbackSolverWrapper(_solver.release(),fallback);
      }
    }
    break;
#endif
    default:
      ASSERTION_VIOLATION_REP(opt.satSolver());
  }


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

  cout<< "------------------ Setup Sat solver ----------------------------" << endl;
  setupSolver();
  cout<< "----------------------------------------------" << endl;
}

bool BindingFragments::OneBindingSat::solve(){
  cout<< "-------------------- Solve --------------------------" << endl;
  while (_solver->solve() == SAT::SATSolver::SATISFIABLE){
    printAssignment();

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

    while (groupIt.hasNext()){
      auto group = groupIt.next();
      auto tree = _buildSubstitutionTree(implicants, group);
//      cout<< "Substitution tree for arity " << groupIt.arity() << endl;
//      cout<< multiline(tree) << endl;

      MaximalUnifiableSubsets mus(group, tree);

      while (group.hasNext()){
        auto lit = group.next();
        cout<< " -------------------- Maximal unifiable subsets for " << lit->toString() << " ------------------" << endl;
        mus.mus(lit);
        cout<< " ------------------------------------------------------------------" << endl;
      }
    }

    cout<< "------ blocking model  -----------------------" << endl;
    blockModel();
  }

  cout<< "----------------------------------------------" << endl;
  return false;
}


void BindingFragments::OneBindingSat::generateSATClauses(Unit* unit){
  auto satFormula = generateSatFormula(unit->getFormula());
  Unit* satUnit = new FormulaUnit(satFormula, unit->inference());

  cout<< "Unit: " << unit->toString() << endl;
  cout<< "SAT Unit: " << satUnit->toString() << endl << endl;
  CNF cnf;
  cnf.clausify(satUnit, *_satClauses);
}

Formula *BindingFragments::OneBindingSat::generateSatFormula(Formula *formula){
  switch (formula->connective()) {
    case LITERAL:
      return new AtomicFormula(formula->literal());
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
      cout<< "Most left literal: " << mll->toString() << endl;
      auto arg = mll->args();
      auto arity = mll->arity();
      DArray<TermList> term(arity);
      for(unsigned int i = 0; i < arity; i++){
        term[i] = arg[0];
        arg = arg->next();
      }

      unsigned freshPredicate = env.signature->addFreshPredicate(arity, "$b");
      //vstring name =  reinterpret_cast<basic_string<char, std::char_traits<char>, Lib::STLAllocator<char>> &&>(to_string(_bindingCount++).append("_Binding"));
      //return new NamedFormula(name);
      //Literal lit(freshFunctor, size, false, false);
      auto newLit = Literal::create(freshPredicate, arity, true, false, term.array());
      return new AtomicFormula(newLit);
    }
    default:
      ASSERTION_VIOLATION; // TODO
  }
}
void BindingFragments::OneBindingSat::setupSolver(){
  ClauseStack::Iterator cIt(*_satClauses);
  SATClauseStack clauses;
  while (cIt.hasNext()){
    auto clause = cIt.next();
    auto satClause = _sat2fo.toSAT(clause);
    cout<< "Clause: " << clause->toString() << endl;
    cout<< "SAT Clause: " << satClause->toString() << endl << endl;
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

Indexing::SubstitutionTree BindingFragments::OneBindingSat::_buildSubstitutionTree(const Array<Literal*>& literals, ArityGroupIterator::GroupIterator& group){
  SubstitutionTree tree(false, false);
  while (group.hasNext()){
    auto lit = group.next();
    tree.handle(lit, SubstitutionTree::LeafData(nullptr, lit), true);
  }
  group.reset();
  return tree;
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
    ArityGroupIterator::GroupIterator &group, Indexing::SubstitutionTree &tree):
          _group(group), _tree(tree){
}
void BindingFragments::MaximalUnifiableSubsets::mus(Literal *literal){
  while (_group.hasNext()){
      auto lit = _group.next();
      if(_s[lit] != -1)
        _s[lit] = 0;
  }
  _group.reset();

  _s[literal] = 1;
  _mus(literal, 2);
  _s[literal] = -1;
}
void BindingFragments::MaximalUnifiableSubsets::_mus(Literal *literal, int depth){
  bool isMax = true;
  auto uIt = _tree.iterator<SubstitutionTree::UnificationsIterator>(literal, true, false);

  while (uIt.hasNext()){
      auto res = uIt.next();
      auto u = res.data->literal;
      if(_s[u] == 0){
        _s[u] = depth;
        cout<< "push: " << u->toString() << endl;
        isMax = false;
        _mus(res.subst->applyToQuery(literal), depth+1);
        _s[u] = -depth;
        cout<< "pop&block: " << u->toString() << endl;
      }
  }

  if(isMax){
    cout<< "Maximal unifiable subset: ";
    for(auto & _ : _s){
      if(_.second > 0){
        cout<< _.first->toString() << " ";
      }
    }
    cout<< endl;
  }

  for(auto & _ : _s){
    if(_.second == -depth){
      _s[_.first] = 0;
      cout<< "Unblock: " << _.first->toString() << endl;
    }
  }
}
