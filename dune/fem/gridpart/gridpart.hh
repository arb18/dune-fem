#ifndef DUNE_FEM_GRIDPART_HH
#define DUNE_FEM_GRIDPART_HH

#include <dune/grid/common/grid.hh>
#include <dune/grid/common/datahandleif.hh>
#include <dune/grid/common/entity.hh>

#include <dune/fem/gridpart/defaultindexsets.hh>

#include <dune/common/bartonnackmanifcheck.hh>
#include <dune/common/deprecated.hh>

#include <dune/fem/gridpart/common/capabilities.hh>
#include <dune/fem/gridpart/common/gridpartview.hh>

namespace Dune
{

  /** \addtogroup GridPart
   *
   * Grid parts allow to define a view on a given DUNE grid, treating the
   * underlying grid as a container for entities.
   *
   * All parts of the dune-fem package rely on grid parts to access the entities
   * of the grid. For example, discrete functions are defined on the set of
   * entities accesseable by the given GridPart implementation using the iterator
   * and index set provided by the GridPart.
   *
   * \section GridPart Interface and available Implementations
   * 
   * The interface for a GridPart is implemented by the class template
   * GridPartInterface. Basically, a GridPart provides the following
   * functionality:
   * - The underlying grid can be accessed through the grid method.
   * - The indexSet method provides a suitable dune-fem index set for the grid
   *   part.
   * - Pairs of begin / end methods provide iterators over the entities of a
   *   given codimension belonging to the grid part.
   * - A pair of ibegin / iend methods provide suitable intersection iterators
   *   for a given entity of codimension 0.
   * - For parallel computations, a suitable communicate method is provided.
   * .
   *
   * The following grid parts have been implemented:
   * - LeafGridPart: A view of the leaf grid,
   * - LevelGridPart: A view of a given grid level,
   * - FilteredGridPart: A view filtering another grid part.
   *
   * \todo Implement a grid part for a given grid view (Suggestion: use the
   *       name GridPart).
   */



  /** 
   * @addtogroup GridPart
   *
   * @{ 
   */

  // Forward declarations
  template< class GridImp >
  class LevelGridPartTraits;
  template< class GridImp >
  class LeafGridPartTraits;
  template< class GridImp >
  class HierarchicGridPartTraits;

  //! \brief Interface for the GridPart classes
  //! A GridPart class allows to access only a specific subset of a grid's
  //! entities. A GridPart implementation provides the corresponding index set
  //! and a begin/end iterator pair for accessing those entities, the
  //! corresponding intersection iterators and a appropriate communication
  //! method. 
  //! GridParts are used to parametrize spaces (see DiscreteFunctionSpaceDefault [in dune-fem]).
  template< class GridPartTraits >
  class GridPartInterface
  {
    typedef GridPartInterface< GridPartTraits > ThisType;

  public:
    //! \brief Type of the Traits
    typedef GridPartTraits Traits;

    //! \brief Type of the implementation
    typedef typename Traits::GridPartType GridPartType;
   
    //! \brief type of Grid implementation
    typedef typename Traits::GridType GridType;

    //! \brief Index set implementation
    typedef typename Traits::IndexSetType IndexSetType;

    //! \brief Maximum Partition type, the index set provides indices for
    static const PartitionIteratorType indexSetPartitionType
      = Traits::indexSetPartitionType;
    static const InterfaceType indexSetInterfaceType
      = Traits::indexSetInterfaceType;

    //! \brief type of IntersectionIterator
    typedef typename Traits::IntersectionIteratorType IntersectionIteratorType;

    //! \brief type of Intersection
    typedef typename IntersectionIteratorType::Intersection IntersectionType;

    /** \brief is true if grid on this view only has conforming intersections;
     *         use the grid part capability 
     *         \code         
Dune::Fem::GridPartCapabilities::isConforming< GridPartType >::v
     *         \endcode
     *         instead.
     */
    static const bool conforming DUNE_DEPRECATED = Traits::conforming;

    typedef GridView< Fem::GridPartViewTraits< GridPartType > > GridViewType;

    typedef typename GridType::ctype ctype;

    static const int dimension = GridType::dimension;
    static const int dimensionworld = GridType::dimensionworld;

    template< int codim >
    struct Codim
    {
      typedef typename Traits::template Codim< codim >::GeometryType       GeometryType;
      typedef typename Traits::template Codim< codim >::LocalGeometryType  LocalGeometryType;

      typedef typename Traits::template Codim< codim >::EntityPointerType  EntityPointerType;
      typedef typename Traits::template Codim< codim >::EntityType         EntityType;

      template< PartitionIteratorType pitype >
      struct Partition
      {
        typedef typename Traits::template Codim< codim >::template Partition< pitype >::IteratorType
          IteratorType;
      };

      typedef typename Partition< InteriorBorder_Partition >::IteratorType IteratorType;
    };
    
  public:
    //! \brief Returns const reference to the underlying grid
    const GridType &grid () const
    { 
      CHECK_INTERFACE_IMPLEMENTATION((asImp().grid()));
      return asImp().grid(); 
    }
    //! \brief Returns reference to the underlying grid
    GridType &grid ()
    { 
      CHECK_INTERFACE_IMPLEMENTATION((asImp().grid()));
      return asImp().grid(); 
    }

    GridViewType gridView () const
    {
      typedef typename GridViewType::GridViewImp Impl;
      return GridViewType( Impl( asImp() ) );
    }
    
    //! \brief Returns reference to index set of the underlying grid
    const IndexSetType& indexSet() const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION((asImp().indexSet()));
      return asImp().indexSet(); 
    }

    /** \brief obtain begin iterator for the interior-border partition
     *
     *  \tparam  codim  codimension for which the iterator is requested
     */
    template< int codim >
    typename Codim< codim >::IteratorType
    begin () const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().template begin< codim >()) );
      return asImp().template begin< codim >();
    }

    /** \brief obtain begin iterator for the given partition
     *
     *  \tparam  codim   codimension for which the iterator is requested
     *  \tparam  pitype  requested partition iterator type
     */
    template< int codim, PartitionIteratorType pitype >
    typename Codim< codim >::template Partition< pitype >::IteratorType
    begin () const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().template begin< codim, pitype >()) );
      return asImp().template begin< codim, pitype >(); 
    }

    /** \brief obtain end iterator for the interior-border partition
     *
     *  \tparam  codim  codimension for which the iterator is requested
     */
    template< int codim >
    typename Codim< codim >::IteratorType
    end () const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().template end< codim >()) );
      return asImp().template end< codim >();
    }

    /** \brief obtain end iterator for the given partition
     *
     *  \tparam  codim   codimension for which the iterator is requested
     *  \tparam  pitype  requested partition iterator type
     */
    template< int codim, PartitionIteratorType pitype >
    typename Codim< codim >::template Partition< pitype >::IteratorType
    end () const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().template end< codim, pitype >()) );
      return asImp().template end< codim, pitype >();
    }

    //! \brief Level of the grid part
    int level () const 
    { 
      CHECK_INTERFACE_IMPLEMENTATION((asImp().level()));
      return asImp().level(); 
    }

    //! \brief ibegin of corresponding intersection iterator for given entity
    IntersectionIteratorType
    ibegin ( const typename Codim< 0 >::EntityType &entity ) const
    {
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().ibegin( entity )) );
      return asImp().ibegin( entity ); 
    }
    
    //! \brief iend of corresponding intersection iterator for given entity
    IntersectionIteratorType iend ( const typename Codim< 0 >::EntityType &entity ) const 
    {
      CHECK_INTERFACE_IMPLEMENTATION( (asImp().iend( entity )) );
      return asImp().iend( entity ); 
    }

    int boundaryId ( const IntersectionType &intersection ) const
    {
      CHECK_INTERFACE_IMPLEMENTATION( asImp().boundaryId( intersection ) );
      return asImp().boundaryId( intersection );
    }

    //! \brief corresponding communication method for grid part
    template< class DataHandleImp, class DataType >
    void communicate ( CommDataHandleIF< DataHandleImp, DataType > &data,
                       InterfaceType iftype, CommunicationDirection dir ) const
    {
      CHECK_AND_CALL_INTERFACE_IMPLEMENTATION( (asImp().communicate( data, iftype, dir )) );
    }

    /*! \brief convert the grid's entity to a grid part entity
        Usually the parameter is GridType :: Codim< codim > :: Entity  
        and the return is Codim< codim > :: EntityType. 
        In general these types are the same, but for overloaded entities on grid parts
        this can differ. 
      */
    template <class Entity> 
    const Entity& convert( const Entity& entity ) const 
    {
      CHECK_INTERFACE_IMPLEMENTATION( asImp().convert( entity ) );
      return asImp().convert( entity );
    }

  protected: 
    //! do not create explicit instances of this class 
    GridPartInterface () {}  

  private:
    GridPartType &asImp () { return static_cast< GridPartType & >( *this ); }
    const GridPartType &asImp () const { return static_cast< const GridPartType & >( *this ); }
  };



  //! \brief Default implementation for the GridPart classes
  template< class GridPartTraits >
  class GridPartDefault
  : public GridPartInterface< GridPartTraits >
  {
    typedef GridPartDefault< GridPartTraits > ThisType;
    typedef GridPartInterface< GridPartTraits > BaseType;

  public:
    //! Grid implementation
    typedef typename BaseType::GridType GridType;
    //! Index set implementation
    typedef typename BaseType::IndexSetType IndexSetType;

  protected:
    GridType &grid_;

  protected:
    //! constructor
    GridPartDefault ( GridType &grid )
    : BaseType(),
      grid_( grid )
    {}

    GridPartDefault ( const ThisType &other )
    : BaseType(),
      grid_( other.grid_ )
    {}

    ~GridPartDefault ()
    {}

  public:  
    //! Returns const reference to the underlying grid
    const GridType &grid () const { return grid_; }

    //! Returns reference to the underlying grid
    GridType &grid () { return grid_; }

    /** \brief obtain begin iterator for the interior-border partition
     *
     *  \tparam  codim  codimension for which the iterator is requested
     */
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    begin () const 
    { 
      return BaseType::template begin< codim, InteriorBorder_Partition >();
    }

    /** \brief obtain end iterator for the interior-border partition
     *
     *  \tparam  codim  codimension for which the iterator is requested
     */
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    end () const 
    {
      return BaseType::template end< codim, InteriorBorder_Partition >();
    }

    /* \brief \copydoc GridPartInterface::convert 
       
       The default implementation does nothing but return the same entity 
     */
    template <class Entity> 
    const Entity& convert( const Entity& entity ) const 
    {
      return entity;
    }

  private:
    template< int codim, PartitionIteratorType pitype >
    typename BaseType::template Codim< codim >::template Partition< pitype >::IteratorType
    begin () const;

    template< int codim, PartitionIteratorType pitype >
    typename BaseType::template Codim< codim >::template Partition< pitype >::IteratorType
    end () const;
  };



  //! \brief Selects a specific level of a grid
  template< class GridImp >
  class LevelGridPart
  : public GridPartDefault< LevelGridPartTraits< GridImp > >
  {
    typedef LevelGridPart< GridImp > ThisType;
    typedef GridPartDefault< LevelGridPartTraits< GridImp > > BaseType;

  public:
    //- Public typedefs and enums
    //! Corresponding type definitions
    typedef LevelGridPartTraits< GridImp > Traits;

    //! Grid implementation
    typedef typename Traits::GridType GridType;
    //! Level index set that corresponds to the grid
    typedef typename Traits::IndexSetType IndexSetType;
    
    //! The corresponding IntersectionIterator 
    typedef typename Traits::IntersectionIteratorType IntersectionIteratorType ;

    typedef typename IntersectionIteratorType::Intersection IntersectionType;

    typedef typename GridType::template Partition< All_Partition > :: LevelGridView  LevelGridView;

  private:
    typedef typename GridType::template Codim< 0 >::Entity EntityCodim0Type;

  public:
    //! Constructor
    LevelGridPart ( GridType &grid, const int level )
    : BaseType( grid ),
      levelView_( grid.levelView( level ) ),
      isetWrapper_( grid, level ),
      level_( level )
    {}
    
    //! Constructor, choosing maxLevel
    explicit LevelGridPart ( GridType &grid )
    : BaseType( grid ),
      levelView_( grid.levelView( grid.maxLevel() ) ),
      isetWrapper_( grid, grid.maxLevel() ),
      level_( grid.maxLevel() )
    {}
    
    //! copy constructor
    LevelGridPart ( const ThisType &other )
    : BaseType( other ),
      levelView_( other.levelView_ ),
      isetWrapper_( other.grid(), other.level_ ),
      level_( other.level_ )
    {}

    using BaseType::grid;

    //! Returns reference to index set of the underlying grid
    const IndexSetType &indexSet () const
    {
      return isetWrapper_;
    }

    //! Begin iterator on the leaf level
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    begin () const
    {
      return BaseType::template begin< codim >();
    }

    //! Begin iterator on the leaf level
    template< int codim, PartitionIteratorType pitype >
    typename Traits::template Codim< codim >::template Partition< pitype >::IteratorType
    begin () const
    {
      return levelView_.template begin< codim, pitype >();
    }

    //! Begin iterator on the GridPart's level
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    end () const
    {
      return BaseType::template end< codim >();
    }

    //! End iterator on the GridPart's level
    template< int codim, PartitionIteratorType pitype >
    typename Traits::template Codim< codim >::template Partition< pitype >::IteratorType
    end () const
    {
      return levelView_.template end< codim, pitype >();
    }

    //! ibegin of corresponding intersection iterator for given entity
    IntersectionIteratorType ibegin ( const EntityCodim0Type &entity ) const
    {
      return levelView_.ibegin( entity );
    }
    
    //! iend of corresponding intersection iterator for given entity
    IntersectionIteratorType iend ( const EntityCodim0Type &entity ) const
    {
      return levelView_.iend( entity );
    }

    int boundaryId ( const IntersectionType &intersection ) const
    {
      return intersection.boundaryId();
    }

    //! Level which this GridPart belongs to
    int level() const { return level_; }

    //! corresponding communication method for this grid part
    template< class DataHandleImp, class DataType >
    void communicate ( CommDataHandleIF< DataHandleImp, DataType > &data,
                       InterfaceType iftype, CommunicationDirection dir ) const
    {
      levelView_.communicate( data, iftype, dir );
    }

  private:
    LevelGridView levelView_;
    //! GridDefaultIndexSet Wrapper 
    IndexSetType isetWrapper_;
    const int level_;
  };



  //! Type definitions for the LevelGridPart class
  template< class GridImp >
  struct LevelGridPartTraits
  {
      /** \brief The type of the grid */
    typedef GridImp GridType;

      /** \brief The type of the corresponding grid part class */
    typedef LevelGridPart< GridImp > GridPartType;

      /** \brief The appropriate index set */
    typedef WrappedLevelIndexSet<GridType> IndexSetType;

    static const PartitionIteratorType indexSetPartitionType = All_Partition;
    static const InterfaceType indexSetInterfaceType = All_All_Interface;

      /** \brief The appropriate intersection iterator */
    typedef typename GridType::template Codim<0>::Entity::LevelIntersectionIterator
      IntersectionIteratorType;

      /** \brief Iterators over the entities of codimension <tt>cd</tt> of this grid part */
    template< int codim >
    struct Codim
    {
      typedef typename GridType::template Codim< codim >::Geometry GeometryType;
      typedef typename GridType::template Codim< codim >::LocalGeometry LocalGeometryType;

      typedef typename GridType::template Codim< codim >::Entity EntityType;
      typedef typename GridType::template Codim< codim >::EntityPointer EntityPointerType;

      template< PartitionIteratorType pitype >
      struct Partition
      {
        typedef typename GridType::template Codim< codim >::template Partition< pitype >::LevelIterator IteratorType;
      };
    };

    //! \brief is true if grid on this view only has conforming intersections
    static const bool conforming = Capabilities::isLevelwiseConforming< GridType >::v;
  };



  //! \brief Selects the leaf level of a grid
  template< class GridImp >
  class LeafGridPart
  : public GridPartDefault< LeafGridPartTraits< GridImp > >
  {
    typedef LeafGridPart< GridImp > ThisType;
    typedef GridPartDefault< LeafGridPartTraits< GridImp > > BaseType;

  public:
    //- Public typedefs and enums
    //! Type definitions
    typedef LeafGridPartTraits< GridImp > Traits;

    //! Grid implementation type
    typedef typename Traits::GridType GridType;
    //! The leaf index set of the grid implementation
    typedef typename Traits::IndexSetType IndexSetType;
    
    //! The corresponding IntersectionIterator 
    typedef typename Traits::IntersectionIteratorType IntersectionIteratorType ;

    typedef typename IntersectionIteratorType::Intersection IntersectionType;
  
    //! the leaf grid view from the grid 
    typedef typename GridType::template Partition< All_Partition >::LeafGridView LeafGridView;

  private:
    typedef typename GridType::template Codim<0>::Entity EntityCodim0Type;

  public:
    //- Public methods
    //! Constructor
    explicit LeafGridPart ( GridType &grid )
    : BaseType( grid ),
      leafView_( grid.leafView() ),
      isetWrapper_( grid )
    {}

    //! copy constructor
    LeafGridPart ( const ThisType &other )
    : BaseType( other ),
      leafView_( other.leafView_ ),
      isetWrapper_( other.grid() )
    {}

    using BaseType::grid;

    //! Returns reference to index set of the underlying grid
    const IndexSetType &indexSet () const
    {
      return isetWrapper_;
    }

    //! Begin iterator on the leaf level
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    begin () const
    {
      return BaseType::template begin< codim >();
    }

    //! Begin iterator on the leaf level
    template< int codim, PartitionIteratorType pitype >
    typename Traits::template Codim< codim >::template Partition< pitype >::IteratorType
    begin () const
    {
      return leafView_.template begin< codim, pitype >();
    }

    //! Begin iterator on the leaf level
    template< int codim >
    typename BaseType::template Codim< codim >::IteratorType
    end () const
    {
      return BaseType::template end< codim >();
    }

    //! End iterator on the leaf level
    template< int codim, PartitionIteratorType pitype >
    typename Traits::template Codim< codim >::template Partition< pitype >::IteratorType
    end () const
    {
      return leafView_.template end< codim, pitype >();
    }

    //! ibegin of corresponding intersection iterator for given entity
    IntersectionIteratorType ibegin ( const EntityCodim0Type &entity ) const
    {
      return leafView_.ibegin( entity );
    }
    
    //! iend of corresponding intersection iterator for given entity
    IntersectionIteratorType iend ( const EntityCodim0Type &entity ) const
    {
      return leafView_.iend( entity );
    }

    int boundaryId ( const IntersectionType &intersection ) const
    {
      return intersection.boundaryId();
    }

    //! Returns maxlevel of the grid
    int level() const { return grid().maxLevel(); }

    //! corresponding communication method for this grid part
    template <class DataHandleImp,class DataType>
    void communicate(CommDataHandleIF<DataHandleImp,DataType> & data, 
                     InterfaceType iftype, CommunicationDirection dir) const 
    {
      leafView_.communicate( data, iftype, dir );
    }

  private: 
    //! leaf grid view 
    LeafGridView leafView_ ;
    //! GridDefaultIndexSet Wrapper 
    IndexSetType isetWrapper_;
  };



  //! Type definitions for the LeafGridPart class
  template< class GridImp >
  struct LeafGridPartTraits
  {
    /** \brief The type of the grid */
    typedef GridImp GridType;

    /** \brief The type of the corresponding grid part class */
    typedef LeafGridPart< GridImp > GridPartType;

    /** \brief The appropriate index set */
    typedef WrappedLeafIndexSet<GridType> IndexSetType;

    static const PartitionIteratorType indexSetPartitionType = All_Partition;
    static const InterfaceType indexSetInterfaceType = All_All_Interface;

    /** \brief The appropriate intersection iterator */
    typedef typename GridType::template Codim<0>::Entity::LeafIntersectionIterator
      IntersectionIteratorType;

    /** \brief Iterators over the entities of codimension <tt>cd</tt> of this grid part */
    template< int codim >
    struct Codim
    {
      typedef typename GridType::template Codim< codim >::Geometry GeometryType;
      typedef typename GridType::template Codim< codim >::LocalGeometry LocalGeometryType;

      typedef typename GridType::template Codim< codim >::Entity EntityType;
      typedef typename GridType::template Codim< codim >::EntityPointer EntityPointerType;

      template< PartitionIteratorType pitype >
      struct Partition
      {
        typedef typename GridType::template Codim< codim >::template Partition< pitype >::LeafIterator
          IteratorType;
      };
    };

    //! \brief is true if grid on this view only has conforming intersections 
    static const bool conforming = Capabilities::isLeafwiseConforming< GridType >::v;
  };



  //**************************************************************
  /** \brief Selects the leaf level of a grid together with the 
      HierarchicIndexSet available for ALUGrid and AlbertaGrid. 
      The HierarchicIndexSet is basically the LocalIdSet of the grid 
      extended by a size method to implement the IndexSet interface. 
      For all other grids the default LeafIndexSet is selected.
  */
  template< class GridImp >
  class HierarchicGridPart
  : public GridPartDefault< HierarchicGridPartTraits< GridImp > >
  {
    typedef HierarchicGridPart< GridImp > ThisType;
    typedef GridPartDefault< HierarchicGridPartTraits< GridImp > > BaseType;

  public:
    //- Public typedefs and enums
    //! Type definitions
    typedef HierarchicGridPartTraits< GridImp > Traits;

    //! Grid implementation type
    typedef typename Traits::GridType GridType;
    //! The leaf index set of the grid implementation
    typedef typename Traits::IndexSetType IndexSetType;
    
    //! The corresponding Intersection
    typedef typename Traits::IntersectionType IntersectionType ;

    //! The corresponding IntersectionIterator 
    typedef typename Traits::IntersectionIteratorType IntersectionIteratorType ;

    //! the leaf grid view from the grid 
    typedef typename GridType :: template Partition < All_Partition > :: LeafGridView LeafGridView;

  private:
    typedef typename GridType::template Codim<0>::Entity EntityCodim0Type;

  public:
    //- Public methods
    //! constructor 
    explicit HierarchicGridPart ( GridType &grid )
    : BaseType( grid ),
      leafView_( grid.leafView() ),
      isetWrapper_( grid )
    {}

    //! constructor
    HierarchicGridPart ( GridType &grid, const IndexSetType & )
    : BaseType( grid ),
      leafView_( grid.leafView() ),
      isetWrapper_( grid )
    {}

    //! copy constructor
    HierarchicGridPart ( const ThisType &other )
    : BaseType( other ),
      leafView_( other.leafView_ ),
      isetWrapper_( other.grid() )
    {}

    //! Returns reference to index set of the underlying grid
    const IndexSetType &indexSet () const
    {
      return isetWrapper_;
    }

    //! Begin iterator on the leaf level
    template< int codim >
    typename BaseType :: template Codim< codim > :: IteratorType
    begin () const
    {
      return BaseType :: template begin< codim >();
    }

    //! Begin iterator on the leaf level
    template< int codim, PartitionIteratorType pitype >
    typename Traits :: template Codim< codim > :: template Partition< pitype > :: IteratorType
    begin () const
    {
      return leafView_.template begin< codim, pitype >();
    }

    //! Begin iterator on the leaf level
    template< int codim >
    typename BaseType :: template Codim< codim > :: IteratorType
    end () const
    {
      return BaseType :: template end< codim >();
    }

    //! End iterator on the leaf level
    template< int codim, PartitionIteratorType pitype >
    typename Traits :: template Codim< codim > :: template Partition< pitype > :: IteratorType
    end () const
    {
      return leafView_.template end< codim, pitype >();
    }

    //! ibegin of corresponding intersection iterator for given entity
    IntersectionIteratorType ibegin(const EntityCodim0Type & en) const 
    {
      return en.ileafbegin();
    }
    
    //! iend of corresponding intersection iterator for given entity
    IntersectionIteratorType iend(const EntityCodim0Type & en) const 
    {
      return en.ileafend();
    }

    int boundaryId ( const IntersectionType &intersection ) const
    {
      return intersection.boundaryId();
    }

    //! Returns maxlevel of the grid
    int level() const { return this->grid().maxLevel(); }

    //! corresponding communication method for this grid part
    template <class DataHandleImp,class DataType>
    void communicate(CommDataHandleIF<DataHandleImp,DataType> & data, 
                     InterfaceType iftype, CommunicationDirection dir) const 
    {
      this->grid().communicate(data,iftype,dir);
    }

  private: 
    //! leaf grid view 
    LeafGridView leafView_ ;
    //! GridDefaultIndexSet Wrapper 
    IndexSetType isetWrapper_;
  };

  //! Type definitions for the HierarchicGridPart class
  template< class GridImp >
  struct HierarchicGridPartTraits
  {
    /** \brief The type of the grid */
    typedef GridImp GridType;
    /** \brief The type of the corresponding grid part class */
    typedef HierarchicGridPart< GridImp > GridPartType;

    /** \brief The appropriate index set */
    typedef WrappedHierarchicIndexSet<GridType> IndexSetType;

    static const PartitionIteratorType indexSetPartitionType = All_Partition;
    static const InterfaceType indexSetInterfaceType = All_All_Interface;

      /** \brief The appropriate intersection */
    typedef typename GridType::Traits::
      LeafIntersection IntersectionType;

    /** \brief The appropriate intersection iterator */
    typedef typename GridType::template Codim<0>::Entity::LeafIntersectionIterator
      IntersectionIteratorType;

    /** \brief Iterators over the entities of codimension <tt>cd</tt> of this grid part */
    template< int codim >
    struct Codim
    {
      typedef typename GridType::template Codim< codim >::Geometry GeometryType;
      typedef typename GridType::template Codim< codim >::LocalGeometry LocalGeometryType;

      typedef typename GridType::template Codim< codim >::Entity EntityType;
      typedef typename GridType::template Codim< codim >::EntityPointer EntityPointerType;

      template< PartitionIteratorType pitype >
      struct Partition
      {
        typedef typename GridType::template Codim< codim >::template Partition< pitype >::LeafIterator IteratorType;
      };
    };

    //! \brief is true if grid on this view only has conforming intersections 
    static const bool conforming = Capabilities::isLeafwiseConforming< GridType >::v;
  };

#undef CHECK_INTERFACE_IMPLEMENTATION
#undef CHECK_AND_CALL_INTERFACE_IMPLEMENTATION
  /** @} */

  namespace Fem
  {

    template< class Entity >
    struct GridEntityAccess;

    template< int codim, int dim, class Grid, template< int, int, class > class EntityImpl >
    struct GridEntityAccess< Dune::Entity< codim, dim, Grid, EntityImpl > >
    {
      typedef Dune::Entity< codim, dim, Grid, EntityImpl > EntityType;
      typedef Dune::Entity< codim, dim, Grid, EntityImpl > GridEntityType;

      static const GridEntityType &gridEntity ( const EntityType &entity )
      {
        return entity;
      }
    };

    template< class Entity >
    const typename GridEntityAccess< Entity >::GridEntityType &
    gridEntity ( const Entity &entity )
    {
      return GridEntityAccess< Entity >::gridEntity( entity );
    }

  } // end namespace Fem

} // end namespace Dune

#endif // #ifndef DUNE_FEM_GRIDPART_HH
