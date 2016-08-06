#ifndef DUNE_FEM_LPNORM_HH
#define DUNE_FEM_LPNORM_HH

#include <type_traits>

#include <dune/grid/common/rangegenerators.hh>

#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/quadrature/integrator.hh>

#include <dune/fem/function/common/gridfunctionadapter.hh>

#include <dune/common/bartonnackmanifcheck.hh>
#include <dune/fem/misc/bartonnackmaninterface.hh>

namespace Dune
{

  namespace Fem
  {

    // LPNormBase
    // ----------

    template< class GridPart, class NormImplementation >
    class LPNormBase
      : public BartonNackmanInterface< LPNormBase< GridPart, NormImplementation >,
                                       NormImplementation >
    {
      typedef BartonNackmanInterface< LPNormBase< GridPart, NormImplementation >,
                                      NormImplementation >  BaseType ;
      typedef LPNormBase< GridPart, NormImplementation > ThisType;

    public:
      typedef GridPart GridPartType;

    protected:
      using BaseType :: asImp ;

      typedef typename GridPartType::template Codim< 0 >::EntityType EntityType;

      template <bool uDiscrete, bool vDiscrete>
      struct ForEachCaller
      {
        template <class UDiscreteFunctionType, class VDiscreteFunctionType, class ReturnType, class PartitionSet>
        static ReturnType forEach ( const ThisType &norm, const UDiscreteFunctionType &u, const VDiscreteFunctionType &v,
                                    const ReturnType &initialValue,
                                    const PartitionSet& partitionSet,
                                    unsigned int order )
        {
          static_assert( uDiscrete && vDiscrete, "Distance can only be calculated between GridFunctions" );

          ReturnType sum( 0 );
          for( const EntityType &entity : elements( norm.gridPart_, partitionSet ) )
          {
            const typename UDiscreteFunctionType::LocalFunctionType uLocal = u.localFunction( entity );
            const typename VDiscreteFunctionType::LocalFunctionType vLocal = v.localFunction( entity );
            const unsigned int uOrder = uLocal.order();
            const unsigned int vOrder = vLocal.order();
            const unsigned int orderLocal = (order == 0 ? 2*std::max( uOrder, vOrder ) : order);
            norm.distanceLocal( entity, orderLocal, uLocal, vLocal, sum );
          }
          return sum;
        }
      };

      // this specialization creates a grid function adapter
      template <bool vDiscrete>
      struct ForEachCaller<false, vDiscrete>
      {
        template <class F,
                  class VDiscreteFunctionType,
                  class ReturnType,
                  class PartitionSet>
        static
        ReturnType forEach ( const ThisType& norm,
                             const F& f, const VDiscreteFunctionType& v,
                             const ReturnType& initialValue,
                             const PartitionSet& partitionSet,
                             const unsigned int order )
        {
          typedef GridFunctionAdapter< F, GridPartType>  GridFunction ;
          GridFunction u( "LPNorm::adapter" , f , v.space().gridPart(), v.space().order() );

          return ForEachCaller< true, vDiscrete > ::
            forEach( norm, u, v, initialValue, partitionSet, order );
        }
      };

      // this specialization simply switches arguments
      template <bool uDiscrete>
      struct ForEachCaller<uDiscrete, false>
      {
        template <class UDiscreteFunctionType,
                  class F,
                  class ReturnType,
                  class PartitionSet>
        static
        ReturnType forEach ( const ThisType& norm,
                             const UDiscreteFunctionType& u,
                             const F& f,
                             const ReturnType& initialValue,
                             const PartitionSet& partitionSet,
                             const unsigned int order )
        {
          return ForEachCaller< false, uDiscrete > ::
            forEach( norm, f, u, initialValue, partitionSet, order );
        }
      };

      template< class DiscreteFunctionType, class ReturnType, class PartitionSet >
      ReturnType forEach ( const DiscreteFunctionType &u, const ReturnType &initialValue,
                           const PartitionSet& partitionSet,
                           unsigned int order = 0 ) const
      {
        static_assert( (std::is_base_of<Fem::HasLocalFunction, DiscreteFunctionType>::value),
                            "Norm only implemented for quantities implementing a local function!" );

        ReturnType sum( 0 );
        for( const EntityType &entity : elements( gridPart_, partitionSet ) )
        {
          const typename DiscreteFunctionType::LocalFunctionType uLocal = u.localFunction( entity );
          const unsigned int orderLocal = (order == 0 ? 2*uLocal.order() : order);
          normLocal( entity, orderLocal, uLocal, sum );
        }
        return sum;
      }

      template< class UDiscreteFunctionType, class VDiscreteFunctionType, class ReturnType, class PartitionSet >
      ReturnType forEach ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v,
                           const ReturnType &initialValue, const PartitionSet& partitionSet,
                           unsigned int order = 0 ) const
      {
        enum { uDiscrete = std::is_convertible<UDiscreteFunctionType, HasLocalFunction>::value };
        enum { vDiscrete = std::is_convertible<VDiscreteFunctionType, HasLocalFunction>::value };

        // call forEach depending on which argument is a grid function,
        // i.e. has a local function
        return ForEachCaller< uDiscrete, vDiscrete > ::
                  forEach( *this, u, v, initialValue, partitionSet, order );
      }

    public:
      explicit LPNormBase ( const GridPartType &gridPart )
        : gridPart_( gridPart )
      {}

    protected:
      template< class ULocalFunctionType, class VLocalFunctionType, class ReturnType >
      void distanceLocal ( const EntityType &entity, unsigned int order, const ULocalFunctionType &uLocal, const VLocalFunctionType &vLocal, ReturnType &sum ) const
      {
        CHECK_AND_CALL_INTERFACE_IMPLEMENTATION( asImp().distanceLocal( entity, order, uLocal, vLocal, sum ) );
      }

      template< class LocalFunctionType, class ReturnType >
      void normLocal ( const EntityType &entity, unsigned int order, const LocalFunctionType &uLocal, ReturnType &sum ) const
      {
        CHECK_AND_CALL_INTERFACE_IMPLEMENTATION( asImp().normLocal( entity, order, uLocal, sum ) );
      }

      const GridPartType &gridPart () const { return gridPart_; }

      const typename GridPartType::CollectiveCommunicationType& comm () const
      {
        return gridPart().comm();
      }

      bool checkCommunicateFlag( bool communicate ) const
      {
#ifndef NDEBUG
        bool commMax = communicate;
        assert( communicate == comm().max( commMax ) );
#endif
        return communicate;
      }

    private:
      const GridPartType &gridPart_;
    };



    // TODO weighte LP norm might be adapted later
    // LPNorm
    //
    // !!!!! It is not cleared which quadrature order have to be applied for p > 2!!!!
    // !!!!! For p = 1 this norm does not work !!!!
    // ------


    //! Quadrature Order Interface
    struct OrderCalculatorInterface
    {
      virtual int operator() (const double p)=0;
    };

    //! default Quadrature Order Calculator
    //  can be re-implemented in order to use
    //  a different type of calculation
    //  which can be sepcified in the second template argument of LPNorm
    struct DefaultOrderCalculator : public OrderCalculatorInterface
    {
      int operator() (const double p)
      {
        int ret=0;
        const double q = p / (p-1);
        double max = std::max(p,q);
        assert(max < std::numeric_limits<int>::max()/2. );
        ret = max +1;
        return ret;
      }
    };

    template< class GridPart, class OrderCalculator = DefaultOrderCalculator >
    class LPNorm : public LPNormBase < GridPart, LPNorm< GridPart, OrderCalculator > >
    {
      typedef LPNorm< GridPart, OrderCalculator > ThisType;
      typedef LPNormBase< GridPart, ThisType > BaseType ;

    public:
      typedef GridPart GridPartType;

      using BaseType::gridPart;
      using BaseType::comm;

    protected:
      template< class Function >
      struct FunctionMultiplicator;

      template< class UFunction, class VFunction >
      struct FunctionDistance;

      typedef typename BaseType::EntityType EntityType;
      typedef CachingQuadrature< GridPartType, 0 > QuadratureType;
      typedef Integrator< QuadratureType > IntegratorType;

    public:
      /** \brief constructor
       *    \param gridPart     specific gridPart for selection of entities
       *    \param p            p in Lp-norm
       *    \param communicate  if true global (over all ranks) norm is computed (default = true)
       */
      explicit LPNorm ( const GridPartType &gridPart, const double p, const bool communicate = true  );

      //! || u ||_Lp on given set of entities (partition set)
      template< class DiscreteFunctionType, class PartitionSet >
      typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type
      norm ( const DiscreteFunctionType &u, const PartitionSet& partitionSet ) const;

      //! || u ||_Lp on interior partition entities
      template< class DiscreteFunctionType >
      typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type
      norm ( const DiscreteFunctionType &u ) const
      {
        return norm( u, Partitions::interior );
      }

      //! || u - v ||_Lp on given set of entities (partition set)
      template< class UDiscreteFunctionType, class VDiscreteFunctionType, class PartitionSet >
      typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type
      distance ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v, const PartitionSet& partitionSet ) const;

      //! || u - v ||_Lp on interior partition entities
      template< class UDiscreteFunctionType, class VDiscreteFunctionType >
      typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type
      distance ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v ) const
      {
        return distance( u, v, Partitions::interior );
      }

      template< class ULocalFunctionType, class VLocalFunctionType, class ReturnType >
      void distanceLocal ( const EntityType &entity, unsigned int order, const ULocalFunctionType &uLocal, const VLocalFunctionType &vLocal, ReturnType &sum ) const;

      template< class LocalFunctionType, class ReturnType >
      void normLocal ( const EntityType &entity, unsigned int order, const LocalFunctionType &uLocal, ReturnType &sum ) const;

      int order ( const int spaceOrder ) const ;

    protected:
      double p_ ;
      OrderCalculator *orderCalc_;
      const bool communicate_;
    };




    // WeightedLPNorm
    // --------------

    template< class WeightFunction, class OrderCalculator = DefaultOrderCalculator >
    class WeightedLPNorm
    : public LPNorm< typename WeightFunction::DiscreteFunctionSpaceType::GridPartType,
                     OrderCalculator >
    {
      typedef WeightedLPNorm< WeightFunction, OrderCalculator > ThisType;
      typedef LPNorm< typename WeightFunction::DiscreteFunctionSpaceType::GridPartType, OrderCalculator> BaseType;

    public:
      typedef WeightFunction WeightFunctionType;

      typedef typename WeightFunctionType::DiscreteFunctionSpaceType WeightFunctionSpaceType;
      typedef typename WeightFunctionSpaceType::GridPartType GridPartType;

    protected:
      template< class Function >
      struct WeightedFunctionMultiplicator;

      typedef typename WeightFunctionType::LocalFunctionType LocalWeightFunctionType;
      typedef typename WeightFunctionType::RangeType WeightType;

      typedef typename BaseType::EntityType EntityType;
      typedef typename BaseType::IntegratorType IntegratorType;

      using BaseType::gridPart;
      using BaseType::comm;

    public:
      using BaseType::norm;
      using BaseType::distance;

      explicit WeightedLPNorm ( const WeightFunctionType &weightFunction, const double p, const bool communicate = true );

      template< class LocalFunctionType, class ReturnType >
      void normLocal ( const EntityType &entity, unsigned int order, const LocalFunctionType &uLocal, ReturnType &sum ) const;

      template< class ULocalFunctionType, class VLocalFunctionType, class ReturnType >
      void distanceLocal ( const EntityType &entity, unsigned int order, const ULocalFunctionType &uLocal, const VLocalFunctionType &vLocal, ReturnType &sum ) const;

    private:
      const WeightFunctionType &weightFunction_;
      const double p_;
    };


    // Implementation of LPNorm
    // ------------------------

    template< class GridPart, class OrderCalculator >
    inline LPNorm< GridPart, OrderCalculator >::LPNorm ( const GridPartType &gridPart, const double p, const bool communicate )
      :  BaseType( gridPart ),
         p_(p),
         orderCalc_( new OrderCalculator() ),
         communicate_( BaseType::checkCommunicateFlag( communicate ) )
    {
    }

    template< class GridPart, class OrderCalculator>
    inline int LPNorm< GridPart, OrderCalculator>::order(const int spaceOrder) const
    {
      return spaceOrder * orderCalc_->operator() (p_);
    }


    template< class GridPart, class OrderCalculator>
    template< class DiscreteFunctionType, class PartitionSet >
    inline typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type
    LPNorm< GridPart, OrderCalculator >::norm ( const DiscreteFunctionType &u, const PartitionSet& partitionSet ) const
    {
      typedef typename DiscreteFunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type RealType;
      typedef FieldVector< RealType, 1 > ReturnType ;

      // calculate integral over each element
      ReturnType sum = BaseType :: forEach( u, ReturnType(0), partitionSet );

      // communicate_ indicates global norm
      if( communicate_ )
      {
        sum[ 0 ] = comm().sum( sum[ 0 ] );
      }

      // return result
      return std::pow ( sum[ 0 ], (1.0 / p_) );
    }

    template< class GridPart, class OrderCalculator >
    template< class UDiscreteFunctionType, class VDiscreteFunctionType, class PartitionSet >
    inline typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type
    LPNorm< GridPart, OrderCalculator >
      ::distance ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v, const PartitionSet& partitionSet ) const
    {
      typedef typename UDiscreteFunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type RealType;
      typedef FieldVector< RealType, 1 > ReturnType ;

      // calculate integral over each element
      ReturnType sum = BaseType :: forEach( u, v, ReturnType(0), partitionSet );

      // communicate_ indicates global norm
      if( communicate_ )
      {
        sum[ 0 ] = comm().sum( sum[ 0 ] );
      }

      // return result
      return std::pow( sum[ 0 ], (1.0/p_) );
    }

    template< class GridPart, class OrderCalculator >
    template< class LocalFunctionType, class ReturnType >
    inline void
    LPNorm< GridPart, OrderCalculator >::normLocal ( const EntityType &entity, unsigned int order, const LocalFunctionType &uLocal, ReturnType &sum ) const
    {
      IntegratorType integrator( order );

      FunctionMultiplicator< LocalFunctionType > uLocalp( uLocal, p_ );

      integrator.integrateAdd( entity, uLocalp, sum );
    }

    template< class GridPart, class OrderCalculator >
    template< class ULocalFunctionType, class VLocalFunctionType, class ReturnType >
    inline void
    LPNorm< GridPart, OrderCalculator >::distanceLocal ( const EntityType &entity, unsigned int order, const ULocalFunctionType &uLocal, const VLocalFunctionType &vLocal, ReturnType &sum ) const
    {
      typedef FunctionDistance< ULocalFunctionType, VLocalFunctionType > LocalDistanceType;

      IntegratorType integrator( order );

      LocalDistanceType dist( uLocal, vLocal );
      FunctionMultiplicator< LocalDistanceType > distp( dist, p_ );

      integrator.integrateAdd( entity, distp, sum );
    }


    template< class GridPart, class OrderCalculator >
    template< class Function >
    struct LPNorm< GridPart, OrderCalculator >::FunctionMultiplicator
    {
      typedef Function FunctionType;

      typedef typename FunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type RealType;
      typedef FieldVector< RealType, 1 > RangeType ;

      explicit FunctionMultiplicator ( const FunctionType &function, double p )
      : function_( function ),
        p_(p)
      {}

      template< class Point >
      void evaluate ( const Point &x, RangeType &ret ) const
      {
        typename FunctionType::RangeType phi;
        function_.evaluate( x, phi );
        ret = std :: pow ( phi.two_norm(), p_);
      }

    private:
      const FunctionType &function_;
      double p_;
    };


    template< class GridPart, class OrderCalculator >
    template< class UFunction, class VFunction >
    struct LPNorm< GridPart, OrderCalculator >::FunctionDistance
    {
      typedef UFunction UFunctionType;
      typedef VFunction VFunctionType;

      typedef typename UFunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type RealType;
      typedef typename UFunctionType::RangeType RangeType;
      typedef typename UFunctionType::JacobianRangeType JacobianRangeType;

      FunctionDistance ( const UFunctionType &u, const VFunctionType &v )
      : u_( u ), v_( v )
      {}

      template< class Point >
      void evaluate ( const Point &x, RangeType &ret ) const
      {
        RangeType phi;
        u_.evaluate( x, ret );
        v_.evaluate( x, phi );
        ret -= phi;
      }

      template< class Point >
      void jacobian ( const Point &x, JacobianRangeType &ret ) const
      {
        JacobianRangeType phi;
        u_.jacobian( x, ret );
        v_.jacobian( x, phi );
        ret -= phi;
      }

    private:
      const UFunctionType &u_;
      const VFunctionType &v_;
    };


    // Implementation of WeightedL2Norm
    // --------------------------------

    template< class WeightFunction, class OrderCalculator >
    inline WeightedLPNorm< WeightFunction, OrderCalculator >
      ::WeightedLPNorm ( const WeightFunctionType &weightFunction, double p, const bool communicate )
    : BaseType( weightFunction.space().gridPart(), p, communicate ),
      weightFunction_( weightFunction ),
      p_(p)
    {
      static_assert( (WeightFunctionSpaceType::dimRange == 1),
                          "Weight function must be scalar." );
    }


    template< class WeightFunction, class OrderCalculator >
    template< class LocalFunctionType, class ReturnType >
    inline void
    WeightedLPNorm< WeightFunction, OrderCalculator >::normLocal ( const EntityType &entity, unsigned int order, const LocalFunctionType &uLocal, ReturnType &sum ) const
    {
      // !!!! order !!!!
      IntegratorType integrator( order );

      LocalWeightFunctionType wfLocal = weightFunction_.localFunction( entity );

      WeightedFunctionMultiplicator< LocalFunctionType > uLocal2( wfLocal, uLocal, p_ );

      integrator.integrateAdd( entity, uLocal2, sum );
    }


    template< class WeightFunction,class OrderCalculator >
    template< class ULocalFunctionType, class VLocalFunctionType, class ReturnType >
    inline void
    WeightedLPNorm< WeightFunction, OrderCalculator >::distanceLocal ( const EntityType &entity, unsigned int order, const ULocalFunctionType &uLocal, const VLocalFunctionType &vLocal, ReturnType &sum ) const
    {
      typedef typename BaseType::template FunctionDistance< ULocalFunctionType, VLocalFunctionType > LocalDistanceType;

      // !!!! order !!!!
      IntegratorType integrator( order );

      LocalWeightFunctionType wfLocal = weightFunction_.localFunction( entity );

      LocalDistanceType dist( uLocal, vLocal );
      WeightedFunctionMultiplicator< LocalDistanceType > dist2( wfLocal, dist );

      integrator.integrateAdd( entity, dist2, sum );
    }


    template< class WeightFunction, class OrderCalculator>
    template< class Function >
    struct WeightedLPNorm< WeightFunction, OrderCalculator>::WeightedFunctionMultiplicator
    {
      typedef Function FunctionType;

      typedef typename FunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type RealType;
      typedef FieldVector< RealType, 1 > RangeType;

      WeightedFunctionMultiplicator ( const LocalWeightFunctionType &weightFunction,
                                      const FunctionType &function,
                                      double p )
      : weightFunction_( weightFunction ),
        function_( function ),
        p_(p)
      {}

      template< class Point >
      void evaluate ( const Point &x, RangeType &ret ) const
      {
        WeightType weight;
        weightFunction_.evaluate( x, weight );

        typename FunctionType::RangeType phi;
        function_.evaluate( x, phi );
        ret = weight[ 0 ] * std::pow ( phi.two_norm(), p_);
      }

    private:
      const LocalWeightFunctionType &weightFunction_;
      const FunctionType &function_;
      double p_;
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_LPNORM_HH
