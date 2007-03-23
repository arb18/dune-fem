/**************************************************************************
**       Title: elliptic.cc
**    $RCSfile$
**   $Revision$$Name$
**       $Date$
**   Copyright: GPL Author: R. Kloefkorn & B. Haasdonk 
** Description: File demonstrating a simple numerics problem on arbitrary
**              grids: general elliptic problem with known solution is given
**              and compared with numerical solution (EOC)
**              The model treated is specified in ellipticmodel.hh
**              For setting up the system, ElementMatrixIntegrators and 
**              ElementRhsIntegrators are used in combination with the FEOp
**              finite element operator. The general linear problem is
**
**     - div ( a grad u - b u) + cu =  f                                   
**                                u = g_D on Dirichlet-boundary            
**               (a grad u - bu ) n = g_N on Neuman boundary                
**     (a grad u - bu ) n + alpha u = g_R on Robin boundary      
**
**
**         Three problems of above type are implemented, which can be selected
**         By setting the defines below appropriately: either #define POISSON
**         or #define ELLIPTIC
**         The dimensionality is taken from the gridtype: As the Dune grid 
**         parser is used, the Gridtype and the dimension can be specified
**         during compile-time:
**
**         FOR POISSON:
**
**              make clean
**
**              and one of the following
**
**              make
**              make GRIDTYPE=YASPGRID       (default)
**                   ./elliptic 4 ==> EOC 2.00029 
**              make GRIDTYPE=SGRID
**                    ./elliptic 4 ==> EOC = 2.00029
**                    ./elliptic 5 ==> correct EOC = 2.00004        
**              make GRIDTYPE=ALBERTAGRID
**                   Compilieren OK, EOC 1.98 bis 2.005
**              make GRIDTYPE=ALUGRID_SIMPLEX
**                   Compilieren OK, EOC 1.97 bis 2.000
**
**         FOR ELLIPTIC2D:
**
**              make clean
**
**              and one of the following
**
**              make GRIDTYPE=YASPGRID       (default)
**                   YASPGRID: EOC non-informative, as error is immediately 
**                   small (~1e-14)
**                   (reason probably: exact solution is polynomial!)
**              make GRIDTYPE=SGRID
**                   EOC non-informative, as error is immediately small
**              make GRIDTYPE=ALBERTAGRID
**                   EOC is about 2, very nice convergence
**              make GRIDTYPE=ALUGRID_SIMPLEX
**                   EOC is about 2, very nice convergence
**                   results seem identical to ALBERTAGRID
**
**         FOR ELLIPTIC3D:
**
**              make clean
**
**              and one of the following
**
**              make GRIDDIM=3 GRIDTYPE=YASPGRID       
**                   YASPGRID: EOC non-informative, as error is immediately 
**                             small (~1e-14)
**              make GRIDDIM=3 GRIDTYPE=SGRID
**                   YASPGRID: EOC non-informative, as error is immediately 
**                             small (~1e-14)
**              make GRIDDIM=3 GRIDTYPE=ALBERTAGRID
**                   EOC fine, going down from 3 to 2 with refinement
**              make GRIDDIM=3 GRIDTYPE=ALUGRID_SIMPLEX
**                   EOC sequence: 1.21079, 1.21079, 1.67512,  1.84767
**                   increasing
**
**************************************************************************/

// select problem Type by uncommenting one of the two following
#define POISSON
//#define ELLIPTIC

// define SKIP_GRAPE, if you don't want visualization.
//#define SKIP_GRAPE

// Select the polynomial order of the calculation.
enum { polynomialOrder = 1 };

// select, whether Kronecker-Treatment of Matrix should be performed,
// i.e. kronecker rows are extended to kronecker columns. For symmetric 
// problems this is beneficial as the matrix gets symmetric

//#define ACTIVATE_KRONECKER_TREATMENT 0
#define ACTIVATE_KRONECKER_TREATMENT 1

// select, whether matrix and rhs are to be written to file or not:
// i.e. comment or uncomment the following
#define FEOP_SAVE_MATRIX_WANTED

// for debugging: force Neumann-Boundary-Values in Models
// #define FORCENEUMANN

#include <iostream>
#include <config.h>
#include <dune/common/stdstreams.cc>
#include <dune/fem/io/matlab/matlabhelper.hh>

// save GRIDDIM for later selection of problem depending on dimension
#ifdef GRIDDIM
  #if GRIDDIM == 2
    #define PDIM 2
  #elif GRIDDIM == 3
    #define PDIM 3
  #else
    #error "dimension other than 2,3 not supported in elliptic.cc"
  #endif
#else
  #warning "GRIDDIM is not set, defaulting to 2 in ellipt.cc"
  #define PDIM 2
#endif

// GRIDDIM is deleted in GRIDTYPE !!
#include <dune/grid/io/file/dgfparser/gridtype.hh>

using namespace Dune;

// Dune includes 
#include <dune/grid/common/gridpart.hh>
#include <dune/grid/common/referenceelements.hh>

#if HAVE_GRAPE
#include <dune/grid/io/visual/grapedatadisplay.hh>
#endif

// local includes 
#include <dune/fem/operator/discreteoperatorimp.hh>

#include <dune/fem/operator/feop.hh>
#include <dune/fem/operator/matrix/spmatrix.hh>

//#include "spmatrix.hh"
#include <dune/fem/misc/l2error.hh>
#include <dune/fem/space/lagrangespace.hh>
#include <dune/fem/space/common/filteredgrid.hh>
//#include <dune/fem/discretefunction/dfadapt.hh>
#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/operator/inverseoperators.hh>
#include <dune/fem/solver/oemsolver/oemsolver.hh>
#include <dune/fem/operator/elementintegrators.hh>
#include <dune/fem/operator/elementintegratortraits.hh>
#include "ellipticmodel.hh"
#include "elementintegratortraits.hh"

//! definition of model class, see ellipticmodel.hh

typedef EllipticElementIntegratorTraits< polynomialOrder > IntegratorTraits;

#ifdef POISSON
  typedef PoissonModel< IntegratorTraits > EllipticModelType;
#endif

#ifdef ELLIPTIC
#if PDIM==2
  typedef Elliptic2dModel< IntegratorTraits > EllipticModelType;
#elif PDIM==3
  typedef Elliptic3dModel< IntegratorTraits > EllipticModelType;
#endif

#endif // if ELLIPTIC


//! definition of exact solution, see ellipticmodel.hh
#ifdef POISSON
  typedef PoissonExactSolution< IntegratorTraits > ExactSolutionType;
#endif

#ifdef ELLIPTIC
#if PDIM == 2
  typedef Elliptic2dExactSolution< IntegratorTraits > ExactSolutionType;
#elif PDIM == 3
  typedef Elliptic3dExactSolution< IntegratorTraits > ExactSolutionType;
#endif
#endif // elliptic

//! definition of traits class, which already defines various 
//! basic settings such as gridparts, etc.
//! if you want another settings, simply generate your own Traits class
typedef EllipticModelType :: TraitsType ElementIntegratorTraitsType;

//! the grid part we are using 
//typedef LevelGridPart < GridType > GridPartType;
//typedef LeafGridPart<GridType> GridPartImpType;
//typedef HierarchicGridPart<GridType> GridPartType;  // the underlying GridPart
//typedef RadialFilter<GridType> FilterType;   // type of the filter we use
//typedef FilteredGridPart<GridPartImpType,FilterType> GridPartType;
typedef ElementIntegratorTraitsType::GridPartType GridPartType;

//! define the function space, \f[ \R^2 \rightarrow \R \f]
// see dune/common/functionspace.hh
//typedef FunctionSpace < double , double, dimworld , 1 > FuncSpace;

//! define the function space our unkown belong to 
//! see dune/fem/lagrangebase.hh
typedef ElementIntegratorTraitsType::DiscreteFunctionSpaceType 
                                     DiscreteFunctionSpaceType;

//typedef LagrangeDiscreteFunctionSpace < FuncSpace , GridPartType , 1 > 
//        FuncSpaceType ;

//! define the type of discrete function we are using , see
//! dune/fem/discfuncarray.hh
//typedef DFAdapt < FuncSpaceType > DiscreteFunctionType;
//typedef AdaptiveDiscreteFunction < FuncSpaceType > DiscreteFunctionType;
typedef ElementIntegratorTraitsType::DiscreteFunctionType DiscreteFunctionType;

//! define the discrete laplace operator, see ./fem.cc
// typedef LaplaceFEOp< DiscreteFunctionType, Tensor, 1 > LaplaceOperatorType;

//! definition of the problem specific ElementRhsIntegrator
class MyElementRhsIntegrator: 
    public DefaultElementRhsIntegrator<ElementIntegratorTraitsType, 
                                       EllipticModelType, 
                                       MyElementRhsIntegrator>
{
public:
  //! constructor with model must be implemented as a forward to Base class
  MyElementRhsIntegrator(EllipticModelType& model, int verbose=0)
          : DefaultElementRhsIntegrator<ElementIntegratorTraitsType, 
                                        EllipticModelType, 
                                        MyElementRhsIntegrator>(model,verbose)
        {};
  //! access function, which is the essence and can be used to implement 
  //! arbitrary operators
  template <class EntityType, class ElementRhsType>
  void addElementRhs(EntityType &entity, 
                   ElementRhsType &elRhs, 
                   double coef=1.0) // const
        {
          // arbitrary combination of existing or new methods
          addSourceElementRhs(entity,elRhs,coef);
          addNeumannElementRhs(entity,elRhs,coef);
          addRobinElementRhs(entity,elRhs,coef);
        };
};

typedef MyElementRhsIntegrator ElementRhsIntegratorType;

//! Definition of the RhsAssembler
typedef RhsAssembler<ElementRhsIntegratorType> RhsAssemblerType;

//! Definition of the ElementMatrixIntegrator as derivation of Default class:

/*======================================================================*/
/*! 
 *   The class provides method for computing the following matrix, 
 +   where i,j run over the local dofs
 *   of base functions, which have support on an entity.
 *
 *            /      \int_\entity   [a     grad(phi_j) ]^T  grad(phi_i) 
 *    L_ij :=<    +  \int_\entity   [-  b   phi_j]^T         grad(phi_i) 
 *            \   +  \int_\entity   c          phi_i        phi_j
 *             \  +  \int_{R boundary of entity} alpha      phi_i  phi_j     
 *
 *   The computation is based on the 4 contributions implemented in the 
 *   Default class. Dirichlet-entries are set to Kronecker-Rows in the 
 *   FEOp after assembling
 */
/*======================================================================*/

class MyElementMatrixIntegrator: 
    public DefaultElementMatrixIntegrator<
                               ElementIntegratorTraitsType, 
                               EllipticModelType,
                               MyElementMatrixIntegrator>
{
public:
    typedef ElementIntegratorTraitsType               TraitsType;
    typedef EllipticModelType                         ModelType;

    typedef TraitsType::ElementMatrixType    ElementMatrixType;
    typedef TraitsType::EntityType           EntityType;
    
public:
  //! constructor with model instance is implemented in default-class, so a
  //! similar constructor is required in derived classes, which simply
  //! calls the base-class constructor
  MyElementMatrixIntegrator(ModelType& model, int verbose=0)
          :   DefaultElementMatrixIntegrator<
                               ElementIntegratorTraitsType, 
                               EllipticModelType,
                               MyElementMatrixIntegrator> (model, verbose) 
        {};  

  //! The crucial method for matrix computation: collecting of contributions
  void addElementMatrix(EntityType& entity, 
                        ElementMatrixType& mat, 
                        double coef = 1.0) // const
        {
          addDiffusiveFluxElementMatrix (entity, mat, coef); 
          addConvectiveFluxElementMatrix(entity, mat, coef); 
          addMassElementMatrix          (entity, mat, coef); 
          addRobinElementMatrix         (entity, mat, coef); 
        }
}; // end class MyElementMatrixIntegrator

//! definition of element-matrix Integrator type providing elementwise matrices
typedef MyElementMatrixIntegrator ElementMatrixIntegratorType;

//! definition of the global matrix type to be used in the FEOp
typedef SparseRowMatrix<double> SystemMatrixType;

//! definition of FEM operator, see feop.hh
typedef FEOp<SystemMatrixType,ElementMatrixIntegratorType> 
        EllipticOperatorType;

//! define the inverse operator we are using to solve the system 
// see dune/fem/inverseoperators.hh 
//typedef CGInverseOp < DiscreteFunctionType, EllipticOperatorType >    InverseOperatorType;
// or ../../solvers/oemsolver/oemsolvers.hh
//typedef OEMCGOp<DiscreteFunctionType,EllipticOperatorType> InverseOperatorType;
typedef OEMBICGSTABOp<DiscreteFunctionType,EllipticOperatorType> InverseOperatorType;
//typedef OEMBICGSQOp<DiscreteFunctionType,EllipticOperatorType> InverseOperatorType;

// GMRES seems to miss some libraries...
//typedef OEMGMRESOp<DiscreteFunctionType,EllipticOperatorType> InverseOperatorType;

//! define the type of mapping which is used by inverseOperator 
//typedef Mapping<double ,double,DiscreteFunctionType,DiscreteFunctionType > MappingType;

double algorithm (const char * filename , int maxlevel, int turn )
{

   GridPtr<GridType> gridptr(filename); 
   gridptr->globalRefine (maxlevel);
   std::cout << "maxlevel = "<< maxlevel << "\n";
   // if a filteredgridpart is used
   //Dune::FieldVector<GridType::ctype,GridType::dimension> C(0.5);
   //ElementIntegratorTraitsType::FilterType filter(C, 0.4);
   //
   //GridPartType part(*gridptr,filter);
   GridPartType part(*gridptr);

   DiscreteFunctionSpaceType linFuncSpace ( part );
   std::cout << "\nSolving for " << linFuncSpace.size() << 
       " number of unkowns. \n\n";
   DiscreteFunctionType solution ( "sol", linFuncSpace );
   solution.clear();
   DiscreteFunctionType rhs ( "rhs", linFuncSpace );
   rhs.clear();
   
   // decide, whether you want to have detailed verbosity output 
   //   int verbose = 1;
   int verbose = 0;
   
   // initialize Model and Exact solution
   EllipticModelType model(linFuncSpace);
   std::cout << "initialized model\n";

   // initialize elementmatrix-provider
   ElementMatrixIntegratorType elMatInt(model, verbose);
   std::cout << "initialized element-matrix integrator\n";

   // initialize ElementRhsIntegrator
   ElementRhsIntegratorType elRhsInt(model, verbose);
   std::cout << "initialized element-rhs integrator\n";

   // initialize RhsAssembler
   RhsAssemblerType rhsAssembler(elRhsInt, verbose);
   std::cout << "initialized rhs assembler\n";
   
   // initialize Operator  
//   const int numNonZero = 27;   
   const int numNonZero = 100;
   EllipticOperatorType elliptOp 
        ( elMatInt , 
          EllipticOperatorType::ASSEMBLED,
          numNonZero, verbose);
   std::cout << "initialized operator (= matrix assembler)\n";

// checkConsistency only required for new SparseMatrix
   assert(elliptOp.systemMatrix().checkConsistency());
   std::cout << "System matrix passed consistency check.\n";

   // assemble matrix and perform dirichlet-row killing
   elliptOp.assemble();
   std::cout << "assembled matrix with Dirichlet treatment\n";

   // build right hand side and dirichlet-Dof setting
   rhsAssembler.assemble(rhs);
   std::cout << "assembled Rhs with Dirichlet treatment\n";

#ifdef FEOP_SAVE_MATRIX_WANTED
   {     
   // save matrix and rhs after kronecker-treatment
   ostringstream oss;
   oss << "linearmatrix_before_symm" << turn << ".bin";
   string matfn(oss.str());
   
   ostringstream oss2;
   oss2 << "rhsdofvector_before_symm" << turn << ".bin";
   string rhsfn(oss2.str());
   
   MatlabHelper mhelp; 
   mhelp.saveSparseMatrixBinary(matfn.c_str(), elliptOp.systemMatrix());  
   mhelp.saveDofVectorBinary(rhsfn.c_str(),rhs);
   }
#endif



   // if symmetrization of system is wanted, execute the following
#if ACTIVATE_KRONECKER_TREATMENT
   elliptOp.matrixKroneckerColumnsTreatment();
   std::cout << "finished matrix Kronecker column treatment\n";

   if (verbose)
   {
     std::cout << "Values of matrix: \n";
     elliptOp.systemMatrix().printReal(std::cout);

     std::cout << "Columns of matrix: \n";
     elliptOp.systemMatrix().printColumns(std::cout);
     std::cout << "Nonzero-Array of matrix: \n";
     elliptOp.systemMatrix().printNonZeros(std::cout);
     std::cout << "\n";
     assert(elliptOp.systemMatrix().checkConsistency());
     std::cout << "\n";
   }
   
   elliptOp.rhsKroneckerColumnsTreatment(rhs);
   if (verbose)
   {
     std::cout << "finished Rhs Kronecker column treatment\n";
   }
   
#endif

#ifdef FEOP_SAVE_MATRIX_WANTED
   // save matrix and rhs after kronecker-treatment
   {     
     ostringstream oss;
     oss << "linearmatrix_after_symm" << turn << ".bin";
     string matfn(oss.str());
     
     ostringstream oss2;
     oss2 << "rhsdofvector_after_symm" << turn << ".bin";
     string rhsfn(oss2.str());
     
     MatlabHelper mhelp;     
     mhelp.saveSparseMatrixBinary(matfn.c_str(), elliptOp.systemMatrix());  
     mhelp.saveDofVectorBinary(rhsfn.c_str(),rhs);
   }
   
#endif
   
   // cg-operator needs only boolean verbosity flag
   bool bverbose = (verbose>0);

   double dummy = 12345.67890;
   InverseOperatorType cg ( elliptOp, dummy , 1E-15 , 20000 , bverbose);
     
   // solve linear system with cg 
   cg(rhs,solution);

   // initialize Exactsolution
   ExactSolutionType u ( linFuncSpace ); 

   // calculation of L2 error 

   L2Error < DiscreteFunctionType > l2err;
   DiscreteFunctionType err ( "error", linFuncSpace );

   LagrangeInterpolator interpol;
   interpol.interpolFunction(u, err);
   err -= solution;
   
   // pol ord for calculation the error chould by higher than 
   // pol for evaluation the basefunctions 
   // double error = l2err.norm<EllipticModelType::TraitsType::quadDegree + 2> 
   //    (u ,solution, 0.0);
   double error = l2err.norm
       (u ,solution, EllipticModelType::TraitsType::quadDegree, 0.0);
   std::cout << "\nL2 Error : " << error << "\n\n";

#if HAVE_GRAPE
   // if grape was found then display solution 
   if(turn > 0)
   {
#ifdef SKIP_GRAPE
#else
     GrapeDataDisplay < GridType > grape(*gridptr); 
//     grape.dataDisplay( solution );
     double time = 0.0;
     bool vector = false;
     grape.addData(solution, solution.name(), 
                          time, vector);
     grape.addData(err, "error", time , vector);
     grape.display();
#endif
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
  if(argc != 2)
  {
    fprintf(stderr,"usage: %s <maxlevel> \n",argv[0]);
    exit(1);
  }
  
  int ml = atoi( argv[1] );
  double error[2];

#if PDIM == 2
  std::string macroGridName ("square.dgf");
#else
  std::string macroGridName ("cube.dgf");
#endif

  std::cout << "loading dgf " << macroGridName << "\n";
  // old:
  //ml -= refStepsForHalf;
    // new:
    ml -= DGFGridInfo<GridType>::refineStepsForHalf();
  if(ml < 0) ml = 0;
  for(int i=0; i<2; i++)
  {
    error[i] = algorithm ( macroGridName.c_str() ,  ml , i);
    // old:
    //ml += refStepsForHalf;
    // new:
     ml += DGFGridInfo<GridType>::refineStepsForHalf() ;
  }
  double eoc = log( error[0]/error[1]) / M_LN2; 
  std::cout << "EOC = " << eoc << " \n";
  return 0;
}

