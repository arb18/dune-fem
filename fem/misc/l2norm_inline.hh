namespace Dune
{
  
  template< class GridPartImp >
  inline L2Norm< GridPartImp > :: L2Norm ( const GridPartType &gridPart )
  : gridPart_( gridPart )
  {
  }



  template< class GridPartImp >
  inline L2Norm< GridPartImp > :: L2Norm ( const ThisType &other )
  : gridPart_( other.gridPart_ )
  {
  }


  
  template< class GridPartImp >
  template< class DiscreteFunctionType >
  inline typename DiscreteFunctionType :: RangeFieldType
  L2Norm< GridPartImp > :: norm ( const DiscreteFunctionType &u ) const
  {
    typedef typename DiscreteFunctionType :: RangeFieldType RangeFieldType;
    typedef typename DiscreteFunctionType :: RangeType RangeType;

    typedef typename DiscreteFunctionType :: LocalFunctionType LocalFunctionType;

    RangeFieldType sum( 0 );
    unsigned int order = 2 * u.space().order();

    const GridIteratorType end = gridPart_.template end< 0 >();
    for( GridIteratorType it = gridPart_.template begin< 0 >(); it != end; ++it )
    {
      const EntityType &entity = *it;
      const GeometryType &geometry = entity.geometry();

      LocalFunctionType ulocal = u.localFunction( entity );
      
      QuadratureType quadrature( entity, order );
      const unsigned int numQuadraturePoints = quadrature.nop();
      for( unsigned int qp = 0; qp < numQuadraturePoints; ++qp )
      {
        const RangeFieldType factor
          = quadrature.weight( qp )
            * geometry.integrationElement( quadrature.point( qp ) );
        
        RangeType uqp;
        ulocal.evaluate( quadrature[ qp ], uqp );

        sum += factor * (uqp * uqp);
      }
    }

    sum = gridPart_.grid().comm().sum( sum );
    return sqrt( sum );
  }

 
  
  template< class GridPartImp >
  template< class UDiscreteFunctionType, class VDiscreteFunctionType >
  inline typename UDiscreteFunctionType :: RangeFieldType
  L2Norm< GridPartImp > :: distance ( const UDiscreteFunctionType &u,
                                      const VDiscreteFunctionType &v ) const
  {
    typedef typename UDiscreteFunctionType :: RangeFieldType RangeFieldType;
    typedef typename UDiscreteFunctionType :: RangeType RangeType;

    typedef typename UDiscreteFunctionType :: LocalFunctionType ULocalFunctionType;
    typedef typename VDiscreteFunctionType :: LocalFunctionType VLocalFunctionType;

    RangeFieldType sum( 0 );
    const unsigned int uorder = 2 * u.space().order();
    const unsigned int vorder = 2 * v.space().order();
    const unsigned int order = (uorder >= vorder ? uorder : vorder);

    const GridIteratorType end = gridPart_.template end< 0 >();
    for( GridIteratorType it = gridPart_.template begin< 0 >(); it != end; ++it )
    {
      const EntityType &entity = *it;
      const GeometryType &geometry = entity.geometry();

      ULocalFunctionType ulocal = u.localFunction( entity );
      VLocalFunctionType vlocal = v.localFunction( entity );
      
      QuadratureType quadrature( entity, order );
      const unsigned int numQuadraturePoints = quadrature.nop();
      for( unsigned int qp = 0; qp < numQuadraturePoints; ++qp )
      {
        const RangeFieldType factor
          = quadrature.weight( qp )
            * geometry.integrationElement( quadrature.point( qp ) );
        
        RangeType uqp;
        ulocal.evaluate( quadrature[ qp ], uqp );
        
        RangeType vqp;
        vlocal.evaluate( quadrature[ qp ], vqp );

        uqp -= vqp;
        sum += factor * (uqp * uqp);
      }
    }

    sum = gridPart_.grid().comm().sum( sum );
    return sqrt( sum );
  }

}
