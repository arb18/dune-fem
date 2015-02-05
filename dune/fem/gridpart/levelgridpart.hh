#ifndef DUNE_FEM_GRIDPART_LEVELGRIDPART_HH
#define DUNE_FEM_GRIDPART_LEVELGRIDPART_HH

#include <dune/grid/common/capabilities.hh>

#include <dune/fem/gridpart/common/capabilities.hh>
#include <dune/fem/gridpart/common/gridview2gridpart.hh>
#include <dune/fem/gridpart/defaultindexsets.hh>

namespace Dune
{

  namespace Fem
  {

    // LevelGridPart
    // -------------

    template< class Grid >
    class LevelGridPart
      : public GridView2GridPart< typename Grid::LevelGridView, WrappedLevelIndexSet< Grid >, LevelGridPart< Grid > >
    {
      typedef GridView2GridPart< typename Grid::LevelGridView, WrappedLevelIndexSet< Grid >, LevelGridPart< Grid > > BaseType;

    public:
      /** \copydoc Dune::Fem::GridPartInterface::GridType */
      typedef typename BaseType::GridType GridType;

      /** \copydoc Dune::Fem::GridPartInterface::IndexSetType */
      typedef typename BaseType::IndexSetType IndexSetType;

      /** \name Construction
       *  \{
       */

      LevelGridPart ( GridType &grid, int level )
        : BaseType( grid.levelGridView( level ) ),
          grid_( grid ),
          level_( level ),
          indexSet_( grid, level )
      {}

      /** \} */

      /** \name Public member methods
       *  \{
       */

      /** \copydoc Dune::Fem::GridPartInterface::grid */
      GridType &grid () { return grid_; }

      /** \copydoc Dune::Fem::GridPartInterface::grid */
      const GridType &grid () const { return grid_; }

      /** \copydoc Dune::Fem::GridPartInterface::indexSet */
      const IndexSetType &indexSet () const { return indexSet_; }

      /** \copydoc Dune::Fem::GridPartInterface::level */
      int level () const { return level_; }

      /** \} */

    private:
      GridType &grid_;
      int level_;
      IndexSetType indexSet_;
    };



    namespace GridPartCapabilities
    {

      template< class Grid >
      struct hasGrid< LevelGridPart< Grid > >
      {
        static const bool v = true;
      };

      template< class Grid >
      struct hasSingleGeometryType< LevelGridPart< Grid > >
       : public Dune::Capabilities::hasSingleGeometryType< Grid >
      {};

      template< class Grid >
      struct isCartesian< LevelGridPart< Grid > >
       : public Dune::Capabilities::isCartesian< Grid >
      {};

      template< class Grid, int codim  >
      struct hasEntity< LevelGridPart< Grid >, codim >
       : public Dune::Capabilities::hasEntity< Grid, codim >
      {};

      template< class Grid >
      struct isParallel< LevelGridPart< Grid > >
       : public Dune::Capabilities::isParallel< Grid >
      {};

      template< class Grid, int codim  >
      struct canCommunicate< LevelGridPart< Grid >, codim >
       : public Dune::Capabilities::canCommunicate< Grid, codim >
      {};

      template< class Grid >
      struct isConforming< LevelGridPart< Grid > >
      {
        static const bool v = Dune::Capabilities::isLevelwiseConforming< Grid >::v;
      };

    } // namespace GridPartCapabilities

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_GRIDPART_LEVELGRIDPART_HH
