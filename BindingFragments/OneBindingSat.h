//
// Created by Matte on 25/09/2023.
//

#ifndef VAMPIRE_ONEBINDINGSAT_H
#define VAMPIRE_ONEBINDINGSAT_H

#include "Kernel/Problem.hpp"
#include "Kernel/Formula.hpp"
#include "SAT/SAT2FO.hpp"
#include "SAT/SATSolver.hpp"

using namespace Kernel;
namespace BindingFragments{
  class OneBindingSat {
    public:
      OneBindingSat(Problem &prb, const Options &opt);

      bool solve();

    private:
      PrimitiveProofRecordingSATSolver* _solver;
      SAT2FO _sat2fo;
      Problem& _problem;
      const Options& _options;

      ClauseStack* _satClauses;
      unsigned _bindingCount = 0;
      unsigned _satVarCount = 0;


      Formula* generateSatFormula(Formula* formula);
      void generateSATClauses(Unit* unit);

      void setupSolver();
      void printAssignment();
      void blockModel();
  };
}
#endif // VAMPIRE_ONEBINDINGSAT_H
