/**
 * @file Z3InterpolantExtractor.hpp
 * Defines class Z3InterpolantExtractor.
 */

#ifndef __Z3InterpolantExtractor__
#define __Z3InterpolantExtractor__

#include <utility>

#include "Forwards.hpp"

#include "Lib/DHMap.hpp"
#include "Lib/List.hpp"
#include "Lib/Stack.hpp"

#include "Kernel/Term.hpp"
#include "Kernel/Theory.hpp"

#include "Shell/LispParser.hpp"

namespace VUtils {

using namespace Lib;
using namespace Kernel;
using namespace Shell;

typedef LispParser::Expression LExpr;
typedef List<LExpr*> LExprList;

class ZIE {
public:
  LExpr* getInput();
  int perform(int argc, char** argv);

private:
  Unit* getZ3Refutation();


  static string hypothesesToString(List<TermList>* hypotheses);

  struct ProofObject {
    ProofObject() {}
    ProofObject(Unit* unit, List<TermList>* hypotheses)
    : unit(unit), hypotheses(hypotheses) {}

    string hypothesesToString()
    { return ZIE::hypothesesToString(hypotheses); }

    Unit* unit;
    List<TermList>* hypotheses;
  };

  bool tryReadNumber(LExpr* expr, TermList& res);

  bool isTermVariable(string name) { return name[0]=='$' || name[0]=='?'; }
  bool isProofVariable(string name) { return name[0]=='@'; }

  bool readLet(LExpr* expr, LExpr*& tail);
  void processLets();

  TermList negate(TermList term);

  TermList readTerm(LExpr* term);
  ProofObject readProofObject(LExpr* term);

  TermList getTermAssignment(string name);
  ProofObject getProofObjectAssignment(string name);

  Formula* termToFormula(TermList trm);
  Formula* termToFormula(TermList trm, List<TermList>* hypotheses);

  void resolveHypotheses(List<TermList>*& hypotheses, TermList lemma);

  typedef pair<string,LExpr*> LetRecord;
  Stack<LetRecord> _letRecords;

  DHMap<string, TermList> _termAssignments;
  DHMap<string, ProofObject> _proofAssignments;

  /**
   * All units in the proof.
   *
   * Consequences are closer to the top of the stack than their
   * premises.
   */
  Stack<Unit*> _allUnits;
  Stack<Unit*> _allUnitsColored;
  Stack<Unit*> _allUnitsLocal;


  Stack<Unit*> _inputUnits;
private:

  struct UnaryFunctionInfo
  {
    UnaryFunctionInfo() {}
    UnaryFunctionInfo(TermList firstArg);

    void onNewArg(TermList firstArg);

    bool numericArgsOnly;

    InterpretedType minArg;
    InterpretedType maxArg;
  };

  void onFunctionApplication(TermList fn);
  bool colorProof(Stack<Unit*>& derivation, Stack<Unit*>& coloredDerivationTgt);

  typedef DHMap<unsigned, UnaryFunctionInfo> UnaryInfoMap;
  UnaryInfoMap _unaryFnInfos;

private:
  void outputInterpolantStats(Unit* refutation);
};

}

#endif // __Z3InterpolantExtractor__