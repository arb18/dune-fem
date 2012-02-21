#include <config.h>

#define USE_GRAPE HAVE_GRAPE

#ifndef POLORDER
#define POLORDER 1
#endif // #ifndef POLORDER

#include <iostream>

#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/gridpart/leafgridpart.hh>
#include <dune/fem/operator/lagrangeinterpolation.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/space/lagrangespace.hh>

#if USE_GRAPE
#include <dune/grid/io/visual/grapedatadisplay.hh>
#endif

#include "sinespace.hh"


using namespace Dune;

const int polOrder = POLORDER;

typedef LeafGridPart< GridSelector::GridType > GridPartType;

typedef FunctionSpace< double, double, GridSelector::dimworld, 1 > FunctionSpaceType;

typedef LagrangeDiscreteFunctionSpace< FunctionSpaceType, GridPartType, polOrder, CachingStorage >
  DiscreteBaseFunctionSpaceType;

typedef SineReducedBasisSpace< DiscreteBaseFunctionSpaceType, 4 > DiscreteFunctionSpaceType;
typedef AdaptiveDiscreteFunction< DiscreteFunctionSpaceType > DiscreteFunctionType;



template< class FunctionSpaceImp >
class ExactSolution
: public Fem::Function< FunctionSpaceImp, ExactSolution< FunctionSpaceImp > > 
{
  typedef ExactSolution< FunctionSpaceImp > ThisType;
  typedef Fem::Function< FunctionSpaceImp, ThisType > BaseType;

public:
  typedef FunctionSpaceImp FunctionSpaceType;

  typedef typename FunctionSpaceType :: DomainType DomainType;
  typedef typename FunctionSpaceType :: RangeType RangeType;

  typedef typename FunctionSpaceType :: DomainFieldType DomainFieldType;
  typedef typename FunctionSpaceType :: RangeFieldType RangeFieldType;

  enum { DimDomain = FunctionSpaceType :: DimDomain };
  enum { DimRange = FunctionSpaceType :: DimRange };

  void evaluate ( const DomainType &x, RangeType &y ) const
  {
    y = 1;
    for( unsigned int i = 0; i < DimDomain; ++i )
    {
      const DomainFieldType &xi = x[ i ];
      y *= xi - xi * xi;
    }
  }
  
  void evaluate ( const DomainType &x, const RangeFieldType t, RangeType &y ) const
  {
    evaluate( x, y );
  }
};



template< class DiscreteFunctionImp >
class L2Projection
{
public:
  typedef DiscreteFunctionImp DiscreteFunctionType;

private:
  typedef L2Projection< DiscreteFunctionType > ThisType;

public:
  typedef typename DiscreteFunctionType::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;

  typedef typename DiscreteFunctionSpaceType::DomainType DomainType;
  typedef typename DiscreteFunctionSpaceType::RangeType RangeType;

  typedef typename DiscreteFunctionSpaceType::DomainFieldType DomainFieldType;
  typedef typename DiscreteFunctionSpaceType::RangeFieldType RangeFieldType;

public:
  template< class FunctionType >
  static inline void project ( const FunctionType &function,
                               DiscreteFunctionType &discreteFunction )
  {
    typedef typename DiscreteFunctionType::LocalFunctionType LocalFunctionType;

    typedef typename LocalFunctionType::BaseFunctionSetType BaseFunctionSetType;

    typedef typename DiscreteFunctionSpaceType::GridPartType GridPartType;
    typedef typename DiscreteFunctionSpaceType::IteratorType IteratorType;

    typedef typename GridPartType :: GridType :: template Codim< 0 > :: Entity :: Geometry
      GeometryType;

    typedef CachingQuadrature< GridPartType, 0 > QuadratureType;
    typedef typename QuadratureType :: CoordinateType QuadraturePointType;
    
    const DiscreteFunctionSpaceType &dfSpace = discreteFunction.space();

    discreteFunction.clear();

    const IteratorType end = dfSpace.end();
    for( IteratorType it = dfSpace.begin(); it != end; ++it )
    {
      LocalFunctionType localFunction = discreteFunction.localFunction( *it );
      
      const BaseFunctionSetType &baseFunctionSet = localFunction.baseFunctionSet();
      const unsigned int numBaseFunctions = baseFunctionSet.size();

      QuadratureType quadrature( *it, 2*dfSpace.order() + 2);
      const unsigned int numQuadraturePoints = quadrature.nop();
      for( unsigned int pt = 0; pt < numQuadraturePoints; ++pt )
      {
        const QuadraturePointType &point = quadrature.point( pt );
        
        const GeometryType &geometry = it->geometry();

        RangeFieldType weight = quadrature.weight( pt ) * geometry.integrationElement( point ); 
        
        RangeType y;
        function.evaluate( geometry.global( point ), y );

        for( unsigned int i = 0; i < numBaseFunctions; ++i )
        {
          RangeType phi;
          baseFunctionSet.evaluate( i, quadrature[ pt ], phi );
          localFunction[ i ] += weight * (y * phi);
        }
      }
    }
  }
};



template< class DiscreteFunctionImp >
class L2Error
{
public:
  typedef DiscreteFunctionImp DiscreteFunctionType;
  

private:
  typedef L2Error< DiscreteFunctionType > ThisType;

public:
  typedef typename DiscreteFunctionType::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;
  
  typedef typename DiscreteFunctionSpaceType::DomainType DomainType;
  typedef typename DiscreteFunctionSpaceType::RangeType RangeType;

  typedef typename DiscreteFunctionSpaceType::DomainFieldType DomainFieldType;
  typedef typename DiscreteFunctionSpaceType::RangeFieldType RangeFieldType;

  enum { DimDomain = DiscreteFunctionSpaceType::DimDomain };
  enum { DimRange = DiscreteFunctionSpaceType::DimRange };

public:
  template< class FunctionType >
  static inline void norm ( const FunctionType &function,
                            const DiscreteFunctionType &discreteFunction,
                            RangeType &error )
  {
    typedef typename DiscreteFunctionType::LocalFunctionType LocalFunctionType;

    typedef typename DiscreteFunctionSpaceType::GridPartType GridPartType;
    typedef typename DiscreteFunctionSpaceType::IteratorType IteratorType;
    
    typedef typename GridPartType::GridType::template Codim< 0 >::Entity::Geometry
      GeometryType;

    typedef CachingQuadrature< GridPartType, 0 > QuadratureType;
    typedef typename QuadratureType :: CoordinateType QuadraturePointType;
    
    const DiscreteFunctionSpaceType &dfSpace = discreteFunction.space();

    error = 0;

    const IteratorType end = dfSpace.end();
    for( IteratorType it = dfSpace.begin(); it != end ; ++it )
    {
      LocalFunctionType localFunction = discreteFunction.localFunction( *it );
      
      QuadratureType quadrature( *it, 2*dfSpace.order() + 2 );
      const unsigned int numQuadraturePoints = quadrature.nop();
      for( unsigned int pt = 0; pt < numQuadraturePoints; ++pt )
      {
        const QuadraturePointType &point = quadrature.point( pt );
        
        const GeometryType &geometry = it->geometry();

        RangeFieldType weight = quadrature.weight( pt ) * geometry.integrationElement( point );

        RangeType y;
        function.evaluate( geometry.global( point ), y );
        
        RangeType phi;
        localFunction.evaluate( quadrature[ pt ], phi );

        for( unsigned int i = 0; i < DimRange; ++i )
          error[ i ] += weight * SQR( y[ i ] - phi[ i ] );
      }
    }
    
    for( int i = 0; i < DimRange; ++i)
      error[ i ] = sqrt( error[ i ] );
  }
};



double algorithm ( GridPartType &gridPart )
{
  DiscreteBaseFunctionSpaceType baseFunctionSpace( gridPart );
  DiscreteFunctionSpaceType discreteFunctionSpace( baseFunctionSpace );

  DiscreteFunctionType solution( "solution", discreteFunctionSpace );
  ExactSolution< FunctionSpaceType > exactSolution;
  L2Projection< DiscreteFunctionType > :: project( exactSolution, solution );

  FunctionSpaceType :: RangeType error;
  L2Error< DiscreteFunctionType > :: norm( exactSolution, solution, error );
  std :: cout << "L2 Error: " << error << std :: endl;

  #if USE_GRAPE
    GrapeDataDisplay< GridPartType::GridType > grape( gridPart ); 
    grape.dataDisplay( solution );
  #endif
   
  return sqrt( error * error );
}



int main ( int argc, char **argv )
{
  MPIManager::initialize( argc, argv );

  if( argc != 2 )
  {
    std :: cerr << "Usage: " << argv[ 0 ] << " <maxlevel>" << std :: endl;
    return 1;
  }

  try
  {
    typedef GridSelector::GridType GridType;

    unsigned int maxlevel = atoi( argv[ 1 ] );
    double* error = new double[ maxlevel ];

    const unsigned int step = DGFGridInfo< GridType > :: refineStepsForHalf();

    std :: ostringstream macroGridNameStream;
    macroGridNameStream << GridType :: dimension << "dgrid.dgf";
    std :: string macroGridName = macroGridNameStream.str();

    GridPtr< GridType > gridptr( macroGridName );
    GridType &grid = *gridptr;
    GridPartType gridPart( grid );
    
    for( unsigned int i = 0; i <= maxlevel; ++i )
    {
      grid.globalRefine( step );

      error[ i ] = algorithm( gridPart);
      if(i > 0)
      {
        double eoc = log( error[ i-1 ] / error[ i ] ) / M_LN2;
        std :: cout << "EOC = " << eoc << std :: endl;
      }
    }

    delete[] error;
    return 0;
  }
  catch( Exception e )
  {
    std :: cerr << e.what() << std :: endl;
    return 1;
  }
}

