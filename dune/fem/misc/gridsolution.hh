#ifndef DUNE_FEM_GRIDSOLUTION_HH
#define DUNE_FEM_GRIDSOLUTION_HH

#include <dune/grid/utility/hierarchicsearch.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/io/file/datawriter.hh>

namespace Dune {

namespace Fem {
  
// GridSolution
//-------------

/** \class GridSolution
 *  \brief creates a function with evaluate method from a check point
 *
 *  \tparam GridImp Grid type
 *  \tparam DiscreteFunctionImp Discrete function type
 */
template <class GridImp, class DiscreteFunctionImp > 
class GridSolution
{
  typedef GridImp  GridType;
  typedef DiscreteFunctionImp DiscreteFunctionType;
  typedef typename DiscreteFunctionType :: DiscreteFunctionSpaceType  DiscreteFunctionSpaceType;
  typedef typename DiscreteFunctionSpaceType :: RangeType RangeType ;
  typedef typename DiscreteFunctionSpaceType :: DomainType DomainType ;
  typedef typename DiscreteFunctionSpaceType :: GridPartType   GridPartType ;
  typedef typename GridPartType :: IndexSetType IndexSetType;
  typedef CheckPointer< GridType >   CheckPointerType;

  typedef typename GridType :: template Codim<0> :: EntityPointer EntityPointerType;
  typedef typename GridType :: template Codim<0> :: Entity        EntityType;

  typedef HierarchicSearch< GridType, IndexSetType > HierarchicSearchType;

  typedef tuple< DiscreteFunctionType* > IOTupleType;

  GridType* grid_;
  GridPtr< GridType > gridPtr_;
  GridPartType gridPart_;
  DiscreteFunctionSpaceType space_;
  DiscreteFunctionType discreteFunction_;
  IOTupleType data_;
  HierarchicSearchType hierarchicSearch_;

  GridType& grid() { assert( grid_ ); return *grid_ ; }
public:  
  //! Constructor
  explicit GridSolution(const std::string checkPointFile) :
    grid_( CheckPointerType :: restoreGrid( checkPointFile ) ), 
    gridPtr_(),
    gridPart_( grid() ),
    space_( gridPart_ ),
    discreteFunction_("grid-sol", space_),
    data_( &discreteFunction_ ),
    hierarchicSearch_( grid(), gridPart_.indexSet() )
  {
    // store grid pointer
    gridPtr_ = grid_;
    // restore data from checkpoint
    CheckPointerType :: restoreData( grid(), data_, checkPointFile );
  }

  /** \brief evaluates in a given space-time point
   *  \param[in] x Point in global coordinates
   *  \param[in] time Time
   *  \param[out] result The value of the discrete function in space-time point \f[ (x,time)\f]
   *
   *  \tparam PointType The point type
   */
  template <class PointType> 
  void evaluate(const PointType& x, const double time, RangeType& result) const 
  {
    evaluate(x, result );
  }

  /** \brief evaluates in a given space point
   *  \param[in] x Point in global coordinates
   *  \param[out] result The value of the discrete function in space point @a x
   *
   *  \tparam PointType The point type
   */
  template <class PointType> 
  void evaluate(const PointType& x, RangeType& result) const 
  {
    // search entity 
    EntityPointerType ep = hierarchicSearch_.findEntity( coordinate( x ) );
    const EntityType& entity = *ep ;

    const DomainType local = entity.geometry().local( x );

    // evaluate discrete function 
    discreteFunction_.localFunction( entity ).evaluate( local, result );
  }

  //! \brief writes a discrete function 
  static void writeDiscreteFunction(const GridType& grid, 
                                    const DiscreteFunctionType& discreteFunction,
                                    const double time ) 
  {
    typedef tuple< const DiscreteFunctionType* > OutputTuple;
    OutputTuple data( &discreteFunction );
    CheckPointerType :: writeSingleCheckPoint( grid, data, time, false );
  }
};

} // end namespace Fem 
} // end namespace Dune 
#endif