#ifndef DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LOCALINTERPOLATION_HH
#define DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LOCALINTERPOLATION_HH

// dune-fem includes
#include <dune/grid/common/capabilities.hh>
#include <dune/fem/operator/1order/localmassmatrix.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>

/**
  @file
  @author Christoph Gersbacher
  @brief Local interpolation for Discontinuous Galerkin spaces
*/

namespace Dune
{

  namespace Fem
  {

    // DiscontinuousGalerkinLocalInterpolation
    // ---------------------------------------

    /**
     * Local interpolation for Discontinuous Galerkin spaces.
     */
    template< class DiscreteFunctionSpace, template< class, int > class Quadrature = CachingQuadrature >
    class DiscontinuousGalerkinLocalInterpolation
    {
      typedef DiscontinuousGalerkinLocalInterpolation< DiscreteFunctionSpace, Quadrature > ThisType;

    public:
      typedef DiscreteFunctionSpace DiscreteFunctionSpaceType;
      typedef typename DiscreteFunctionSpaceType::GridType  GridType;
      typedef typename DiscreteFunctionSpaceType::EntityType EntityType;

      static const bool isAlwaysAffine = Dune::Capabilities::isCartesian< GridType >::v ||
         ( Dune::Capabilities::hasSingleGeometryType< GridType >::v &&  ((Dune::Capabilities::hasSingleGeometryType< GridType >::topologyId >> 1) == 0)) ;
      // always true for orthonormal spaces
      //static const bool isAlwaysAffine = true;

    private:
      typedef typename DiscreteFunctionSpaceType::RangeType RangeType;
      typedef typename RangeType::value_type RangeFieldType;

      typedef typename DiscreteFunctionSpaceType::GridPartType GridPartType;
      typedef Quadrature< GridPartType, EntityType::codimension > QuadratureType;

      typedef LocalMassMatrix< DiscreteFunctionSpaceType, QuadratureType > LocalMassMatrixType;

    public:
      DiscontinuousGalerkinLocalInterpolation ( const DiscreteFunctionSpaceType &space, const int order = -1 )
      : order_( space.order() ),
        massMatrix_( space, (order < 0 ? 2*space.order() : order) ),
        values_()
      {}

      DiscontinuousGalerkinLocalInterpolation ( const ThisType &other ) = default;

      ThisType &operator= ( const ThisType &other ) = delete;

      template< class LocalFunction, class LocalDofVector >
      void operator () ( const LocalFunction &localFunction, LocalDofVector &dofs ) const
      {
        // set all dofs to zero
        dofs.clear();

        // get entity and geometry
        const EntityType &entity = localFunction.entity();

        QuadratureType quadrature( entity, localFunction.order() + order_);

        const int nop = quadrature.nop();
        // adjust size of values
        values_.resize( nop );

        // evaluate local function for all quadrature points
        localFunction.evaluateQuadrature( quadrature, values_ );

        bool isAffine = isAlwaysAffine ;
        if( ! isAlwaysAffine )
        {
          const auto geometry = entity.geometry();
          isAffine = geometry.affine();

          if( ! isAffine )
          {
            // apply weight
            for(auto qp : quadrature )
              values_[ qp.index() ] *= qp.weight() * geometry.integrationElement( qp.position() );
          }
        }

        if( isAffine )
        {
          // apply weight only
          for(auto qp : quadrature )
            values_[ qp.index() ] *= qp.weight();
        }

        // add values to local function
        dofs.axpyQuadrature( quadrature, values_ );

        if( ! isAffine )
        {
          // apply inverse of mass matrix
          massMatrix().applyInverse( entity, dofs );
        }
      }

    private:
      const LocalMassMatrixType &massMatrix () const { return massMatrix_; }

      const int order_;
      LocalMassMatrixType massMatrix_;
      mutable std::vector< RangeType > values_;
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LOCALINTERPOLATION_HH
