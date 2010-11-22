
#include <iostream>
#include <sstream>
#include <map>

#include "Api/FormulaBuilder.hpp"
#include "Api/Problem.hpp"

#include "Lib/DHSet.hpp"
#include "Lib/Environment.hpp"

#include "Kernel/Formula.hpp"
#include "Kernel/FormulaUnit.hpp"
#include "Kernel/Inference.hpp"
#include "Kernel/Signature.hpp"
#include "Kernel/Term.hpp"
#include "Kernel/Unit.hpp"

#include "Shell/Options.hpp"
#include "Shell/Property.hpp"
#include "Shell/Preprocess.hpp"

#include "Test/UnitTesting.hpp"

#define UNIT_ID ite
UT_CREATE;

using namespace std;



TEST_FUN(iteFormula)
{
  using namespace Api;
  try {
    FormulaBuilder api(true);

    Var xv = api.var("X"); // variable x
    Var yv = api.var("Y"); // variable y
    Term x =  api.varTerm(xv); // term x
    Term y =  api.varTerm(yv); // term y
    Function f = api.function("f",1);
    Term fx = api.term(f,x); // f(x)
    Term fy = api.term(f,y); // f(y)
    Predicate p=api.predicate("p",3);
    Formula pXfXfY = api.formula(p,x,fx,fy); // p(X,f(X),f(Y))

//    Predicate q1=api.predicate("q1",0);
    Predicate q2=api.predicate("q2",0);
    Predicate q3=api.predicate("q3",0);
    Formula f1 = api.formula(FormulaBuilder::ITE,pXfXfY,api.formula(q2),api.formula(q3));
    Formula f2 = api.formula(FormulaBuilder::ITE,f1,api.formula(q2),api.formula(q3));
    AnnotatedFormula af1 = api.annotatedFormula(f2, FormulaBuilder::ASSUMPTION);

    cout << af1 << endl;

    Problem p1;
    p1.addFormula(af1);
    Problem p2=p1.clausify();

    AnnotatedFormulaIterator fit1=p2.formulas();
    while(fit1.hasNext()) {
      cout<<fit1.next()<<endl;
    }
  }
  catch (FormulaBuilderException e)
  {
    cout<< "Exception: "<<e.msg()<<endl;
  }
}

TEST_FUN(iteTerm)
{
  using namespace Kernel;
  using namespace Shell;

  TermList x0(0,false); //X0
  TermList x1(1,false); //X1
  unsigned p=env.signature->addPredicate("p",2);
  unsigned q=env.signature->addPredicate("q",1);
  unsigned f=env.signature->addFunction("f",2);
  unsigned g=env.signature->addFunction("g",1);

  TermList f01=TermList(Term::create2(f, x0, x1));
  Literal* p01=Literal::create2(p, true, x0, x1);
  Literal* p10=Literal::create2(p, true, x1, x0);
  TermList g0=TermList(Term::create1(g, x0));
  TermList g1=TermList(Term::create1(g, x1));
  Literal* pg0x0=Literal::create2(p, true, g0, x0);
  TermList gITE=TermList(Term::createTermITE(new AtomicFormula(pg0x0), g0, g1));

  TermList gf01=TermList(Term::create1(g, f01));
  TermList tlet=TermList(Term::createTermLet(f01, gITE, gf01));  //term let in term
  TermList tlet2=TermList(Term::createFormulaLet(p01, new AtomicFormula(p10), tlet));  //term let in term

  Literal* x0EQtlet=Literal::createEquality(true, x0, tlet2);

  Literal* x0EQx1=Literal::createEquality(true, x0, x1);
  Formula* fletTgt = new BinaryFormula(IMP, new AtomicFormula(p01), new AtomicFormula(x0EQx1));
  Formula* flet = new FormulaLetFormula(p01, fletTgt, new AtomicFormula(x0EQtlet));  //formula let in formula

  Formula* flet2 = new TermLetFormula(g1, x1, flet);  //term let in formula
  cout << flet2->toString() <<endl;


  Formula* simple1 = new AtomicFormula(Literal::create1(q, true, gITE));
  Formula* simple2 = new TermLetFormula(g1, x1, simple1);
  Formula* simple3 = new FormulaLetFormula(p01, new AtomicFormula(x0EQx1), simple2);
  Formula* simple4 = new FormulaLetFormula(p01, fletTgt, simple2);

  TermList t1 = TermList(Term::createTermLet(g1,x1,gITE));
  TermList t2 = TermList(Term::createFormulaLet(p01,new AtomicFormula(x0EQx1),t1));
  TermList t3 = TermList(Term::createFormulaLet(p01,flet,t1));

  FormulaUnit* u = new FormulaUnit(flet2, new Inference(Inference::INPUT), Unit::CONJECTURE);

  FormulaUnit* us1 = new FormulaUnit(simple1, new Inference(Inference::INPUT), Unit::AXIOM);
  FormulaUnit* us2 = new FormulaUnit(simple2, new Inference(Inference::INPUT), Unit::AXIOM);
  FormulaUnit* us3 = new FormulaUnit(simple3, new Inference(Inference::INPUT), Unit::AXIOM);
  FormulaUnit* us4 = new FormulaUnit(simple4, new Inference(Inference::INPUT), Unit::AXIOM);

  FormulaUnit* ut1 = new FormulaUnit(new AtomicFormula(Literal::create1(q, true, t1)), new Inference(Inference::INPUT), Unit::AXIOM);
  FormulaUnit* ut2 = new FormulaUnit(new AtomicFormula(Literal::create1(q, true, t2)), new Inference(Inference::INPUT), Unit::AXIOM);
  FormulaUnit* ut3 = new FormulaUnit(new AtomicFormula(Literal::create1(q, true, t3)), new Inference(Inference::INPUT), Unit::AXIOM);

  UnitList* prob = 0;

  UnitList::push(u, prob);
  UnitList::push(ut3, prob);
  UnitList::push(ut2, prob);
  UnitList::push(ut1, prob);
  UnitList::push(us4, prob);
  UnitList::push(us3, prob);
  UnitList::push(us2, prob);
  UnitList::push(us1, prob);

  UnitList::Iterator uit0(prob);
  while(uit0.hasNext()) {
    cout << uit0.next()->toString() <<endl;
  }

  Property property;
  property.scan(prob);
  env.options->setUnusedPredicateDefinitionRemoval(false);
  Preprocess prep(property, *env.options);

  prep.preprocess(prob);

  UnitList::Iterator uit(prob);
  while(uit.hasNext()) {
    cout << uit.next()->toString() <<endl;
  }

}