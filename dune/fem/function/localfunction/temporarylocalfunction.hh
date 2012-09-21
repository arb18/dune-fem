#ifndef DUNE_FEM_TEMPORARYLOCALFUNCTION_HH
#define DUNE_FEM_TEMPORARYLOCALFUNCTION_HH

#include <dune/fem/storage/array.hh>
#include <dune/fem/function/localfunction/localfunction.hh>

namespace Dune
{

  namespace Fem 
  {

    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator = DefaultArrayAllocator >
    class TemporaryLocalFunctionImpl;

    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator = DefaultArrayAllocator >
    class TemporaryLocalFunction;

    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator >
    struct TemporaryLocalFunctionTraits
    {
      typedef DiscreteFunctionSpace DiscreteFunctionSpaceType;

      typedef TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator > LocalFunctionImpType;
      typedef TemporaryLocalFunction< DiscreteFunctionSpace, ArrayAllocator > LocalFunctionUserType;
    };



    // TemporaryLocalFunction
    // ----------------------
    
    /** \ingroup LocalFunction
     *  \class TemporaryLocalFunction
     *  \brief A temporary function carrying values for one entity
     *
     *  A TemporaryLocalFunction is a LocalFunction which is not associated with
     *  any DiscreteFunction. It can be used when generating discrete functions
     *  to temporarily store values for one entity.
     *
     *  \note Local DoF numbers correspond directly to array indices. Hence it
     *  may be more cache efficient to generate a TemporaryLocalFunction and then
     *  do only one update step on the discrete function's LocalFunction.
     *
     *  \param DiscreteFunctionSpaceImp type of the discrete function space, the
     *                                  local function shall belong to
     */
    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator >
    class TemporaryLocalFunction
    : public LocalFunction
      < TemporaryLocalFunctionTraits< DiscreteFunctionSpace, ArrayAllocator > >
    {
    public:
      typedef TemporaryLocalFunctionTraits
        < DiscreteFunctionSpace, ArrayAllocator >
        Traits;

      //! type of the discrete function space
      typedef typename Traits :: DiscreteFunctionSpaceType
        DiscreteFunctionSpaceType;
      
      //! type of the local function implementation (engine concept)
      typedef typename Traits :: LocalFunctionImpType LocalFunctionImpType;

    private:
      typedef TemporaryLocalFunction< DiscreteFunctionSpaceType, ArrayAllocator >
        ThisType;
      typedef LocalFunction< Traits > BaseType;

      friend class EngineWrapper< LocalFunctionImpType, ThisType >;

    public:
      typedef typename BaseType :: EntityType EntityType;

      /** \brief constructor creating a local function without binding it to an 
       *         entity
       *
       *  Creates the local function without initializing the fields depending on
       *  the current entity.
       *
       *  \note Before using the local function it must be initilized by
       *  \code
       *  localFunction.init( entity );
       *  \endcode
       *
       *  \param[in] dfSpace discrete function space the local function shall
       *                     belong to
       */
      explicit TemporaryLocalFunction ( const DiscreteFunctionSpaceType &dfSpace )
      : impl_( dfSpace )
      {}
      
      /** \brief constructor creating a local function and binding it to an
       *         entity
       *
       *  Creates the local function and initilizes the fields depending on the
       *  current entity. It is not necessary, though allowed, to call init
       *  before using the discrete function.
       *
       *  \note The degrees of freedom are not initialized by this function.
       *  
       *  \param[in] dfSpace discrete function space the local function shall
       *                     belong to
       *  \param[in] entity  entity for initialize the local function to
       */
      TemporaryLocalFunction ( const DiscreteFunctionSpaceType &dfSpace,
                               const EntityType &entity )
      : impl_( dfSpace, entity )
      {}

      /** \brief copy constructor
       *
       *  Creates the local function as a copy of the specified one. It is bound
       *  to an entity if and only if the copied local function is bound to an
       *  entity.
       *
       *  \note The degrees of freedom are always copied, even if the copied
       *        local function is not bound to an entity.
       * 
       *  \param[in]  other  TemporaryLocalFunction to copy
       */
      TemporaryLocalFunction ( const ThisType &other )
      : impl_( other.impl_ )
      {}

    protected:
      const LocalFunctionImpType &asImp () const
      { 
        return impl_;
      } 

      LocalFunctionImpType &asImp () 
      {
        return impl_;
      } 

      LocalFunctionImpType impl_;
    };

    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator = DefaultArrayAllocator >
    class ConstLocalFunction;

    
    /** \ingroup LocalFunction
     *  \class ConstLocalFunction
     *  \brief A constant local function carrying values for one entity
     *
     *  A ConstLocalFunction is a LocalFunction which is basically doing the same as the 
     *  LocalFunction of a discrete function. The difference is that the local dofs 
     *  are not kept as references but are copied to a local storage. 
     *  Therefore, this is a const local function and any modification of dofs is not
     *  allowed. 
     *
     *  \note Local DoF numbers correspond directly to array indices. Hence it
     *  may be more cache efficient to generate a ConstLocalFunction when only a 
     *  const access to the local function is needed. 
     *
     *  \param DiscreteFunction type of the discrete function, the
                                local function shall belong to
     */
    template< class DiscreteFunction,
              template< class > class ArrayAllocator >
    class ConstLocalFunction 
    : public TemporaryLocalFunction< typename DiscreteFunction :: DiscreteFunctionSpaceType , ArrayAllocator >
    {
      typedef TemporaryLocalFunction< typename DiscreteFunction ::
        DiscreteFunctionSpaceType, ArrayAllocator > BaseType;
      typedef ConstLocalFunction< DiscreteFunction, ArrayAllocator > ThisType;
    public:
      typedef DiscreteFunction DiscreteFunctionType ;
      typedef typename BaseType :: DiscreteFunctionSpaceType DiscreteFunctionSpaceType ;
      typedef typename BaseType :: EntityType EntityType ;
      typedef typename BaseType :: RangeFieldType RangeFieldType ;

      /** \brief constructor creating a local function without binding it to an 
       *         entity
       *
       *  Creates the local function without initializing the fields depending on
       *  the current entity.
       *
       *  \note Before using the local function it must be initilized by
       *  \code
       *  localFunction.init( entity );
       *  \endcode
       *
       *  \param[in] df discrete function the local function shall belong to
       */
      explicit ConstLocalFunction ( const DiscreteFunctionType &df )
      : BaseType( df.space() ),
        discreteFunction_( df )
      {}
      
      /** \brief constructor creating a local function and binding it to an
       *         entity
       *
       *  Creates the local function and initilizes the fields depending on the
       *  current entity. It is not necessary, though allowed, to call init
       *  before using the discrete function.
       *
       *  \note The degrees of freedom are not initialized by this function.
       *  
       *  \param[in] df      discrete function the local function shall
       *                     belong to
       *  \param[in] entity  entity for initialize the local function to
       */
      ConstLocalFunction ( const DiscreteFunctionType &df, const EntityType &entity )
      : BaseType( df.space(), entity ),
        discreteFunction_( df )
      {}

      /** \brief copy constructor
       *
       *  Creates the local function as a copy of the specified one. It is bound
       *  to an entity if and only if the copied local function is bound to an
       *  entity.
       *
       *  \note The degrees of freedom are always copied, even if the copied
       *        local function is not bound to an entity.
       * 
       *  \param[in]  other  TemporaryLocalFunction to copy
       */
      ConstLocalFunction ( const ThisType &other )
      : BaseType( other ),
        discreteFunction_( other.discreteFunction_ )
      {}

      using BaseType::operator [];

      /** \copydoc Dune::Fem::LocalFunction :: init */
      void init ( const EntityType &entity )
      {
        asImp().init( entity, discreteFunction_ );
      }

    protected:
      // reference to discrete function 
      const DiscreteFunctionType& discreteFunction_ ;

    protected:
      // make all methods that modify the local function protected 
      // to prohibit modifying of the local dofs 
      RangeFieldType &operator[] ( const int num )
      {
        return asImp()[ num ];
      }

      using BaseType::clear;
      using BaseType::asImp; 
      using BaseType::assign; 
      using BaseType::operator +=;
      using BaseType::operator -=;
      using BaseType::axpy;
    };



    // TemporaryLocalFunctionImpl
    // --------------------------

    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator >
    class TemporaryLocalFunctionImpl
    : public LocalFunctionDefault
      < DiscreteFunctionSpace,
        TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator > >
    {
      template< class DiscreteFunction >
      struct AssignDofs;

    public:
      //! type of the discrete function space
      typedef DiscreteFunctionSpace DiscreteFunctionSpaceType;

    private:
      typedef TemporaryLocalFunctionImpl< DiscreteFunctionSpaceType, ArrayAllocator >
        ThisType;
      typedef LocalFunctionDefault< DiscreteFunctionSpaceType, ThisType > BaseType;
      
    protected:
      typedef typename DiscreteFunctionSpaceType :: GridPartType GridPartType;
      typedef typename DiscreteFunctionSpaceType :: GridType GridType;

    public:
      //! type of the base function set
      typedef typename DiscreteFunctionSpaceType :: BasisFunctionSetType
        BasisFunctionSetType;

      //! type of the entity, this local function is associated with
      typedef typename DiscreteFunctionSpaceType :: EntityType EntityType;

      //! type of domain vectors
      typedef typename DiscreteFunctionSpaceType :: DomainType DomainType;
      //! type of range vectors
      typedef typename DiscreteFunctionSpaceType :: RangeType RangeType;

      //! type of the jacobian
      typedef typename DiscreteFunctionSpaceType :: JacobianRangeType JacobianRangeType;
      
      //! field type of domain vectors
      typedef typename DiscreteFunctionSpaceType :: DomainFieldType DomainFieldType;
      //! field type of range vectors
      typedef typename DiscreteFunctionSpaceType :: RangeFieldType RangeFieldType;

      enum { dimDomain = DiscreteFunctionSpaceType :: dimDomain };
      enum { dimRange = DiscreteFunctionSpaceType :: dimRange };

    protected:
      typedef DynamicArray< RangeFieldType, ArrayAllocator > DofArrayType;

    public:
      /** \brief constructor creating a local function without binding it to an 
       *         entity
       *
       *  Creates the local function without initializing the fields depending on
       *  the current entity.
       *
       *  \note Before using the local function it must be initilized by
       *  \code
       *  localFunction.init( entity );
       *  \endcode
       *
       *  \param[in] dfSpace discrete function space the local function shall
       *                     belong to
       */
      explicit TemporaryLocalFunctionImpl ( const DiscreteFunctionSpaceType &dfSpace );
      
      /** \brief constructor creating a local function and binding it to an
       *         entity
       *
       *  Creates the local function and initilizes the fields depending on the
       *  current entity. It is not necessary, though allowed, to call init
       *  before using the discrete function.
       *
       *  \note The degrees of freedom are not initialized by this function.
       *  
       *  \param[in] dfSpace discrete function space the local function shall
       *                     belong to
       *  \param[in] entity  entity for initialize the local function to
       */
      TemporaryLocalFunctionImpl ( const DiscreteFunctionSpaceType &dfSpace,
                                   const EntityType &entity );

      /** \brief copy constructor
       *
       *  Creates the local function as a copy of the specified one. It is bound
       *  to an entity if and only if the copied local function is bound to an
       *  entity.
       *
       *  \note The degrees of freedom are always copied, even if the copied
       *        local function is not bound to an entity.
       * 
       *  \param[in]  other  TemporaryLocalFunction to copy
       */
      TemporaryLocalFunctionImpl ( const ThisType &other );

      /** \copydoc Dune::Fem::LocalFunction::operator[]( const int num ) const */
      const RangeFieldType &operator[] ( const int num ) const;

      /** \copydoc Dune::Fem::LocalFunction::operator[]( const int num ) */
      RangeFieldType &operator[] ( const int num );

      /** \copydoc Dune::Fem::LocalFunction::order() const */
      int order () const;

      /** \copydoc Dune::Fem::LocalFunction::basisFunctionSet() const */
      const BasisFunctionSetType &basisFunctionSet () const;

      /** \copydoc Dune::Fem::LocalFunction::entity() const */
      const EntityType &entity () const;

      /** \brief initialize the local function for an entity
       *
       *  Binds the local function to an entity.
       *
       *  \note A local function must be initialized to an entity before it can
       *        be used.
       *        
       *  \note This function can be called multiple times to use the local
       *        function for more than one entity.
       *
       *  \param[in] entity entity to bind the local function to
       */
      void init ( const EntityType &entity );

      /** \brief initialize the local function for an entity
       *
       *  Binds the local function to an entity and copies the local dofs of the discrete
       *  function to the local storage.
       *
       *  \note A local function must be initialized to an entity before it can
       *        be used.
       *        
       *  \note This function can be called multiple times to use the local
       *        function for more than one entity.
       *
       *  \param[in] entity           entity to bind the local function to
       *  \param[in] discreteFunction discrete function the local dofs are copied from 
       *
       *  \note The dofs of the discrete function are not modified in any way. 
       */
      template< class DiscreteFunction >
      void init ( const EntityType &entity, const DiscreteFunction& discreteFunction );

      /** \copydoc Dune::Fem::LocalFunction::numDofs() const */
      int numDofs () const;

    protected:
      const DiscreteFunctionSpaceType &discreteFunctionSpace_;
      const EntityType *entity_;

      BasisFunctionSetType basisFunctionSet_;

      DofArrayType dofs_;

      bool needCheckGeometry_;
    };



    // TemporaryLocalFunctionImpl::AssignDofs
    // --------------------------------------

    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    template< class DiscreteFunction >
    struct TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >::AssignDofs
    {
      AssignDofs ( const DiscreteFunction &discreteFunction, DofArrayType &dofs )
      : discreteFunction_( discreteFunction ), dofs_( dofs )
      {}

      template < class GlobalKey >
      void operator () ( const std::size_t local, const GlobalKey& globalKey )
      {
        typedef typename DiscreteFunction::ConstDofBlockPtrType ConstDofBlockPtrType;
        static const unsigned int blockSize = DiscreteFunctionSpaceType::localBlockSize;
      
        ConstDofBlockPtrType blockPtr = discreteFunction_.block( globalKey );
        for( unsigned int i = 0; i < blockSize; ++i )
          dofs_[ (local*blockSize) + i ] = (*blockPtr)[ i ];
      }

    private:
      const DiscreteFunction &discreteFunction_;
      DofArrayType &dofs_;
    };





    // TemporaryLocalFunctionFactory
    // -----------------------------
    
    template< class DiscreteFunctionSpace,
              template< class > class ArrayAllocator = DefaultArrayAllocator >
    class TemporaryLocalFunctionFactory
    {
    public:
      typedef DiscreteFunctionSpace DiscreteFunctionSpaceType;

    private:
      typedef TemporaryLocalFunctionFactory
        < DiscreteFunctionSpaceType, ArrayAllocator >
        ThisType;

    public:
      typedef TemporaryLocalFunctionImpl< DiscreteFunctionSpaceType, ArrayAllocator >
        ObjectType;

      explicit TemporaryLocalFunctionFactory ( const DiscreteFunctionSpaceType &dfSpace )
      : discreteFunctionSpace_( dfSpace )
      {}

      ObjectType *newObject () const
      {
        return new ObjectType( discreteFunctionSpace_ );
      }

    protected:
      const DiscreteFunctionSpaceType &discreteFunctionSpace_;
    };



    // Implementation of TemporaryLocalFunctionImpl
    // --------------------------------------------

    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: TemporaryLocalFunctionImpl ( const DiscreteFunctionSpaceType &dfSpace )
    : discreteFunctionSpace_( dfSpace ),
      entity_( 0 ),
      basisFunctionSet_(),
      dofs_( DiscreteFunctionSpace::localBlockSize * discreteFunctionSpace_.blockMapper().maxNumDofs() ),
      needCheckGeometry_( true )
    {}


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: TemporaryLocalFunctionImpl ( const DiscreteFunctionSpaceType &dfSpace,
                                      const EntityType &entity )
    : discreteFunctionSpace_( dfSpace ),
      entity_( &entity ),
      basisFunctionSet_( discreteFunctionSpace_.basisFunctionSet( entity ) ),
      dofs_( DiscreteFunctionSpace::localBlockSize * discreteFunctionSpace_.blockMapper().maxNumDofs() ),
      needCheckGeometry_( true )
    {}


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: TemporaryLocalFunctionImpl ( const ThisType &other )
    : discreteFunctionSpace_( other.discreteFunctionSpace_ ),
      entity_( other.entity_ ),
      basisFunctionSet_( other.basisFunctionSet_ ),
      dofs_( other.dofs_ ),
      needCheckGeometry_( true )
    {}

    
    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline
    const typename TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: RangeFieldType &
    TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: operator[] ( const int num ) const
    {
      assert( num < numDofs() );
      return dofs_[ num ];
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline
    typename TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: RangeFieldType &
    TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: operator[] ( const int num )
    {
      assert( num < numDofs() );
      return dofs_[ num ];
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline int
    TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >::order () const
    {
      return discreteFunctionSpace_.order( entity() );
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline
    const typename TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: BasisFunctionSetType &
    TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: basisFunctionSet () const
    {
      assert( entity_ != 0 );
      return basisFunctionSet_;
    }

    
    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline
    const typename TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: EntityType &
    TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: entity () const
    {
      assert( entity_ != 0 );
      return *entity_;
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline void TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: init ( const EntityType &entity )
    {
      const bool multipleBaseSets = discreteFunctionSpace_.multipleBasisFunctionSets();

      if( multipleBaseSets || needCheckGeometry_ )
      {
        // if multiple base sets skip geometry call
        bool updateBaseSet = true;
        if( !multipleBaseSets && (entity_ != 0) )
          updateBaseSet = (basisFunctionSet_.geometryType() != entity.type());
        
        if( multipleBaseSets || updateBaseSet )
        {
          basisFunctionSet_ = discreteFunctionSpace_.basisFunctionSet( entity );
          needCheckGeometry_ = discreteFunctionSpace_.multipleGeometryTypes();
        }
      }
      assert( basisFunctionSet_.size() <= dofs_.size() );

      entity_ = &entity;
      assert( basisFunctionSet_.geometryType() == entity.type() );
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    template< class DiscreteFunction >
    inline void TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      ::init ( const EntityType &entity, const DiscreteFunction &discreteFunction )
    {
      // initialize 
      init( entity );

      // copy dofs to local storage (modifying not allowed)
      assert( numDofs() <= (int) dofs_.size() ); 
      assert( &discreteFunctionSpace_ == &discreteFunction.space() );
      AssignDofs< DiscreteFunction > assignDofs( discreteFunction, dofs_ );
      discreteFunctionSpace_.blockMapper().mapEach( entity, assignDofs );
    }


    template< class DiscreteFunctionSpace, template< class > class ArrayAllocator >
    inline int TemporaryLocalFunctionImpl< DiscreteFunctionSpace, ArrayAllocator >
      :: numDofs () const
    {
      return basisFunctionSet_.size();
    }

  } // namespace Fem 

#if DUNE_FEM_COMPATIBILITY  
  // put this in next version 1.4 

  using Fem :: TemporaryLocalFunction ;
  using Fem :: TemporaryLocalFunctionFactory ;
  using Fem :: ConstLocalFunction ;

#endif // DUNE_FEM_COMPATIBILITY

} // namespace Dune

#endif // #ifndef DUNE_FEM_TEMPORARYLOCALFUNCTION_HH
