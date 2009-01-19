#undef NDEBUG

#include <config.h>
#include <iostream>

#include <dune/fem/gridpart/gridpart.hh>
#include <dune/fem/space/dgspace/dgadaptiveleafgridpart.hh>

#include <dune/fem/misc/gridwidth.hh>

#if defined USE_BLOCKVECTORFUNCTION
#include <dune/fem/function/blockvectorfunction.hh>
#elif defined USE_VECTORFUNCTION
#include <dune/fem/storage/vector.hh>
#include <dune/fem/function/vectorfunction.hh>
#elif defined USE_ATTACHEDFUNCTION
#include <dune/fem/function/attachedfunction/function.hh>
#else
#include <dune/fem/function/adaptivefunction.hh>
#endif

#if defined  USE_FILTEREDGRID 
#include <dune/fem/gridpart/filteredgrid.hh>
#endif

#include "testgrid.hh"
#include "dfspace.hh"
#include "dgl2projection.hh"
#include "exactsolution.hh"

using namespace Dune;

// typedef HierarchicGridPart< GridType >  ContainedGridPartType;
typedef DGAdaptiveLeafGridPart< GridType > ContainedGridPartType;

// use filtered grid for testing 
#if defined  USE_FILTEREDGRID 
  typedef RadialFilter< ContainedGridPartType > FilterType;
  typedef FilteredGridPart<ContainedGridPartType, FilterType> GridPartType;
#else
  typedef ContainedGridPartType GridPartType;
#endif

typedef TestFunctionSpace FunctionSpaceType;
typedef TestDiscreteFunctionSpace< GridPartType > DiscreteFunctionSpaceType;

#if defined USE_BLOCKVECTORFUNCTION
typedef BlockVectorDiscreteFunction< DiscreteFunctionSpaceType >
  DiscreteFunctionType;
#elif defined USE_VECTORFUNCTION
typedef ManagedDiscreteFunction
  < VectorDiscreteFunction
    < DiscreteFunctionSpaceType,
      DynamicVector< FunctionSpaceType :: RangeFieldType > > >
  DiscreteFunctionType;
#elif defined USE_ATTACHEDFUNCTION
typedef AttachedDiscreteFunction< DiscreteFunctionSpaceType >
  DiscreteFunctionType;
#else
typedef AdaptiveDiscreteFunction< DiscreteFunctionSpaceType >
  DiscreteFunctionType;
#endif

typedef ExactSolution< FunctionSpaceType > ExactSolutionType;

int main(int argc, char ** argv) 
{
  MPIManager :: initialize( argc, argv );
  try
  {
    GridType &grid = TestGrid :: grid();
    const int step = TestGrid :: refineStepsForHalf();
    grid.globalRefine( 2*step );

    GridPartType gridPart( grid );
    // add check for grid width 
    std::cout << "Grid width: " 
      << GridWidth :: calcGridWidth( gridPart ) << std::endl; 

    DiscreteFunctionSpaceType discreteFunctionSpace( gridPart );
    ExactSolutionType exactSolution( discreteFunctionSpace );
    DiscreteFunctionType solution( "solution", discreteFunctionSpace );
    solution.clear();
  
    // perform the L2Projection
    DGL2Projection< DiscreteFunctionType > :: project( exactSolution, solution );

    return 0;
  }
  catch( Exception e )
  {
    std :: cerr << e.what() << std :: endl;
    return 1;
  }
}
