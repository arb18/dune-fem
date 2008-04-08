#ifndef DUNE_SPACEOPERATORIF_HH
#define DUNE_SPACEOPERATORIF_HH

//- system includes 
#include <utility>

//-Dune fem includes 
#include <dune/fem/operator/common/operator.hh>
#include <dune/fem/operator/common/objpointer.hh>

namespace Dune {

/** @ingroup OperatorCommon
  \brief SpaceOperatorInterface for Operators of the type 
  \f$L: X \longrightarrow X\f$ where \f$X\f$ is a discrete function space.
  
  \interfaceclass
*/
template <class DestinationImp>
class SpaceOperatorInterface 
: public Operator< typename DestinationImp :: RangeFieldType,
                   typename DestinationImp :: RangeFieldType,
                   DestinationImp,
                   DestinationImp>
{
protected:
  SpaceOperatorInterface() {}

public:
  //! type of argument and destination 
  typedef DestinationImp DestinationType;
  
  //! type of discrete function space 
  typedef typename DestinationType :: DiscreteFunctionSpaceType SpaceType;
  
  // destructor 
  virtual ~SpaceOperatorInterface() {}

  //! apply operator 
  virtual void operator () (const DestinationType& arg, DestinationType& dest) const = 0;

  //! return reference to space (needed by ode solvers)
  virtual const SpaceType& space() const = 0;

  /** \brief set time for operators 
      \param time current time of evaluation 
  */
  virtual void setTime(const double time) {}

  /** \brief returns maximal possible dt to assure stabil 
       explicit Runge Kutta ODE Solver. */
  virtual double timeStepEstimate () const 
  {
    return std::numeric_limits<double>::max();  
  }

  //! return reference to pass's local memory  
  virtual const DestinationType* destination() const { return 0; }

  template <class TimeProviderImp> 
  void DUNE_DEPRECATED timeProvider(TimeProviderImp* tp)
  {
    assert( tp );
    // deprecated method 
    this->setTime( tp->time() );
  }
};

//! only for kepping the pointer 
template <class OperatorType>
class SpaceOperatorPtr
: public SpaceOperatorInterface<typename OperatorType::DestinationType>,
  public ObjPointerStorage                  
{
  typedef typename OperatorType::DestinationType DestinationType;
  OperatorType* op_;
  
  //! type of discrete function space 
  typedef typename DestinationType :: DiscreteFunctionSpaceType SpaceType;
  
  //! copying not allowed 
  SpaceOperatorPtr(const SpaceOperatorPtr& org);
  SpaceOperatorPtr& operator = (const SpaceOperatorPtr& org);

public:
  //! constructor storing pointer 
  SpaceOperatorPtr(OperatorType * op)
    : op_(op)
  {}

  //! destructor deletes operator 
  ~SpaceOperatorPtr()
  {
    // delete operator before destructor of base class is called
    delete op_; op_ = 0;
  }

  //! application operator does nothing here
  void operator () (const DestinationType& arg, DestinationType& dest) const
  {
    // this method should not be called 
    assert(false);
    abort();
  }

  //! return reference to space 
  const SpaceType& space() const 
  {
    return (*op_).space(); 
  }
    
  /** @copydoc SpaceOperatorInterface::setTime  */
  void setTime(const double time) { (*op_).setTime(time); }

  /** @copydoc SpaceOperatorInterface::timeStepEstimate */
  double timeStepEstimate () const { return (*op_).timeStepEstimate(); }

  //! return reference to pass 
  OperatorType& pass() 
  { 
    assert( op_ );
    return (*op_); 
  }

  //! return reference to pass's local memory  
  const DestinationType* destination() const 
  {
    (*op_).allocateLocalMemory();
    return & ((*op_).destination());
  }
};

//! apply wrapper 
template <class OperatorType>
class SpaceOperatorWrapper
: public SpaceOperatorInterface<typename OperatorType::DestinationType>,
  public ObjPointerStorage                  
{
  // operator pointer 
  OperatorType* op_;

  //! copying not allowed
  SpaceOperatorWrapper(const SpaceOperatorWrapper& org);
  SpaceOperatorWrapper& operator = (const SpaceOperatorWrapper& org);
public:
  //! type of Argument and Destination 
  typedef typename OperatorType::DestinationType DestinationType;
  //! type of discrete function space 
  typedef typename DestinationType :: DiscreteFunctionSpaceType SpaceType;
  
  
  //! constructor storing pointer 
  SpaceOperatorWrapper(OperatorType * op)
    : op_(op)
  {}
    
  //! destructor deletes operator 
  ~SpaceOperatorWrapper()
  {
    // delete operator before destructor of base class is called
    delete op_; op_ = 0;
  }

  //! call application operator of internal operator  
  void operator () (const DestinationType& arg, DestinationType& dest) const
  {
    assert( op_ );
    (*op_)(arg,dest);
  }
  
  //! return reference to space 
  const SpaceType& space() const 
  {
    return (*op_).space(); 
  }
  
  /** @copydoc SpaceOperatorInterface::setTime  */
  void setTime(const double time) { (*op_).setTime(time); }

  /** @copydoc SpaceOperatorInterface::timeStepEstimate */
  double timeStepEstimate () const { return (*op_).timeStepEstimate(); }

  //! return reference to pass's local memory  
  const DestinationType* destination() const 
  {
    (*op_).allocateLocalMemory();
    return & ((*op_).destination());
  }
};

} // end namespace Dune 
#endif
