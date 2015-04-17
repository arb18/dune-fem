#ifndef DUNE_FEM_L2NORM_HH
#define DUNE_FEM_L2NORM_HH

#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/quadrature/integrator.hh>

#include <dune/fem/misc/lpnorm.hh>

namespace Dune
{

  namespace Fem
  {

    // L2Norm
    // ------

    template< class GridPart >
    class L2Norm : public LPNormBase< GridPart, L2Norm< GridPart > >
    {
      typedef LPNormBase< GridPart, L2Norm< GridPart > > BaseType ;
      typedef L2Norm< GridPart > ThisType;

    public:
      typedef GridPart GridPartType;

      using BaseType :: gridPart ;
      using BaseType :: comm ;

      template< class Function >
      struct FunctionSquare;

      template< class UFunction, class VFunction >
      struct FunctionDistance;

    protected:
      typedef typename GridPartType::template Codim< 0 >::IteratorType GridIteratorType;
      typedef typename GridIteratorType::Entity EntityType;
      typedef CachingQuadrature< GridPartType, 0 > QuadratureType;
      typedef Integrator< QuadratureType > IntegratorType;

      const unsigned int order_;
    public:
      explicit L2Norm ( const GridPartType &gridPart, const unsigned int order = 0 );

      template< class DiscreteFunctionType >
      typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type
      norm ( const DiscreteFunctionType &u ) const;

      template< class UDiscreteFunctionType, class VDiscreteFunctionType >
      typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type
      distance ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v ) const;

      template< class UDiscreteFunctionType,
                class VDiscreteFunctionType,
                class ReturnType >
      inline void
      distanceLocal ( const EntityType& entity, const unsigned int order,
                      const UDiscreteFunctionType &u,
                      const VDiscreteFunctionType &v,
                      ReturnType& sum ) const ;

      template< class UDiscreteFunctionType,
                class ReturnType >
      inline void
      normLocal ( const EntityType& entity, const unsigned int order,
                      const UDiscreteFunctionType &u,
                      ReturnType& sum ) const ;
    };


    // Implementation of L2Norm
    // ------------------------

    template< class GridPart >
    inline L2Norm< GridPart >::L2Norm ( const GridPartType &gridPart, const unsigned int order )
    : BaseType( gridPart ),
      order_( order )
    {}


    template< class GridPart >
    template< class DiscreteFunctionType >
    typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type
    L2Norm< GridPart >::norm ( const DiscreteFunctionType &u ) const
    {
      typedef typename Dune::FieldTraits< typename DiscreteFunctionType::RangeFieldType >::real_type real_type;
      typedef FieldVector< real_type, 1 > ReturnType ;

      // calculate integral over each element
      ReturnType sum = BaseType :: forEach( u, ReturnType(0), order_ );

      // return result, e.g. sqrt of calculated sum
      return sqrt( comm().sum( sum[ 0 ] ) );
    }


    template< class GridPart >
    template< class UDiscreteFunctionType, class VDiscreteFunctionType >
    typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type
    L2Norm< GridPart >
      ::distance ( const UDiscreteFunctionType &u, const VDiscreteFunctionType &v ) const
    {
      typedef typename Dune::FieldTraits< typename UDiscreteFunctionType::RangeFieldType >::real_type real_type;
      typedef FieldVector< real_type, 1 > ReturnType ;

      // calculate integral over each element
      ReturnType sum = BaseType :: forEach( u, v, ReturnType(0), order_ );

      // return result, e.g. sqrt of calculated sum
      return sqrt( comm().sum( sum[ 0 ] ) );
    }

    template< class GridPart >
    template< class DiscreteFunctionType, class ReturnType >
    inline void
    L2Norm< GridPart >::normLocal ( const EntityType& entity, const unsigned int order,
                                    const DiscreteFunctionType &u,
                                    ReturnType& sum ) const
    {
      typedef typename DiscreteFunctionType::LocalFunctionType LocalFunctionType;

      // evaluate norm locally
      IntegratorType integrator( order );

      LocalFunctionType ulocal = u.localFunction( entity );
      FunctionSquare< LocalFunctionType > ulocal2( ulocal );

      integrator.integrateAdd( entity, ulocal2, sum );
    }

    template< class GridPart >
    template< class UDiscreteFunctionType,
              class VDiscreteFunctionType,
              class ReturnType >
    inline void
    L2Norm< GridPart >::distanceLocal ( const EntityType& entity, const unsigned int order,
                                        const UDiscreteFunctionType &u,
                                        const VDiscreteFunctionType &v,
                                        ReturnType& sum ) const
    {
      typedef typename UDiscreteFunctionType::LocalFunctionType ULocalFunctionType;
      typedef typename VDiscreteFunctionType::LocalFunctionType VLocalFunctionType;

      // evaluate norm locally
      IntegratorType integrator( order );

      ULocalFunctionType ulocal = u.localFunction( entity );
      VLocalFunctionType vlocal = v.localFunction( entity );

      typedef FunctionDistance< ULocalFunctionType, VLocalFunctionType > LocalDistanceType;

      LocalDistanceType dist( ulocal, vlocal );
      FunctionSquare< LocalDistanceType > dist2( dist );

      integrator.integrateAdd( entity, dist2, sum );
    }


    template< class GridPart >
    template< class Function >
    struct L2Norm< GridPart >::FunctionSquare
    {
      typedef Function FunctionType;

      typedef typename FunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type real_type;
      typedef FieldVector< real_type, 1 > RangeType;

      explicit FunctionSquare ( const FunctionType &function )
      : function_( function )
      {}

      template< class Point >
      void evaluate ( const Point &x, RangeType &ret ) const
      {
        typename FunctionType::RangeType phi;
        function_.evaluate( x, phi );
        ret = real( phi * phi );
      }

    private:
      const FunctionType &function_;
    };


    template< class GridPart >
    template< class UFunction, class VFunction >
    struct L2Norm< GridPart >::FunctionDistance
    {
      typedef UFunction UFunctionType;
      typedef VFunction VFunctionType;

      typedef typename UFunctionType::RangeFieldType RangeFieldType;
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



    // WeightedL2Norm
    // --------------

    template< class WeightFunction >
    class WeightedL2Norm
    : public L2Norm< typename WeightFunction::DiscreteFunctionSpaceType::GridPartType >
    {
      typedef WeightedL2Norm< WeightFunction > ThisType;
      typedef L2Norm< typename WeightFunction::DiscreteFunctionSpaceType::GridPartType > BaseType;

    public:
      typedef WeightFunction WeightFunctionType;

      typedef typename WeightFunctionType::DiscreteFunctionSpaceType WeightFunctionSpaceType;
      typedef typename WeightFunctionSpaceType::GridPartType GridPartType;

    protected:
      template< class Function >
      struct WeightedFunctionSquare;

      typedef typename WeightFunctionType::LocalFunctionType LocalWeightFunctionType;
      typedef typename WeightFunctionType::RangeType WeightType;

      typedef typename BaseType::GridIteratorType GridIteratorType;
      typedef typename BaseType::IntegratorType IntegratorType;

      typedef typename GridIteratorType::Entity EntityType;

      using BaseType::gridPart;
      using BaseType::comm;

    public:

      using BaseType::norm;
      using BaseType::distance;

      explicit WeightedL2Norm ( const WeightFunctionType &weightFunction, const unsigned int order = 0 );

      template< class UDiscreteFunctionType, class ReturnType >
      void normLocal ( const EntityType &entity, const int order,
              const UDiscreteFunctionType &u,
              ReturnType& sum ) const;

      template< class UDiscreteFunctionType, class VDiscreteFunctionType, class ReturnType >
      void distanceLocal ( const EntityType &entity, const int order,
              const UDiscreteFunctionType &u,
              const VDiscreteFunctionType &v,
              ReturnType& sum ) const;

    private:
      const WeightFunctionType &weightFunction_;
    };




    // Implementation of WeightedL2Norm
    // --------------------------------

    template< class WeightFunction >
    inline WeightedL2Norm< WeightFunction >
      ::WeightedL2Norm ( const WeightFunctionType &weightFunction, const unsigned int order )
    : BaseType( weightFunction.space().gridPart(), order ),
      weightFunction_( weightFunction )
    {
      static_assert( (WeightFunctionSpaceType::dimRange == 1),
                     "Wight function must be scalar." );
    }


    template< class WeightFunction >
    template< class UDiscreteFunctionType, class ReturnType >
    inline void
    WeightedL2Norm< WeightFunction >
      ::normLocal ( const EntityType &entity, const int order,
          const UDiscreteFunctionType &u,
          ReturnType& sum ) const
    {
      typedef typename UDiscreteFunctionType::LocalFunctionType LocalFunctionType;

      // !!!! order !!!!
      IntegratorType integrator( order );

      LocalWeightFunctionType wflocal = weightFunction_.localFunction( entity );
      LocalFunctionType ulocal = u.localFunction( entity );

      WeightedFunctionSquare< LocalFunctionType > ulocal2( wflocal, ulocal );

      integrator.integrateAdd( entity, ulocal2, sum );
    }


    template< class WeightFunction >
    template< class UDiscreteFunctionType, class VDiscreteFunctionType, class ReturnType >
    inline void
    WeightedL2Norm< WeightFunction >
      ::distanceLocal ( const EntityType &entity, const int order,
          const UDiscreteFunctionType &u,
          const VDiscreteFunctionType &v,
          ReturnType &sum ) const
    {

      typedef typename UDiscreteFunctionType::LocalFunctionType ULocalFunctionType;
      typedef typename VDiscreteFunctionType::LocalFunctionType VLocalFunctionType;

      typedef typename BaseType::template FunctionDistance
        < ULocalFunctionType, VLocalFunctionType >
        LocalDistanceType;

      // !!!! order !!!!
      IntegratorType integrator( order );

      LocalWeightFunctionType wflocal = weightFunction_.localFunction( entity );
      ULocalFunctionType ulocal = u.localFunction( entity );
      VLocalFunctionType vlocal = v.localFunction( entity );

      LocalDistanceType dist( ulocal, vlocal );
      WeightedFunctionSquare< LocalDistanceType > dist2( wflocal, dist );

      integrator.integrateAdd( entity, dist2, sum );
    }


    template< class WeightFunction >
    template< class Function >
    struct WeightedL2Norm< WeightFunction >::WeightedFunctionSquare
    {
      typedef Function FunctionType;

      typedef typename FunctionType::RangeFieldType RangeFieldType;
      typedef typename Dune::FieldTraits< RangeFieldType >::real_type real_type;
      typedef FieldVector< real_type, 1 > RangeType;

      WeightedFunctionSquare ( const LocalWeightFunctionType &weightFunction,
                               const FunctionType &function )
      : weightFunction_( weightFunction ),
        function_( function )
      {}

      template< class Point >
      void evaluate ( const Point &x, RangeType &ret ) const
      {
        WeightType weight;
        weightFunction_.evaluate( x, weight );

        typename FunctionType::RangeType phi;
        function_.evaluate( x, phi );
        ret = weight[ 0 ] * (phi * phi);
      }

    private:
      const LocalWeightFunctionType &weightFunction_;
      const FunctionType &function_;
    };

  } // end namespace Fem

} // end namespace Dune

#endif // #ifndef DUNE_FEM_L2NORM_HH
