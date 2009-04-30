#ifndef DUNEFEM_INDEXSETS_HH
#define DUNEFEM_INDEXSETS_HH

//- system includes 
#include <iostream>
#include <string> 
#include <rpc/xdr.h>
#include <cassert>

//- Dune includes 
#include <dune/common/bartonnackmanifcheck.hh>
#include <dune/grid/common/indexidset.hh>

//- Dune fem includes 
#include <dune/fem/gridpart/emptyindexset.hh>
#include <dune/fem/space/common/dofmanager.hh>

/** @file
 @brief Provides default index set class for persistent index sets. 
*/

namespace Dune
{

  /** \brief  class implementing the DUNE grid index set interface for any DUNE
              fem index set. However, one should not cast to this class as
              an interface class.
  */
  template< class GridImp, class Imp >
  class DuneGridIndexSetAdapter 
  : public BartonNackmanInterface< DuneGridIndexSetAdapter< GridImp, Imp >, Imp >,
    public EmptyIndexSet,
  #if DUNE_VERSION_NEWER(DUNE_GRID,1,3,0)
    public IndexSet< GridImp, Imp >
  #else
    public IndexSet< GridImp, Imp, DefaultLeafIteratorTypes< GridImp > >
  #endif
  {
    typedef DuneGridIndexSetAdapter< GridImp, Imp > ThisType;
    typedef BartonNackmanInterface< ThisType, Imp > BaseType;

    friend class Conversion< ThisType, EmptyIndexSet >;

  protected:
    using BaseType::asImp;

  #if DUNE_VERSION_NEWER(DUNE_GRID,1,3,0)
    typedef IndexSet< GridImp, Imp > DuneIndexSetType;
  #else
    typedef IndexSet< GridImp, Imp, DefaultLeafIteratorTypes< GridImp > > DuneIndexSetType;
  #endif

  public:
    //! type of grid 
    typedef GridImp GridType;

    //! type of index (i.e. unsigned int)
    typedef typename DuneIndexSetType::IndexType IndexType;

    using DuneIndexSetType::index; 

    //! type of codimension 0 entity 
    typedef typename GridType::template Codim< 0 >::Entity EntityCodim0Type; 

    //! constructor storing grid reference 
    explicit DuneGridIndexSetAdapter ( const GridType &grid )
    : grid_(grid) 
    {}

    //! copy constructor 
    DuneGridIndexSetAdapter ( const ThisType &other )
    : grid_( other.grid_ )
    {}
    
  public:
    //****************************************************************
    //
    //  INTERFACE METHODS for DUNE INDEX SETS 
    //
    //****************************************************************
    //! return global index of entity 
    template< class EntityType >
    IndexType index ( const EntityType &entity ) const
    {
      // return index of entity 
      enum { codim = EntityType::codimension };
      return this->template index< codim >( entity, 0 );
    }

    //! return subIndex of given entity
    template< int codim >
    IndexType subIndex ( const EntityCodim0Type &entity, const int localNum ) const
    {
      // return sub index of entity 
      return this->template index< codim >( entity, localNum );
    }

#if not DUNE_VERSION_NEWER(DUNE_GRID,1,3,0) && defined INDEXSET_HAS_ITERATORS
    template< int codim, PartitionIteratorType pitype >
    typename DefaultLeafIteratorTypes< GridType >::template Codim< codim >::template Partition< pitype >::Iterator
    begin () const
    {
      DUNE_THROW( NotImplemented, "DUNE-FEM index sets do not provide deprecated methods begin / end." );
      return this->grid_.template leafend< codim, pitype > ();
    }

    template< int codim, PartitionIteratorType pitype >
    typename DefaultLeafIteratorTypes< GridType >::template Codim< codim >::template Partition< pitype >::Iterator
    end () const
    {
      DUNE_THROW( NotImplemented, "DUNE-FEM index sets do not provide deprecated methods begin / end." );
      return this->grid_.template leafend< codim, pitype > ();
    }
#endif // #if not DUNE_VERSION_NEWER(DUNE_GRID,1,3,0) && defined INDEXSET_HAS_ITERATORS
   
    //////////////////////////////////////////////////////////////////
    //
    //  DUNE fem index method implementer interface  
    //
    //////////////////////////////////////////////////////////////////
  protected:
    //! return index for entity  
    template< int codim, class EntityType >
    IndexType indexImp ( const EntityType &entity, const int localNum ) const
    {
      CHECK_INTERFACE_IMPLEMENTATION( asImp().template indexImp< codim >( entity, localNum ) );
      return asImp().template indexImp< codim >( entity, localNum );
    } 

  public:  
    //////////////////////////////////////////////////////////////////
    //
    //  DUNE fem index method user interface  
    //
    //////////////////////////////////////////////////////////////////
    //! return index for entity  
    template< int codim, class EntityType >
    IndexType index ( const EntityType &entity, const int localNum ) const
    {
      return this->template indexImp< codim >( entity, localNum );
    } 

    //! insert new index for entity to set 
    void insertEntity ( const EntityCodim0Type &entity )
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION( asImp().insertEntity( entity ) );
    }

    //! remove index for entity from index set 
    void removeEntity ( const EntityCodim0Type &entity )
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION( asImp().removeEntity( entity ) );
    }
    
  protected:  
    //! reference to grid 
    const GridType &grid_;
  };



  /** \brief ConsecutivePersistentIndexSet is the base class for 
      all index sets that are persistent. Implementations of this type
      are for example all ConsecutivePersistenIndexSets. 
  */
  template< class GridImp, class Imp >
  class PersistentIndexSet
  : public DuneGridIndexSetAdapter< GridImp, Imp >
  {
    typedef PersistentIndexSet< GridImp, Imp > ThisType;
    typedef DuneGridIndexSetAdapter< GridImp, Imp > BaseType;

    typedef Imp ImplementationType;

  public:  
    //! type of entity with codimension 0
    typedef typename BaseType::EntityCodim0Type EntityCodim0Type;
    
    //! type of grid 
    typedef GridImp GridType;
    //! type of DoF manager
    typedef DofManager< GridType > DofManagerType;
    //! type of DoF manager factory
    typedef DofManagerFactory< DofManagerType > DofManagerFactoryType;

  protected:
    /** \brief constructor */
    explicit PersistentIndexSet ( const GridType &grid )
      // here false, because methods have to be overloaded
      : BaseType(grid)
      , dofManager_( DofManagerFactoryType :: getDofManager( grid ) ) 
    {
      // add persistent index set to dofmanagers list 
      dofManager_.addIndexSet( asImp() );
    }

  private:
    // no copying & no assignment
    PersistentIndexSet ( const ThisType & );
    ThisType &operator= ( const ThisType & );

  public:
    //! destructor remoing index set from dof manager  
    ~PersistentIndexSet () 
    {
      // remove persistent index set from dofmanagers list 
      dofManager_.removeIndexSet( asImp() );
    }

  #if 0
    //! insert index for father, mark childs index for removal  
    inline void restrictLocal ( const EntityCodim0Type &father,
                                const EntityCodim0Type &son,
                                bool initialize )
    {
      // important, first remove old, because 
      // on father indices might be used aswell 
      asImp().removeEntity( son );
      asImp().insertEntity( father );
    }

    //! insert indices for children , mark fathers index for removal  
    inline void prolongLocal ( const EntityCodim0Type &father,
                               const EntityCodim0Type &son,
                               bool initialize )
    {
      // important, first remove old, because 
      // on children indices might be used aswell 
      asImp().removeEntity( father );
      asImp().insertEntity( son );
    }
  #endif

    //! return true if the index set is persistent 
    bool persistent () const
    {
      return true;
    }

  protected:
    using BaseType::asImp;

    // reference to dof manager 
    DofManagerType& dofManager_;
  };



  /** \brief ConsecutivePersistentIndexSet is the base class for all index sets that 
      are consecutive and also persistent. Implementations of this type
      are for example AdaptiveLeafIndexSet and DGAdaptiveLeafIndexSet. 
  */
  template< class GridImp, class Imp >
  class ConsecutivePersistentIndexSet
  : public PersistentIndexSet< GridImp, Imp >
  {
    typedef ConsecutivePersistentIndexSet< GridImp, Imp > ThisType;
    typedef PersistentIndexSet< GridImp, Imp > BaseType;

    typedef Imp ImplementationType;

  public:  
    //! type of grid 
    typedef GridImp GridType;

  protected:
    //! Conschdrugdor 
    explicit ConsecutivePersistentIndexSet ( const GridType &grid )
    : BaseType( grid )
    {}

  private:
    // no copying & no assignment
    ConsecutivePersistentIndexSet ( const ThisType & );
    ThisType &operator= ( const ThisType & );

  public:
    //! returns true since we deal with a consecutive index set 
    bool consecutive () const
    {
      return true;
    }

    //! remove holes and make index set consecutive 
    bool compress()
    {
      return asImp().compress();
    } 

  protected:
    // use asImp from BaseType
    using BaseType::asImp;
  };

} // end namespace Dune

#endif
