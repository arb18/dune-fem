#ifndef DUNE_FEM_CAPABILITIES_HH
#define DUNE_FEM_CAPABILITIES_HH

#include <dune/common/version.hh>

#include <dune/grid/common/capabilities.hh>

#include <dune/fem/misc/metaprogramming.hh>

namespace Dune
{

  // Forward Declarations for all Standard Grids
  // -------------------------------------------

  template< int dim, int dimw >
  class ALUConformGrid;

  template< int dim, int dimw >
  class ALUCubeGrid;

  template< int dim, int dimw >
  class ALUSimplexGrid;

  class OneDGrid;

  template< int dim, int dimw, class ctype >
  class SGrid;

  template< int dim >
  class UGGrid;

  template< int dim >
  class YaspGrid;

  template< int dim, int dimworld >
  class AlbertaGrid;

  template< class HostGrid, class CoordFunction, class Allocator >
  class GeometryGrid;

  // Capabilities
  // ------------

  namespace Capabilities
  {

    // hasHierarchicIndexSet
    // ---------------------

    template< class Grid >
    struct hasHierarchicIndexSet;

    template< class Grid >
    struct hasHierarchicIndexSet< const Grid >
    {
      static const bool v = hasHierarchicIndexSet< Grid >::v;
    };


    template< int dim, int dimw >
    struct hasHierarchicIndexSet< ALUConformGrid< dim, dimw > >
    {
      static const bool v = true;
    };

    template< int dim, int dimw >
    struct hasHierarchicIndexSet< ALUCubeGrid< dim, dimw > >
    {
      static const bool v = true;
    };

    template< int dim, int dimw >
    struct hasHierarchicIndexSet< ALUSimplexGrid< dim, dimw > >
    {
      static const bool v = true;
    };

    template<>
    struct hasHierarchicIndexSet< OneDGrid >
    {
      static const bool v = false;
    };

    template< int dim, int dimw, class ctype >
    struct hasHierarchicIndexSet< SGrid< dim, dimw, ctype > >
    {
      static const bool v = false;
    };

    template< int dim >
    struct hasHierarchicIndexSet< UGGrid< dim > >
    {
      static const bool v = false;
    };

    template< int dim >
    struct hasHierarchicIndexSet< YaspGrid< dim > >
    {
      static const bool v = false;
    };


    template< class HostGrid, class CoordFunction, class Allocator >
    struct hasHierarchicIndexSet< GeometryGrid< HostGrid, CoordFunction, Allocator > >
    {
      //static const bool v = hasHierarchicIndexSet< HostGrid > :: v;
      static const bool v = false ;
    };



    // hasAllCodimEntities
    // -------------------

    template< class Grid >
    class hasAllCodimEntities
    {
      template< unsigned int codim >
      struct Codim
      : public hasEntity< Grid, codim >
      {};
    
    public:
      static const bool v = Loop< MetaAnd, Codim, Grid :: dimension > :: v;
      static const bool value = v;
    };



    // supportsCallbackAdaptation
    // --------------------------

    template< class Grid >
    struct supportsCallbackAdaptation
    {
      static const bool v = false;
    };

    template< class Grid >
    struct supportsCallbackAdaptation< const Grid >
    {
      static const bool v = Dune :: Capabilities :: supportsCallbackAdaptation< Grid > :: v;
    };

    template< int dim, int dimworld >
    struct supportsCallbackAdaptation< ALUSimplexGrid< dim, dimworld > >
    {
      static const bool v = true;
    };

    template< int dim, int dimworld >
    struct supportsCallbackAdaptation< ALUCubeGrid< dim, dimworld > >
    {
      static const bool v = true;
    };

    template< int dim, int dimworld >
    struct supportsCallbackAdaptation< ALUConformGrid< dim, dimworld > >
    {
      static const bool v = true;
    };


    template< int dim, int dimworld >
    struct supportsCallbackAdaptation< AlbertaGrid< dim, dimworld > >
    {
      static const bool v = true;
    };

    template< class HostGrid, class CoordFunction, class Allocator >
    struct supportsCallbackAdaptation< GeometryGrid< HostGrid, CoordFunction, Allocator > >
    {
      static const bool v = supportsCallbackAdaptation< HostGrid > :: v;
    };


    // IsUnstructured
    // --------------

#if DUNE_VERSION_NEWER(DUNE_GRID,2,1,0)
    template< class Grid >
    struct IsUnstructured
    {
      static const bool v = true;
    };

    template< class Grid >
    struct IsUnstructured< const Grid >
    {
      static const bool v = IsUnstructured< Grid >::v;
    };

    template< int dim >
    struct IsUnstructured< YaspGrid< dim > >
    {
      static const bool v = false;
    };

    template< int dim, int dimw, class ctype >
    struct IsUnstructured< SGrid< dim, dimw, ctype > >
    {
      static const bool v = false;
    };

    template< class HostGrid, class CoordFunction, class Allocator >
    struct IsUnstructured< GeometryGrid< HostGrid, CoordFunction, Allocator > >
    {
      static const bool v = true ;
    };

#endif // #if DUNE_VERSION_NEWER(DUNE_GRID,2,1,0)

  }

}

#endif // #ifndef DUNE_FEM_CAPABILITIES_HH
