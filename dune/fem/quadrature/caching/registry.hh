#ifndef DUNE_FEM_QUADRATURE_CACHING_REGISTRY_HH
#define DUNE_FEM_QUADRATURE_CACHING_REGISTRY_HH

// system includes
#include <cstddef>
#include <algorithm>
#include <list>

#include <dune/common/visibility.hh>

// dune-geometry includes
#include <dune/geometry/type.hh>

// dune-fem includes
#include <dune/fem/misc/threads/threadmanager.hh>

namespace Dune
{

  namespace Fem
  {

    // QuadratureStorageRegistry
    // -------------------------

    class QuadratureStorageRegistry
    {
      typedef QuadratureStorageRegistry ThisType;

    public:
      struct StorageInterface
      {
        virtual ~StorageInterface () {}
        virtual void cacheQuadrature ( std::size_t id, std::size_t codim, std::size_t quadSize ) = 0;
        virtual GeometryType type () const = 0;
      };

    private:
      typedef std::list< StorageInterface * > StorageListType;

      struct QuadratureInfo
      {
        std::size_t id;
        std::size_t codim;
        std::size_t size;
        GeometryType type;
      };

      typedef std::list< QuadratureInfo > QuadratureInfoListType;

      DUNE_EXPORT static StorageListType &storageList ()
      {
        static StorageListType storageList;
        return storageList;
      }

      DUNE_EXPORT static QuadratureInfoListType &quadratureInfoList ()
      {
        static QuadratureInfoListType quadratureInfoList;
        return quadratureInfoList;
      }

    public:
      /** \brief initialize static variables */
      static void initialize ()
      {
        storageList();
        quadratureInfoList();
      }

      static void registerStorage ( StorageInterface &storage )
      {
        assert( ThreadManager::singleThreadMode() );
        storageList().push_back( &storage );

        const GeometryType type = storage.type();
        for( QuadratureInfoListType::iterator it = quadratureInfoList().begin(); it != quadratureInfoList().end(); ++it )
        {
          // only cache shape functions for quadratures with same geometry type
          if( type == it->type )
            storage.cacheQuadrature( it->id, it->codim, it->size );
        }
      }

      static void unregisterStorage ( StorageInterface &storage )
      {
        assert( ThreadManager::singleThreadMode() );
        const StorageListType::iterator pos
          = std::find( storageList().begin(), storageList().end(), &storage );
        if( pos != storageList().end() )
          storageList().erase( pos );
      }

      template< class Quadrature >
      static void registerQuadrature ( const Quadrature &quadrature )
      {
        registerQuadrature( quadrature, quadrature.geometryType(), Quadrature::codimension );
      }

      template< class Quadrature >
      static void registerQuadrature ( const Quadrature &quadrature,
                                       const GeometryType &type, std::size_t codim )
      {
        assert( ThreadManager::singleThreadMode() );
        QuadratureInfo quadInfo = { quadrature.id(), codim, std::size_t( quadrature.nop() ), type };
        quadratureInfoList().push_back( quadInfo );

        for( typename StorageListType::iterator it = storageList().begin(); it != storageList().end(); ++it )
        {
          // only cache shape functions for quadratures with same geometry type
          if( (*it)->type() == type )
            (*it)->cacheQuadrature( quadInfo.id, quadInfo.codim, quadInfo.size );
        }
      }
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_QUADRATURE_CACHING_REGISTRY_HH
