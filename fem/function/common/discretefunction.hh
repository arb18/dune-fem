#ifndef DUNE_DISCRETEFUNCTION_HH
#define DUNE_DISCRETEFUNCTION_HH

//- system includes 
#include <string>

//- Dune inlcudes 
#include <dune/common/bartonnackmanifcheck.hh>
#include <dune/grid/common/grid.hh>


//- local includes 
#include "function.hh"
#include <dune/fem/space/common/discretefunctionspace.hh>
#include <dune/fem/space/common/objectstack.hh>
#include "dofiterator.hh"
#include "localfunctionwrapper.hh"

namespace Dune{


  /** @addtogroup DiscreteFunction 
      The DiscreteFunction is responsible for the dof storage. This can be
      done in various ways an is left to the user. The user has to derive his
      own implementation from the DiscreteFunctionDefault class. If some of
      the implementations in the default class are for ineffecient for the
      dof storage in the derived class these functions can be overloaded.

      \remarks 
      The interface for using a DiscreteFunction is defined by
      the class DiscreteFunctionInterface.
  
      @{
  */

  /** Base class for determing whether a class is a discrete function or not. 
  */
  class IsDiscreteFunction
  {
  };
  
  /** Base class for determing whether a function has local functions or not.
  */
  class HasLocalFunction
  {
  };

  template <class Traits>
  class DiscreteFunctionDefault;
  
  //----------------------------------------------------------------------
  //-
  //-  --DiscreteFunctionInterface
  //-
  //----------------------------------------------------------------------
  /** This is the interface of a discrete function which describes the
      features of a discrete function. 
      It contains a local function and a dof iterator which can 
      iterate over all dofs of one level. Via the method access the local
      dofs and basis functions can be accessed for a given entity.
      The DOF-Iterators are STL-like Iterators, i.e. they can be dereferenced
      giving the corresponding DOF.

      \interfaceclass
  */
  template<class DiscreteFunctionTraits>
  class DiscreteFunctionInterface : 
    public IsDiscreteFunction , 
    public HasLocalFunction , 
    public Function<typename DiscreteFunctionTraits::DiscreteFunctionSpaceType,
                    DiscreteFunctionInterface<DiscreteFunctionTraits> > 
  {
  private:
    typedef DiscreteFunctionInterface< DiscreteFunctionTraits > ThisType;

  public:
    //- Typedefs and enums

    //! types of function base class 
    typedef Function<
      typename DiscreteFunctionTraits::DiscreteFunctionSpaceType,
      DiscreteFunctionInterface<DiscreteFunctionTraits> 
    > FunctionType;

    //! type of default implementation 
    typedef DiscreteFunctionDefault<DiscreteFunctionTraits>
      DiscreteFunctionDefaultType;
        
    //! type of discrete function space for discrete function 
    typedef typename DiscreteFunctionTraits::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;

    //! type of domain field, i.e. type of coordinate component
    typedef typename DiscreteFunctionSpaceType :: DomainFieldType DomainFieldType;
    //! type of range field, i.e. dof type 
    typedef typename DiscreteFunctionSpaceType::RangeFieldType RangeFieldType;
    //! type of domain, i.e. type of coordinates 
    typedef typename DiscreteFunctionSpaceType::DomainType DomainType;
    //! type of range, i.e. result of evaluation 
    typedef typename DiscreteFunctionSpaceType::RangeType RangeType;
    //! type of jacobian, i.e. type of evaluated gradient 
    typedef typename DiscreteFunctionSpaceType::JacobianRangeType JacobianRangeType;
 
    //! Type of the underlying grid
    typedef typename DiscreteFunctionSpaceType::GridType GridType;

    //! Type of the discrete function implementation
    typedef typename DiscreteFunctionTraits::DiscreteFunctionType DiscreteFunctionType;

    //! Type of exported local function
    typedef typename DiscreteFunctionTraits::LocalFunctionType LocalFunctionType;

    //! Type of the local function implementation
    typedef typename DiscreteFunctionTraits::LocalFunctionImp LocalFunctionImp;

    //! Type of the dof iterator used in the discrete function implementation.
    typedef typename DiscreteFunctionTraits::DofIteratorType DofIteratorType;

    //! Type of the constantdof iterator used in the discrete function implementation
    typedef typename DiscreteFunctionTraits::ConstDofIteratorType ConstDofIteratorType;

    //! type of mapping base class for this discrete function 
    typedef Mapping<DomainFieldType, RangeFieldType,
                    DomainType, RangeType> MappingType;

  public:
    //- Public Methods

    /** \brief Constructor storing discrete function space 
        \param[in] f discrete function space 
    */
    inline explicit DiscreteFunctionInterface ( const DiscreteFunctionSpaceType &dfSpace )
    : FunctionType( dfSpace )
    {
    }

  private:
    // Disallow copying
    ThisType &operator= ( const ThisType &other );
    
  public:
    /** \brief returns name of discrete function 
        \return string holding name of discrete function 
    */
    std::string name() const 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().name()); 
      return asImp().name();
    }

    /** \brief return object of LocalFunction of the discrete function associated with given entity
        \param[in] entity Entity to focus view of discrete function 
        \return LocalFunction associated with entity
    */
    template< class EntityType >
    const LocalFunctionType localFunction ( const EntityType &entity ) const
    {
      return asImp().localFunction( entity );
    }
    
    /** \brief return object of LocalFunction of the discrete function associated with given entity
        \param[in] entity Entity to focus view of discrete function 
        \return LocalFunction associated with entity
    */
    template< class EntityType >
    LocalFunctionType localFunction ( const EntityType &entity )
    {
      return asImp().localFunction( entity );
    }

    /** \brief set all degrees of freedom to zero
    */
    void clear()
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().clear());
    }

    /** \brief returns total number of degrees of freedom, i.e. size of discrete function space 
        \return total number of dofs 
    */
    int size() const 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().size()); 
      return asImp().size();
    }

    /** \brief returns dof iterator pointing to the first degree of freedom of this discrete function 
        \return dof iterator pointing to first dof 
    */
    DofIteratorType dbegin () 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().dbegin ());
      return asImp().dbegin ();
    }

    /** \brief returns dof iterator pointing behind the last degree of freedom of this discrete function 
        \return dof iterator pointing behind the last dof 
    */
    DofIteratorType dend () 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().dend ());
      return asImp().dend ();
    }

    /** \brief returns dof iterator pointing to the first degree of freedom of this discrete function 
        \return dof iterator pointing to first dof 
    */
    ConstDofIteratorType dbegin () const  
    {
      return asImp().dbegin ();
    }

    /** \brief returns dof iterator pointing behind the last degree of freedom of this discrete function 
        \return dof iterator pointing behind the last dof 
    */
    ConstDofIteratorType dend () const  
    {
      return asImp().dend ();
    }

    /** \brief axpy operation
        \param[in] g discrete function that is added 
        \param[in] c scalar value to scale 
    */
    void addScaled(const DiscreteFunctionType& g, const RangeFieldType& c)
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().addScaled(g,c));
    }

    /** \brief Evaluate a scalar product of the dofs of two DiscreteFunctions
        \param[in] g discrete function for evaluating scalar product with  
        \return returns the scalar product of the dofs 
    */
    RangeFieldType scalarProductDofs(const DiscreteFunctionType& g) const
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().scalarProductDofs(g));
      return asImp().scalarProductDofs(g);
    }

    /** \brief print all degrees of freedom of this function to stream (for debugging purpose)
        \param[out] s std::ostream (e.g. std::cout)
    */
    void print(std::ostream & s) const
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().print(s));
    }

    /** \brief check for NaNs
        \return if one  of the dofs is NaN \b false is returned, otherwise \b true 
    */
    inline bool dofsValid () const
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().dofsValid());
      return asImp().dofsValid();
    }
    
    /** \brief Set all DoFs to a scalar value
        \param[in] s scalar value to assign for 
    */
    inline DiscreteFunctionType &assign ( const RangeFieldType s )
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().assign(s));
      return asImp().assign(s);
    }

    /** \brief evaluate Function f 
        \param[in] arg global coordinate
        \param[out] dest f(arg)
    */ 
    void evaluate (const DomainType & arg, RangeType & dest) const 
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().evaluate(arg,dest));
    }

    /** \brief evaluate Function f
        \param[in] diffVariable derivation determizer 
        \param[in] arg global coordinate
        \param[out] dest f(arg)
    */ 
    template <int derivation>
    void evaluate  ( const FieldVector<deriType, derivation> &diffVariable, 
                     const DomainType& arg, RangeType & dest) const 
    { 
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().evaluate(diffVariable,arg,dest));
    }

    /** \brief assign all degrees of freedom from given discrete function using the dof iterators 
        \param[in] g discrete function which is copied 
    */
    void assign(const DiscreteFunctionType& g)
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION(asImp().assign(g));
    }

    /** \brief add all degrees of freedom from given discrete function using the dof iterators 
        \param[in] g discrete function which is added to this discrete function 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator += (const DiscreteFunctionType& g) 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().operator += (g));
      return asImp().operator += (g);
    }

    /** \brief substract all degrees of freedom from given discrete function using the dof iterators 
        \param[in] g discrete function which is substracted from this discrete function 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator -= (const DiscreteFunctionType& g) 
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().operator -= (g));
      return asImp().operator -= (g);
    }
 
    /** \brief multiply all degrees of freedom with given scalar factor using the dof iterators 
        \param[in] scalar factor with which all dofs are scaled 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator *= (const RangeFieldType &scalar)
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().operator *= (scalar));
      return asImp().operator *= (scalar);
    }

    /** \brief devide all degrees of freedom with given scalar factor using the dof iterators 
        \param[in] scalar factor with which all dofs are devided  
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator /= (const RangeFieldType &scalar)
    {
      CHECK_INTERFACE_IMPLEMENTATION(asImp().operator /= (scalar));
      return asImp().operator /= (scalar);
    }
    
    /** \brief write discrete function to file with given filename using xdr encoding
        \param[in] filename name of file to which discrete function should be written using xdr 
        \return \b true if operation was successful 
    */
    virtual bool write_xdr(const std::string filename) const = 0; 

    /** \brief write discrete function to file with given filename using ascii encoding
        \param[in] filename name of file to which discrete function should be written using ascii 
        \return \b true if operation was successful 
    */
    virtual bool write_ascii(const std::string filename) const = 0;

    /** \brief write discrete function to file with given filename using pgm encoding
        \param[in] filename name of file to which discrete function should be written using pgm 
        \return \b true if operation was successful 
    */
    virtual bool write_pgm(const std::string filename) const = 0;

    /** \brief read discrete function from file with given filename using xdr decoding
        \param[in] filename name of file from which discrete function should be read using xdr 
        \return \b true if operation was successful 
    */
    virtual bool read_xdr(const std::string filename) const = 0;
    /** \brief read discrete function from file with given filename using ascii decoding
        \param[in] filename name of file from which discrete function should be read using ascii 
        \return \b true if operation was successful 
    */
    virtual bool read_ascii(const std::string filename) const = 0;
    /** \brief read discrete function from file with given filename using pgm decoding
        \param[in] filename name of file from which discrete function should be read using pgm 
        \return \b true if operation was successful 
    */
    virtual bool read_pgm(const std::string filename) const = 0;

  protected:
    /** \brief return pointer to new object of local function implementation 
        \return pointer to new object of local function implementation
    */
    LocalFunctionImp* newObject() const {
      return asImp().newObject();
    }

 protected:
    //! \brief Barton-Nackman trick 
    DiscreteFunctionType& asImp() 
    { 
      return static_cast<DiscreteFunctionType&>(*this); 
    }

    //! \brief Barton-Nackman trick 
    const DiscreteFunctionType &asImp() const 
    { 
      return static_cast<const DiscreteFunctionType&>(*this); 
    }
  };



  //*************************************************************************
  //
  //  --DiscreteFunctionDefault
  //  
  //! Default implementation of the discrete function. This class provides 
  //! is responsible for the dof storage. Different implementations of the
  //! discrete function use different dof storage. 
  //! The default implementation provides +=, -= and so on operators and 
  //! a DofIterator access, which can run over all dofs in an efficient way. 
  //! Furthermore with an entity you can access a local function to evaluate
  //! the discrete function by multiplying the dofs and the basefunctions. 
  //! 
  //*************************************************************************
  template< class DiscreteFunctionTraits >
  class DiscreteFunctionDefault
  : public DiscreteFunctionInterface< DiscreteFunctionTraits > 
  { 
  private:
    typedef DiscreteFunctionDefault< DiscreteFunctionTraits > ThisType;
    typedef DiscreteFunctionInterface< DiscreteFunctionTraits > BaseType;
    
  protected:
    using BaseType :: asImp;

  private:
    typedef DiscreteFunctionInterface< DiscreteFunctionTraits >
      DiscreteFunctionInterfaceType;

    typedef DiscreteFunctionDefault< DiscreteFunctionTraits >
      DiscreteFunctionDefaultType;

    enum { myId_ = 0 };
  
  public:
    //! type of discrete function space
    typedef typename DiscreteFunctionTraits :: DiscreteFunctionSpaceType
      DiscreteFunctionSpaceType;
    
    //! type of domain vector
    typedef typename DiscreteFunctionSpaceType :: DomainType DomainType;
    //! type of range vector
    typedef typename DiscreteFunctionSpaceType :: RangeType RangeType;
  
    //! type of domain field (usually a float type)
    typedef typename DiscreteFunctionSpaceType :: DomainFieldType DomainFieldType;
    //! type of range field (usually a float type)
    typedef typename DiscreteFunctionSpaceType :: RangeFieldType RangeFieldType;

    //! type of mapping base class for this discrete function 
    typedef Mapping< DomainFieldType, RangeFieldType, DomainType, RangeType>
      MappingType;

    //! type of the discrete function (Barton-Nackman parameter)
    typedef typename DiscreteFunctionTraits :: DiscreteFunctionType
      DiscreteFunctionType;

     //! type of the dof iterator
    typedef typename DiscreteFunctionTraits :: DofIteratorType DofIteratorType;
    //! type of the const dof iterator
    typedef typename DiscreteFunctionTraits :: ConstDofIteratorType
      ConstDofIteratorType;
 
    //! type of the local function implementation 
    typedef typename DiscreteFunctionTraits :: LocalFunctionImp LocalFunctionImp;
   
     //! type of the local function
    typedef typename DiscreteFunctionTraits :: LocalFunctionType LocalFunctionType;

   //! type of object to create from stack 
    typedef LocalFunctionImp ObjectType;
    //! type of local function stack 
    typedef ObjectStack < DiscreteFunctionDefaultType > LocalFunctionStorageType;

    friend class ObjectStack< DiscreteFunctionDefaultType >;
    friend class LocalFunctionWrapper< DiscreteFunctionType >;

  public:
    /** \brief Constructor storing discrete function space
     *
     *  The discrete function space is passed to the interface class and the
     *  local function storage is initialized.
     * 
     *  \param[in]  dfSpace  discrete function space 
     */
    inline explicit DiscreteFunctionDefault ( const DiscreteFunctionSpaceType &dfSpace )
    : DiscreteFunctionInterfaceType( dfSpace ),
      lfStorage_( *this )
    {
    }

    /** \brief Copy constructor
     *  
     *  \note The local function storage cannot simple be copied; we must
     *        create a new object (otherwise the other discrete function
     *        will be used as local function factory by the storage).
     *
     *  \param[in]  other  discrete function to copy
     */
    inline DiscreteFunctionDefault ( const ThisType &other )
    : DiscreteFunctionInterfaceType ( other ),
      lfStorage_( *this )
    {
    }

  private:
    // Disallow copying
    ThisType &operator= ( const ThisType &other );
    
  public:
    /** \brief @copydoc DiscreteFunctionInterface::print */
    void print(std::ostream & s) const;

    /** \brief @copydoc DiscreteFunctionInterface::dofsValid */ 
    inline bool dofsValid () const
    {
      const ConstDofIteratorType end = this->dend();
      for( ConstDofIteratorType it = this->dbegin(); it != end; ++it )
      {
        if( *it != *it )
          return false;
      }

      return true;
    }
    
    /** \brief @copydoc DiscreteFunctionInterface::clear */
    void clear();

    /** \brief @copydoc DiscreteFunctionInterface::assign */
    inline DiscreteFunctionType &assign ( const RangeFieldType s )
    {
      const DofIteratorType end = this->dend();
      for( DofIteratorType it = this->dbegin(); it != end; ++it )
        *it = s;
      return asImp();
    }

    /** \brief @copydoc DiscreteFunctionInterface::addScaled */
    void addScaled(const DiscreteFunctionType& g, const RangeFieldType& c);

    /** \brief @copydoc DiscreteFunctionInterface::scalarProductDofs */
    RangeFieldType scalarProductDofs(const DiscreteFunctionType& g) const;

    /** \brief @copydoc DiscreteFunctionInterface::assign */
    void assign(const DiscreteFunctionType& g);

    /** \brief add all degrees of freedom from given discrete function using the dof iterators 
        \param[in] g discrete function which is added to this discrete function 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator += (const DiscreteFunctionType& g);

    /** \brief substract all degrees of freedom from given discrete function using the dof iterators 
        \param[in] g discrete function which is substracted from this discrete function 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator -= (const DiscreteFunctionType& g);
 
    /** \brief multiply all degrees of freedom with given scalar factor using the dof iterators 
        \param[in] scalar factor with which all dofs are scaled 
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator *= (const RangeFieldType &scalar);

    /** \brief devide all degrees of freedom with given scalar factor using the dof iterators 
        \param[in] scalar factor with which all dofs are devided  
        \return reference to this (i.e. *this)
    */
    DiscreteFunctionType& operator /= (const RangeFieldType &scalar);
    
    /** \copydoc LocalFunctionInterface::localFunction */
    template< class EntityType >
    const LocalFunctionType localFunction ( const EntityType &entity ) const
    {
      return LocalFunctionType( entity, asImp() );
    }
    
    /** \copydoc LocalFunctionInterface::localFunction */
    template< class EntityType >
    LocalFunctionType localFunction ( const EntityType &entity )
    {
      return LocalFunctionType( entity, asImp() );
    }

    /** \brief @copydoc DiscreteFunctionInterface::write_xdr */
    virtual bool write_xdr(const std::string filename) const { return true; }

    /** \brief @copydoc DiscreteFunctionInterface::write_ascii */
    virtual bool write_ascii(const std::string filename) const { return true; }

    /** \brief @copydoc DiscreteFunctionInterface::write_pgm */
    virtual bool write_pgm(const std::string filename) const { return true; }

    /** \brief @copydoc DiscreteFunctionInterface::read_xdr */
    virtual bool read_xdr(const std::string filename) const { return true; }
    
    /** \brief @copydoc DiscreteFunctionInterface::read_ascii */
    virtual bool read_ascii(const std::string filename) const { return true; }
    
    /** \brief @copydoc DiscreteFunctionInterface::read_pgm */
    virtual bool read_pgm(const std::string filename) const { return true; }

  protected: 
    //this methods are used by the LocalFunctionStorage class 

    /** \brief return reference for local function storage  
        \return reference to local function storage 
    */
    LocalFunctionStorageType& localFunctionStorage() const 
    { 
      return lfStorage_; 
    }

  private:    
    // the local function storage stack 
    mutable LocalFunctionStorageType lfStorage_;
  }; // end class DiscreteFunctionDefault 
  
///@} 
} // end namespace Dune
#include "discretefunction.cc"
#include "discretefunctionadapter.hh"
#endif
