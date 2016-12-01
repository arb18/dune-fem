#include <config.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include <dune/common/exceptions.hh>

#include <dune/fem/gridpart/leafgridpart.hh>
#include <dune/fem/gridpart/adaptiveleafgridpart.hh>

#include <dune/fem/test/testgrid.hh>



int main ( int argc, char ** argv )
try
{
  Dune::Fem::MPIManager::initialize( argc, argv );

  // create grid
  auto& grid = Dune::Fem::TestGrid::grid();

  // refine grid
  const int step = Dune::Fem::TestGrid::refineStepsForHalf();
  grid.globalRefine( 2*step );
  grid.loadBalance();

  // create grid part
  using GridType = Dune::GridSelector::GridType;
  using GridPartType = Dune::Fem::AdaptiveLeafGridPart< GridType >;
  using GridViewType = typename GridPartType::GridViewType;

  GridPartType gp( grid );

  auto gv1 = static_cast< GridViewType >( gp );
  auto gv2 = static_cast< GridViewType >( gp );

  gv1 = gv2;

  return 0;
}
catch( const Dune::Exception &e )
{
  std::cerr << e << std::endl;
  return 1;
}
catch( ... )
{
  std::cerr << "Generic exception!" << std::endl;
  return 2;
}
