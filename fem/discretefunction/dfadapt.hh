#ifndef DUNE_DFADAPT_HH
#define DUNE_DFADAPT_HH

//- system includes 
#include <fstream>
#include <rpc/xdr.h>

//- Dune includes 
#include <dune/common/array.hh>
#include <dune/common/geometrytype.hh>

#include "common/discretefunction.hh"
#include "common/localfunction.hh"
#include "common/dofiterator.hh"
#include "../space/common/dofmanager.hh"

namespace Dune{

  template <class DiscreteFunctionSpaceImp>    class LocalFunctionAdapt;
  template <class DofType, class DofArrayType>  class DofIteratorAdapt;
  template <class DiscreteFunctionSpaceImp> class DFAdapt;

  template <class DiscreteFunctionSpaceImp>
  struct DFAdaptTraits {
    typedef DiscreteFunctionSpaceImp DiscreteFunctionSpaceType;
  
    typedef DFAdapt<DiscreteFunctionSpaceImp> DiscreteFunctionType;
    typedef LocalFunctionAdapt<DiscreteFunctionType> LocalFunctionImp;
    typedef LocalFunctionWrapper< DiscreteFunctionType > LocalFunctionType;

    typedef typename DofArray<
      typename DiscreteFunctionSpaceImp::RangeFieldType
    >::DofIteratorType DofIteratorType;
    typedef typename DofArray<
      typename DiscreteFunctionSpaceImp::RangeFieldType
    >::ConstDofIteratorType ConstDofIteratorType;
  };

//**********************************************************************
//
//  --DFAdapt 
//
//! this is one special implementation of a discrete function using an
//! array for storing the dofs.  
//!
//**********************************************************************
template<class DiscreteFunctionSpaceType>
class DFAdapt 
  : public DiscreteFunctionDefault<
  DFAdaptTraits<DiscreteFunctionSpaceType> 
>
{
public:
  typedef typename DiscreteFunctionSpaceType::RangeFieldType RangeFieldType;
  typedef DofArray< RangeFieldType > DofArrayType;

private:
  typedef DiscreteFunctionDefault<
    DFAdaptTraits<DiscreteFunctionSpaceType> 
  > DiscreteFunctionDefaultType;
  friend class DiscreteFunctionDefault< DFAdaptTraits<DiscreteFunctionSpaceType> >;
  
  enum { myId_ = 0};

public:
  typedef typename DiscreteFunctionSpaceType::GridType GridType;

  typedef DofManager<GridType> DofManagerType;
  typedef DofManagerFactory<DofManagerType> DofManagerFactoryType;

  typedef typename DiscreteFunctionSpaceType::Traits::MapperType MapperType;
  typedef typename DiscreteFunctionSpaceType::Traits::RangeFieldType DofType;
  typedef typename DofArrayType::DofIteratorType DofIteratorType;
  typedef typename DofArrayType::ConstDofIteratorType ConstDofIteratorType;

  typedef DofArrayType DofStorageType;
  typedef MemObjectInterface MemObjectInterfaceType;
    
  //! type of this class 
  typedef DFAdapt <DiscreteFunctionSpaceType> DiscreteFunctionType;
  typedef DiscreteFunctionType ThisType;
  //! LocalFunctionImp is the implementation 
  typedef LocalFunctionAdapt < DiscreteFunctionType > LocalFunctionImp;

  //! LocalFunctionType is the exported lf type 
  typedef LocalFunctionWrapper < DiscreteFunctionType > LocalFunctionType;

  typedef DiscreteFunctionSpaceType FunctionSpaceType;
  typedef DFAdaptTraits<DiscreteFunctionSpaceType> Traits;

  /** \brief For ISTL-compatibility */
  typedef FieldVector<DofType,1> block_type;
  
  typedef LocalFunctionStorage< DiscreteFunctionType > LocalFunctionStorageType; 

  friend class LocalFunctionAdapt< ThisType> ;
public:

  //! Constructor make Discrete Function
  DFAdapt(const DiscreteFunctionSpaceType& f);

  //! Constructor creating discrete functions with name name  
  //! for given functions space f 
  DFAdapt (std::string name, const DiscreteFunctionSpaceType & f ) ;
  
  //! Constructor creating discrete functions with name name  
  //! for given functions space f and using given double * as vector 
  //! VectorPointerType should be of the underlying array pointer type 
  template <class VectorPointerType>
  DFAdapt (std::string name, const DiscreteFunctionSpaceType & f , VectorPointerType * vec ) ;
  
  //! Constructor make Discrete Function   
  DFAdapt (const DFAdapt <DiscreteFunctionSpaceType> & df); 

  //! delete stack of free local functions belonging to this discrete
  //! function 
  ~DFAdapt ();
 
  DiscreteFunctionType & argument    () { return *this; }
  const DiscreteFunctionType & argument () const { return *this; }
  DiscreteFunctionType & destination () { return *this; }
       
  // ***********  Interface  *************************
  //! return empty object of a local fucntion 
  //! old function, will be removed soon 
  inline
  LocalFunctionType newLocalFunction () DUNE_DEPRECATED;

  //! return local function for given entity
  template <class EntityType>
  inline
  LocalFunctionType localFunction(const EntityType& en) const;

  //! update LocalFunction to given Entity en  
  //! old function, will be removed soon 
  template <class EntityType> 
  inline
  void localFunction ( const EntityType &en, LocalFunctionType & lf) DUNE_DEPRECATED; 

  //! points to the first dof of type cc
  inline
  DofIteratorType dbegin ( );
  
  //! points behind the last dof of type cc
  inline
  DofIteratorType dend   ( );

  //! const version of dof iterator  
  inline
  ConstDofIteratorType dbegin ( ) const;
  
  //! const version of dof iterator  
  inline
  ConstDofIteratorType dend   ( ) const;

  //! set all dofs to zero  
  inline
  void clear( );

  //! set all dof to value x 
  inline
  void set( RangeFieldType x ); 

  //! \todo Please do me!
  inline
  void addScaled (const DFAdapt <DiscreteFunctionSpaceType> & g,
      const RangeFieldType &scalar); 

  //! \todo Please do me!
  template <class EntityType>
  inline
  void addScaledLocal (EntityType &en, 
      const DFAdapt <DiscreteFunctionSpaceType> & g,
      const RangeFieldType &scalar); 
 
  //! add g to this on local entity
  template <class EntityType>
  inline
  void addLocal (EntityType &it, 
      const DFAdapt <DiscreteFunctionSpaceType> & g); 
  
  //! add g to this on local entity
  template <class EntityType>
  inline
  void subtractLocal (EntityType &it, 
                      const DFAdapt <DiscreteFunctionSpaceType> & g); 

  //! \todo Please do me!
  template <class EntityType>
  inline
  void setLocal (EntityType &it, const RangeFieldType &scalar);
  
  //! print all dofs 
  inline
  void print(std::ostream& s) const; 

  //! write data of discrete function to file filename|timestep 
  //! with xdr methods 
  inline
  bool write_xdr(std::string filename);

  //! write data of discrete function to file filename|timestep 
  //! with xdr methods 
  inline
  bool read_xdr(std::string filename);

  //! write function data to file filename|timestep in ascii Format
  inline
  bool write_ascii(std::string filename);

  //! read function data from file filename|timestep in ascii Format
  inline
  bool read_ascii(std::string filename);

  //! write function data in pgm fromat file
  inline
  bool write_pgm(std::string filename);

  //! read function data from pgm fromat file
  inline
  bool read_pgm(std::string filename); 

  //! return name of this discrete function
  std::string name () const { return name_; }

  //! return siz fo this discrete function
  int size() const { return dofVec_.size(); }

  //! return pointer to internal array for use of BLAS routines 
  DofType * leakPointer () { return dofVec_.leakPointer();  };
  //! return pointer to internal array for use of BLAS routines 
  const DofType * leakPointer () const { return dofVec_.leakPointer(); };

private:  
  //! return object pointer of type LocalFunctionImp 
  LocalFunctionImp * newLocalFunctionObject () const;

  // name of this func
  std::string name_;

  // DofManager
  DofManager<GridType>& dm_;

  // MemObject that manages the memory for the dofs of this function
  std::pair<MemObjectInterface*, DofStorageType*> memPair_;
  
  //! array containing the dof of this function, see dofmanager.hh
  //! the array is stored within the mem object 
  DofArrayType & dofVec_;

  // one local function 
  LocalFunctionType localFunc_;

  
}; // end class DFAdapt 


//**************************************************************************
//
//  --LocalFunctionAdapt
//
//! Implementation of the local functions 
//
//**************************************************************************
template < class DiscreteFunctionType> 
class LocalFunctionAdapt 
: public LocalFunctionDefault <typename DiscreteFunctionType::DiscreteFunctionSpaceType ,
  LocalFunctionAdapt <DiscreteFunctionType>  >
{
public:
  typedef typename DiscreteFunctionType :: DiscreteFunctionSpaceType  DiscreteFunctionSpaceType;
  typedef typename DiscreteFunctionSpaceType::BaseFunctionSetType BaseFunctionSetType;
  typedef LocalFunctionAdapt<DiscreteFunctionType> MyType;
  typedef DiscreteFunctionType DiscFuncType;
  friend class LocalFunctionWrapper< DiscreteFunctionType >; 

  enum { dimrange = DiscreteFunctionSpaceType::DimRange };

  friend class DFAdapt <DiscreteFunctionSpaceType>;
  typedef typename DiscreteFunctionSpaceType::Traits::RangeFieldType RangeFieldType;
  typedef typename DiscreteFunctionSpaceType::Traits::DomainType DomainType;
  typedef typename DiscreteFunctionSpaceType::Traits::RangeType RangeType;
  typedef RangeFieldType DofType;
  typedef typename DiscreteFunctionSpaceType::Traits::JacobianRangeType JacobianRangeType;

  typedef typename DiscFuncType::DofArrayType DofArrayType;

  typedef typename DiscreteFunctionSpaceType::GridType GridType;
  typedef typename GridType::Traits::LocalIdSet LocalIdSetType;
  typedef typename LocalIdSetType::IdType IdType;
  //! Constructor 

public:
  //! Constructor taking DiscreteFunctionSpace and DofArrayType
  inline
  LocalFunctionAdapt (const DiscreteFunctionType &df);

  //! Destructor 
  inline
  ~LocalFunctionAdapt ();

  //! return name of local function 
  std::string name () const;

  //! access to dof number num, all dofs of the dof entity
  inline
  RangeFieldType & operator [] (int num);
  
  //! access to dof number num, all dofs of the dof entity
  inline
  const RangeFieldType & operator [] (int num) const;

  //! return number of degrees of freedom 
  inline
  int numberOfDofs () const DUNE_DEPRECATED;

  //! return number of degrees of freedom 
  inline
  int numDofs () const;

  //! sum over all local base functions 
  template <class EntityType> 
  inline
  void evaluate (EntityType &en, const DomainType & x, RangeType & ret) const ;
  
  template <class EntityType>
  inline
  void evaluateLocal(EntityType &en, const DomainType & x, RangeType & ret) const ;
  //! sum over all local base functions evaluated on given quadrature point
  template <class EntityType, class QuadratureType> 
  inline
  void evaluate (EntityType &en, QuadratureType &quad, int quadPoint , RangeType & ret) const;
  
  //! sum over all local base functions evaluated on given quadrature point
  template <class EntityType, class QuadratureType> 
  inline
  void jacobian (EntityType &en, QuadratureType &quad, int quadPoint , JacobianRangeType & ret) const;
  
  template <class EntityType>
  inline
  void jacobianLocal(EntityType& en, const DomainType& x, JacobianRangeType& ret) const ;

  template <class EntityType>
  inline
  void jacobian(EntityType& en, const DomainType& x, JacobianRangeType& ret) const;

  inline
  void assign(int numDof, const RangeType& dofs);
  
  inline
  const BaseFunctionSetType& getBaseFunctionSet() const;
protected: 
  //! update local function for given Entity  
  template <class EntityType > 
  inline
  void init ( const EntityType &en ) const;

  //! Forbidden! Would wreak havoc
  LocalFunctionAdapt(const LocalFunctionAdapt&);
  inline
  MyType& operator= (const MyType& other);

  //! needed once 
  mutable RangeType tmp_;
  mutable DomainType xtmp_;

  //! needed once 
  mutable JacobianRangeType tmpGrad_;

  //! diffVar for evaluate, is empty 
  const DiffVariable<0>::Type diffVar;

  //! number of all dofs 
  mutable int numOfDof_;

  //! DiscreteFunction we belong to
  const DiscreteFunctionType & df_; 
  
  //! the corresponding function space which provides the base function set
  const DiscreteFunctionSpaceType& fSpace_;
  
  //! Array holding pointers to the local dofs 
  mutable Array < RangeFieldType * > values_;

  //! dofVec from all levels of the discrete function 
  DofArrayType & dofVec_;

  //! the corresponding base function set 
  mutable const BaseFunctionSetType* baseSet_;
  
  //! is it initialised?
  mutable bool init_;

  //! actual geometry type 
  mutable GeometryType geoType_;

}; // end LocalFunctionAdapt 

} // end namespace Dune

#include "dfadapt_imp.cc"
#endif
