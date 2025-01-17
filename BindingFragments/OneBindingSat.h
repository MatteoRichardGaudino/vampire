//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_ONEBINDINGSAT_H
#define VAMPIRE_ONEBINDINGSAT_H

#include "Kernel/Problem.hpp"
#include "Kernel/Formula.hpp"
#include "SAT/SAT2FO.hpp"
#include "SAT/SATSolver.hpp"
#include "Indexing/SubstitutionTree.hpp"
#include "Kernel/LiteralComparators.hpp"

#include "preprocess/BindingClassifier.h"

using namespace Kernel;
namespace BindingFragments{


class ArityGroupIterator{
public:
  ArityGroupIterator(const Array<Literal*>& literals, unsigned int literalSize)
      : _literals(literals), _literalSize(literalSize), _start(0), _end(0){};

  bool hasNext() const{
    return _end < _literalSize;
  }

  class GroupIterator{
  public:
    GroupIterator(const Array<Literal*>& literals, unsigned int start, unsigned int end)
        : _literals(literals), _start(start), _end(end){
      _i = _start;
    };

    bool hasNext() const{
      return _i < _end;
    }
    Literal* next(){
      return _literals[_i++];
    }
    void reset(){
      _i = _start;
    }
  private:
    const Array<Literal*>& _literals;
    unsigned int _start;
    unsigned int _i;
    unsigned int _end;
  };

  GroupIterator next();

  unsigned int arity() const{
    return _arity;
  }

private:
  const Array<Literal*>& _literals;
  unsigned int _literalSize;
  unsigned int _start;
  unsigned int _end;
  unsigned int _arity = 0;
};


class MaximalUnifiableSubsets{
public:
  MaximalUnifiableSubsets(ArityGroupIterator::GroupIterator group, std::function<bool(LiteralStack*)> fun);
  MaximalUnifiableSubsets(
    ArityGroupIterator::GroupIterator group,
    std::function<bool(LiteralStack *)> fun,
    std::function<bool(Literal*)> onAdded,
    std::function<void(void)> onStart,
    std::function<void(void)> onEnd
    );

  bool _groundLiteralMus(Literal * literal);
  bool mus(Literal* literal);

  bool _groundLiteralMusV2(Literal * literal);
  bool musV2(Literal* literal);
  bool _musV2(Literal* literal, LiteralList*& fToFree);

  LiteralStack* getSolution(){
    return &_solution;
  }

private:
  ArityGroupIterator::GroupIterator _group;
  Indexing::SubstitutionTree _tree;
  std::map<Literal*, int> _s;
  std::function<bool(LiteralStack*)> _fun;

  std::function<void(void)> _onGmusStart = nullptr;
  std::function<bool(Literal*)> _onGmusAdded = nullptr;
  std::function<void(void)> _onGmusEnd = nullptr;

  LiteralStack _solution;

  void _buildTree();
  bool _mus(Literal* literal, int depth);

  void _buildSolution();
  void _sReplace(int a, int b);
};

bool GroundArityAndTermComparator(Literal *&l1, Literal *&l2);
bool GroundAndArityComparator(Literal *&l1, Literal *&l2);
bool ArityComparator(Literal *&l1, Literal *&l2);


class OneBindingSat {
  public:
    OneBindingSat(Problem &prb, const Options &opt);

    bool solve();

  private:

    PrimitiveProofRecordingSATSolver* _solver;
    SAT2FO _sat2fo;
    const Options& _options;

    ClauseStack* _satClauses;
    LiteralList*  _satLiterals;

    std::map<Literal*, SATClauseStack*> _bindings;
    unsigned int _bindingCount = 0;
    unsigned int _maxBindingVarNo = 0;

    bool _showProof;


    Formula* generateSatFormula(Formula* formula);
    void generateSATClauses(Unit* unit);


    PrimitiveProofRecordingSATSolver* _newSatSolver();
    void setupSolver();
    void printAssignment();
    void blockModel(Array<Literal*>& implicants, int size);

    void _addBinding(Literal* literal);
    void _addBinding(Literal* literal, Formula* formula);

    void _printBindings();
};

}
#endif // VAMPIRE_ONEBINDINGSAT_H




/*
bool satV1(Formula formula){
    foreach impl in implicants(formula):
        res = true
        foreach s subset of impl:
            if terms(s) is unifiable:
                if bool(&&relations(s)) is not sat:
                        res = false
                        break
         if res == true:
            return true
    return false
}
*/