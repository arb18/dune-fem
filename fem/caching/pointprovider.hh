#ifndef DUNE_POINTPROVIDER_HH
#define DUNE_POINTPROVIDER_HH

//- System includes
#include <vector>
#include <map>

//- Dune includes
#include <dune/common/misc.hh>

//- Local includes
#include "pointmapper.hh"

namespace Dune {

  template <class ct, int dim>
  class MapperStorage 
  {
  public:
    typedef typename CachingTraits<ct, dim>::MapperType MapperType;

  public:
    MapperStorage() {}

    MapperStorage(int numFaces) :
      mappers_(numFaces)
    {}

    void addMapper(const MapperType& mapper, int face) {
      assert(face >= 0 && face < mappers_.size());
      mappers_[face] = mapper;
    }
    
    const MapperType& getMapper(int face) const {
      assert(face >= 0 && face < mappers_.size());
      return mappers_[face];
    }
    
  private:
    typedef typename CachingTraits<ct, dim>::MapperVectorType MapperVectorType;

  private:
    MapperVectorType mappers_;
  };

  template <class ct, int dim, int codim>
  class PointProvider 
  {
    typedef CompileTimeChecker<false> 
    Point_Provider_exists_only_for_codims_1_and_2;
  };

  template <class ct, int dim>
  class PointProvider<ct, dim, 0>
  {
  private:
    typedef CachingTraits<ct, dim> Traits;

  public:
    typedef typename Traits::QuadratureType QuadratureType;
    typedef typename Traits::PointVectorType GlobalPointVectorType;
    
  public:
    static void registerQuadrature(const QuadratureType& quad);

    static const GlobalPointVectorType& getPoints(size_t id,
                                                  GeometryType elementGeo);

  private:
    typedef std::map<size_t, GlobalPointVectorType> PointContainerType;
    typedef typename PointContainerType::iterator PointIteratorType;

  private:
    static PointContainerType points_;
  };

  // * Add elemGeo later
  template <class ct, int dim>
  class PointProvider<ct, dim, 1>
  {
    enum { codim = 1 };
    typedef CachingTraits<ct, dim-codim> Traits;
 
  public:
    typedef typename Traits::QuadratureType QuadratureType;
    typedef typename Traits::PointType LocalPointType;
    typedef typename Traits::PointVectorType LocalPointVectorType;
    typedef typename Traits::MapperType MapperType;
    typedef typename Traits::MapperVectorType MapperVectorType;
    typedef FieldVector<ct, dim> GlobalPointType;
    typedef std::vector<GlobalPointType> GlobalPointVectorType;
    
  public:
    static const MapperVectorType& getMappers(const QuadratureType& quad,
                                              GeometryType elementGeo);
    // Access for non-symmetric quadratures
    static const MapperVectorType& getMappers(const QuadratureType& quad,
                                              const LocalPointVectorType& pts,
                                              GeometryType elementGeo);

    static const GlobalPointVectorType& getPoints(size_t id,
                                                  GeometryType elementGeo);
    
  private:
    // * ersetze MapperStorage durch std::vector<MapperType>
    typedef MapperStorage<ct, dim-codim> MapperStorageType;
    typedef std::map<size_t, GlobalPointVectorType> PointContainerType;
    typedef std::map<size_t, MapperStorageType> MapperContainerType;
    typedef typename PointContainerType::iterator PointIteratorType;
    typedef typename MapperContainerType::iterator MapperIteratorType;

  private:
    static MapperIteratorType addEntry(const QuadratureType& quad,
                                       const LocalPointVectorType& pts,
                                       GeometryType elementGeo);

  private:
    static PointContainerType points_;
    static MapperContainerType mappers_;
  };

} // end namespace Dune

#include "pointprovider.cc"

#endif
