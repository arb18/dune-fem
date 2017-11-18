#ifndef DUNE_FEM_XDRSTREAMS_HH
#define DUNE_FEM_XDRSTREAMS_HH

#include <cassert>

#if HAVE_XDR
#include <rpc/types.h>
#include <rpc/xdr.h>
#endif // #if HAVE_XDR

#include <dune/common/exceptions.hh>
#include <dune/fem/io/streams/streams.hh>

namespace Dune
{

  namespace Fem
  {

    template< class OutStreamImp >
    struct XDROutStreamTraits
    {
      typedef OutStreamImp OutStreamType;
    };



    /** \class XDRBasicOutStream
     *  \ingroup InOutStreams
     *  \brief base implementation for XDR output streams
     *
     *  This class implements the writing functions for an XDR stream. It must
     *  be associated to a stream by a child class.
     *
     *  The following XDR output streams have been implemented:
     *  -XDRFileOutStream
     */
    template< class OutStreamImp >
    class XDRBasicOutStream
    : public OutStreamInterface< XDROutStreamTraits< OutStreamImp > >
    {
      typedef XDRBasicOutStream< OutStreamImp > ThisType;
      typedef OutStreamInterface< XDROutStreamTraits< OutStreamImp > > BaseType;

    public:
      //! type of the implementaton (Barton-Nackman)
      typedef OutStreamImp OutStreamType;

      //! type of the traits
      typedef XDROutStreamTraits< OutStreamType > Traits;

      enum { maxStringSize = 2<<18 };

    protected:
#if HAVE_XDR
      XDR xdrs_;
#endif // #if HAVE_XDR

    protected:
      using BaseType::writeError;

    protected:
      XDRBasicOutStream ()
      {}

    public:
      /** \copydoc Dune::Fem::OutStreamInterface::writeDouble */
      void writeDouble ( double value )
      {
#if HAVE_XDR
        if( xdr_double( xdrs(), &value ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeFloat */
      void writeFloat ( float value )
      {
#if HAVE_XDR
        if( xdr_float( xdrs(), &value ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeInt */
      void writeInt ( int value )
      {
#if HAVE_XDR
        if( xdr_int( xdrs(), &value ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeChar */
      void writeChar ( char value )
      {
#if HAVE_XDR
        if( xdr_char( xdrs(), &value ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeBool */
      void writeBool ( const bool value )
      {
        // convert to character and write
        char val = ( value == true ) ? 1 : 0;
        writeChar( val );
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeString */
      void writeString ( const std :: string &s )
      {
#if HAVE_XDR
        assert( s.size() < maxStringSize );
        const char *cs = s.c_str();
        if( xdr_string( xdrs(), (char **)&cs, maxStringSize ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeUnsignedInt */
      void writeUnsignedInt ( unsigned int value )
      {
#if HAVE_XDR
        if( xdr_u_int( xdrs(), &value ) == 0 )
          writeError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::writeUnsignedInt64 */
      void writeUnsignedInt64 ( uint64_t value )
      {
#if HAVE_XDR
#ifdef XDR_UINT64_FUNC
        // use u_int64_t since xdr_u_long is buggy
        u_int64_t val = value ;
        // XDR_UINT64_FUNC is defined in config.h
        if( XDR_UINT64_FUNC( xdrs(), &val ) == 0 )
          writeError();
#else
        DUNE_THROW(NotImplemented,"xdr_uint64_t is missing");
#endif
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

    protected:
#if HAVE_XDR
      XDR *xdrs ()
      {
        return &xdrs_;
      }
#endif // #if HAVE_XDR
    };



    template< class InStreamImp >
    struct XDRInStreamTraits
    {
      typedef InStreamImp InStreamType;
    };


    /** \class XDRBasicInStream
     *  \ingroup InOutStreams
     *  \brief base implementation for XDR input streams
     *
     *  This class implements the reading functions for an XDR stream. It must
     *  be associated to a stream by a child class.
     *
     *  The following XDR input streams have been implemented:
     *  -XDRFileInStream
     */
    template< class InStreamImp >
    class XDRBasicInStream
    : public InStreamInterface< XDRInStreamTraits< InStreamImp > >
    {
      typedef XDRBasicInStream< InStreamImp > ThisType;
      typedef InStreamInterface< XDRInStreamTraits< InStreamImp > > BaseType;

    public:
      //! type of the implementation (Barton-Nackman)
      typedef InStreamImp InStreamType;

      //! type of the traits
      typedef XDRInStreamTraits< InStreamType > Traits;

    private:
      enum { maxStringSize = 2<<18 };

    protected:
#if HAVE_XDR
      XDR xdrs_;
#endif // #if HAVE_XDR

    protected:
      using BaseType::readError;

    protected:
      XDRBasicInStream ()
      {}

    public:
      /** \copydoc Dune::Fem::InStreamInterface::readDouble */
      void readDouble ( double &value )
      {
#if HAVE_XDR
        if( xdr_double( xdrs(), &value ) == 0 )
          readError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readFloat */
      void readFloat ( float &value )
      {
#if HAVE_XDR
        if( xdr_float( xdrs(), &value ) == 0 )
          readError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readInt */
      void readInt ( int &value )
      {
#if HAVE_XDR
        if( xdr_int( xdrs(), &value ) == 0 )
          readError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readChar */
      void readChar ( char &value )
      {
#if HAVE_XDR
        if( xdr_char( xdrs(), &value ) == 0 )
          readError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readBool */
      void readBool ( bool &value )
      {
        char val;
        readChar( val );
        // convert to boolean
        value = ( val == 1 ) ? true : false;
      }

      /** \copydoc Dune::Fem::InStreamInterface::readString */
      void readString ( std::string &s )
      {
#if HAVE_XDR
        char data[ maxStringSize ];
        char *cs = &(data[ 0 ]);
        if( xdr_string( xdrs(), &cs, maxStringSize ) == 0 )
          readError();
        s = data;
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readUnsignedInt */
      void readUnsignedInt ( unsigned int &value )
      {
#if HAVE_XDR
        if( xdr_u_int( xdrs(), &value ) == 0 )
          readError();
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::InStreamInterface::readUnsignedInt64 */
      void readUnsignedInt64 ( uint64_t &value )
      {
#if HAVE_XDR
#ifdef XDR_UINT64_FUNC
        // use u_int64_t since xdr_u_long is buggy
        u_int64_t val ;
        // XDR_UINT64_FUNC is defined in config.h
        if( XDR_UINT64_FUNC( xdrs(), &val ) == 0 )
          readError();
        else
          value = val;
#else
        DUNE_THROW(NotImplemented,"xdr_uint64_t is missing");
#endif
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

    protected:
#if HAVE_XDR
      XDR *xdrs ()
      {
        return &xdrs_;
      }
#endif // #if HAVE_XDR
    };



    /** \class XDRFileOutStream
     *  \ingroup InOutStreams
     *  \brief XDR output stream writing into a file
     *
     *  \newimplementation
     */
    class XDRFileOutStream
    : public XDRBasicOutStream< XDRFileOutStream >
    {
    private:
      typedef XDRFileOutStream ThisType;
      typedef XDRBasicOutStream< ThisType > BaseType;

    protected:
#if HAVE_XDR
      using BaseType :: xdrs;
#endif // #if HAVE_XDR

    protected:
      FILE *file_;

    public:
      /** \brief constructor
       *
       *  \param[in]  filename  name of the file to write to
       *  \param[in]  append    true if stream appends to file filename
       *                        (default = false)
       */
      explicit XDRFileOutStream ( const std::string &filename,
                                  const bool append = false   )
      {
#if HAVE_XDR
        const char * flags = ( append ) ? "ab" : "wb";
        file_ = fopen( filename.c_str(), flags );
        if( file_ == 0 )
          DUNE_THROW( IOError, "XDRFileOutStream: Unable to open file '" << filename << "'." );
        xdrstdio_create( xdrs(), file_, XDR_ENCODE );
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \brief destructor */
      inline ~XDRFileOutStream ()
      {
#if HAVE_XDR
        xdr_destroy( xdrs() );
        fclose( file_ );
#endif // #if HAVE_XDR
      }

      /** \copydoc Dune::Fem::OutStreamInterface::flush */
      inline void flush ()
      {
        fflush( file_ );
      }
    };



    /** \class XDRFileInStream
     *  \ingroup InOutStreams
     *  \brief XDR output stream reading from a file
     *
     *  \newimplementation
     */
    class XDRFileInStream
    : public XDRBasicInStream< XDRFileInStream >
    {
    private:
      typedef XDRFileInStream ThisType;
      typedef XDRBasicInStream< ThisType > BaseType;

    protected:
#if HAVE_XDR
      using BaseType :: xdrs;
#endif // #if HAVE_XDR

    protected:
      FILE *file_;

    public:
      /** \brief constructor
       *
       *  \param[in]  filename  name of the file to read from
       *  \param[in]  pos       starting position for file read
       *                        (default = 0)
       */
      explicit XDRFileInStream ( const std::string &filename,
                                 const size_t pos = 0 )
      {
#if HAVE_XDR
        file_ = fopen( filename.c_str(), "rb" );
        if( file_ == 0 )
          DUNE_THROW( IOError, "XDRFileInStream: Unable to open file '" << filename << "'." );

        // if pos = 0 this will do nothing
        fseek( file_, pos , SEEK_SET );

        xdrstdio_create( xdrs(), file_, XDR_DECODE );
#else // #if HAVE_XDR
        DUNE_THROW( NotImplemented, "XDR not available" );
#endif // #else // #if HAVE_XDR
      }

      /** \brief destructor
       */
      inline ~XDRFileInStream ()
      {
#if HAVE_XDR
        xdr_destroy( xdrs() );
        fclose( file_ );
#endif // #if HAVE_XDR
      }
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_XDRSTREAMS_HH
