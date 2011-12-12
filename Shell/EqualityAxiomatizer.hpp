/**
 * @file EqualityAxiomatizer.hpp
 * Defines class EqualityAxiomatizer.
 */

#ifndef __EqualityAxiomatizer__
#define __EqualityAxiomatizer__

#include "Forwards.hpp"

#include "Lib/DHSet.hpp"

#include "Options.hpp"

namespace Shell {

using namespace Lib;
using namespace Kernel;

/**
 * Adds specified equality axioms.
 *
 * We scan the problem and avoi adding unnecessary axioms (for functions
 * and predicates that do not appear in the problem, and for sorts that do
 * not occur in any equalities).
 */
class EqualityAxiomatizer {
private:
  Options::EqualityProxy _opt;

  typedef DHSet<unsigned> SymbolSet;
  typedef DHSet<unsigned> SortSet;

  SymbolSet _fns;
  SymbolSet _preds;
  SortSet _eqSorts;

  void saturateEqSorts();

  void addLocalAxioms(UnitList*& units, unsigned sort);
  void addCongruenceAxioms(UnitList*& units);
  Clause* getFnCongruenceAxiom(unsigned fn);
  Clause* getPredCongruenceAxiom(unsigned pred);
  bool getArgumentEqualityLiterals(BaseType* symbolType, LiteralStack& lits, Stack<TermList>& vars1,
      Stack<TermList>& vars2);

  void scan(UnitList* units);
  void scan(Literal* lit);
  UnitList* getAxioms();
public:
  /**
   * Which axioms to add. Can be one of EP_RS, EP_RST, EP_RSTC.
   *
   * EP_R is not an option, because since we're using the built-in equality literals,
   * symmetry is implicit due to normalization in term sharing.
   */
  EqualityAxiomatizer(Options::EqualityProxy opt) : _opt(opt)
  {
    ASS_REP(opt==Options::EP_RS || opt==Options::EP_RST || opt==Options::EP_RSTC, opt);
  }

  void apply(Problem& prb);
  void apply(UnitList*& units);

};

}

#endif // __EqualityAxiomatizer__