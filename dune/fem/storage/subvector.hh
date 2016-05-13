#ifndef DUNE_FEM_SUBVECTOR_HH
#define DUNE_FEM_SUBVECTOR_HH

#include <dune/common/densevector.hh>
#include <dune/common/ftraits.hh>

#include <dune/fem/misc/bartonnackmaninterface.hh>

namespace Dune
{

  namespace Fem
  {
    // forward declaration
    template< class K, class M > class SubVector;
  }

  // specialization of DenseMatVecTraits for SubVector
  template< class K, class M >
  struct DenseMatVecTraits< Fem::SubVector< K, M > >
  {
    typedef Fem::SubVector< K, M > derived_type;
    typedef K container_type;

    typedef typename K::value_type value_type;
    typedef typename K::size_type size_type;
  };

  template< class K, class M >
  struct FieldTraits< Fem::SubVector< K, M > >
  {
    typedef typename FieldTraits< typename K::value_type >::field_type field_type;
    typedef typename FieldTraits< typename K::value_type >::real_type real_type;
  };

  namespace Fem
  {

    //! Abstract index mapper interface
    template< class IM >
    class IndexMapperInterface
    : public BartonNackmanInterface< IndexMapperInterface< IM >, IM >
    {
      typedef IndexMapperInterface< IM > ThisType;
      typedef BartonNackmanInterface< ThisType, IM > BaseType;

    public:
      //! Type of the implementation (Barton-Nackman)
      typedef IM IndexMapperType;

      //! Type of the interface
      typedef ThisType IndexMapperInterfaceType;

      //! Maps an index onto another one
      unsigned int operator[] ( unsigned int index ) const
      {
        return asImp().operator[]( index );
      }

      //! Returns the map's range
      unsigned int range () const
      {
        return asImp().range();
      }

      //! Returns the map's size
      unsigned int size () const
      {
        return asImp().size();
      }

    protected:
      using BaseType::asImp;
    };



    //! Index mapper interface which simply adds an offset to the index
    class OffsetSubMapper
    : public IndexMapperInterface< OffsetSubMapper >
    {
      typedef OffsetSubMapper ThisType;
      typedef IndexMapperInterface< OffsetSubMapper > BaseType;

    public:
      OffsetSubMapper( unsigned int size, unsigned int offset )
      : size_( size ), offset_( offset )
      {}

      OffsetSubMapper( const ThisType& ) = default;
      ThisType& operator=( const ThisType& ) = default;

      unsigned int size() const
      {
        return size_;
      }

      unsigned int range() const
      {
        return size_;
      }

      unsigned int operator[]( unsigned int i) const
      {
        return i+offset_;
      }

    private:
      const unsigned int size_;
      const unsigned int offset_;
    };



    /** \brief An implementation of DenseVector to extract a portion, not necessarly contiguos, of a vector
     *
     * \tparam BaseVectorImp The base vector
     * \tparam IndexMapperImp The index mapper
     */
    template< class BaseVectorImp, class IndexMapperImp >
    class SubVector : public DenseVector< SubVector< BaseVectorImp, IndexMapperImp > >
    {
      typedef SubVector< BaseVectorImp, IndexMapperImp > ThisType;
      typedef DenseVector< SubVector < BaseVectorImp, IndexMapperImp > > BaseType;

    public:
      typedef typename BaseType::size_type size_type;
      typedef typename BaseType::value_type value_type;

      //! Type of the base vector
      typedef BaseVectorImp BaseVectorType;

      //! Type of the index mapper
      typedef IndexMapperImp IndexMapperType;

      //! Type of vector elements
      typedef value_type FieldType;

      //! Constructor
      explicit SubVector( BaseVectorType& baseVector, const IndexMapperType& indexMapper )
      : baseVector_( baseVector ), indexMapper_( indexMapper )
      {}

      SubVector( const ThisType & other )
        : baseVector_( other.baseVector_ ), indexMapper_( other.indexMapper_ )
      {}

      ThisType& operator=( const ThisType & ) = default;

      void resize( size_type )
      {}

      const value_type& operator[]( size_type i ) const
      {
        return baseVector_[ indexMapper_[ i ] ];
      }

      value_type& operator[]( size_type i )
      {
        return baseVector_[ indexMapper_[ i ] ];
      }

      size_type size() const
      {
        return indexMapper_.size();
      }

    private:
      BaseVectorType& baseVector_;
      const IndexMapperType& indexMapper_;
    };


  } // namespace Fem

} // namespace Dune

#endif