#ifndef DUNE_LAGRANGESPACE_GENERICBASEFUNCTIONS_HH
#define DUNE_LAGRANGESPACE_GENERICBASEFUNCTIONS_HH

#include <dune/config.h>
#include <dune/common/fvector.hh>

#include <dune/fem/space/common/functionspace.hh>
#include <dune/fem/space/common/basefunctionfactory.hh>

#include "genericlagrangepoints.hh"

namespace Dune
{

  template< class FunctionSpaceType, class GeometryType, unsigned int order >
  class GenericLagrangeBaseFunction;
 


  template< class FunctionSpaceType, unsigned int order >
  class GenericLagrangeBaseFunction< FunctionSpaceType, PointGeometry, order >
  : public BaseFunctionInterface< FunctionSpaceType >
  {
  public:
    typedef PointGeometry GeometryType;
      
    enum { polynomialOrder = order };

    typedef GenericLagrangePoint< GeometryType, polynomialOrder >
      LagrangePointType;
    enum { numBaseFunctions = LagrangePointType :: numLagrangePoints };
   
    typedef typename FunctionSpaceType :: DomainType DomainType;
    typedef typename FunctionSpaceType :: RangeType RangeType;

    typedef typename FunctionSpaceType :: DomainFieldType DomainFieldType;
    typedef typename FunctionSpaceType :: RangeFieldType RangeFieldType;

  private:
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         GeometryType,
                                         polynomialOrder >
      ThisType;
    typedef BaseFunctionInterface< FunctionSpaceType > BaseType;

  private:
    CompileTimeChecker< (FunctionSpaceType :: DimRange == 1) > __assert_DimRange__;
    
    const LagrangePointType lagrangePoint_;

  public:
    inline GenericLagrangeBaseFunction( unsigned int baseNum )
    : BaseType(),
      lagrangePoint_( baseNum ) 
    {
    }

    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 1;
    }

    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 0;
    }
    
    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 0;
    }

    virtual void evaluate ( const FieldVector< deriType, 0 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 1 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 2 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }
  };



  template< class FunctionSpaceType, class BaseGeometryType >
  class GenericLagrangeBaseFunction< FunctionSpaceType,
                                     PyramidGeometry< BaseGeometryType >,
                                     0 >
  : public BaseFunctionInterface< FunctionSpaceType >
  {
  public:
    typedef PyramidGeometry< BaseGeometryType > GeometryType;
      
    enum { polynomialOrder = 0 };

    typedef GenericLagrangePoint< GeometryType, polynomialOrder >
      LagrangePointType;
    enum { numBaseFunctions = LagrangePointType :: numLagrangePoints };

   
    typedef typename FunctionSpaceType :: DomainType DomainType;
    typedef typename FunctionSpaceType :: RangeType RangeType;

    typedef typename FunctionSpaceType :: DomainFieldType DomainFieldType;
    typedef typename FunctionSpaceType :: RangeFieldType RangeFieldType;

  private:
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         GeometryType,
                                         polynomialOrder >
      ThisType;
    typedef BaseFunctionInterface< FunctionSpaceType > BaseType;

  private:
    CompileTimeChecker< (FunctionSpaceType :: DimRange == 1) > __assert_DimRange__;

    const LagrangePointType lagrangePoint_;

  public:
    inline GenericLagrangeBaseFunction( unsigned int baseNum )
    : BaseType(),
      lagrangePoint_( baseNum )
    {
    }

    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 1;
    }
    
    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }
  
    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 0;
    }
    
    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }
   
    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      phi[ 0 ] = 0;
    }

    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 0 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 1 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 2 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }
  };
 

  
  template< class FunctionSpaceType,
            class BaseGeometryType,
            unsigned int order >
  class GenericLagrangeBaseFunction< FunctionSpaceType,
                                     PyramidGeometry< BaseGeometryType >,
                                     order >
  : public BaseFunctionInterface< FunctionSpaceType >
  {
  public:
    typedef PyramidGeometry< BaseGeometryType > GeometryType;
      
    enum { polynomialOrder = order };

    typedef GenericLagrangePoint< GeometryType, polynomialOrder >
      LagrangePointType;
    enum { numBaseFunctions = LagrangePointType :: numLagrangePoints };

   
    typedef typename FunctionSpaceType :: DomainType DomainType;
    typedef typename FunctionSpaceType :: RangeType RangeType;

    typedef typename FunctionSpaceType :: DomainFieldType DomainFieldType;
    typedef typename FunctionSpaceType :: RangeFieldType RangeFieldType;

  private:
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         GeometryType,
                                         polynomialOrder >
      ThisType;
    typedef BaseFunctionInterface< FunctionSpaceType > BaseType;

    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         BaseGeometryType,
                                         polynomialOrder >
      DimensionReductionType;
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         GeometryType,
                                         polynomialOrder - 1 >
      OrderReductionType;

  private:
    CompileTimeChecker< (FunctionSpaceType :: DimRange == 1) > __assert_DimRange__;

    const LagrangePointType lagrangePoint_;

  public:
    inline GenericLagrangeBaseFunction( unsigned int baseNum )
    : BaseType(),
      lagrangePoint_( baseNum )
    {
    }

    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      const RangeFieldType divisor = polynomialOrder;
      const RangeFieldType myfactor = porder / divisor;
      const RangeFieldType myshift = (porder - polynomialOrder) / divisor;

      if( LagrangePointType :: useDimReduction( dofCoordinate ) ) {
        DimensionReductionType :: evaluate
          ( dofCoordinate.base(), diffVariable, myfactor * factor, x.base(), phi );
        const unsigned int height
          = LagrangePointType :: height( dofCoordinate );
        for( unsigned int i = 0; i < height; ++i ) {
          ++(*dofCoordinate);
          RangeType psi;
          evaluate< LocalDofCoordinateType, LocalCoordinateType, porder >
            ( dofCoordinate, diffVariable, factor, x, psi );
          phi -= psi;
        }
        (*dofCoordinate) -= height;
      } else {
        --(*dofCoordinate);
        OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                 LocalCoordinateType,
                                                 porder >
          ( dofCoordinate, diffVariable, factor, x, phi );
        ++(*dofCoordinate);
        phi[ 0 ] *= (polynomialOrder / ((RangeFieldType)(*dofCoordinate)))
                  * (myfactor * factor * (*x) - myshift);
      }
    }
    
    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }
   
    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      const RangeFieldType divisor = polynomialOrder;
      const RangeFieldType myfactor = porder / divisor;
      const RangeFieldType myshift = (porder - polynomialOrder) / divisor;

      FieldVector< deriType, 0 > dv;

      if( LagrangePointType :: useDimReduction( dofCoordinate ) ) {
        DimensionReductionType :: evaluate
          ( dofCoordinate.base(), diffVariable, myfactor * factor, x.base(), phi );
        const unsigned int height
          = LagrangePointType :: height( dofCoordinate );
        for( unsigned int i = 0; i < height; ++i ) {
          ++(*dofCoordinate);
          RangeType psi;
          evaluate< LocalDofCoordinateType, LocalCoordinateType, porder >
            ( dofCoordinate, diffVariable, factor, x, psi );
          phi -= psi;
        }
        (*dofCoordinate) -= height;
      } else {
        --(*dofCoordinate);
        OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                 LocalCoordinateType,
                                                 porder >
          ( dofCoordinate, diffVariable, factor, x, phi );
        phi *= (myfactor * factor * (*x) - myshift);
        if( diffVariable[ 0 ] == LocalDofCoordinateType :: index ) {
          RangeType psi;
          OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                   LocalCoordinateType,
                                                   porder >
            ( dofCoordinate, dv, factor, x, psi );
          // psi *= factor; phi += psi;
          phi.axpy( factor, psi );
        }
        ++(*dofCoordinate);
        phi *= (myfactor * polynomialOrder)
             / ((RangeFieldType)(*dofCoordinate));
      }
    }
    
    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }
   
    template< class LocalDofCoordinateType,
              class LocalCoordinateType,
              unsigned int porder >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      const RangeFieldType divisor = polynomialOrder;
      const RangeFieldType myfactor = porder / divisor;
      const RangeFieldType myshift = (porder - polynomialOrder) / divisor;

      FieldVector< deriType, 1 > dv0( diffVariable[ 0 ] );
      FieldVector< deriType, 1 > dv1( diffVariable[ 1 ] );

      if( LagrangePointType :: useDimReduction( dofCoordinate ) ) {
        DimensionReductionType :: evaluate
          ( dofCoordinate.base(), diffVariable, myfactor * factor, x.base(), phi );
        const unsigned int height
          = LagrangePointType :: height( dofCoordinate );
        for( unsigned int i = 0; i < height; ++i ) {
          ++(*dofCoordinate);
          RangeType psi;
          evaluate< LocalDofCoordinateType, LocalCoordinateType, porder >
            ( dofCoordinate, diffVariable, factor, x, psi );
          phi -= psi;
        }
        (*dofCoordinate) -= height;
      } else {
        RangeType psi;
        --(*dofCoordinate);
        OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                 LocalCoordinateType,
                                                 porder >
          ( dofCoordinate, diffVariable, factor, x, phi );
        phi *= (myfactor * factor * (*x) - myshift);
        if( diffVariable[ 0 ] == LocalDofCoordinateType :: index ) {
          OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                   LocalCoordinateType,
                                                   porder >
            ( dofCoordinate, dv1, factor, x, psi );
          //psi *= factor; phi += psi;
          phi.axpy( factor, psi );
        }
        if( diffVariable[ 1 ] == LocalDofCoordinateType :: index ) {
          OrderReductionType :: template evaluate< LocalDofCoordinateType,
                                                   LocalCoordinateType,
                                                   porder >
            ( dofCoordinate, dv0, factor, x, psi );
          // psi *= factor; phi += psi;
          phi.axpy( factor, psi );
        }
        ++(*dofCoordinate);
        phi *= (myfactor * myfactor * polynomialOrder)
             / ((RangeFieldType)(*dofCoordinate));
      }
    }
    
    template< class LocalDofCoordinateType,
              class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      return evaluate< LocalDofCoordinateType,
                       LocalCoordinateType,
                       polynomialOrder >
        ( dofCoordinate, diffVariable, factor, x, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 0 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 1 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 2 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }
  };
 

  
  template< class FunctionSpaceType,
            class FirstGeometryType,
            class SecondGeometryType,
            unsigned int order >
  class GenericLagrangeBaseFunction< FunctionSpaceType,
                                     ProductGeometry< FirstGeometryType,
                                                      SecondGeometryType >,
                                     order >
  : public BaseFunctionInterface< FunctionSpaceType >
  {
  public:
    typedef ProductGeometry< FirstGeometryType, SecondGeometryType >
      GeometryType;
      
    enum { polynomialOrder = order };

    typedef GenericLagrangePoint< GeometryType, polynomialOrder >
      LagrangePointType;
    enum { numBaseFunctions = LagrangePointType :: numLagrangePoints };

    typedef typename FunctionSpaceType :: DomainType DomainType;
    typedef typename FunctionSpaceType :: RangeType RangeType;

    typedef typename FunctionSpaceType :: DomainFieldType DomainFieldType;
    typedef typename FunctionSpaceType :: RangeFieldType RangeFieldType;

  private:
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         GeometryType,
                                         polynomialOrder >
      ThisType;
    typedef BaseFunctionInterface< FunctionSpaceType > BaseType;

    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         FirstGeometryType,
                                         polynomialOrder >
      FirstReductionType;
    typedef GenericLagrangeBaseFunction< FunctionSpaceType,
                                         SecondGeometryType,
                                         polynomialOrder >
      SecondReductionType;

  private:
    CompileTimeChecker< (FunctionSpaceType :: DimRange == 1) > __assert_DimRange__;

    const LagrangePointType lagrangePoint_;

  public:
    inline GenericLagrangeBaseFunction( unsigned int baseNum )
    : BaseType(),
      lagrangePoint_( baseNum )
    {
    }

    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 0 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      RangeType psi;
      FirstReductionType :: evaluate
        ( dofCoordinate.first(), diffVariable, factor, x.first(), phi );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), diffVariable, factor, x.second(), psi );
      phi[ 0 ] *= psi[ 0 ];
    }
    
    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 1 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      FieldVector< deriType, 0 > dv;
      RangeType psi1, psi2;
      
      FirstReductionType :: evaluate
        ( dofCoordinate.first(), diffVariable, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), dv, factor, x.second(), psi2 );
      phi[ 0 ] = psi1[ 0 ] * psi2[ 0 ];

      FirstReductionType :: evaluate
        ( dofCoordinate.first(), dv, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), diffVariable, factor, x.second(), psi2 );
      phi[ 0 ] += psi1[ 0 ] * psi2[ 0 ];
    }
    
    template< class LocalDofCoordinateType, class LocalCoordinateType >
    inline static void evaluate ( LocalDofCoordinateType &dofCoordinate,
                                  const FieldVector< deriType, 2 > &diffVariable,
                                  DomainFieldType factor,
                                  const LocalCoordinateType &x,
                                  RangeType &phi )
    {
      FieldVector< deriType, 0 > dv;
      FieldVector< deriType, 1 > dv0( diffVariable[ 0 ] );
      FieldVector< deriType, 1 > dv1( diffVariable[ 1 ] );
      RangeType psi1, psi2;
      
      FirstReductionType :: evaluate
        ( dofCoordinate.first(), diffVariable, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), dv, factor, x.second(), psi2 );
      phi[ 0 ] = psi1[ 0 ] * psi2[ 0 ];

      FirstReductionType :: evaluate
        ( dofCoordinate.first(), dv0, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), dv1, factor, x.second(), psi2 );
      phi[ 0 ] += psi1[ 0 ] * psi2[ 0 ];
      
      FirstReductionType :: evaluate
        ( dofCoordinate.first(), dv1, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), dv0, factor, x.second(), psi2 );
      phi[ 0 ] += psi1[ 0 ] * psi2[ 0 ];
      
      FirstReductionType :: evaluate
        ( dofCoordinate.first(), dv, factor, x.first(), psi1 );
      SecondReductionType :: evaluate
        ( dofCoordinate.second(), diffVariable, factor, x.second(), psi2 );
      phi[ 0 ] += psi1[ 0 ] * psi2[ 0 ];
    }

    virtual void evaluate ( const FieldVector< deriType, 0 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 1 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }

    virtual void evaluate ( const FieldVector< deriType, 2 > &diffVariable,
                            const DomainType &x,
                            RangeType &phi ) const
    {
      const LocalCoordinate< GeometryType, DomainType >
        xlocal( const_cast< DomainType& >( x ) );
      LagrangePointType point( lagrangePoint_ );
      evaluate( point.localDofCoordinate_, diffVariable, 1, xlocal, phi );
    }
  };
}

#endif
