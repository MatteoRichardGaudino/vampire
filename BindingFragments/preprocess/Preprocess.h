//
// Created by Matte on 09/10/2023.
//

#ifndef VAMPIRE_PREPROCESS_H
#define VAMPIRE_PREPROCESS_H

#include "BindingClassifier.h"
#include "../../Kernel/Unit.hpp"
#include "../../Lib/DHMap.hpp"
#include "SAT/SAT2FO.hpp"
#include "SAT/SATLiteral.hpp"

using namespace Kernel;

namespace BindingFragments {
class Preprocess {
public:
  explicit Preprocess(
    bool skolemize = true,
    bool clausify = false,
    bool distributeForall = true
  ):_skolemize(skolemize), _clausify(clausify), _distributeForall(distributeForall) {};
  void preprocess(Problem& prb);
  void clausify(Problem& prb);

  /**
   * Convert the problem from default mode i.e. A1 & A2 & ... & An & ~C
   * to (A1 & A2 & ... & An) => C === ~A1 | ~A2 | ... | ~An | C
   */
  static void negatedProblem(Problem& prb);

  void setSkolemize(bool skolemize) { _skolemize = skolemize; }
  void setClausify(bool clausify) { _clausify = clausify; }
  void setDistributeForall(bool distribute_forall) { _distributeForall = distribute_forall; }


private:
  bool _skolemize;
  bool _clausify;
  bool _distributeForall;
};

typedef DHMap<Literal*, Formula*> BindingFormulaMap;

typedef DHMap<Literal*, LiteralList*> BooleanToLiteralBindingMap;
typedef DHMap<Literal*, Literal*> LiteralToBooleanBindingMap;

typedef DHMap<Literal*, SAT::SATClauseStack*> BindingClauseMap;

class PreprocessV2 {
public:
  Problem &prb;
  Fragment fragment;

  bool emptyClauseFound = false;

  explicit PreprocessV2(Problem& prb, Fragment fragment);


  // Top Boolean Formula pre-processing
  void ennf();
  void topBooleanFormulaAndBindings();
  void naming();
  void nnf();
  //void skolemize(); // se si eseguono le funzione in ordine skolemize non serve visto che la formula e' ground
  // void distributeForall();
  //void clausify();
  void satClausify();

  // Binding pre-processing





  // void printBindings();
  void printSatClauses();


  SAT::SATLiteral toSAT(Literal* literal){
    return _sat2fo.toSAT(literal);
  }

  unsigned maxSatVar(){
    return _sat2fo.maxSATVar();
  }
  unsigned maxBindingVarNo(){
    return _maxBindingVarNo;
  }
  bool isGround(){
    return _bindingCount == 0;
  }

  SAT::SATClauseStack* satClauses(){
    return &_clauses;
  }
  LiteralList *literals(){
    return _literals;
  }

  SAT::SATClauseStack* getSatClauses(Literal* literal){
    ASS(!isBooleanBinding(literal))
    return _bindingClauses.get(literal);
  }

  Literal* getBooleanBinding(Literal* literalBinding){
    ASS(isBindingLiteral(literalBinding))
    return _literalToBooleanBindings.get(literalBinding);
  }

  LiteralList* getLiteralBindings(Literal* booleanBinding){
    ASS(isBooleanBinding(booleanBinding))
    return _booleanToLiteralBindings.get(booleanBinding);
  }

  bool isBooleanBinding(Literal* literal) const{
    return literal->functor() >= _minBooleanBindingFunctor && literal->functor() <= _maxBooleanBindingFunctor;
  }
  bool isBindingLiteral(Literal* literal) const{
    return literal->functor() >= _minLiteralBindingFunctor && literal->functor() <= _maxLiteralBindingFunctor;
  }


  void printBindingFormulas(){
    if(!env.options->showPreprocessing()) return;

    env.beginOutput();
    env.out() << "======== Binding Formulas ========" << std::endl;
    auto it = _bindingFormulas.items();
    while (it.hasNext()) {
      auto item = it.next();
      env.out() << item.first->toString() << " => " << item.second->toString() << std::endl;
    }
    env.out() << "======== End Binding Formulas ========" << std::endl;
    env.endOutput();
  }

  void printLiteralToBooleanBindings(){
    if(!env.options->showPreprocessing()) return;

    env.beginOutput();

    env.out() << "======== Literal Bindings to Boolean Bindings ========" << std::endl;
    auto it = _literalToBooleanBindings.items();
    while (it.hasNext()) {
      auto item = it.next();
      env.out() << item.first->toString() << " => " << item.second->toString() << std::endl;
    }
    env.out() << "======== End Literal Bindings to Boolean Bindings ========" << std::endl;
    env.endOutput();

  }

  void printBooleanToLiteralBindings(){
    if(!env.options->showPreprocessing()) return;

    env.beginOutput();

    env.out() << "======== Boolean to Literal Bindings ========" << std::endl;
    auto it = _booleanToLiteralBindings.items();
    while (it.hasNext()) {
      auto item = it.next();
      env.out() << item.first->toString() << " => [";
      LiteralList::Iterator lbIt(item.second);
      while (lbIt.hasNext()) {
        env.out() << lbIt.next()->toString() << (lbIt.hasNext()? ", " : "");
      }
      env.out() << "]" << std::endl;
    }
    env.out() << "======== End Boolean to Literal Bindings ========" << std::endl;
    env.endOutput();

  }
private:
  BindingFormulaMap _bindingFormulas;

  BooleanToLiteralBindingMap _booleanToLiteralBindings;
  LiteralToBooleanBindingMap _literalToBooleanBindings;

  BindingClauseMap _bindingClauses;

  // return a new literal with arity 0
  Literal *_newBooleanBinding();
  // return a new literal with same terms and arity of literal
  Literal *_newBindingLiteral(Literal *literal);
  // return an atomicFormula containing a new booleanBinding ($bx) and add ($bx => formula) to _bindingFormulas
  Formula *_addBindingFormula(Formula *formula);





  unsigned _maxBindingVarNo = 0;
  int _bindingCount = 0;
  int _minBooleanBindingFunctor = -1;
  int _maxBooleanBindingFunctor = 0;

  int _minLiteralBindingFunctor = -1;
  int _maxLiteralBindingFunctor = 0;




  SAT::SAT2FO _sat2fo;
  SAT::SATClauseStack _clauses;
  LiteralList* _literals = LiteralList::empty();

  void _addLiterals(Clause* clause);


  SAT::SATClauseStack* _getSingleLiteralClause(Literal* literal);
  // Formula* _newBinding(Literal* literal);
  // Formula* _newBinding(Formula* formula);

  SAT::SATClauseStack* _clausifyBindingFormula(FormulaUnit* formula);

  Formula* _topBooleanFormula(Formula* formula);
};
}
#endif // VAMPIRE_PREPROCESS_H
