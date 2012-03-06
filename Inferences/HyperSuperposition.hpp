/**
 * @file HyperSuperposition.hpp
 * Defines class HyperSuperposition
 *
 */

#ifndef __HyperSuperposition__
#define __HyperSuperposition__

#include "Forwards.hpp"

#include "InferenceEngine.hpp"

namespace Inferences
{

using namespace Kernel;
using namespace Indexing;
using namespace Saturation;

class HyperSuperposition
: public ForwardSimplificationEngine
{
public:
  HyperSuperposition() : _index(0) {}

  void attach(SaturationAlgorithm* salg);
  void detach();

  Clause* generateClause(Clause* queryCl, Literal* queryLit, SLQueryResult res, Limits* limits=0);
  ClauseIterator generateClauses(Clause* premise);

  void perform(Clause* cl, ForwardSimplificationPerformer* simplPerformer);

private:
  typedef pair<TermList,TermList> TermPair;
  /** The int here is a variable bank index of the term's variables */
  typedef pair<TermPair, int> RewriterEntry;
  typedef Stack<RewriterEntry> RewriterStack;

  /**
   * Used for returning the results internally. The first clause is after applying the rewriting
   * and the second is after the simplification we were aiming for is done.
   */
  typedef pair<Clause*,Clause*> ClausePair;
  typedef Stack<ClausePair> ClausePairStack;

  bool tryToUnifyTwoTermPairs(RobSubstitution& subst, TermList tp1t1, int bank11,
      TermList tp1t2, int bank12, TermList tp2t1, int bank21, TermList tp2t2, int bank22);

  bool tryMakeTopUnifiableByRewriter(TermList t1, TermList t2, int t2Bank, int& nextAvailableBank, ClauseStack& premises,
      RewriterStack& rewriters, RobSubstitution& subst, Color& infClr);

  bool tryGetRewriters(Term* t1, Term* t2, int t2Bank, int& nextAvailableBank, ClauseStack& premises,
      RewriterStack& rewriters, RobSubstitution& subst, Color& infClr);

  void tryUnifyingSuperpositioins(Clause* cl, unsigned literalIndex, Term* t1, Term* t2,
      bool disjointVariables, ClauseStack& acc);

  bool tryGetUnifyingPremises(Term* t1, Term* t2, Color clr, bool disjointVariables, ClauseStack& premises);

  /**
   * premises should contain any other premises that can be needed
   * (e.g. an unit literal to resolve with)
   */
  Clause* tryGetContradictionFromUnification(Clause* cl, Term* t1, Term* t2, bool disjointVariables, ClauseStack& premises);

  bool trySimplifyingFromUnification(Clause* cl, Term* t1, Term* t2, bool disjointVariables, ClauseStack& premises,
      ForwardSimplificationPerformer* simplPerformer);
  bool tryUnifyingNonequalitySimpl(Clause* cl, ForwardSimplificationPerformer* simplPerformer);
  bool tryUnifyingToResolveSimpl(Clause* cl, ForwardSimplificationPerformer* simplPerformer);


  Literal* getUnifQueryLit(Literal* base);
  void resolveFixedLiteral(Clause* cl, unsigned litIndex, ClausePairStack& acc);

  void tryUnifyingNonequality(Clause* cl, unsigned literalIndex, ClausePairStack& acc);
  void tryUnifyingToResolveWithUnit(Clause* cl, unsigned literalIndex, ClausePairStack& acc);

  static bool rewriterEntryComparator(RewriterEntry p1, RewriterEntry p2);

  UnitClauseLiteralIndex* _index;
};

};

#endif /*__HyperSuperposition__*/