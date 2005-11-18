#include <iostream>
#include <cmath>
#include "../../../misc/timeutility.hh"

 #include "ode/function.hpp"
 #include "ode/ode_solver.hpp"
 #include "ode/linear_solver.hpp"
 #include "ode/bulirsch_stoer.cpp"  
 #include "ode/iterative_solver.cpp"  
 #include "ode/ode_solver.cpp"     
 #include "ode/sirk.cpp"   
// #include "ode/communicator.cpp"    
 #include "ode/matrix.cpp"            
 #include "ode/qr_solver.cpp"   
 #include "ode/ssp.cpp"
 #include "ode/dirk.cpp"     
 #include "ode/runge_kutta.cpp"
 #include "ode/vector.cpp"


namespace DuneODE {
  using namespace Dune;
using namespace std;

template <class Operator>
class OperatorWrapper : public DuneODE::Function {
 public:
  OperatorWrapper(const Operator& op) : op_(op) {}
  void operator()(const double *u, double *f, int i = 0) {
    typename Operator::DestinationType *arg 
      = new typename Operator::DestinationType ("ARG",op_.space(),u);
    typename Operator::DestinationType *dest
      = new typename Operator::DestinationType ("DEST",op_.space(),f);
    op_(*arg,*dest);
  }
  int dim_of_argument(int i = 0) const 
  { 
    if (i==0) return op_.space().size();
    else assert(0);
  }
  int dim_of_value(int i = 0) const 
  { 
    if (i==0) return op_.space().size();
    else assert(0);
  }
 private:
  const Operator& op_;
};
template<class Operator>
class ExplTimeStepper : public TimeProvider {
  typename Operator::DestinationType U1,U2;
 public:
  ExplTimeStepper(Operator& op,int pord,double cfl) :
    U1("U1",op.space()),
    U2("U2",op.space()),
    op_(op),
    expl_(op),
    ode_(0),
    cfl_(cfl),
    savetime_(0.1), savestep_(1)
  {
    op.timeProvider(this);
    op(U1,U2);
    switch (pord) {
    case 1: ode_=new ExplicitEuler(comm,expl_); break;
    case 2: ode_=new ExplicitTVD2(comm,expl_); break;
    case 3: ode_=new ExplicitTVD3(comm,expl_); break;
    case 4: ode_=new ExplicitRK4(comm,expl_); break;
    default : std::cerr << "Runge-Kutta method of this order not implemented" 
			<< std::endl;
      abort();
    }
    ode_->DynamicalObject::set_output(cout);
  }
  ~ExplTimeStepper() {delete ode_;}
  double solve(typename Operator::DestinationType& U0) {
    resetTimeStepEstimate();
    double t=time();
    double dt=0.0019; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    double* u=U0.leakPointer();
    const bool convergence = ode_->step(t, dt, u);
    assert(convergence);
    setTime(t+dt);
    return time();
  }
  void printGrid(int nr, 
		 const typename Operator::DestinationType& U) {
    if (time()>savetime_) {
      printSGrid(time(),savestep_*10+nr,op_.space(),U);
      ++savestep_;
      savetime_+=0.1;
    }
  }
 private:
  DuneODE::Communicator comm;
  const Operator& op_;
  OperatorWrapper<Operator> expl_;
  DuneODE::ODESolver* ode_;
double cfl_;
  int savestep_;
  double savetime_;
};
template<class Operator>
class ImplTimeStepper : public TimeProvider {
  typename Operator::DestinationType U1,U2;
 public:
  ImplTimeStepper(Operator& op,int pord,double cfl) :
    U1("U1",op.space()),
    U2("U2",op.space()),
    op_(op),
    expl_(op),
    ode_(0),
    linsolver_(comm),
    cfl_(cfl),
    savetime_(0.1), savestep_(1)
  {
    op.timeProvider(this);
    op(U1,U2);
    linsolver_.set_tolerance(1.0e-8);
    linsolver_.set_max_number_of_iterations(1000);
    switch (pord) {
    case 1: ode_=new ImplicitEuler(comm,expl_); break;
      //case 2: ode_=new ExplicitTVD2(comm,expl_); break;
      //case 3: ode_=new ExplicitTVD3(comm,expl_); break;
      //case 4: ode_=new ExplicitRK4(comm,expl_); break;
    default : std::cerr << "Runge-Kutta method of this order not implemented" 
			<< std::endl;
      abort();
    }
    ode_->set_linear_solver(linsolver_);
    ode_->set_tolerance(1.0e-8);
    ode_->IterativeSolver::set_output(cout);
    ode_->DynamicalObject::set_output(cout);
  }
  ~ImplTimeStepper() {delete ode_;}
  double solve(typename Operator::DestinationType& U0) {
    resetTimeStepEstimate();
    double t=time();
    double dt=0.0019; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    double* u=U0.leakPointer();
    const bool convergence = ode_->step(t, dt, u);
    assert(convergence);
    setTime(t+dt);
    return time();
  }
  void printGrid(int nr, 
		 const typename Operator::DestinationType& U) {
    if (1 || time()>savetime_) {
      printSGrid(time(),savestep_*10+nr,op_.space(),U);
      ++savestep_;
      savetime_+=0.1;
    }
  }
 private:
  DuneODE::Communicator comm;	  
  const Operator& op_;
  OperatorWrapper<Operator> expl_;
  DuneODE::DIRK* ode_;
  DuneODE::GMRES<20> linsolver_;
  double cfl_;
  int savestep_;
  double savetime_;
};
template<class Operator>
class ExplRungeKutta : public TimeProvider {
 public:
  enum {maxord=10};
  typedef typename Operator::SpaceType SpaceType;
  typedef typename Operator::DestinationType DestinationType;
 private:
  double cfl_;
  double **a;
  double *b;
  double *c;
  int ord_;
  std::vector<DestinationType*> Upd;
public:
  ExplRungeKutta(Operator& op,int pord,double cfl) :
    op_(op),
    cfl_(cfl), ord_(pord), Upd(0),
    savetime_(0.1), savestep_(1)
  {
    op.timeProvider(this);
    assert(ord_>0);
    a=new double*[ord_];
    for (int i=0;i<ord_;i++)
      a[i]=new double[ord_];
    b=new double [ord_];
    c=new double [ord_];
    switch (ord_) {
    case 4 :
      a[0][0]=0.;     a[0][1]=0.;     a[0][2]=0.;    a[0][3]=0.;
      a[1][0]=1.0;    a[1][1]=0.;     a[1][2]=0.;    a[1][3]=0.;
      a[2][0]=0.25;   a[2][1]=0.25;   a[2][2]=0.;    a[2][3]=0.;
      a[3][0]=1./6.;  a[3][1]=1./6.;  a[3][2]=2./3.; a[3][3]=0.;
      b[0]=1./6.;     b[1]=1./6.;     b[2]=2./3.;    b[3]=0.;
      c[0]=0.;        c[1]=1.0;       c[2]=0.5;      c[3]=1.0;
      break;
    case 3 :
      a[0][0]=0.;     a[0][1]=0.;     a[0][2]=0.;
      a[1][0]=1.0;    a[1][1]=0.;     a[1][2]=0.;
      a[2][0]=0.25;   a[2][1]=0.25;   a[2][2]=0.;
      b[0]=1./6.;     b[1]=1./6.;     b[2]=2./3.;
      c[0]=0.;        c[1]=1;         c[2]=0.5;
      break;
    case 2 :
      a[0][0]=0.;     a[0][1]=0.;
      a[1][0]=1.0;    a[1][1]=0.;
      b[0]=0.5;       b[1]=0.5;
      c[0]=0;         c[1]=1;
      break;
    case 1:
      a[0][0]=0.;
      b[0]=1.;
      c[0]=0.;
      break;
    default : std::cerr << "Runge-Kutta method of this order not implemented" 
			<< std::endl;
              abort();
    }
    for (int i=0;i<ord_;i++)
      Upd.push_back(new DestinationType("URK",op_.space()) );
    Upd.push_back(new DestinationType("Ustep",op_.space()) );
  }
  double solve(typename Operator::DestinationType& U0) {
    resetTimeStepEstimate();
    double t=time();
    // Compute Steps
    op_(U0,*(Upd[0]));
    double dt=cfl_*timeStepEstimate();
    dt=0.0019; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (int i=1;i<ord_;i++) {
      (Upd[ord_])->assign(U0);
      for (int j=0;j<i;j++) 
	(Upd[ord_])->addScaled(*(Upd[j]),(a[i][j]*dt));
      setTime(t+c[i]*dt);
      op_(*(Upd[ord_]),*(Upd[i]));
      double ldt=cfl_*timeStepEstimate();
    }
    // Perform Update
    for (int j=0;j<ord_;j++) {
      U0.addScaled(*(Upd[j]),(b[j]*dt));
    }
    setTime(t+dt);
    return time();
  }
  void printGrid(int nr, 
		 const typename Operator::DestinationType& U) {
    if (time()>savetime_) {
      printSGrid(time(),savestep_*10+nr,op_.space(),U);
      ++savestep_;
      savetime_+=0.1;
    }
  }
 private:
  const Operator& op_;
  int savestep_;
  double savetime_;
};

}
