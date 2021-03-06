#include <iostream>
#include <config.h>
#include <string>
#include <sstream>

static const int dimw = Dune::GridSelector::dimworld;

#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/space/discontinuousgalerkin.hh>
#include <dune/fem/space/lagrange.hh>

#include <dune/fem/gridpart/adaptiveleafgridpart.hh>
#include <dune/fem/space/common/adaptationmanager.hh>

#include <dune/fem/space/common/interpolate.hh>
#include <dune/fem/misc/l2norm.hh>

#include <dune/fem/misc/double.hh>

// to use grape, set to WANT_GRAPE to 1
#ifndef WANT_GRAPE
#define WANT_GRAPE 0
#endif

#if HAVE_GRAPE
  #define USE_GRAPE WANT_GRAPE
#else
  #define USE_GRAPE 0
  #if WANT_GRAPE
    #warning "Grape was not found by configure."
  #endif
#endif

#if USE_GRAPE
  #include <dune/grid/io/visual/grapedatadisplay.hh>
#endif

#include <dune/fem/test/testgrid.hh>

using namespace Dune;
using namespace Fem;

// polynom approximation order of quadratures,
// at least polynom order of basis functions
const int polOrd = POLORDER;

//***********************************************************************
/*! L2 Projection of a function f:

  This is an example how to solve the equation on
  \f[\Omega = (0,1)^2 \f]

  \f[ \int_{\Omega} u \phi = \int_{\Omega} f \phi  \ \ \ in \Omega \f]
  \f[ f(x,y) = x ( 1 - x) y ( 1 - y ) \f]

  Here u is the L_2 projection of f.

  The Projection should converge to the given function f.
  with the finite element method using lagrangian elements of polynom order +1.
*/
//***********************************************************************

//! the index set we are using
typedef GridSelector::GridType MyGridType;
//typedef HierarchicGridPart< MyGridType > GridPartType;
//typedef DGAdaptiveLeafGridPart< MyGridType > GridPartType;
typedef AdaptiveLeafGridPart< MyGridType > GridPartType;

//! define the function space, \f[ \R^2 \rightarrow \R \f]
// see dune/common/functionspace.hh
//typedef MatrixFunctionSpace < double , double, dimw , 3,5 > FuncSpace;
typedef FunctionSpace < MyGridType::ctype, double, dimw, 2 > FuncSpace;

//! define the function space our unkown belong to
//! see dune/fem/lagrangebase.hh
typedef DiscontinuousGalerkinSpace<FuncSpace, GridPartType,
	polOrd,CachingStorage> DiscreteFunctionSpaceType;

//! define the type of discrete function we are using , see
//! dune/fem/discfuncarray.hh
typedef AdaptiveDiscreteFunction < DiscreteFunctionSpaceType > DiscreteFunctionType;

//! Get the Dofmanager type
typedef DofManager< MyGridType > DofManagerType;


// the exact solution to the problem for EOC calculation
struct ExactSolution
: public Fem::Function< FuncSpace, ExactSolution >
{
  typedef FuncSpace::RangeType RangeType;
  typedef FuncSpace::RangeFieldType RangeFieldType;
  typedef FuncSpace::DomainType DomainType;

  //! f(x,y) = x*(1-x)*y*(1-y)
  void evaluate (const DomainType & x , RangeType & ret)  const
  {
    ret = 2.; // maximum of function is 2
    for (int j=0;j<RangeType::dimension; j++)
      for(int i=0; i<DomainType::dimension; i++)
	ret[j] *= pow(x[i]*(1.0 -x[i])*4.,double(j+1));
  }

  void evaluate (const DomainType & x , RangeFieldType time , RangeType & ret) const
  {
    evaluate ( x , ret );
  }
};

// ********************************************************************
double algorithm ( MyGridType &grid, DiscreteFunctionType &solution, bool display )
{
   // create exact solution for error evaluation
   ExactSolution f;

   // L2 error class
   Dune :: Fem :: L2Norm< GridPartType > l2norm( solution.gridPart() );

   //! perform l2-projection
   interpolate(f, solution);

   // calculation L2 error
   // pol ord for calculation the error should be higher than
   // pol for evaluation the basefunctions
   typedef GridFunctionAdapter< ExactSolution, GridPartType >  GridFunctionType;
   GridFunctionType exactSolution( "exact solution", f, solution.gridPart(), solution.space().order() + 1 );
   double error = l2norm.distance( exactSolution, solution );
   std::cout << "\nL2 Error: " << error << "\n\n";

#if USE_GRAPE
   // if Grape was found, then display last solution
   if( display )
   {
     GrapeDataDisplay< MyGridType > grape( solution.space().gridPart() );
     grape.dataDisplay( solution );
   }
#endif

   return error;
}


//**************************************************
//
//  main programm, run algorithm twice to calc EOC
//
//**************************************************
int main (int argc, char **argv)
{
  MPIManager :: initialize( argc, argv );
  try
  {

  int ml = 2;
  if( argc > 1 )
    ml = atoi( argv[1] );

  std::vector< double> error(ml);

  MyGridType &grid = Dune::Fem::TestGrid::grid();
  const int step = Dune::Fem::TestGrid::refineStepsForHalf();

  GridPartType part ( grid );
  DiscreteFunctionSpaceType linFuncSpace ( part );
  DiscreteFunctionType solution ( "sol", linFuncSpace );
  solution.clear();

  for(int i=0; i<ml; i+=step)
  {
    GlobalRefine::apply(grid,step);
    error[i] = algorithm ( grid , solution , i==ml-1);
    if (i>0)
    {
      double eoc = log( error[i-step]/error[i]) / M_LN2;
      std::cout << "EOC = " << eoc << " \n";
    }
  }
  return 0;
  }
  catch( const Exception &exception )
  {
    std::cerr << exception << std::endl;
    return 1;
  }
}

