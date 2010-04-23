#ifndef RUNGEKUTTA_ODE_SOLVER_HH
#define RUNGEKUTTA_ODE_SOLVER_HH

// inlcude all used headers before, that they don not appear in DuneODE 

//- system includes 
#include <iostream>
#include <cmath>
#include <vector>
#include <pthread.h>
#include <cassert>
#include <sys/times.h>
#if HAVE_MPI
#include <mpi.h>
#endif

//- Dune includes 
#include <dune/fem/operator/common/spaceoperatorif.hh>
#include <dune/fem/solver/timeprovider.hh>
#include <dune/fem/io/parameter.hh>

//- include runge kutta ode solver 
#include <dune/fem/solver/rungekutta.hh>

// include headers of PARDG 
#include "pardg.hh"

namespace DuneODE {
struct ODEParameters
: public LocalParameter< ODEParameters, ODEParameters >
{ 
  ODEParameters() : 
    min_it( Parameter::getValue< int >( "fem.ode.miniterations" , 14 ) ),
    max_it( Parameter::getValue< int >( "fem.ode.maxiterations" , 16 ) ),
    sigma( Parameter::getValue< double >( "fem.ode.cflincrease" , 1.1 ) )
  {
  }
  virtual PARDG::IterativeLinearSolver *linearSolver(PARDG::Communicator & comm) const
  {
    int cycles = Parameter::getValue< int >( "fem.ode.gmrescycles" , 15 );
    PARDG::IterativeLinearSolver* solver = new PARDG::GMRES(comm,cycles);
    double tol = Parameter::getValue< double >( "fem.ode.solver.tolerance" , 1e-6 );
    static const std::string errorTypeTable[]
      = { "absolute", "relative" };
    int errorType = Parameter::getEnum( "fem.ode.solver.errormeassure", errorTypeTable, 0 );
    solver->set_tolerance(tol,(errorType==1));
    int maxIter = Parameter::getValue< int >( "fem.ode.solver.iterations" , 1000 );
    solver->set_max_number_of_iterations(maxIter);
    return solver;
  }
  virtual double tolerance() const
  {
    return Parameter::getValue< double >( "fem.ode.tolerance" , 1e-8 );
  }
  virtual int iterations() const
  {
    return Parameter::getValue< int >( "fem.ode.iterations" , 1000 );
  }
  virtual int verbose() const
  {
    static const std::string verboseTypeTable[]
      = { "none", "cfl", "full" };
    return Parameter::getValue< int >( "fem.ode.verbose" , 0 );
  }
  virtual bool cflFactor( const PARDG::ODESolver &ode,
                          const PARDG::IterativeLinearSolver &solver,
                          bool converged,
                          double &factor) const
  {
    const int iter = solver.number_of_iterations();
    factor = 1.;
    bool changed = false;
    if (converged) 
    {
      if (iter < min_it) 
      {
        factor = sigma;
        changed = true;
      }
      else if (iter > max_it) 
      {
        factor = (double)max_it/(sigma*(double)iter);
        changed = true;
      }
    }
    else
    {
      factor = 0.5;
      changed = true;
    }
    return changed;
  }
  const int min_it,max_it;
  const double sigma;
};
#ifdef USE_PARDG_ODE_SOLVER
template <class Operator>
class OperatorWrapper : public PARDG::Function 
{
  // type of discrete function 
  typedef typename Operator::DestinationType DestinationType;
  // type of discrete function space 
  typedef typename DestinationType :: DiscreteFunctionSpaceType SpaceType;
 public:
  //! constructor 
  OperatorWrapper(Operator& op) 
    : op_(op) , space_(op_.space()) 
  {}

  //! apply operator applies space operator and creates temporary
  //! discrete function using the memory from outside 
  void operator()(const double *u, double *f, int i = 0) 
  {
    // create fake argument 
    DestinationType arg("ARG",space_,u);
    // create fake destination 
    DestinationType dest("DEST",space_,f);
    
    // set actual time of iteration step
    op_.setTime( this->time() );
    
    // call operator apply 
    op_(arg,dest);
  }

  //! return size of argument 
  int dim_of_argument(int i = 0) const 
  { 
    if (i==0) return op_.space().size();
    else 
    {
      assert(0);
      abort();
      return -1;
    }
  }
  
  //! return size of destination  
  int dim_of_value(int i = 0) const 
  { 
    if (i==0) return op_.space().size();
    else 
    {
      assert(0);
      abort();
      return -1;
    }
  }
private:
  // operator to call 
  Operator& op_;
  // discrete function space 
  const SpaceType& space_;
};


/**
   @ingroup ODESolver
   @{
 **/

/* \brief Explicit ODE Solver base class */
template<class Operator>
class ExplTimeStepperBase 
{
  typedef typename Operator::DestinationType DestinationType; 
public:
  ExplTimeStepperBase(Operator& op, 
                      Dune::TimeProviderBase& tp, 
                      int pord, 
                      bool verbose) :
    ord_(pord),
    comm_(PARDG::Communicator::instance()),
    op_(op),
    expl_(op),
    ode_(0),
    initialized_(false)
  {
    switch (pord) {
      case 1: ode_ = new PARDG::ExplicitEuler(comm_,expl_); break;
      case 2: ode_ = new PARDG::ExplicitTVD2(comm_,expl_); break;
      case 3: ode_ = new PARDG::ExplicitTVD3(comm_,expl_); break;
      case 4: ode_ = new PARDG::ExplicitRK4(comm_,expl_); break;
      default : ode_ = new PARDG::ExplicitBulirschStoer(comm_,expl_,7);
                std::cerr << "Runge-Kutta method of this order not implemented.\n" 
                          << "Using 7-stage Bulirsch-Stoer scheme.\n"
                          << std::endl;
    }

    if(verbose)
    {
      ode_->DynamicalObject::set_output(cout);
    }
  }
 
  // initialize time step size 
  bool initialize (const DestinationType& U0) 
  {
    // initialized dt on first call
    if ( ! initialized_ )     
    {
      DestinationType tmp(U0);
      this->op_(U0,tmp);
      initialized_ = true;
      return true;
    }
    return false;
  }

  //! destructor  
  ~ExplTimeStepperBase() { delete ode_; }

  //! return reference to ode solver 
  PARDG::ODESolver& odeSolver() 
  {
    assert( ode_ );
    return *ode_;
  }
  void printmyInfo(string filename) const 
  {
    std::ostringstream filestream;
    filestream << filename;
    std::ofstream ofs(filestream.str().c_str(), std::ios::app);
    ofs << "Explicit ODE solver, steps: " << this->ord_ << "\n\n";
    ofs.close();
    //this->op_.printmyInfo(filename);
  }
  
protected:
  int ord_;
  PARDG::Communicator & comm_;
  const Operator& op_;
  OperatorWrapper<Operator> expl_;
  PARDG::ODESolver* ode_;
  bool initialized_;
};

//! ExplicitOdeSolver 
template<class DestinationImp>
class ExplicitOdeSolver : 
  public OdeSolverInterface<DestinationImp> ,
  public ExplTimeStepperBase<SpaceOperatorInterface<DestinationImp> >  
{
  typedef DestinationImp DestinationType; 
  typedef SpaceOperatorInterface<DestinationType> OperatorType;
  typedef ExplTimeStepperBase<OperatorType> BaseType; 
public:
  //! constructor 
  ExplicitOdeSolver(OperatorType& op, Dune :: TimeProviderBase &tp, int pord, bool verbose = false) :
    BaseType(op,tp,pord,verbose),
    timeProvider_(tp)
  {}

  //! destructor 
  virtual ~ExplicitOdeSolver() {}
 
  //! initialize solver 
  void initialize(const DestinationType& U0)
  {
    BaseType :: initialize (U0);
    timeProvider_.provideTimeStepEstimate( this->op_.timeStepEstimate() );
  }

  //! solve system 
  void solve(DestinationType& U0) 
  {
    // initialize 
    if( ! this->initialized_ ) 
    {
      DUNE_THROW(InvalidStateException,"ExplicitOdeSolver wasn't initialized before first call!");
    }
    
    // get dt 
    const double dt = timeProvider_.deltaT();
    
    // should be larger then zero 
    assert( dt > 0.0 );
    
    // get time 
    const double time = timeProvider_.time();

    // get leakPointer 
    double* u = U0.leakPointer();
    
    // call ode solver 
    const bool convergence = this->odeSolver().step(time, dt , u);

    // set time step estimate of operator 
    timeProvider_.provideTimeStepEstimate( this->op_.timeStepEstimate() );
    
    assert(convergence);
    if(!convergence) 
    {
      timeProvider_.invalidateTimeStep();
      std::cerr << "No Convergence of ExplicitOdeSolver! \n";
    }
  }

protected:
  Dune::TimeProviderBase& timeProvider_;
};

//////////////////////////////////////////////////////////////////
//
//  --ImplTimeStepperBase
//
//////////////////////////////////////////////////////////////////
template<class Operator>
class ImplTimeStepperBase
{
  typedef typename Operator :: DestinationType DestinationType; 
public:
  ImplTimeStepperBase(Operator& op, Dune :: TimeProviderBase &tp,
                      int pord,
                      const ODEParameters& parameter=ODEParameters()) :
    ord_(pord),
    comm_(PARDG::Communicator::instance()),
    op_(op),
    impl_(op),
    ode_(0),
    linsolver_(0),
    initialized_(false),
    verbose_(parameter.verbose()),
    param_(parameter.clone())
  {
    linsolver_ = parameter.linearSolver( comm_ );
    switch (pord) 
    {
      case 1: ode_ = new PARDG::ImplicitEuler(comm_,impl_); break;
      case 2: ode_ = new PARDG::Gauss2(comm_,impl_); break;
      case 3: ode_ = new PARDG::DIRK3(comm_,impl_); break;
      //case 4: ode_ = new PARDG::ExplicitRK4(comm,expl_); break;
      default : std::cerr << "Runge-Kutta method of this order not implemented" 
                          << std::endl;
                abort();
    }
    ode_->set_linear_solver(*linsolver_);
    ode_->set_tolerance( parameter.tolerance() );
    ode_->set_max_number_of_iterations( parameter.iterations() );
    
    if( verbose_ ==2 ) 
    {
      ode_->IterativeSolver::set_output(cout);
      ode_->DynamicalObject::set_output(cout);
    }
  }
  
  // initialize time step size 
  bool initialize (const DestinationType& U0) 
  {
    // initialized dt on first call
    if ( ! initialized_ )     
    {
      DestinationType tmp(U0);
      this->op_(U0,tmp);
      initialized_ = true;
      return true;
    }
    return false;
  }
  //! destructor 
  ~ImplTimeStepperBase() {delete ode_;delete linsolver_;}
  
  // return reference to ode solver 
  PARDG::DIRK& odeSolver() 
  {
    assert( ode_ );
    return *ode_;
  }
  void printmyInfo(string filename) const {
    std::ostringstream filestream;
    filestream << filename;
    std::ofstream ofs(filestream.str().c_str(), std::ios::app);
    ofs << "Implicit ODE solver, steps: " << this->ord_ << "\n\n";
    ofs.close();
    // this->op_.printmyInfo(filename);
  }
  
protected:
  int ord_;
  PARDG::Communicator & comm_;   
  const Operator& op_;
  OperatorWrapper<Operator> impl_;
  PARDG::DIRK* ode_;
  PARDG::IterativeLinearSolver* linsolver_;
  bool initialized_;
  int verbose_;
  const ODEParameters* param_;
};


///////////////////////////////////////////////////////
//
//  --ImplicitOdeSolver 
//
///////////////////////////////////////////////////////
template<class DestinationImp>
class ImplicitOdeSolver : 
  public OdeSolverInterface<DestinationImp> ,
  public ImplTimeStepperBase<SpaceOperatorInterface<DestinationImp> > 
{
  typedef DestinationImp DestinationType;
  typedef SpaceOperatorInterface<DestinationImp> OperatorType;
  typedef ImplTimeStepperBase<OperatorType> BaseType;

private:
  Dune :: TimeProviderBase &timeProvider_;
  double cfl_;

public:
  ImplicitOdeSolver(OperatorType& op, Dune::TimeProviderBase& tp,
                    int pord, bool verbose ) DUNE_DEPRECATED :
    BaseType(op,tp,pord),
    timeProvider_(tp),
    cfl_(1.0)
  {
  }
  ImplicitOdeSolver(OperatorType& op, Dune::TimeProviderBase& tp,
                    int pord,
                    const ODEParameters& parameter=ODEParameters()) :
    BaseType(op,tp,pord),
    timeProvider_(tp),
    cfl_(1.0)
  {
  }

  virtual ~ImplicitOdeSolver() {}
  
  //! initialize solver 
  void initialize(const DestinationType& U0)
  {
    // initialize solver 
    BaseType :: initialize (U0);

    // set time step estimate of operator 
    timeProvider_.provideTimeStepEstimate( cfl_ * this->op_.timeStepEstimate() );
  }

  //! solve 
  void solve(DestinationType& U0) 
  {
    // initialize 
    if( ! this->initialized_ ) 
    {
      DUNE_THROW(InvalidStateException,"ImplicitOdeSolver wasn't initialized before first call!");
    }

    const double dt   = timeProvider_.deltaT();
    assert( dt > 0.0 );
    const double time = timeProvider_.time();

     // get pointer to solution
    double* u = U0.leakPointer();
      
    const bool convergence = this->odeSolver().step(time , dt , u);

    double factor;
    bool changed = BaseType::param_->cflFactor(this->odeSolver(),
                                               *(this->linsolver_),
                                               convergence,factor);
    cfl_ *= factor;
    if (convergence)
    {
      timeProvider_.provideTimeStepEstimate( cfl_ * this->op_.timeStepEstimate() );

      if( changed && this->verbose_>=1 )
        derr << " New cfl number is: "<< cfl_ << " (number of iterations ("
             << "linear: " << this->linsolver_->number_of_iterations() 
             << ", ode: " << this->odeSolver().number_of_iterations()
             << ")"
             << std::endl;
    } 
    else 
    {
      timeProvider_.provideTimeStepEstimate( cfl_ * dt );
      timeProvider_.invalidateTimeStep();
     
      // output only in verbose mode 
      if( this->verbose_>=1 )
      {
        derr << "No convergence: New cfl number is "<< cfl_ << std :: endl;
      }
    }
    this->linsolver_->reset_number_of_iterations();
  }

}; // end ImplicitOdeSolver

//////////////////////////////////////////////////////////////////
//
//  --SemiImplTimeStepperBase
//
//////////////////////////////////////////////////////////////////
template<class OperatorExpl, class OperatorImpl>
class SemiImplTimeStepperBase
{
  typedef typename OperatorExpl :: DestinationType DestinationType; 
public:
  SemiImplTimeStepperBase(OperatorExpl& explOp, OperatorImpl & implOp, Dune :: TimeProviderBase &tp, 
                      int pord, 
                      const ODEParameters& parameter=ODEParameters()) :
    ord_(pord),
    comm_(PARDG::Communicator::instance()),
    explOp_(explOp),
    implOp_(implOp),
    expl_(explOp),
    impl_(implOp),
    ode_(0),
    linsolver_(0),
    initialized_(false),
    verbose_(parameter.verbose()),
    param_(parameter.clone())
  {
    linsolver_ = parameter.linearSolver( comm_ );
    switch (pord) {
      case 1: ode_=new PARDG::SemiImplicitEuler(comm_,impl_,expl_); break;
      case 2: ode_=new PARDG::IMEX_SSP222(comm_,impl_,expl_); break;
      case 3: ode_=new PARDG::SIRK33(comm_,impl_,expl_); break;
      default : std::cerr << "Runge-Kutta method of this order not implemented" 
                          << std::endl;
                abort();
    }
    ode_->set_linear_solver(*linsolver_);
    ode_->set_tolerance( parameter.tolerance() );
    ode_->set_max_number_of_iterations( parameter.iterations() );
    
    if( verbose_==2 ) 
    {
      ode_->IterativeSolver::set_output(cout);
      ode_->DynamicalObject::set_output(cout);
    }
  }
  
  // initialize time step size 
  bool initialize (const DestinationType& U0) 
  {
    // initialized dt on first call
    if ( ! initialized_ )     
    {
      DestinationType tmp(U0);
      this->explOp_(U0,tmp);
      initialized_ = true;
      return true;
    }
    return false;
  }
  //! destructor 
  ~SemiImplTimeStepperBase() {delete ode_;delete linsolver_;}
  
  // return reference to ode solver 
  PARDG::SIRK& odeSolver() 
  {
    assert( ode_ );
    return *ode_;
  }
  void printmyInfo(string filename) const {
    std::ostringstream filestream;
    filestream << filename;
    std::ofstream ofs(filestream.str().c_str(), std::ios::app);
    ofs << "Semi Implicit ODE solver, steps: " << this->ord_ << "\n\n";
    ofs.close();
    // this->op_.printmyInfo(filename);
  }
  
protected:
  int ord_;
  PARDG::Communicator & comm_;   
  const OperatorExpl& explOp_;
  const OperatorImpl& implOp_;
  OperatorWrapper<OperatorExpl> expl_;
  OperatorWrapper<OperatorImpl> impl_;
  PARDG::SIRK* ode_;
  PARDG::IterativeLinearSolver* linsolver_;
  bool initialized_;
  int verbose_;
  const ODEParameters* param_;
};


///////////////////////////////////////////////////////
//
//  --SemiImplicitOdeSolver 
//
///////////////////////////////////////////////////////
template<class DestinationImp>
class SemiImplicitOdeSolver : 
  public OdeSolverInterface<DestinationImp> ,
  public SemiImplTimeStepperBase<SpaceOperatorInterface<DestinationImp>, SpaceOperatorInterface<DestinationImp> > 
{
  typedef DestinationImp DestinationType;
  typedef SpaceOperatorInterface<DestinationImp> OperatorType;
  typedef SemiImplTimeStepperBase<OperatorType, OperatorType> BaseType;

protected:
  Dune :: TimeProviderBase &timeProvider_;
  double cfl_;

public:
  SemiImplicitOdeSolver(OperatorType& explOp, OperatorType& implOp, Dune::TimeProviderBase& tp,
                    int pord, bool verbose ) DUNE_DEPRECATED :
    BaseType(explOp,implOp,tp,pord),
    timeProvider_(tp),
    cfl_(1.0)
  {
  }
  SemiImplicitOdeSolver(OperatorType& explOp, OperatorType& implOp, Dune::TimeProviderBase& tp,
                    int pord,
                    const ODEParameters& parameter=ODEParameters()) :
    BaseType(explOp,implOp,tp,pord,parameter),
    timeProvider_(tp),
    cfl_(1.0)
  {
  }

  virtual ~SemiImplicitOdeSolver() {}
  
  //! initialize solver 
  void initialize(const DestinationType& U0)
  {
    // initialize solver 
    BaseType :: initialize (U0);

    // set time step estimate of operator 
    timeProvider_.provideTimeStepEstimate( cfl_ * this->explOp_.timeStepEstimate() );
  }

  //! solve 
  void solve(DestinationType& U0) 
  {
    // initialize 
    if( ! this->initialized_ ) 
    {
      DUNE_THROW(InvalidStateException,"ImplicitOdeSolver wasn't initialized before first call!");
    }
    
    const double dt   = timeProvider_.deltaT();
    assert( dt > 0.0 );
    const double time = timeProvider_.time();

     // get pointer to solution
    double* u = U0.leakPointer();
      
    const bool convergence = this->odeSolver().step(time , dt , u);

    double factor;
    bool changed = BaseType::param_->cflFactor(this->odeSolver(),
                                               *(this->linsolver_),
                                               convergence,factor);
    cfl_ *= factor;
    if (convergence)
    {
      timeProvider_.provideTimeStepEstimate( cfl_ * this->op_.timeStepEstimate() );

      if( changed && this->verbose_>=1 )
        derr << " New cfl number is: "<< cfl_ << " (number of iterations: "
             << this->linsolver_->number_of_iterations() 
             << std::endl;
    } 
    else 
    {
      timeProvider_.provideTimeStepEstimate( cfl_ * dt );
      timeProvider_.invalidateTimeStep();
     
      // output only in verbose mode 
      if( this->verbose_>=1 )
      {
        derr << "No convergence: New cfl number is "<< cfl_ << std :: endl;
      }
    }
    this->linsolver_->reset_number_of_iterations();
  }

}; // end SemiImplicitOdeSolver

#endif // USE_PARDG_ODE_SOLVER
 
/**
 @} 
**/

} // end namespace DuneODE

#undef USE_EXTERNAL_BLAS
#endif
