#ifndef DUNE_FEM_SHAPEFUNCTIONSET_VECTORIAL_HH
#define DUNE_FEM_SHAPEFUNCTIONSET_VECTORIAL_HH

#include <algorithm>

#include <dune/geometry/type.hh>

#include <dune/fem/space/common/functionspace.hh>
#include <dune/fem/space/shapefunctionset/shapefunctionset.hh>

namespace Dune
{

  namespace Fem 
  {

    // MakeVectorial
    // -------------

    template< class ScalarFunctionSpace, class RangeVector >
    struct MakeVectorial;

    template< class DomainField, class RangeField, int dimD, int dimR >
    struct MakeVectorial< FunctionSpace< DomainField, RangeField, dimD, 1 >, FieldVector< RangeField, dimR > >
    {
      typedef FunctionSpace< DomainField, RangeField, dimD, 1 > ScalarFunctionSpaceType;
      typedef FunctionSpace< DomainField, RangeField, dimD, dimR > VectorialFunctionSpaceType;

      static const int dimRangeFactor = dimR;

      static typename VectorialFunctionSpaceType::RangeType
      makeVectorial ( int k, const typename ScalarFunctionSpaceType::RangeType &scalarValue )
      {
        typename VectorialFunctionSpaceType::RangeType vectorialValue( RangeField( 0 ) );
        vectorialValue[ k ] = scalarValue[ 0 ];
      }

      static typename VectorialFunctionSpaceType::JacobianRangeType
      makeVectorial ( int k, const typename ScalarFunctionSpaceType::JacobianRangeType &scalarValue )
      {
        typename VectorialFunctionSpaceType::JacobianRangeType vectorialValue( RangeField( 0 ) );
        vectorialValue[ k ] = scalarValue[ 0 ];
      }

      static typename VectorialFunctionSpaceType::HessianRangeType
      makeVectorial ( int k, const typename ScalarFunctionSpaceType::HessianRangeType &scalarValue )
      {
        typename VectorialFunctionSpaceType::HessianRangeType vectorialValue( RangeField( 0 ) );
        vectorialValue[ k ] = scalarValue[ 0 ];
      }
    };



    // VectorialShapeFunctionSet
    // -------------------------

    template< class ScalarShapeFunctionSet, class RangeVector >
    class VectorialShapeFunctionSet
    : public ShapeFunctionSet< typename MakeVectorial< typename ScalarShapeFunctionSet::FunctionSpaceType, RangeVector >::VectorialFunctionSpaceType, 
                               VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >
                             >
    {
      typedef VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector > ThisType;
      typedef ShapeFunctionSet< typename MakeVectorial< typename ScalarShapeFunctionSet::FunctionSpaceType, RangeVector >::VectorialFunctionSpaceType, 
                                VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >
                              > BaseType;

    public:
      typedef ScalarShapeFunctionSet ScalarShapeFunctionSetType;

    protected:
      typedef typename ScalarShapeFunctionSetType::FunctionSpaceType ScalarFunctionSpaceType;
      typedef MakeVectorial< ScalarFunctionSpaceType, RangeVector > MakeVectorialType;

      static const int dimRangeFactor = MakeVectorialType::dimRangeFactor;

      template< class Functor >
      struct VectorialFunctor;

    public:
      typedef typename BaseType::FunctionSpaceType FunctionSpaceType;

      explicit VectorialShapeFunctionSet ( const ScalarShapeFunctionSetType &scalarShapeFunctionSet = ScalarShapeFunctionSetType() )
      : scalarShapeFunctionSet_( scalarShapeFunctionSet )
      {}

      const ScalarShapeFunctionSetType &scalarShapeFunctionSet () const { return scalarShapeFunctionSet_; }

      // Shape Function Set Interface Methods

      GeometryType type () const { return scalarShapeFunctionSet_.type(); }
      
      std::size_t size () const { return dimRangeFactor * scalarShapeFunctionSet().size(); }

      template< class Point, class Functor >
      void evaluateEach ( const Point &x, Functor functor ) const;

      template< class Point, class Functor > 
      void jacobianEach ( const Point &x, Functor functor ) const;

      template< class Point, class Functor > 
      void hessianEach ( const Point &x, Functor functor ) const;
     
    protected:
      ScalarShapeFunctionSet scalarShapeFunctionSet_;
    };



    // VectorialShapeFunctionSet::VectorialFunctor
    // -------------------------------------------

    template< class ScalarShapeFunctionSet, class RangeVector >
    template< class Functor >
    struct VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >::VectorialFunctor
    {
      explicit VectorialFunctor ( const Functor &functor )
      : functor_( functor )
      {}

      template< class Value >
      void operator() ( const std::size_t i, const Value &value )
      {
        for( int k = 0; k < dimRangeFactor; ++k )
          functor_( i*dimRangeFactor+k, MakeVectorialType::makeVectorial( k, value ) );
      }

    private:
      Functor functor_;
    };



    // Implementation of VectorialShapeFunctionSet
    // -------------------------------------------

    template< class ScalarShapeFunctionSet, class RangeVector >
    template< class Point, class Functor >
    inline void VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >
      ::evaluateEach ( const Point &x, Functor functor ) const
    {
      scalarShapeFunctionSet().evaluateEach( x, VectorialFunctor< Functor >( functor ) );
    }
    

    template< class ScalarShapeFunctionSet, class RangeVector >
    template< class Point, class Functor >
    inline void VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >
      ::jacobianEach ( const Point &x, Functor functor ) const
    {
      scalarShapeFunctionSet().jacobianEach( x, VectorialFunctor< Functor >( functor ) );
    }


    template< class ScalarShapeFunctionSet, class RangeVector >
    template< class Point, class Functor >
    inline void VectorialShapeFunctionSet< ScalarShapeFunctionSet, RangeVector >
      ::hessianEach ( const Point &x, Functor functor ) const
    {
      scalarShapeFunctionSet().hessianEach( x, VectorialFunctor< Functor >( functor ) );
    }

  } // namespace Fem 

} // namespace Dune

#endif // #ifndef DUNE_FEM_SHAPEFUNCTIONSET_VECTORIAL_HH
