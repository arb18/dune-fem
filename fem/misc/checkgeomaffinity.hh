#ifndef DUNE_CHECKGEOMETRYAFFINITY_HH
#define DUNE_CHECKGEOMETRYAFFINITY_HH

#include <dune/common/fvector.hh>


namespace Dune {

/*! @addtogroup HelperClasses
 ** @{
 */
  
//! Helper class to check affinity of the grid's geometries  
template <class QuadratureType>
struct GeometryAffinityCheck 
{
  //! check whether all geometry mappings are affine 
  template <class IteratorType>
  static inline bool checkAffinity(const IteratorType& begin,
                                   const IteratorType& endit, 
                                   const int quadOrd)  
  {
    bool affinity = true ;
    typedef typename IteratorType :: Entity :: Geometry Geometry;
    for(IteratorType it = begin; it != endit; ++it)
    {
      // get quadrature of desired order 
      QuadratureType volQuad( *it, quadOrd );
      const int nop = volQuad.nop();
      const Geometry& geo = it->geometry();

      // check all integration elements against the first 
      const double oldIntel = geo.integrationElement( volQuad.point(0) );
      for(int l=1; l<nop; ++l)
      {
        const double intel = geo.integrationElement( volQuad.point(l) );
        if( std::abs( oldIntel - intel ) > 1e-12 ) affinity = false;
      }
    }
    return affinity;
  }
};

//! Helper class to check whether grid is structured or not 
struct CheckCartesian 
{
protected:  
  //! check whether this is a cartesian grid or not 
  template <class GridType, class IndexSetType>
  static inline bool doCheck(const GridType& grid, const IndexSetType& indexSet)
  {
    typedef typename GridType :: template Codim<0> :: LevelIterator MacroIteratorType; 
    typedef typename GridType :: template Codim<0> :: EntityPointer  EntityPointerType; 
    typedef typename GridType :: template Codim<0> :: Entity  EntityType; 
    typedef typename GridType :: template Codim<0> :: Geometry Geometry; 
    
    const MacroIteratorType endit = grid.template lend<0> (0);
    MacroIteratorType it = grid.template lbegin<0> (0);

    // empty grids are considerd as cartesian 
    if( it == endit ) return true;

    typedef AllGeomTypes< IndexSetType, GridType> GeometryInformationType;
    GeometryInformationType geoInfo( indexSet );

    // if we have more than one geometry Type return false 
    if( geoInfo.geomTypes(0).size() > 1 ) return false;

    // if this type is not cube return false 
    if( ! geoInfo.geomTypes(0)[0].isCube() ) return false; 

    typedef typename GridType :: ctype ctype;
    enum { dimension = GridType :: dimension };
    enum { dimworld  = GridType :: dimensionworld };
    
    // grid width 
    FieldVector<ctype, dimension> h(0);
    FieldVector<ctype, dimworld> enBary;
    FieldVector<ctype, dimension-1> mid(0.5);
    
    const int map[3] = {1, 2, 4};
    {
      const Geometry& geo = it->geometry();
      if ( ! geo.type().isCube() ) return false;

      // calculate grid with 
      for(int i=0; i<dimension; ++i) 
      {
        h[i] = (geo[0] - geo[map[i]]).two_norm();
      }
    }

    // loop over all macro elements 
    for(MacroIteratorType it = grid.template lbegin<0> (0); 
        it != endit; ++it)
    {
      const EntityType& en = *it;
      const Geometry& geo = en.geometry();
      
      const FieldVector<ctype, dimworld> enBary = 
        geo.global( geoInfo.localCenter( geo.type() ));

      typedef typename GridType :: Traits :: LevelIntersectionIterator IntersectionIteratorType;

      // if geometry is not cube, it's not a cartesian grid 
      if ( ! geo.type().isCube() ) return false;

      for(int i=0; i<dimension; ++i) 
      {
        const ctype w = (geo[0] - geo[map[i]]).two_norm();
        if( std::abs( h[i] - w ) > 1e-15 ) return false; 
      }

      IntersectionIteratorType endnit = en.ilevelend();
      for(IntersectionIteratorType nit = en.ilevelbegin();
          nit != endnit; ++nit)
      {
        if( nit.neighbor() )
        {
          EntityPointerType ep = nit.outside();
          const EntityType& nb = *ep;
          const Geometry& nbGeo = nb.geometry();
          
          FieldVector<ctype, dimworld> diff = nbGeo.global( geoInfo.localCenter( nbGeo.type() ));
          diff -= enBary;
          assert( diff.two_norm() > 1e-15 );
          diff /= diff.two_norm();

          // scalar product should be 1 or -1 
          if( std::abs(std::abs((diff * nit.unitOuterNormal(mid))) - 1.0) > 1e-12 ) return false;
        }
      }
    }
    return true;
  }
  
public:  
  //! check whether all the is grid is a cartesian grid 
  template <class GridType, class IndexSetType>
  static inline bool check(const GridType& grid, const IndexSetType& indexSet)
  {
    bool cartesian = doCheck( grid , indexSet );
    int val = (cartesian) ? 1 : 0;
    // take global minimum  
    val = grid.comm().min( val );
    return (val == 1) ? true : false;
  }
};

//! @}  
} // end namespace Dune 
#endif
