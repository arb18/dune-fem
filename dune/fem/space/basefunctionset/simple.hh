#ifndef DUNE_FEM_BASEFUNCTIONSET_SIMPLE_HH
#define DUNE_FEM_BASEFUNCTIONSET_SIMPLE_HH

#include <dune/geometry/type.hh>

#include <dune/fem/space/basefunctionset/functor.hh>

namespace Dune
{

  namespace Fem
  {

    template< class ShapeFunctionSet >
    class SimpleBaseFunctionSet
    {
      typedef SimpleBaseFunctionSet< ShapeFunctionSet > ThisType;

    public:
      typedef ShapeFunctionSet ShapeFunctionSetType;

      typedef typename ShapeFunctionSetType::RangeType RangeType;

      typedef typename ShapeFunctionSetType::HessianRangeType LocalHessianRangeType;
      typedef typename ShapeFunctionSetType::JacobianRangeType LocalJacobianRangeType;

      const ShapeFunctionSet &shapeFunctionSet () const { return shapeFunctionSet_; }
      const GeometryType &geometry () const { return geometry_; }


      // interface methods

      template< class Point, class DofVector >
      void axpy ( const Point &x, const RangeType &valueFactor, DofVector &dofs ) const
      {
        FunctionalAxpyFunctor< RangeType, DofVector > f( valueFactor, dofs );
        shapeFunctionSet().evaluateEach( x, f );
      }

      template< class Point, class DofVector >
      void axpy ( const Point &x, const JacobianRangeType &jacobianFactor, DofVector &dofs ) const
      {
        LocalJacobianRangeType tmpJacobianFactor;
        for( int r = 0; r < dimRange; ++r )
          gjit.mtv( jacobianFactor[ r ], tmpJacobianFactor[ r ] );

        FunctionalAxpyFunctor< LocalJacobianRangeType, DofVector > f( jacobianFactor, dofs );
        shapeFunctionSet().evaluateEach( x, f );
      }

      template< class Point, class DofVector >
      void axpy ( const Point &x, const RangeType &valueFactor, const JacobianRangeType &jacobianFactor,
                  DofVector &dofs ) const
      {
        axpy( x, valueFactor, dofs );
        axpy( x, jacobianFactor, dofs );
      }

      template< class Point, class DofVector >
      void evaluateAll ( const Point &x, const DofVector &dofs, RangeType &value ) const
      {
        AxpyFunctor< DofVector, RangeType > f( dofs, value );
        shapeFunctionSet().evaluateEach( x, f );
      }

      template< class Point, class RangeArray >
      void evaluateAll ( const Point &x, RangeArray &values ) const
      {
        AssignFunctor< RangeArray > f( values );
        shapeFunctionSet().evaluateEach( x, f );
      }

      Dune::GeometryType geometryType () const { return geometry().type(); }

      template< class Point, class DofVector >
      void hessianAll ( const Point &x, const DofVector &dofs, HessianRangeType &hessian ) const
      {
        LocalHessianRangeType localHessian( RangeFieldType( 0 ) );
        AxpyFunctor< DofVector, LocalHessianRangeType > f( dofs, localHessian );
        shapeFunctionSet().hessianEach( x, f );

        typedef HessianTransformation< GeometryType > Transformation;
        Transformation transformation( geometry(), coordinate( x ) );
        transformation( localHessian, hessian );
      }

      template< class Point, class HessianRangeArray >
      void hessianAll ( const Point &x, HessianRangeArray &hessians ) const
      {
        typedef HessianTransformation< GeometryType > Transformation;
        Transformation transformation( geometry(), coordinate( x ) );
        AssignFunctor< JacobianRangeArray, Transformation > f( hessians, transformation );
        shapeFunctionSet().hessianEach( x, f );
      }

      template< class Point, class DofVector >
      void jacobianAll ( const Point &x, const DofVector &dofs, JacobianRangeType &jacobian ) const
      {
        LocalJacobianRangeType localJacobian( RangeFieldType( 0 ) );
        AxpyFunctor< DofVector, LocalJacobianRangeType > f( dofs, localJacobian );
        shapeFunctionSet().jacobianEach( x, f );

        typedef JacobianTransformation< GeometryType > Transformation;
        Transformation transformation( geometry(), coordinate( x ) );
        transformation( localJacobian, jacobian );
      }

      template< class Point, class JacobianRangeArray >
      void jacobianAll ( const Point &x, JacobianRangeArray &jacobians ) const
      {
        typedef JacobianTransformation< GeometryType > Transformation;
        Transformation transformation( geometry(), coordinate( x ) );
        AssignFunctor< JacobianRangeArray, Transformation > f( jacobians, transformation );
        shapeFunctionSet().jacobianEach( x, f );
      }

      std::size_t size () const { return shapeFunctionSet().size(); }


      // depreceted methods (dune-fem-plus)

      template< class Point, class GeometryJacobianInverse, class DofVector >
      void axpy ( const Point &x, const GeometryJacobianInverse &gjit,
                  const JacobianRangeType &jacobianFactor, DofVector &dofs ) const
      {
        return axpy( x, jacobianFactor, dofs );
      }

      /** \todo please doc me */
      template< class Point, class GeometryJacobianInverse, class DofVector >
      void axpy ( const Point &x, const GeometryJacobianInverse &gjit,
                  const RangeType &valueFactor, const JacobianRangeType &jacobianFactor,
                  DofVector &dofs ) const
      {
        return axpy( x, valueFactor, jacobianFactor, dofs );
      }

      template< class Point, class Geometry, class DofVector >
      void hessianAll ( const Point &x, const Geometry &geometry,
                        const DofVector &dofs, HessianRangeType &hessian ) const
      {
        hessianAll( x, dofs, hessian );
      }

      template< class Point, class Geometry, class GlobalHessianRangeArray >
      void hessianAll ( const Point &x, const Geometry &geometry, GlobalHessianRangeArray &hessians ) const
      {
        hessianAll( x, dofs, hessians );
      }

      template< class Point, class GeometryJacobianInverse, class DofVector >
      void jacobianAll ( const Point &x, const GeometryJacobianInverse &gjit,
                         const DofVector &dofs, JacobianRangeType &jacobian ) const
      {
        jacobianAll( x, dofs, jacobian );
      }

      template< class Point, class GeometryJacobianInverse, class GlobalJacobianRangeArray >
      void jacobianAll ( const Point &x, const GeometryJacobianInverse &gjit, GlobalJacobianRangeArray &jacobians ) const
      {
        jacobianAll( x, jacobians );
      }

    private:
      const ShapeFunctionSetType &shapeFunctionSet_;
      GeometryType geometry_;
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_BASEFUNCTIONSET_SIMPLE_HH
