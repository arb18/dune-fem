// C++ includes
#include <cassert>

// dune-geometry includes
#include <dune/geometry/referenceelements.hh>

// dune-fem includes
#include <dune/fem/quadrature/caching/registry.hh>
#include <dune/fem/misc/threads/threadmanager.hh>

namespace Dune
{

  namespace Fem
  {

    template <class ct, int dim>
    typename PointProvider<ct, dim, 0>::PointContainerType
    PointProvider<ct, dim, 0>::points_;

    template <class ct, int dim>
    void PointProvider<ct, dim, 0>::
    registerQuadrature(const QuadratureType& quad)
    {
      QuadratureKeyType key( quad.geometryType(), quad.id() );

      if (points_.find( key ) == points_.end() )
      {
        // only register when in single thread mode
        assert( Fem :: ThreadManager :: singleThreadMode() );

        PointIteratorType it =
          points_.insert(std::make_pair
                         (key,
                          GlobalPointVectorType(quad.nop()))
                         ).first;
        for (size_t i = 0; i < quad.nop(); ++i)
          it->second[i] = quad.point(i);

        // register quadrature to existing storages
        QuadratureStorageRegistry::registerQuadrature( quad );
      }
    }

    template <class ct, int dim>
    const typename PointProvider<ct, dim, 0>::GlobalPointVectorType&
    PointProvider<ct, dim, 0>::getPoints(const size_t id, const GeometryType& elementGeo)
    {
      QuadratureKeyType key( elementGeo, id );

      typename PointContainerType::const_iterator pos = points_.find( key );
#ifndef NDEBUG
      if( pos == points_.end() )
      {
        std::cerr << "Unable to find quadrature points in list (key = " << key << ")." << std::endl;
        for( pos = points_.begin(); pos != points_.end(); ++pos )
          std::cerr << "found key: " << pos->first << std::endl;
        std::cerr << "Aborting..." << std::endl;
        abort();
      }
#endif
      return pos->second;
    }

    template <class ct, int dim>
    typename PointProvider<ct, dim, 1>::PointContainerType
    PointProvider<ct, dim, 1>::points_;

    template <class ct, int dim>
    typename PointProvider<ct, dim, 1>::MapperContainerType
    PointProvider<ct, dim, 1>::mappers_;

    template <class ct, int dim>
    const typename PointProvider<ct, dim, 1>::MapperVectorType&
    PointProvider<ct, dim, 1>::getMappers(const QuadratureType& quad,
                                          const GeometryType& elementGeo)
    {
      QuadratureKeyType key( elementGeo , quad.id() );

      MapperIteratorType it = mappers_.find( key );
      if (it == mappers_.end()) {
        std::vector<LocalPointType> pts(quad.nop());
        for (size_t i = 0; i < quad.nop(); ++i) {
          pts[i] = quad.point(i);
        }
        it = addEntry(quad, pts, elementGeo);
      }

      return it->second;
    }

    template <class ct, int dim>
    const typename PointProvider<ct, dim, 1>::MapperVectorType&
    PointProvider<ct, dim, 1>::getMappers(const QuadratureType& quad,
                                          const LocalPointVectorType& pts,
                                          const GeometryType& elementGeo)
    {
      QuadratureKeyType key( elementGeo, quad.id() );

      MapperIteratorType it = mappers_.find( key );
      if (it == mappers_.end()) {
        it = addEntry(quad, pts, elementGeo);
      }

      return it->second;
    }

    template <class ct, int dim>
    const typename PointProvider<ct, dim, 1>::GlobalPointVectorType&
    PointProvider<ct, dim, 1>::getPoints(const size_t id, const GeometryType& elementGeo)
    {
      QuadratureKeyType key( elementGeo, id );

      assert(points_.find(key) != points_.end());
      return points_.find(key)->second;
    }

    template <class ct, int dim>
    typename PointProvider<ct, dim, 1>::MapperIteratorType
    PointProvider<ct, dim, 1>::addEntry(const QuadratureType& quad,
                                        const LocalPointVectorType& points,
                                        GeometryType elementGeo)
    {
      // amke sure we are in single thread mode
      assert( Fem :: ThreadManager::singleThreadMode() );

      // generate key
      QuadratureKeyType key ( elementGeo, quad.id() );

      const auto &refElem = Dune::ReferenceElements<ct, dim>::general(elementGeo);

      const int numLocalPoints = points.size();
      const int numFaces = refElem.size(codim);
      const int numGlobalPoints = numFaces*numLocalPoints;

      PointIteratorType pit =
        points_.insert(std::make_pair(key,
                                      GlobalPointVectorType(numGlobalPoints))).first;
      MapperIteratorType mit =
        mappers_.insert(std::make_pair(key,
                                       MapperVectorType(numFaces))).first;
      int globalNum = 0;
      for (int face = 0; face < numFaces; ++face)
      {
        // Assumption: all faces have the same type
        // (not true for pyramids and prisms)
        MapperType pMap(numLocalPoints);

        for (int pt = 0; pt < numLocalPoints; ++pt, ++globalNum) {
          // Store point on reference element
          pit->second[globalNum] =
            refElem.template geometry<codim>(face).global( points[pt] );

          // Store point mapping
          pMap[pt] = globalNum;
        }
        mit->second[face] = pMap;  // = face*numLocalPoints+pt
      } // end for all faces

      // register quadrature to existing storages
      QuadratureStorageRegistry::registerQuadrature( quad, elementGeo, 1 );

      return mit;
    }

  } // namespace Fem

} // namespace Dune
