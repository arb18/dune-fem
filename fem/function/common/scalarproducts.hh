#ifndef DUNE_FEM_SCALARPRODURCTS_HH
#define DUNE_FEM_SCALARPRODURCTS_HH

//- system includes 
#include <iostream>
#include <map> 
#include <limits>

//- Dune includes  
#include <dune/common/mpihelper.hh>

#include <dune/grid/common/datahandleif.hh>

// in case of ISTL found include some more headers 
#if HAVE_DUNE_ISTL
#include <dune/istl/scalarproducts.hh>
#endif

//- Dune-fem includes 
//#include <dune/fem/space/common/commoperations.hh>
#include <dune/fem/space/common/singletonlist.hh>
#include <dune/fem/space/common/gridpartutility.hh>
#include <dune/fem/space/common/commindexmap.hh>

namespace Dune
{
  
/** @addtogroup Communication Communication 
    @{
**/
  
  template< class Space, class Mapper >
  class SlaveDofs 
  {
  public:
    class SingletonKey;

  private:
    class LinkBuilder;

  public:
    class Factory
    {
    public:
      typedef SlaveDofs<Space,Mapper> ObjectType;

      typedef typename ObjectType :: SingletonKey KeyType;

    public:
      static ObjectType *createObject( const KeyType &key )
      {
        return new ObjectType( key );
      }
      
      static void deleteObject( ObjectType *obj )
      {
        delete obj; 
      }
    };

    //! type of discrete function space 
    typedef Space SpaceType; 
    //! type of grid part 
    typedef typename SpaceType :: GridPartType GridPartType;

    //! type of used mapper 
    typedef Mapper MapperType;

  protected:
    typedef CommunicationIndexMap IndexMapType;

  protected:
    const SpaceType &space_;
    const GridPartType &gridPart_;
    const MapperType &mapper_;

    const int myRank_;
    const int mySize_; 
    
    // type of communication indices 
    IndexMapType slaves_;

    //! know grid sequence number 
    int sequence_; 
    
  public:
    //! constructor taking space 
    inline SlaveDofs ( const SingletonKey &key )
    : space_( key.space() ),
      gridPart_( space_.gridPart() ),
      mapper_( key.mapper() ),
      myRank_( gridPart_.grid().comm().rank() ),
      mySize_( gridPart_.grid().comm().size() ),
      slaves_(),
      sequence_( -1 )
    {}

  private:
    // prohibit copying
    SlaveDofs ( const SlaveDofs & );

  public:
    const int operator [] ( const int index ) const
    {
      return slaves_[ index ];
    }

    const int size () const
    {
      return slaves_.size();
    }

  public:
    inline void insert( const std :: vector< int > &indices )
    {
      slaves_.insert( indices );
    }
    
    inline void initialize ()
    {
      sequence_ = -1;
      slaves_.clear();
    }
    
    inline void finalize ()
    {
      // sort number for cache efficiency 
      slaves_.sort();

      // store actual sequence number 
      sequence_ = space_.sequence();
    }
    
    //! check if grid has changed and rebuild cache if necessary 
    inline void rebuild () 
    {
      // check whether grid has changed. 
      if( sequence_ != space_.sequence() )
      {
        initialize();
        buildMaps();
        finalize();
      }
    }
    
  protected:  
    // build linkage and index maps 
    inline void buildMaps ()
    {
      if( !space_.continuous() )
        buildDiscontinuousMaps();
      else
        buildCommunicatedMaps();
    }
    
    // for discontinuous spaces we don't have to communicate
    inline void buildDiscontinuousMaps ();

    inline void buildCommunicatedMaps ();
  };


  
  template< class Space, class Mapper >
  inline void SlaveDofs< Space, Mapper > :: buildDiscontinuousMaps ()
  {
    typedef typename GridPartNewPartitionType< GridPartType, All_Partition >
      :: NewGridPartType NewGridPartType;
    typedef typename NewGridPartType :: template Codim<0> :: IteratorType IteratorType;

    NewGridPartType gridPart( const_cast< GridPartType & >( gridPart_ ).grid() );

    IteratorType endit = gridPart.template end<0>();
    for( IteratorType it = gridPart.template begin<0>(); it != endit; ++it )
    {
      typedef typename GridPartType :: GridType :: template Codim< 0 > :: Entity
        EntityType;
      
      const EntityType &entity = *it;
      if( entity.partitionType() != InteriorEntity ) 
      {
        // build local mapping 
        const int numDofs = mapper_.numEntityDofs( entity );
        std::vector< int > indices( numDofs );

        // copy numDofs 
        for( int i = 0; i < numDofs; ++i )
          indices[ i ] = mapper_.mapEntityDofToGlobal( entity, i );

        insert( indices ); 
      }
    }

    {
      // insert overall size at the end
      std :: vector< int > indices( 1, mapper_.size() );
      insert( indices );
    }

    //slaveDofs_.print(std::cout,myRank_);
  }



  template< class Space, class Mapper >
  inline void SlaveDofs< Space, Mapper > :: buildCommunicatedMaps ()
  {
    typedef LinkBuilder LinkBuilderHandleType; 
    LinkBuilderHandleType handle( slaves_, space_ );

    gridPart_.communicate
      ( handle, InteriorBorder_All_Interface, ForwardCommunication );
    
    // insert overall size at the end
    std :: vector< int > indices( 1, mapper_.size() );
    insert( indices );
  }



  //! Key for CommManager singleton list
  template< class Space, class Mapper >
  class SlaveDofs< Space, Mapper > :: SingletonKey
  {
  public:
    typedef Space SpaceType;
    typedef Mapper MapperType;
    
  protected:
    const SpaceType &space_;
    const MapperType *const mapper_;

  public:
    //! constructor taking space 
    inline SingletonKey ( const SpaceType &space, 
                          const MapperType &mapper )
    : space_( space ),
      mapper_( &mapper )
    {}

    //! copy constructor  
    inline SingletonKey ( const SingletonKey &other )
    : space_( other.space_ ),
      mapper_( other.mapper_ )
    {}
    
    //! returns true if indexSet pointer and numDofs are equal 
    inline bool operator== ( const SingletonKey &other ) const
    {
      return (space_ == other.space_) && (mapper_ == other.mapper_);
    }

    //! return reference to index set 
    const SpaceType &space () const
    {
      return space_;
    }

    //! return reference to index set 
    const MapperType &mapper () const
    {
      return *mapper_;
    }
  };

 

  template< class Space, class Mapper >
  class SlaveDofs< Space,Mapper > :: LinkBuilder
  : public CommDataHandleIF< LinkBuilder, int >
  {
  public:
    typedef Space SpaceType;
    typedef Mapper MapperType;

  public:
    typedef int DataType;

    const int myRank_;
    const int mySize_;
    
    IndexMapType &slaves_;

    const SpaceType &space_;
    const MapperType &mapper_;

  public:
    LinkBuilder( IndexMapType &slaves,
                 const SpaceType &space )
    : myRank_( space.grid().comm().rank() ),
      mySize_( space.grid().comm().size() ),
      slaves_( slaves ),
      space_( space ),
      mapper_( space.mapper() )
    {}

    bool contains ( int dim, int codim ) const
    {
      return space_.contains( codim );
    }

    bool fixedsize ( int dim, int codim ) const
    {
      return false;
    }

    //! read buffer and apply operation 
    template< class MessageBuffer, class Entity >
    inline void gather ( MessageBuffer &buffer,
                         const Entity &entity ) const
    {
      PartitionType ptype = entity.partitionType();

      if( (ptype == InteriorEntity) || (ptype == BorderEntity) )
        buffer.write( myRank_ );
    }

    //! read buffer and apply operation 
    template< class MessageBuffer, class EntityType >
    inline void scatter ( MessageBuffer &buffer,
                          const EntityType &entity,
                          size_t n )
    {
      PartitionType ptype = entity.partitionType();

      int minRank = std :: numeric_limits< int > :: max();
      if( (ptype == InteriorEntity) || (ptype == BorderEntity) )
        minRank = myRank_;
      for( size_t i = 0; i < n; ++i )
      {
        int rank;
        buffer.read( rank );
        assert( (rank >= 0) && (rank < mySize_) );
        minRank = (rank < minRank ? rank : minRank);
      }

      // minimal rank means master
      assert( minRank < std :: numeric_limits< int > :: max() );
      if( minRank != myRank_ )
      {
        // build local mapping 
        const int numDofs = mapper_.numEntityDofs( entity );
        std :: vector< int > indices( numDofs );

        // copy numDofs 
        for( int i = 0; i < numDofs; ++i )
          indices[ i ] = mapper_.mapEntityDofToGlobal( entity, i );
        
        // insert slave Dofs 
        slaves_.insert( indices );
      }
    }

    //! return local dof size to be communicated 
    template< class Entity >
    size_t size ( const Entity &entity ) const
    {
      PartitionType ptype = entity.partitionType();

      return ((ptype == InteriorEntity) || (ptype == BorderEntity) ? 1 : 0);
    }
  };


#if HAVE_MPI
  //! Proxy class to evaluate ScalarProduct 
  //! holding SlaveDofs which is singleton per space and mapper 
  template< class DiscreteFunction >
  class ParallelScalarProduct 
  {
  public:
    typedef DiscreteFunction DiscreteFunctionType;

  private:
    typedef ParallelScalarProduct< DiscreteFunctionType > ThisType;

  public:
    //! type of the discrete function space
    typedef typename DiscreteFunctionType :: DiscreteFunctionSpaceType
      DiscreteFunctionSpaceType;

    //! type of range field 
    typedef typename DiscreteFunctionSpaceType :: RangeFieldType  RangeFieldType;

    //! type of used mapper 
    typedef typename DiscreteFunctionSpaceType :: BlockMapperType MapperType;
    
    enum { blockSize = DiscreteFunctionSpaceType :: localBlockSize };

    // type of communication manager object which does communication
    typedef SlaveDofs< DiscreteFunctionSpaceType, MapperType > SlaveDofsType;
    typedef typename SlaveDofsType :: SingletonKey SlaveDofsKeyType;

    typedef SingletonList< SlaveDofsKeyType, SlaveDofsType, Factory >
      SlaveDofsProviderType;

    typedef typename DiscreteFunctionType :: DofBlockPtrType DofBlockPtrType;
    typedef typename DiscreteFunctionType :: ConstDofBlockPtrType
      ConstDofBlockPtrType;

  protected:
    const DiscreteFunctionSpaceType &space_; 

    // is singleton per space 
    SlaveDofsType *const slaveDofs_;

  public:  
    //! constructor taking space 
    inline ParallelScalarProduct ( const DiscreteFunctionSpaceType &space )
    : space_( space ),
      slaveDofs_( getSlaveDofs( space_ ) )
    {
    }

  private:
    // prohibit copying
    ParallelScalarProduct( const ThisType & );

  public:
    //! remove object comm
    inline ~ParallelScalarProduct ()
    {
      SlaveDofsProviderType :: removeObject( *slaveDofs_ );
    }

    inline RangeFieldType scalarProductDofs ( const DiscreteFunctionType &x,
                                              const DiscreteFunctionType &y ) const
    {
      SlaveDofsType &slaveDofs = this->slaveDofs();

      RangeFieldType scp = 0;
      int i = 0;

      const int numSlaves = slaveDofs.size();
      for( int slave = 0; slave < numSlaves; ++slave )
      {
        const int nextSlave = slaveDofs[ slave ];
        for(; i < nextSlave; ++i )
        {
          ConstDofBlockPtrType xPtr = x.block( i );
          ConstDofBlockPtrType yPtr = y.block( i );
          for( unsigned int j = 0; j < blockSize; ++j )
            scp += (*xPtr)[ j ] * (*yPtr)[ j ];
          // scp += x.dof( i ) * y.dof( i );
        }
        // skip the slave dof
        ++i;
      }

      scp = space_.grid().comm().sum( scp );
      return scp;
    }

  protected:
    inline SlaveDofsType &slaveDofs () const
    {
      // rebuild slave dofs if grid was changed
      slaveDofs_->rebuild();
      return *slaveDofs_;
    }
    
    inline static SlaveDofsType *getSlaveDofs ( const DiscreteFunctionSpaceType &space )
    {
      SlaveDofsKeyType key( space, space.blockMapper() );
      return &(SlaveDofsProviderType :: getObject( key ));
    }
  };
#else
  //! Proxy class to evaluate ScalarProduct 
  //! holding SlaveDofs which is singleton per space and mapper 
  template< class DiscreteFunction >
  class ParallelScalarProduct 
  {
  public:
    typedef DiscreteFunction DiscreteFunctionType;

  private:
    typedef ParallelScalarProduct< DiscreteFunctionType > ThisType;

  public:
    //! type of the discrete function space
    typedef typename DiscreteFunctionType :: DiscreteFunctionSpaceType
      DiscreteFunctionSpaceType;

    //! type of range field 
    typedef typename DiscreteFunctionSpaceType :: RangeFieldType  RangeFieldType;

    typedef typename DiscreteFunctionType :: ConstDofIteratorType
      ConstDofIteratorType;

  public:  
    //! constructor taking space 
    inline ParallelScalarProduct ( const DiscreteFunctionSpaceType & )
    {
    }

  private:
    // prohibit copying
    ParallelScalarProduct( const ThisType & );

  public:
    inline RangeFieldType scalarProductDofs ( const DiscreteFunctionType &x,
                                              const DiscreteFunctionType &y ) const
    {
      RangeFieldType scp = 0;

      ConstDofIteratorType endit = x.dend ();
      ConstDofIteratorType xit = x.dbegin ();
      ConstDofIteratorType yit = y.dbegin();

      for( ; xit != endit; ++xit, ++yit )
        scp += (*xit) * (*yit);
      return scp;
    }
  };
#endif

#if HAVE_DUNE_ISTL
  template< class DiscreteFunctionSpaceImp >
  class BlockVectorDiscreteFunction;

  //! Proxy class to evaluate ScalarProduct 
  //! holding SlaveDofs which is singleton per space and mapper 
  template< class DiscreteFunctionSpaceImp >
  class ParallelScalarProduct
    < BlockVectorDiscreteFunction< DiscreteFunctionSpaceImp > > 
  : public ScalarProduct
    < typename BlockVectorDiscreteFunction< DiscreteFunctionSpaceImp >
        :: DofStorageType >
  {
    template<class SlaveDofsImp>
    class SlaveDofsProxy
    {
    protected:  
      SlaveDofsImp& slaveDofs_;
      SlaveDofsProxy(const SlaveDofsProxy& org); 
    public:
      SlaveDofsProxy(SlaveDofsImp& sd) : slaveDofs_(sd) 
      {
        slaveDofs_.initialize();
      }
      ~SlaveDofsProxy() 
      { 
        slaveDofs_.finalize(); 
      }

      void insert(const std::vector<int> & indices)
      {
        slaveDofs_.insert( indices );
      }
    };

    //! discrete function type 
    typedef BlockVectorDiscreteFunction<DiscreteFunctionSpaceImp> DiscreteFunctionType;
    //! type of this class 
    typedef ParallelScalarProduct<DiscreteFunctionType> ThisType;
    //! type of BlockVector 
    typedef typename DiscreteFunctionType :: DofStorageType BlockVectorType;
    //! type of discrete function space 
    typedef typename DiscreteFunctionType :: DiscreteFunctionSpaceType
      DiscreteFunctionSpaceType;

    //! type of range field 
    typedef typename DiscreteFunctionSpaceType :: RangeFieldType  RangeFieldType;

    //! type of used mapper 
    typedef typename DiscreteFunctionSpaceType :: BlockMapperType MapperType;
    
  public:  
    // type of communication manager object which does communication 
    typedef SlaveDofs<DiscreteFunctionSpaceType,MapperType> SlaveDofsType;
  private:  

    typedef typename SlaveDofsType :: SingletonKey KeyType;
    typedef typename SlaveDofsType :: Factory  FactoryType;

    typedef SingletonList< KeyType , SlaveDofsType , FactoryType > SlaveDofsProviderType;
  public:
    typedef SlaveDofsProxy<SlaveDofsType> BuildProxyType;
    //! export types
    typedef BlockVectorType domain_type;
    typedef typename BlockVectorType :: block_type :: field_type field_type;

    //! define the category
    enum { category=SolverCategory::sequential };

  protected:
    const DiscreteFunctionSpaceType & space_; 

    // is singleton per space 
    mutable SlaveDofsType& slaveDofs_;

    ParallelScalarProduct(const ThisType& org);
  public:  
    //! constructor taking space 
    ParallelScalarProduct(const DiscreteFunctionSpaceType& space) 
      : space_(space)
      , slaveDofs_(
          SlaveDofsProviderType::getObject(KeyType(space_,space_.blockMapper()))) 
    {
    }

    //! remove object comm
    ~ParallelScalarProduct() 
    {
      SlaveDofsProviderType::removeObject(slaveDofs_);
    }

    //! return build proxy for setup slave dofs 
    std::auto_ptr<BuildProxyType> buildProxy() { return std::auto_ptr<BuildProxyType> (new BuildProxyType(slaveDofs_)); }

    //! return reference to slave dofs
    const SlaveDofsType& slaveDofs() const { return slaveDofs_; }

    /*! \brief Dot product of two discrete functions. 
      It is assumed that the vectors are consistent on the interior+border
      partition.
     */
    RangeFieldType scalarProductDofs(const DiscreteFunctionType& x,
                                     const DiscreteFunctionType& y) const 
    {
      return scalarProductDofs(x.blockVector(),y.blockVector());
    }

    /*! \brief Dot product of two vectors. 
      It is assumed that the vectors are consistent on the interior+border
      partition.
     */
    virtual field_type dot (const BlockVectorType& x, 
                            const BlockVectorType& y)
    {
      return const_cast<ThisType&> (*this).scalarProductDofs(x,y);
    }

    /*! \brief Norm of a right-hand side vector. 
      The vector must be consistent on the interior+border partition
     */
    virtual double norm (const BlockVectorType& x)
    {
      return std::sqrt( const_cast<ThisType&> (*this).scalarProductDofs(x,x) );
    }

    //! delete slave values (for debugging)
    void deleteNonInterior(BlockVectorType& x) const 
    {
#if HAVE_MPI
      // case of ALUGrid and DGSpace or FVSpace 
      const bool deleteGhostEntries = 
          space_.grid().overlapSize(0) == 0 && 
          ! space_.continuous();

      // only delete ghost entries 
      if( deleteGhostEntries ) 
      {
        // rebuild slave dofs if grid was changed  
        slaveDofs_.rebuild();

        // don't delete the last since this is the overall Size 
        const int slaveSize = slaveDofs_.size() - 1;
        for(int slave = 0; slave<slaveSize; ++slave)
        {
          x[ slaveDofs_[slave] ] = 0;
        }
      }
#endif
    }
  protected:    
    /*! \brief Dot product of two block vectors. 
      It is assumed that the vectors are consistent on the interior+border
      partition.
     */
    RangeFieldType scalarProductDofs(const BlockVectorType& x,
                                     const BlockVectorType& y) const 
    {
#if HAVE_MPI
      // rebuild slave dofs if grid was changed  
      slaveDofs_.rebuild();

      RangeFieldType scp = 0;
      int i = 0;
      const int slaveSize = slaveDofs_.size();
      for(int slave = 0; slave<slaveSize; ++slave)
      {
        const int nextSlave = slaveDofs_[slave];
        for(; i<nextSlave; ++i) 
        {
          scp += x[i] * y[i];
        }
        // set i to next valid value 
        ++i;
      }
      scp = space_.grid().comm().sum( scp );
      return scp;
#else 
      // return build-in scalar product 
      RangeFieldType scp = x * y;
      scp = space_.grid().comm().sum( scp );
      return scp;
#endif
    }
  };
#endif

  //@}
} // end namespace Dune 
#endif
