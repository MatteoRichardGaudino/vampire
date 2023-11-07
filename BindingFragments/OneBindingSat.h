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

#include "BindingClassifier.h"

using namespace Kernel;
namespace BindingFragments{


class ArityGroupIterator{
public:
  ArityGroupIterator(const Array<Literal*>& literals, unsigned int literalSize)
      : _literals(literals), _literalSize(literalSize), _start(0), _end(0){};

  inline bool hasNext() const{
    return _end < _literalSize;
  }

  class GroupIterator{
  public:
    GroupIterator(const Array<Literal*>& literals, unsigned int start, unsigned int end)
        : _literals(literals), _start(start), _end(end){
      _i = _start;
    };
    inline bool hasNext() const{
      return _i < _end;
    }
    inline Literal* next(){
      return _literals[_i++];
    }
    inline void reset(){
      _i = _start;
    }
  private:
    const Array<Literal*>& _literals;
    unsigned int _start;
    unsigned int _i;
    unsigned int _end;
  };

  GroupIterator next();

  inline unsigned int arity() const{
    return _arity;
  }



private:
  const Array<Literal*>& _literals;
  unsigned int _literalSize;
  int _start;
  int _end;
  unsigned int _arity = 0;
};


class MaximalUnifiableSubsets{
public:
  MaximalUnifiableSubsets(ArityGroupIterator::GroupIterator& group, Indexing::SubstitutionTree& tree);

  void mus(Literal* literal);

private:
  ArityGroupIterator::GroupIterator _group;
  Indexing::SubstitutionTree& _tree;
  std::map<Literal*, int> _s;

  void _mus(Literal* literal, int depth);
};

class OneBindingSat {
  public:
    OneBindingSat(Problem &prb, const Options &opt);

    bool solve();

  private:
    friend class BindingClassifier;

    PrimitiveProofRecordingSATSolver* _solver;
    SAT2FO _sat2fo;
    Problem& _problem;
    const Options& _options;

    ClauseStack* _satClauses;
    LiteralList* _satLiterals;


    Formula* generateSatFormula(Formula* formula);
    void generateSATClauses(Unit* unit);

    void setupSolver();
    void printAssignment();
    void blockModel();

    Indexing::SubstitutionTree _buildSubstitutionTree(const Array<Literal*>& literals, ArityGroupIterator::GroupIterator& groupIterator);


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