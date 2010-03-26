/**
 * @file CodeTreeInterfaces.cpp
 * Implements indexing structures that use code trees.
 *
 */

#include "../Lib/Allocator.hpp"
#include "../Lib/Recycler.hpp"

#include "../Kernel/Clause.hpp"
#include "../Kernel/FlatTerm.hpp"
#include "../Kernel/Renaming.hpp"
#include "../Kernel/SubstHelper.hpp"
#include "../Kernel/Term.hpp"

#include "CodeTreeInterfaces.hpp"

namespace Indexing
{

using namespace Lib;
using namespace Kernel;

/**
 * A functor object that reveives a @void* as an argument, casts it
 * to type @b T* and checks whether the references object is equal to
 * the object pointed to by the pointer given in the constructor.
 */
template<typename T>
struct EqualityDerefFn
: public CodeTree::SuccessDataMatchingFn
{
  EqualityDerefFn(T* o) : obj(o) {}

  bool operator() (void* arg)
  {
    return *obj==*static_cast<T*>(arg);
  }

private:
  T* obj;
};

template<typename T>
struct EqualityFn
: public CodeTree::SuccessDataMatchingFn
{
  EqualityFn(T o) : obj(o) {}

  bool operator() (T arg)
  {
    return obj==arg;
  }

private:
  T obj;
};

class CodeTreeSubstitution
: public ResultSubstitution
{
public:
  CodeTreeSubstitution(TermCodeTree::TermEContext* ctx, Renaming* resultNormalizer)
  : _ctx(ctx), _resultNormalizer(resultNormalizer),
  _applicator(0)
  {}
  ~CodeTreeSubstitution()
  {
    if(_applicator) {
      delete _applicator;
    }
  }

  CLASS_NAME("CodeTreeSubstitution");
  USE_ALLOCATOR(CodeTreeSubstitution);

  TermList applyToBoundResult(TermList t)
  { return SubstHelper::apply(t, *getApplicator()); }

  Literal* applyToBoundResult(Literal* lit)
  { return SubstHelper::apply(lit, *getApplicator()); }

  bool isIdentityOnQueryWhenResultBound() {return true;}
private:
  struct Applicator
  {
    inline
    Applicator(TermCodeTree::TermEContext* ctx, Renaming* resultNormalizer)
    : _ctx(ctx), _resultNormalizer(resultNormalizer) {}

    TermList apply(unsigned var)
    {
      ASS(_resultNormalizer->contains(var));
      unsigned nvar=_resultNormalizer->get(var);
      TermList res=_ctx->bindings[nvar];
      ASS(res.isTerm()||res.isOrdinaryVar());
      ASSERT_VALID(res);
      return res;
    }

    CLASS_NAME("CodeTreeSubstitution::Applicator");
    USE_ALLOCATOR(Applicator);
  private:
    TermCodeTree::TermEContext* _ctx;
    Renaming* _resultNormalizer;
  };

  Applicator* getApplicator()
  {
    if(!_applicator) {
      _applicator=new Applicator(_ctx, _resultNormalizer);
    }
    return _applicator;
  }

  TermCodeTree::TermEContext* _ctx;
  Renaming* _resultNormalizer;
  Applicator* _applicator;
};

///////////////////////////////////////

struct CodeTreeTIS::TermInfo
{
  TermInfo(TermList t, Literal* lit, Clause* cls)
  : t(t), lit(lit), cls(cls) {}

  inline bool operator==(const TermInfo& o)
  { return cls==o.cls && t==o.t && lit==o.lit; }

  inline bool operator!=(const TermInfo& o)
  { return !(*this==o); }

  CLASS_NAME("CodeTreeTIS::TermInfo");
  USE_ALLOCATOR(TermInfo);

  TermList t;
  Literal* lit;
  Clause* cls;
};

class CodeTreeTIS::ResultIterator
: public IteratorCore<TermQueryResult>
{
public:
  ResultIterator(CodeTreeTIS* tree, TermList t, bool retrieveSubstitutions)
  : _retrieveSubstitutions(retrieveSubstitutions),
    _found(0), _finished(false), _tree(tree)
  {
    Recycler::get(_ctx);
    _ctx->init(t,&_tree->_ct);

    if(_retrieveSubstitutions) {
      Recycler::get(_resultNormalizer);
      _subst=new CodeTreeSubstitution(_ctx, _resultNormalizer);
    }
  }

  ~ResultIterator()
  {
    _ctx->deinit(&_tree->_ct);
    Recycler::release(_ctx);
    if(_retrieveSubstitutions) {
      Recycler::release(_resultNormalizer);
      delete _subst;
    }
  }

  CLASS_NAME("CodeTreeTIS::ResultIterator");
  USE_ALLOCATOR(ResultIterator);

  bool hasNext()
  {
    CALL("CodeTreeTIS::ResultIterator::hasNext");

    if(_found) {
      return true;
    }
    if(_finished) {
      return false;
    }
    void* data;
    if(TermCodeTree::next(*_ctx, data)) {
      _found=static_cast<TermInfo*>(data);
    }
    else {
      _found=0;
      _finished=true;
    }
    return _found;
  }

  TermQueryResult next()
  {
    CALL("CodeTreeTIS::ResultIterator::next");
    ASS(_found);

    TermQueryResult res;
    if(_retrieveSubstitutions) {
      _resultNormalizer->reset();
      _resultNormalizer->normalizeVariables(_found->t);
      res=TermQueryResult(_found->t, _found->lit, _found->cls,
	  ResultSubstitutionSP(_subst,true));
    }
    else {
      res=TermQueryResult(_found->t, _found->lit, _found->cls);
    }
    _found=0;
    return res;
  }
private:

  CodeTreeSubstitution* _subst;
  Renaming* _resultNormalizer;
  bool _retrieveSubstitutions;
  TermInfo* _found;
  bool _finished;
  CodeTreeTIS* _tree;
  TermCodeTree::TermEContext* _ctx;
};

void CodeTreeTIS::insert(TermList t, Literal* lit, Clause* cls)
{
  CALL("CodeTreeTIS::insert");
  ASS_EQ(_ct._initEContextCounter,0); //index must not be modified while traversed

  static CodeTree::CodeStack code;
  code.reset();

  _ct.compile(t,code);
  ASS(code.top().isSuccess());

  //assign the term info
  code.top().result=new TermInfo(t,lit,cls);
  ASS(code.top().isSuccess());

  _ct.incorporate(code);
}

void CodeTreeTIS::remove(TermList t, Literal* lit, Clause* cls)
{
  CALL("CodeTreeTIS::remove");
  ASS_EQ(_ct._initEContextCounter,0); //index must not be modified while traversed

  TermInfo ti(t,lit,cls);

  static TermCodeTree::TermEContext ctx;
  ctx.init(t, &_ct);

  EqualityDerefFn<TermInfo> succFn(&ti);
  CodeTree::remove(ctx, &_ct, &succFn);

  ctx.deinit(&_ct);
}

TermQueryResultIterator CodeTreeTIS::getGeneralizations(TermList t, bool retrieveSubstitutions)
{
  CALL("CodeTreeTIS::getGeneralizations");

  if(_ct.isEmpty()) {
    return TermQueryResultIterator::getEmpty();
  }

  return vi( new ResultIterator(this, t, retrieveSubstitutions) );
}

bool CodeTreeTIS::generalizationExists(TermList t)
{
  CALL("CodeTreeTIS::generalizationExists");

  if(_ct.isEmpty()) {
    return false;
  }

  static TermCodeTree::TermEContext ctx;
  ctx.init(t, &_ct);

  void* aux;
  bool res=TermCodeTree::next(ctx, aux);

  ctx.deinit(&_ct);
  return res;
}

////////////////////////////////////////

struct CodeTreeLIS::LiteralInfo
{
  LiteralInfo(Literal* lit, Clause* cls)
  : lit(lit), cls(cls) {}

  inline bool operator==(const LiteralInfo& o)
  { return cls==o.cls && lit==o.lit; }

  inline bool operator!=(const LiteralInfo& o)
  { return !(*this==o); }

  CLASS_NAME("CodeTreeLIS::LiteralInfo");
  USE_ALLOCATOR(LiteralInfo);

  Literal* lit;
  Clause* cls;
};

class CodeTreeLIS::ResultIterator
: public IteratorCore<SLQueryResult>
{
public:
  ResultIterator(CodeTreeLIS* tree, Literal* lit, bool complementary,
      bool retrieveSubstitutions)
  : _canReorder(lit->commutative()),
    _retrieveSubstitutions(retrieveSubstitutions),
    _found(0), _finished(false), _tree(tree)
  {
    _flatTerm=FlatTerm::create(lit);
    if(complementary) {
      _flatTerm->changeLiteralPolarity();
    }

    Recycler::get(_ctx);
    _ctx->init(_flatTerm,&_tree->_ct);

    if(_retrieveSubstitutions) {
      Recycler::get(_resultNormalizer);
      _subst=new CodeTreeSubstitution(_ctx, _resultNormalizer);
    }
  }

  ~ResultIterator()
  {
    _ctx->deinit(&_tree->_ct);
    Recycler::release(_ctx);
    if(_retrieveSubstitutions) {
      Recycler::release(_resultNormalizer);
      delete _subst;
    }
    _flatTerm->destroy();
  }

  CLASS_NAME("CodeTreeLIS::ResultIterator");
  USE_ALLOCATOR(ResultIterator);

  bool hasNext()
  {
    CALL("CodeTreeLIS::ResultIterator::hasNext");

    if(_found) {
      return true;
    }
    if(_finished) {
      return false;
    }

  retry_search:
    void* data;
    if(TermCodeTree::next(*_ctx, data)) {
      _found=static_cast<LiteralInfo*>(data);
    }
    else {
      if(_canReorder) {
	_canReorder=false;
	_ctx->deinit(&_tree->_ct);
	_flatTerm->swapCommutativePredicateArguments();
	_ctx->init(_flatTerm,&_tree->_ct);
	goto retry_search;
      }
      _found=0;
      _finished=true;
    }
    return _found;
  }

  SLQueryResult next()
  {
    CALL("CodeTreeLIS::ResultIterator::next");
    ASS(_found);

    SLQueryResult res;
    if(_retrieveSubstitutions) {
      _resultNormalizer->reset();
      _resultNormalizer->normalizeVariables(_found->lit);
      res=SLQueryResult(_found->lit, _found->cls,
	  ResultSubstitutionSP(_subst,true));
    }
    else {
      res=SLQueryResult(_found->lit, _found->cls);
    }
    _found=0;
    return res;
  }
private:
  /** True if the query literal is commutative and we still
   * may try swaping its arguments */
  bool _canReorder;
  FlatTerm* _flatTerm;
  CodeTreeSubstitution* _subst;
  Renaming* _resultNormalizer;
  bool _retrieveSubstitutions;
  LiteralInfo* _found;
  bool _finished;
  CodeTreeLIS* _tree;
  TermCodeTree::TermEContext* _ctx;
};

void CodeTreeLIS::insert(Literal* lit, Clause* cls)
{
  CALL("CodeTreeLIS::insert");
  ASS_EQ(_ct._initEContextCounter,0); //index must not be modified while traversed

  static CodeTree::CodeStack code;
  code.reset();

  _ct.compile(lit,code);
  ASS(code.top().isSuccess());

  //assign the term info
  code.top().result=new LiteralInfo(lit,cls);
  ASS(code.top().isSuccess());

  _ct.incorporate(code);
}

void CodeTreeLIS::remove(Literal* lit, Clause* cls)
{
  CALL("CodeTreeLIS::remove");
  ASS_EQ(_ct._initEContextCounter,0); //index must not be modified while traversed

  LiteralInfo li(lit,cls);

  static TermCodeTree::TermEContext ctx;
  ctx.init(lit, &_ct);

  EqualityDerefFn<LiteralInfo> succFn(&li);
  CodeTree::remove(ctx, &_ct, &succFn);

  ctx.deinit(&_ct);
}

SLQueryResultIterator CodeTreeLIS::getGeneralizations(Literal* lit,
	  bool complementary, bool retrieveSubstitutions)
{
  CALL("CodeTreeLIS::getGeneralizations");

  if(_ct.isEmpty()) {
    return SLQueryResultIterator::getEmpty();
  }

  return vi( new ResultIterator(this, lit, complementary,
      retrieveSubstitutions) );
}


///////////////////////////////////////

class CodeTreeSubsumptionIndex::SubsumptionIterator
: public IteratorCore<Clause*>
{
public:
  SubsumptionIterator(CodeTreeSubsumptionIndex* tree, Clause* cl)
  : _found(0), _finished(false), _tree(tree)
  {
    Recycler::get(_ctx);
    _ctx->init(cl,&_tree->_ct);
  }

  ~SubsumptionIterator()
  {
    _ctx->deinit(&_tree->_ct);
    Recycler::release(_ctx);
  }

  CLASS_NAME("CodeTreeSubsumptionIndex::SubsumptionIterator");
  USE_ALLOCATOR(SubsumptionIterator);

  bool hasNext()
  {
    CALL("CodeTreeSubsumptionIndex::SubsumptionIterator::hasNext");

    if(_found) {
      return true;
    }
    if(_finished) {
      return false;
    }

    void* data;
    if(ClauseCodeTree::next(*_ctx, data)) {
      _found=static_cast<Clause*>(data);
    }
    else {
      _found=0;
      _finished=true;
    }
    return _found;
  }

  Clause* next()
  {
    CALL("CodeTreeSubsumptionIndex::SubsumptionIterator::next");
    ASS(_found);

    Clause* res=_found;
    _found=0;
    return res;
  }
private:
  Clause* _found;
  bool _finished;
  CodeTreeSubsumptionIndex* _tree;
  ClauseCodeTree::ClauseEContext* _ctx;
};

void CodeTreeSubsumptionIndex::handleClause(Clause* c, bool adding)
{
  CALL("CodeTreeSubsumptionIndex::handleClause");
  ASS_EQ(_ct._initEContextCounter,0); //index must not be modified while traversed

  TimeCounter tc(TC_FORWARD_SUBSUMPTION_INDEX_MAINTENANCE);

  if(adding) {
    static CodeTree::CodeStack code;
    code.reset();

    _ct.compile(c,code);
    ASS(code.top().isSuccess());

    //assign the term info
    code.top().result=c;
    ASS(code.top().isSuccess());

    _ct.incorporate(code);
  }
  else {
    static ClauseCodeTree::ClauseEContext ctx;
    ctx.init(c, &_ct);

    EqualityFn<void*> succFn(c);
    CodeTree::remove(ctx, &_ct, &succFn);

    ctx.deinit(&_ct);
  }
}

ClauseIterator CodeTreeSubsumptionIndex::getSubsumingClauses(Clause* c)
{
  CALL("CodeTreeSubsumptionIndex::getSubsumingClauses");

  if(_ct.isEmpty()) {
    return ClauseIterator::getEmpty();
  }

  return vi( new SubsumptionIterator(this, c) );
}


}






























