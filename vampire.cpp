/*
 * This file is part of the source code of the software program
 * Vampire. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 */
/**
 * @file vampire.cpp. Implements the top-level procedures of Vampire.
 */

#include <iostream>
#include <ostream>
#include <fstream>

#if VZ3
#include "z3++.h"
#endif

#include "Debug/TimeProfiling.hpp"
#include "Lib/Exception.hpp"
#include "Lib/Environment.hpp"
#include "Lib/Random.hpp"
#include "Lib/Timer.hpp"
#include "Lib/VString.hpp"
#include "Lib/List.hpp"
#include "Lib/System.hpp"
#include "Lib/Metaiterators.hpp"

#include "Kernel/Clause.hpp"
#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/Problem.hpp"
#include "Kernel/Signature.hpp"
#include "Kernel/Term.hpp"

#include "Inferences/InferenceEngine.hpp"
#include "Inferences/TautologyDeletionISE.hpp"

#include "CASC/PortfolioMode.hpp"
#include "CASC/CLTBMode.hpp"
#include "CASC/CLTBModeLearning.hpp"
#include "Shell/CommandLine.hpp"
#include "Shell/Normalisation.hpp"
#include "Shell/Options.hpp"
#include "Shell/Property.hpp"
#include "Saturation/ProvingHelper.hpp"
#include "Shell/Preprocess.hpp"
#include "Shell/TheoryFinder.hpp"
#include "Shell/TPTPPrinter.hpp"
#include "Parse/TPTP.hpp"
#include "Shell/FOOLElimination.hpp"
#include "Shell/Statistics.hpp"
#include "Shell/UIHelper.hpp"
#include "Shell/LaTeX.hpp"

#include "Saturation/SaturationAlgorithm.hpp"

#include "BindingFragments/ProvingHelper.h"
#include "BindingFragments/preprocess/Preprocess.h"
#include "BindingFragments/preprocess/BindingClassifier.h"

#include "FMB/ModelCheck.hpp"

#if CHECK_LEAKS
#include "Lib/MemoryLeak.hpp"
#endif

using namespace std;

/**
 * Return value is non-zero unless we were successful.
 *
 * Being successful for modes that involve proving means that we have
 * either found refutation or established satisfiability.
 *
 *
 * If Vampire was interrupted by a SIGINT, value
 * VAMP_RESULT_STATUS_SIGINT is returned,
 * and in case of other signal we return VAMP_RESULT_STATUS_OTHER_SIGNAL. For implementation
 * of these return values see Lib/System.hpp.
 *
 * In case of an unhandled exception or user error, we return value
 * VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION.
 *
 * In case Vampire was terminated by the timer, return value is
 * uncertain (but definitely not zero), probably it will be 134
 * (we terminate by a call to the @b abort() function in this case).
 */
int vampireReturnValue = VAMP_RESULT_STATUS_UNKNOWN;

/**
 * Preprocess input problem
 *
 */
VWARN_UNUSED
Problem* getPreprocessedProblem()
{
#ifdef __linux__
  unsigned saveInstrLimit = env.options->instructionLimit();
  if (env.options->parsingDoesNotCount()) {
    env.options->setInstructionLimit(0);
  }
#endif

  Problem* prb = UIHelper::getInputProblem(*env.options);

#ifdef __linux__
  if (env.options->parsingDoesNotCount()) {
    env.options->setInstructionLimit(saveInstrLimit+Timer::elapsedMegaInstructions());
  }
#endif

  TIME_TRACE(TimeTrace::PREPROCESSING);

  // this will provide warning if options don't make sense for problem
  if (env.options->mode()!=Options::Mode::SPIDER) {
    env.options->checkProblemOptionConstraints(prb->getProperty(), /*before_preprocessing = */ true);
  }

  Shell::Preprocess prepro(*env.options);
  //phases for preprocessing are being set inside the preprocess method
  prepro.preprocess(*prb);

  return prb;
} // getPreprocessedProblem
Problem* getPreprocessedProblemForBindingFragments(){
#ifdef __linux__
  unsigned saveInstrLimit = env.options->instructionLimit();
  if (env.options->parsingDoesNotCount()) {
    env.options->setInstructionLimit(0);
  }
#endif

  Problem* prb = UIHelper::getInputProblem(*env.options);

#ifdef __linux__
  if (env.options->parsingDoesNotCount()) {
    env.options->setInstructionLimit(saveInstrLimit+Timer::elapsedMegaInstructions());
  }
#endif

  TIME_TRACE(TimeTrace::PREPROCESSING);

  // this will provide warning if options don't make sense for problem
  env.options->checkProblemOptionConstraints(prb->getProperty(), /*before_preprocessing = */ true);

  BindingFragments::Preprocess prepro;
  prepro.preprocess(*prb);

  return prb;
} // getPreprocessedProblem for binding fragments mode


void explainException(Exception& exception)
{
  env.beginOutput();
  exception.cry(env.out());
  env.endOutput();
} // explainException

void getRandomStrategy()
{
  // We might have set random_strategy sat
  if(env.options->randomStrategy()==Options::RandomStrategy::OFF){
    env.options->setRandomStrategy(Options::RandomStrategy::ON);
  }

  // One call to randomize before preprocessing (see Options)
  env.options->randomizeStrategy(0); 
  ScopedPtr<Problem> prb(getPreprocessedProblem());
  // Then again when the property is here
  env.options->randomizeStrategy(prb->getProperty()); 

  // It is possible that the random strategy is still incorrect as we don't
  // have access to the Property when setting preprocessing
  env.options->checkProblemOptionConstraints(prb->getProperty(), /*before_preprocessing = */ false);
}

VWARN_UNUSED
Problem *doProving()
{
  // One call to randomize before preprocessing (see Options)
  env.options->randomizeStrategy(0);

  Problem *prb = getPreprocessedProblem();

  // Then again when the property is here (this will only randomize non-default things if an option is set to do so)
  env.options->randomizeStrategy(prb->getProperty()); 

  // this will provide warning if options don't make sense for problem
  if (env.options->mode()!=Options::Mode::SPIDER) {
    env.options->checkProblemOptionConstraints(prb->getProperty(), /*before_preprocessing = */ false);
  }

  ProvingHelper::runVampireSaturation(*prb, *env.options);
  return prb;
}
VWARN_UNUSED
Problem *do1BProving(){
  Problem *prb = getPreprocessedProblemForBindingFragments();

  // ---------print  INPUT ---------
  // cout<< "----------- Input -----------" << endl;
  // UnitList::Iterator it(prb->units());
  // while (it.hasNext()) {
  //   cout<< it.next()->toString() << endl;
  // }
  // cout<< "----------- end -----------" << endl;



  env.statistics->phase = Statistics::CLASSIFY;
  auto classification = BindingFragments::BindingClassifier::classify(prb->units());

  switch (classification) {
    case BindingFragments::UNIVERSAL_ONE_BINDING:
    case BindingFragments::ONE_BINDING:
    case BindingFragments::CONJUNCTIVE_BINDING:
      BindingFragments::ProvingHelper::run1BSatAlgorithm(*prb, *env.options, classification);
      break;
    case BindingFragments::DISJUNCTIVE_BINDING:
    case BindingFragments::NONE:
      cout<< "Can't solve this fragment: " << BindingFragments::fragmentToString(classification) << endl;
      env.statistics->terminationReason = Statistics::INAPPROPRIATE;
      env.statistics->refutation = 0;
      break;
  }

  return prb;
}

/**
 * Read a problem and output profiling information about it.
 * @since 03/08/2008 Torrevieja
 */
void profileMode()
{
  ScopedPtr<Problem> prb(UIHelper::getInputProblem(*env.options));

  /* CAREFUL: Make sure that the order
   * 1) getProperty, 2) normalise, 3) TheoryFinder::search
   * is the same as in PortfolioMode::searchForProof
   * also, cf. the beginning of Preprocessing::preprocess*/
  Property* property = prb->getProperty();
  Normalisation().normalise(*prb);
  TheoryFinder(prb->units(), property).search();

  env.beginOutput();
  env.out() << property->categoryString() << ' ' << property->props() << ' '
	  << property->atoms() << "\n";
  env.endOutput();

  //we have succeeded with the profile mode, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
} // profileMode

// prints Unit u at an index to latexOut using the LaTeX object
void outputUnitToLaTeX(LaTeX& latex, ofstream& latexOut, Unit* u,unsigned index)
{
    vstring stringform = latex.toString(u);
    latexOut << index++ << " & ";
    unsigned count = 0;
    for(const char* p = stringform.c_str();*p;p++){
      latexOut << *p;
      count++;
      if(count>80 && *p==' '){
        latexOut << "\\\\ \n & ~~~~~";
        count=0;
      }
    }
    latexOut << "\\\\" << endl;
}

// print the clauses of a problem to a LaTeX file
void outputClausesToLaTeX(Problem* prb)
{
  ASS(env.options->latexOutput()!="off");

  BYPASSING_ALLOCATOR; // not sure why we need this yet, ofstream?

  LaTeX latex;
  ofstream latexOut(env.options->latexOutput().c_str());
  latexOut << latex.header() << endl;
  latexOut << "\\section{Problem "<<env.options->problemName() << "}" << endl;
  //TODO output more header
  latexOut << "\\[\n\\begin{array}{ll}" << endl;

  CompositeISE simplifier;
  simplifier.addFront(new TrivialInequalitiesRemovalISE());
  simplifier.addFront(new TautologyDeletionISE());
  simplifier.addFront(new DuplicateLiteralRemovalISE());

  unsigned index=0;
  ClauseIterator cit = prb->clauseIterator();
  while (cit.hasNext()) {
    Clause* cl = cit.next();
    cl = simplifier.simplify(cl);
    if (!cl) {
      continue;
    }
    outputUnitToLaTeX(latex,latexOut,cl,index++);
  }
  latexOut  << "\\end{array}\n\\]" << latex.footer() << "\n";

  //close ofstream?
}

// print the formulas of a problem to a LaTeX file
void outputProblemToLaTeX(Problem* prb)
{
  ASS(env.options->latexOutput()!="off");

  BYPASSING_ALLOCATOR; // not sure why we need this yet, ofstream?

  LaTeX latex;
  ofstream latexOut(env.options->latexOutput().c_str());
  latexOut << latex.header() << endl;
  latexOut << "\\section{Problem "<<env.options->problemName() << "}" << endl;
  //TODO output more header
  latexOut << "\\[\n\\begin{array}{ll}" << endl;

  //TODO  get symbol and sort declarations into LaTeX

  UnitList::Iterator units(prb->units());

  unsigned index = 0;
  while (units.hasNext()) {
    Unit* u = units.next();
    outputUnitToLaTeX(latex,latexOut,u,index++);
  }
  latexOut  << "\\end{array}\n\\]" << latex.footer() << "\n";

  //close ofstream?
}

/**
 * This mode only preprocesses the input using the current preprocessing
 * options and outputs it to stdout. It is useful for either preprocessing
 * per se or also for converting one syntax to another. For the latter, the input
 * and the output syntaxes must be set to different values. Note that for
 * simply translating one syntax to another, output mode is the right one.
 * 
 * @author Andrei Voronkov
 * @since 02/07/2013 Manchester
 */
void preprocessMode(bool theory)
{
  Problem* prb = UIHelper::getInputProblem(*env.options);

  TIME_TRACE(TimeTrace::PREPROCESSING);

  // preprocess without clausification
  Shell::Preprocess prepro(*env.options);
  prepro.turnClausifierOff();
  if(env.options->mode() == Options::Mode::PREPROCESS2){
    prepro.keepSimplifyStep();
  }
  prepro.preprocess(*prb);

  env.beginOutput();
  //outputSymbolDeclarations also deals with sorts for now
  //UIHelper::outputSortDeclarations(env.out());
  UIHelper::outputSymbolDeclarations(env.out());
  UnitList::Iterator units(prb->units());
  while (units.hasNext()) {
    Unit* u = units.next();
    if (!env.options->showFOOL()) {
      if (u->inference().rule() == InferenceRule::FOOL_AXIOM_TRUE_NEQ_FALSE || u->inference().rule() == InferenceRule::FOOL_AXIOM_ALL_IS_TRUE_OR_FALSE) {
        continue;
      }
    }

    if (theory) {
      Formula* f = u->getFormula();

      // CONJECTURE as inputType is evil, as it cannot occur multiple times
      if (u->inference().inputType() == UnitInputType::CONJECTURE) {
        u->inference().setInputType(UnitInputType::NEGATED_CONJECTURE);
      }

      FormulaUnit* fu = new FormulaUnit(f,u->inference()); // we are stealing u's inference which is not nice
      env.out() << TPTPPrinter::toString(fu) << "\n";
    } else {
      env.out() << TPTPPrinter::toString(u) << "\n";
    }
  }
  env.endOutput();

  if(env.options->latexOutput()!="off"){ outputProblemToLaTeX(prb); }

  //we have successfully output all clauses, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
} // preprocessMode

/**
 *
 * @author Giles
 * @since 6/10/2015
 */
void modelCheckMode()
{
  env.options->setOutputAxiomNames(true);
  Problem* prb = UIHelper::getInputProblem(*env.options);

  if(env.getMainProblem()->hasPolymorphicSym() || env.getMainProblem()->isHigherOrder()){
    USER_ERROR("Polymorphic Vampire is not yet compatible with theory reasoning");
  }

  FMB::ModelCheck::doCheck(prb);

} // modelCheckMode


/**
 * This mode only outputs the input problem. It is useful for converting
 * one syntax to another.
 * @author Laura Kovacs and Andrei Voronkov
 * @since 02/07/2013 Gothenburg and Manchester
 */
void outputMode()
{
  Problem* prb = UIHelper::getInputProblem(*env.options);

  env.beginOutput();
  //outputSymbolDeclarations also deals with sorts for now
  //UIHelper::outputSortDeclarations(env.out());
  UIHelper::outputSymbolDeclarations(env.out());
  UnitList::Iterator units(prb->units());

  while (units.hasNext()) {
    Unit* u = units.next();
    env.out() << TPTPPrinter::toString(u) << "\n";
  }
  env.endOutput();

  if(env.options->latexOutput()!="off"){ outputProblemToLaTeX(prb); }

  //we have successfully output all clauses, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
} // outputMode

void vampireMode()
{
  if (env.options->mode() == Options::Mode::CONSEQUENCE_ELIMINATION) {
    env.options->setUnusedPredicateDefinitionRemoval(false);
  }

  ScopedPtr<Problem> prb(doProving());

  env.beginOutput();
  UIHelper::outputResult(env.out());
  env.endOutput();

  if (env.statistics->terminationReason == Statistics::REFUTATION
      || env.statistics->terminationReason == Statistics::SATISFIABLE) {
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
  }
} // vampireMode

void oneBindingMode(){
//  if (env.options->mode() == Options::Mode::CONSEQUENCE_ELIMINATION) {
//      env.options->setUnusedPredicateDefinitionRemoval(false);
//  }

  ScopedPtr<Problem> prb(do1BProving());

  env.beginOutput();
  UIHelper::outputResult(env.out());
  env.endOutput();

  if (env.statistics->terminationReason == Statistics::REFUTATION
      || env.statistics->terminationReason == Statistics::SATISFIABLE) {
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
  }
}

void fragmentClassificationMode(bool sk){
  ScopedPtr<Problem> prb(UIHelper::getInputProblem(*env.options));

  // check if has negated conjecture
  bool containsConjecture = false;
  if(!sk) {
      UnitList::Iterator it(prb->units());
    while (it.hasNext() && !containsConjecture) {
      auto inf = it.next()->inference();
      if(inf.rule() == InferenceRule::NEGATED_CONJECTURE) containsConjecture = true; // TODO probabilmente c'è un modo migliore per fare questo
    }
  }

  BindingFragments::Preprocess prepro(sk, false, false);
  prepro.preprocess(*prb);
  auto classification = BindingFragments::BindingClassifier::classify(prb->units());

  env.beginOutput();
  env.out() << endl;
  if(classification == BindingFragments::DISJUNCTIVE_BINDING && containsConjecture) {
    env.out() << "~";
  }
  env.out() << fragmentToString(classification) << endl;
  env.endOutput();
}

void spiderMode()
{
  env.options->setBadOptionChoice(Options::BadOption::HARD);
  env.options->setOutputMode(Options::Output::SPIDER);
  env.options->setNormalize(true);
  // to start counting instructions
#ifdef __linux__
  Timer::ensureTimerInitialized();
#endif

  Exception* exception = 0;
#if VZ3
  z3::exception* z3_exception = 0;
#endif
  
  bool exceptionRaised = false;
  ScopedPtr<Problem> prb;
  try {
    prb = doProving();
  } catch (Exception& e) {
    exception = &e;
    exceptionRaised = true;
#if VZ3
  } catch(z3::exception& e){
    z3_exception = &e; 
    exceptionRaised = true;
#endif
  } catch (...) {
    exceptionRaised = true;
  }

  env.beginOutput();
  if (!exceptionRaised) {
    switch (env.statistics->terminationReason) {
    case Statistics::REFUTATION:
      reportSpiderStatus('+');
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
      
    case Statistics::TIME_LIMIT:
      reportSpiderStatus('t');
      break;
      
    case Statistics::MEMORY_LIMIT:
      reportSpiderStatus('m');
      break;
      
    case Statistics::UNKNOWN:
    case Statistics::INAPPROPRIATE:
      reportSpiderStatus('u');
      break;
      
    case Statistics::REFUTATION_NOT_FOUND:
      if (env.statistics->discardedNonRedundantClauses > 0) {
        reportSpiderStatus('n');
      }
      else{
        reportSpiderStatus('i');
      }
      break;
      
    case Statistics::SATISFIABLE:
      reportSpiderStatus('-');
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
      
    default:
      ASSERTION_VIOLATION;
    }
    
    env.endOutput();
    return;
  }

  // exception
  vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;

#if VZ3
  if (z3_exception) {
    if (strcmp(z3_exception->msg(),"out of memory\n")) {
      reportSpiderStatus('m');
    }
    else {
      reportSpiderFail();
    }
    
    env.endOutput();
    return;
  }
#endif

  reportSpiderFail();
  env.endOutput();
  
  ASS(exception); 
  explainException(*exception); 
} // spiderMode

void clausifyMode(bool theory)
{
  CompositeISE simplifier;
  simplifier.addFront(new TrivialInequalitiesRemovalISE());
  simplifier.addFront(new TautologyDeletionISE());
  simplifier.addFront(new DuplicateLiteralRemovalISE());

  ScopedPtr<Problem> prb(getPreprocessedProblem());

  env.beginOutput();
  //outputSymbolDeclarations deals with sorts as well for now
  //UIHelper::outputSortDeclarations(env.out());
  UIHelper::outputSymbolDeclarations(env.out());

  ClauseIterator cit = prb->clauseIterator();
  bool printed_conjecture = false;
  while (cit.hasNext()) {
    Clause* cl = cit.next();
    cl = simplifier.simplify(cl);
    if (!cl) {
      continue;
    }
    printed_conjecture |= cl->inputType() == UnitInputType::CONJECTURE || cl->inputType() == UnitInputType::NEGATED_CONJECTURE;
    if (theory) {
      Formula* f = Formula::fromClause(cl);

      // CONJECTURE as inputType is evil, as it cannot occur multiple times
      if (cl->inference().inputType() == UnitInputType::CONJECTURE) {
        cl->inference().setInputType(UnitInputType::NEGATED_CONJECTURE);
      }

      FormulaUnit* fu = new FormulaUnit(f,cl->inference()); // we are stealing cl's inference, which is not nice!
      fu->overwriteNumber(cl->number()); // we are also making sure it's number is the same as that of the original (for Kostya from Russia to CASC, with love, and back again)
      env.out() << TPTPPrinter::toString(fu) << "\n";
    } else {
      env.out() << TPTPPrinter::toString(cl) << "\n";
    }
  }
  if(!printed_conjecture && UIHelper::haveConjecture()){
    unsigned p = env.signature->addFreshPredicate(0,"p");
    Clause* c = new(2) Clause(2,NonspecificInference0(UnitInputType::NEGATED_CONJECTURE,InferenceRule::INPUT));
    (*c)[0] = Literal::create(p,0,true,false,0);
    (*c)[1] = Literal::create(p,0,false,false,0);
    env.out() << TPTPPrinter::toString(c) << "\n";
  }
  env.endOutput();

  if (env.options->latexOutput() != "off") { outputClausesToLaTeX(prb.ptr()); }

  //we have successfully output all clauses, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
} // clausifyMode

void axiomSelectionMode()
{
  env.options->setSineSelection(Options::SineSelection::AXIOMS);

  ScopedPtr<Problem> prb(UIHelper::getInputProblem(*env.options));

  if (prb->hasFOOL()) {
    FOOLElimination().apply(*prb);
  }

  // reorder units
  if (env.options->normalize()) {
    env.statistics->phase = Statistics::NORMALIZATION;
    Normalisation norm;
    norm.normalise(*prb);
  }

  env.statistics->phase = Statistics::SINE_SELECTION;
  SineSelector(*env.options).perform(*prb);

  env.statistics->phase = Statistics::FINALIZATION;

  UnitList::Iterator uit(prb->units());
  env.beginOutput();
  while (uit.hasNext()) {
    Unit* u = uit.next();
    env.out() << TPTPPrinter::toString(u) << "\n";
  }
  env.endOutput();

  //we have successfully output the selected units, so we'll terminate with zero return value
  vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
}

/**
 * The main function.
 * @since 03/12/2003 many changes related to logging
 *        and exception handling.
 * @since 10/09/2004, Manchester changed to use knowledge bases
 */
int main(int argc, char* argv[])
{
  System::registerArgv0(argv[0]);
  System::setSignalHandlers();

  START_CHECKING_FOR_ALLOCATOR_BYPASSES;

  try {
    // read the command line and interpret it
    Shell::CommandLine cl(argc, argv);
    cl.interpret(*env.options);
#if VTIME_PROFILING
    TimeTrace::instance().setEnabled(env.options->timeStatistics());
#endif

    // If any of these options are set then we just need to output and exit
    if (env.options->showHelp() ||
        env.options->showOptions() ||
        env.options->showExperimentalOptions() ||
        !env.options->explainOption().empty() ||
        env.options->printAllTheoryAxioms()) {
      env.beginOutput();
      env.options->output(env.out());
      env.endOutput();
      exit(0);
    }

    Lib::setMemoryLimit(env.options->memoryLimit() * 1048576ul);
    Lib::Random::setSeed(env.options->randomSeed());

    switch (env.options->mode())
    {
    case Options::Mode::AXIOM_SELECTION:
      axiomSelectionMode();
      break;
    case Options::Mode::SPIDER:
      spiderMode();
      break;
    case Options::Mode::RANDOM_STRATEGY:
      getRandomStrategy();
      break;
    case Options::Mode::CONSEQUENCE_ELIMINATION:
    case Options::Mode::VAMPIRE:
      vampireMode();
      break;
    case Options::Mode::ONE_BINDING:
      oneBindingMode();
      break;
    case Options::Mode::FRAGMENT_CLASSIFICATION:
      fragmentClassificationMode(false);
      break;
    case Options::Mode::FRAGMENT_CLASSIFICATION_SK:
      fragmentClassificationMode(true);
    break;

    case Options::Mode::CASC:
      env.options->setIgnoreMissing(Options::IgnoreMissing::WARN);
      env.options->setSchedule(Options::Schedule::CASC);
      env.options->setOutputMode(Options::Output::SZS);
      env.options->setProof(Options::Proof::TPTP);
      env.options->setOutputAxiomNames(true);
      env.options->setNormalize(true);
      env.options->setRandomizeSeedForPortfolioWorkers(false);

      if (CASC::PortfolioMode::perform(env.options->slowness())) {
        vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      break;

    case Options::Mode::CASC_HOL: {
      env.options->setIgnoreMissing(Options::IgnoreMissing::WARN);
      env.options->setSchedule(Options::Schedule::CASC_HOL_2020);
      env.options->setOutputMode(Options::Output::SZS);
      env.options->setProof(Options::Proof::TPTP);
      env.options->setOutputAxiomNames(true);

      if (CASC::PortfolioMode::perform(env.options->slowness())) {
        vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      break;
    }
    case Options::Mode::CASC_SAT:
      env.options->setIgnoreMissing(Options::IgnoreMissing::WARN);
      env.options->setSchedule(Options::Schedule::CASC_SAT);
      env.options->setOutputMode(Options::Output::SZS);
      env.options->setProof(Options::Proof::TPTP);
      env.options->setOutputAxiomNames(true);
      env.options->setNormalize(true);
      env.options->setRandomizeSeedForPortfolioWorkers(false);

      if (CASC::PortfolioMode::perform(env.options->slowness())) {
        vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      break;

    case Options::Mode::SMTCOMP:
      env.options->setIgnoreMissing(Options::IgnoreMissing::OFF);
      env.options->setInputSyntax(Options::InputSyntax::SMTLIB2);
      if(env.options->outputMode() != Options::Output::UCORE){
        env.options->setOutputMode(Options::Output::SMTCOMP);
      }
      env.options->setSchedule(Options::Schedule::SMTCOMP);
      env.options->setProof(Options::Proof::OFF);
      env.options->setNormalize(true);
      env.options->setRandomizeSeedForPortfolioWorkers(false);

      env.options->setMulticore(0); // use all available cores
      env.options->setTimeLimitInSeconds(1800);
      env.options->setStatistics(Options::Statistics::NONE);

      //TODO needed?
      // to prevent from terminating by time limit
      env.options->setTimeLimitInSeconds(100000);

      if (CASC::PortfolioMode::perform(env.options->slowness())){
        vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      else {
        cout << "unknown" << endl;
      }
      break;

    case Options::Mode::PORTFOLIO:
      env.options->setIgnoreMissing(Options::IgnoreMissing::WARN);

      if (CASC::PortfolioMode::perform(env.options->slowness())) {
        vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      }
      break;

    case Options::Mode::CASC_LTB: {
      bool learning = env.options->ltbLearning()!=Options::LTBLearning::OFF;
      try {
        if(learning){
          CASC::CLTBModeLearning::perform();
        }
        else{
          CASC::CLTBMode::perform();
        }
      } catch (Lib::SystemFailException& ex) {
        cerr << "Process " << getpid() << " received SystemFailException" << endl;
        ex.cry(cerr);
        cerr << " and will now die" << endl;
      }
      //we have processed the ltb batch file, so we can return zero
      vampireReturnValue = VAMP_RESULT_STATUS_SUCCESS;
      break;
    }
    case Options::Mode::MODEL_CHECK:
      modelCheckMode();
      break; 

    case Options::Mode::CLAUSIFY:
      clausifyMode(false);
      break;

    case Options::Mode::TCLAUSIFY:
      clausifyMode(true);
      break;

    case Options::Mode::OUTPUT:
      outputMode();
      break;

    case Options::Mode::PROFILE:
      profileMode();
      break;

    case Options::Mode::PREPROCESS:
    case Options::Mode::PREPROCESS2:
      preprocessMode(false);
      break;

    case Options::Mode::TPREPROCESS:
      preprocessMode(true);
      break;
    }
#if CHECK_LEAKS
    delete env.signature;
    env.signature = 0;
#endif
  }
#if VZ3
  catch (z3::exception& exception) {
    BYPASSING_ALLOCATOR;
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    if (outputAllowed()) {
      cout << "Z3 exception:\n" << exception.msg() << endl;
    }
    reportSpiderFail();
  }
#endif
  catch (UserErrorException& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    explainException(exception);
  } 
catch (Parse::TPTP::ParseErrorException& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    explainException(exception);
  }
  catch (Exception& exception) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    env.beginOutput();
    explainException(exception);
    env.endOutput();
  } catch (std::bad_alloc& _) {
    vampireReturnValue = VAMP_RESULT_STATUS_UNHANDLED_EXCEPTION;
    reportSpiderFail();
#if CHECK_LEAKS
    MemoryLeak::cancelReport();
#endif
    env.beginOutput();
    env.out() << "Insufficient system memory" << '\n';
    env.endOutput();
  }

  return vampireReturnValue;
} // main

