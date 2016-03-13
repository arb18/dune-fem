#ifndef DUNE_FEM_DATAOUTPUT_HH
#define DUNE_FEM_DATAOUTPUT_HH

#ifndef USE_VTKWRITER
#define USE_VTKWRITER 1
#endif

//#define USE_GRAPE 0

//- Dune includes
#include <dune/fem/io/file/iointerface.hh>
#include <dune/fem/io/file/iotuple.hh>
#include <dune/fem/io/parameter.hh>
#include <dune/fem/solver/timeprovider.hh>
#include <dune/fem/space/common/loadbalancer.hh>
#include <dune/fem/gridpart/adaptiveleafgridpart.hh>
#include <dune/fem/space/lagrange.hh>
#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>
#include <dune/fem/gridpart/adaptiveleafgridpart.hh>

#ifndef ENABLE_VTXPROJECTION
#define ENABLE_VTXPROJECTION 1
#endif

#if USE_VTKWRITER
#include <dune/fem/io/file/vtkio.hh>
#if ENABLE_VTXPROJECTION
#include <dune/fem/operator/lagrangeinterpolation.hh>
#include <dune/fem/operator/projection/vtxprojection.hh>
#endif // #if ENABLE_VTXPROJECTION
#endif // #if USE_VTKWRITER

#ifndef USE_GRAPE
// define whether to use grape of not
#define USE_GRAPE HAVE_GRAPE
#endif

#if USE_GRAPE
#include <dune/grid/io/visual/grapedatadisplay.hh>
#endif

namespace Dune
{

  namespace Fem
  {

    /** \brief Parameter class for Dune::Fem::DataOutput

        Structure providing the main parameters used to setup the Dune::DataOutput.
        By default these parameters are set through the Dune::Parameter class, i.e.,
        can be set in a parameter file. To override this a user can derive from this
        structure and modify any subset of the parameters. An instance of this modified
        class can then be passed in the construction of the Dune::DataOutput.
     */
    struct DataOutputParameters
#ifndef DOXYGEN
    : public LocalParameter< DataOutputParameters, DataOutputParameters >
#endif
    {
    protected:
      const std::string keyPrefix_;
      ParameterReader parameter_;

    public:
      explicit DataOutputParameters ( std::string keyPrefix, const ParameterReader &parameter = Parameter::container() )
        : keyPrefix_( std::move( keyPrefix ) ), parameter_( parameter )
      {}

      explicit DataOutputParameters ( const ParameterReader &parameter = Parameter::container() )
        : keyPrefix_( "fem.io." ), parameter_( parameter )
      {}

      //! \brief path where the data is stored (always relative to fem.prefix)
      virtual std::string path() const
      {
        return parameter().getValue< std::string >( keyPrefix_ + "path", "./" );
      }

      virtual std::string absolutePath () const
      {
        std::string absPath = parameter().getValue< std::string >( "fem.prefix", "." ) + "/";
        const std::string relPath = path();
        if( relPath != "./" )
          absPath += relPath;
        return absPath;
      }

      //! \brief base of file name for data file (fem.io.datafileprefix)
      virtual std::string prefix () const
      {
        return parameter().getValue< std::string >( keyPrefix_ + "datafileprefix", "" );
      }

      //! \brief format of output (fem.io.outputformat)
      virtual int outputformat () const
      {
        static const std::string formatTable[]
          = { "vtk-cell", "vtk-vertex", "sub-vtk-cell", "binary" , "gnuplot" , "none" };
        int format = parameter().getEnum( keyPrefix_ + "outputformat", formatTable, 1 );
        return format;
      }

      virtual bool conformingoutput () const
      {
        return parameter().getValue< bool >( keyPrefix_ + "conforming", false );
      }

      //! \brief use online grape display (fem.io.grapedisplay)
      virtual bool grapedisplay () const
      {
#if USE_GRAPE
        return (parameter().getValue( keyPrefix_ + "grapedisplay", 0 ) == 1);
#else
        return false;
#endif
      }

      //! \brief save data every savestep interval (fem.io.savestep)
      virtual double savestep () const
      {
        return parameter().getValue< double >( keyPrefix_ + "savestep", 0 );
      }

      //! \brief save data every savecount calls to write method (fem.io.savecount)
      virtual int savecount () const
      {
        return parameter().getValue< int >( keyPrefix_ + "savecount", 0 );
      }

      //! \brief save data every subsamplingLevel (fem.io.subsamplinglevel)
      virtual int subsamplingLevel() const
      {
        return parameter().getValue< int >( keyPrefix_ + "subsamplinglevel", 1 );
      }

      //! \brief number for first data file (no parameter available)
      virtual int startcounter () const
      {
        return 0;
      }

      //! \brief number of first call (no parameter available)
      virtual int startcall () const
      {
        return 0;
      }

      //! \brief value of first save time (no parameter available)
      virtual double startsavetime () const
      {
        return 0.0;
      }

      //! method used for conditional data output - default
      //! value passed as argument.
      virtual bool willWrite ( bool write ) const
      {
        return write;
      }

      //! return true if DataOutput was created for writing (only not true for
      //!  CheckPointer on restore)
      virtual bool writeMode() const
      {
        return true ;
      }

      const ParameterReader &parameter () const noexcept { return parameter_; }
    };



    /** @ingroup DiscFuncIO
       \brief Implementation of the Dune::Fem::IOInterface.
       This class manages data output.
       Available output formats are GRAPE, VTK and VTK Vertex projected
       using the VtxProjection operator. Details can be
       found in \ref DiscFuncIO.
    */
    template< class GridImp, class DataImp >
    class DataOutput
    : public IOInterface
    {
      typedef DataOutput< GridImp, DataImp > ThisType;

    protected:
      template< class Grid, class OutputTuple, int N = std::tuple_size< OutputTuple >::value >
      struct GridPartGetter;

    template< class Grid, class OutputTuple, int N >
    struct GridPartGetter
    {
      typedef typename TypeTraits< typename std::tuple_element< 0, OutputTuple >::type >::PointeeType DFType;
      typedef typename DFType :: DiscreteFunctionSpaceType :: GridPartType GridPartType;

      GridPartGetter ( const Grid &, const OutputTuple &data )
      : gridPart_( getGridPart( data ) )
      {}

      const GridPartType &gridPart () const
      {
        return gridPart_;
      }

    protected:
      static const GridPartType &getGridPart( const OutputTuple& data )
      {
        const DFType *df = std::get< 0 >( data );
        assert( df );
        return df->space().gridPart();
      }

      const GridPartType &gridPart_;
    };


    template< class Grid, class OutputTuple >
    struct GridPartGetter< Grid, OutputTuple, 0 >
    {
      typedef AdaptiveLeafGridPart< Grid > GridPartType;

      GridPartGetter ( const Grid &grid, const OutputTuple & )
      : gridPart_( const_cast< Grid & >( grid ) )
      {}

      const GridPartType &gridPart () const
      {
        return gridPart_;
      }

    protected:
      const GridPartType gridPart_;
    };


#if USE_VTKWRITER
      template< class VTKIOType >
      struct VTKListEntry
      {
        virtual ~VTKListEntry ()
        {}
        virtual void add ( VTKIOType & ) const = 0;

      protected:
        VTKListEntry ()
        {}
      };

#if ENABLE_VTXPROJECTION
      template< class VTKIOType, class DFType >
      class VTKFunc;

      template< class VTKOut >
      struct VTKOutputerLagrange;
#endif // #if ENABLE_VTXPROJECTION

      template< class VTKOut >
      struct VTKOutputerDG;
#endif // #if USE_VTKWRITER

      template< class GridPartType >
      class GnuplotOutputer;

    protected:
      enum OutputFormat { vtk = 0, vtkvtx = 1, subvtk = 2 , binary = 3, gnuplot = 4, none = 5 };

      //! \brief type of grid used
      typedef GridImp GridType;
      //! \brief type of data tuple
      typedef DataImp OutPutDataType;

    public:
     /** \brief Constructor creating data output class
        \param grid corresponding grid
        \param data Tuple containing discrete functions to write
        \param parameter structure for tuning the behavior of the Dune::DataOutput
                         defaults to Dune::DataOutputParameters
      */
      DataOutput ( const GridType &grid, OutPutDataType &data, const DataOutputParameters &parameter );

      DataOutput ( const GridType &grid, OutPutDataType &data, const ParameterReader &parameter = Parameter::container() )
        : DataOutput( grid, data, DataOutputParameters( parameter ) )
      {}

      /** \brief Constructor creating data writer
        \param grid corresponding grid
        \param data Tuple containing discrete functions to write
        \param tp   a time provider to set time (e.g. for restart)
        \param parameter structure for tuning the behavior of the Dune::DataOutput
                         defaults to Dune::DataOutputParameters
      */
      DataOutput ( const GridType &grid, OutPutDataType &data, const TimeProviderBase &tp, const DataOutputParameters &parameter );

      DataOutput ( const GridType &grid, OutPutDataType &data, const TimeProviderBase &tp, const ParameterReader &parameter = Parameter::container() )
        : DataOutput( grid, data, tp, DataOutputParameters( parameter ) )
      {}

      void consistentSaveStep ( const TimeProviderBase &tp ) const;

      //! \brief destructor
      virtual ~DataOutput()
      {
        delete param_;

        if( pvd_ )
        {
          pvd_ << "  </Collection>" << std::endl;
          pvd_ << "</VTKFile>" << std::endl;

          pvd_.close();
        }
      }

    protected:
      //! \brief initialize data writer
      void init ( const DataOutputParameters &parameter );

    public:
      //! \brief returns true if data will be written on next write call
      virtual bool willWrite ( const TimeProviderBase &tp ) const
      {
        // make save step consistent
        consistentSaveStep( tp );

        const double saveTimeEps = 2*std::numeric_limits< double >::epsilon()*saveTime_;
        const bool writeStep  = (saveStep_ > 0) && (tp.time() - saveTime_ >= -saveTimeEps);
        const bool writeCount = (saveCount_ > 0) && (writeCalls_ % saveCount_ == 0);
        return param_->willWrite( writeStep || writeCount );
      }

      //! \brief returns true if data will be written on next write call
      virtual bool willWrite () const
      {
        return param_->willWrite( (saveCount_ > 0) && (writeCalls_ % saveCount_ == 0) );
      }

      /** \brief write given data to disc, evaluates parameter savecount
          \param outstring  pass additional string for naming
      */
      void write( const std::string& outstring ) const
      {
        if( willWrite() )
          writeData( writeCalls_, outstring );
        ++writeCalls_;
      }

      //! \brief write given data to disc, evaluates parameter savecount
      void write() const
      {
        if( willWrite() )
          writeData( writeCalls_, "" );
        ++writeCalls_;
      }

      /** \brief write given data to disc, evaluates parameter savecount and savestep
          \param tp  time provider for time and step
          \param outstring  pass additional string for naming
      */
      void write(const TimeProviderBase& tp, const std::string& outstring ) const
      {
        if( willWrite(tp) )
          writeData( tp.time(), outstring );
        ++writeCalls_;
      }

      /** \brief write given data to disc, evaluates parameter savecount and savestep
          \param tp  time provider for time and step
      */
      void write( const TimeProviderBase& tp ) const
      {
        write( tp, "" );
      }

      /** \brief write data with a given sequence stamp and outstring
          \param sequenceStamp stamp for the data set
          \param outstring pass additional string for naming
      */
      void writeData ( double sequenceStamp, const std::string &outstring ) const;

      /** \brief write data with a given sequence stamp
          \param sequenceStamp stamp for the data set
      */
      void writeData ( double sequenceStamp ) const
      {
        writeData( sequenceStamp, "" );
      }

      //! \brief print class name
      virtual const char* myClassName () const
      {
        return "DataOutput";
      }

      //! \brief return output path name
      const std::string &path () const
      {
        return path_;
      }

      //! \brief return write step
      int writeStep() const
      {
        return writeStep_;
      }

      //! \brief return write calls
      int writeCalls() const
      {
        return writeCalls_;
      }

      //! \brief return save time
      double saveTime() const
      {
        return saveTime_;
      }

    protected:
#if USE_VTKWRITER
      std::string writeVTKOutput () const;
#endif

      std::string writeGnuPlotOutput () const;

      //! \brief write binary data
      virtual void writeBinaryData ( const double ) const
      {
        DUNE_THROW(NotImplemented, "Format 'binary' not supported by DataOutput, use DataWriter instead." );
      }

      //! \brief display data with grape
      virtual void display () const
      {
        if( grapeDisplay_ )
          grapeDisplay( data_ );
      }

    protected:
      //! \brief display data with grape
      template< class OutputTupleType >
      void grapeDisplay ( OutputTupleType &data ) const;

      //! \brief type of this class
      const GridType& grid_;
      OutPutDataType &data_;

      // name for data output
      std::string path_;
      std::string datapref_;
      // use grape display
      bool grapeDisplay_;

      // counter for sequence of files
      mutable int writeStep_;
      // counter for call to write
      mutable int writeCalls_;
      // next point in time to save data
      mutable double saveTime_;
      // time interval to save files
      double saveStep_;
      // number of write call between writting files
      int saveCount_;
      // grape, vtk or ...
      OutputFormat outputFormat_;
      bool conformingOutput_;
      mutable std::ofstream sequence_;
      mutable std::ofstream pvd_;
      const DataOutputParameters* param_;
    }; // end class DataOutput



    // DataOutput::GridPartGetter
    // --------------------------


    // DataOutput::VTKFunc
    // -------------------

#if USE_VTKWRITER
#if ENABLE_VTXPROJECTION
    template< class GridImp, class DataImp >
    template< class VTKIOType, class DFType >
    class DataOutput< GridImp, DataImp >::VTKFunc
    : public VTKListEntry< VTKIOType >
    {
      typedef typename VTKIOType::GridPartType GridPartType;
      typedef typename DFType::DiscreteFunctionSpaceType::FunctionSpaceType FunctionSpaceType;

      typedef LagrangeDiscreteFunctionSpace< FunctionSpaceType, GridPartType, 1 > LagrangeSpaceType;
      typedef AdaptiveDiscreteFunction< LagrangeSpaceType > NewFunctionType;

    public:
      VTKFunc ( const GridPartType &gridPart, const DFType &df )
      : df_( df ),
        space_( const_cast< GridPartType & >( gridPart ) ),
        func_( 0 )
      {
        // space_.setDescription(df.space().getDescription());
      }

      ~VTKFunc ()
      {
        delete func_;
      }

      virtual void add ( VTKIOType &vtkio ) const
      {
        func_ = new NewFunctionType( df_.name()+"vtx-prj" , space_ );
        if( df_.space().continuous() )
        {
          // vtkio.addVertexData( df_ );
          LagrangeInterpolation< DFType, NewFunctionType >::interpolateFunction( df_, *func_ );
        }
        else
        {
          WeightDefault<GridPartType> weight;
          VtxProjectionImpl::project( df_, *func_, weight );
        }
        vtkio.addVertexData( *func_ );
      }

    private:
      VTKFunc ( const VTKFunc & );

      const DFType& df_;
      LagrangeSpaceType space_;
      mutable NewFunctionType *func_;
    };
#endif // #if ENABLE_VTXPROJECTION
#endif // #if USE_VTKWRITER



    // DataOutput::VTKOutputerDG
    // -------------------------

#if USE_VTKWRITER
    template< class GridImp, class DataImp >
    template< class VTKOut >
    struct DataOutput< GridImp, DataImp >::VTKOutputerDG
    {
      //! Constructor
      explicit VTKOutputerDG ( VTKOut &vtkOut, bool conforming = false )
      : vtkOut_( vtkOut ),
        conforming_( conforming )
      {}

      //! Applies the setting on every DiscreteFunction/LocalFunction pair.
      template< class DFType >
      void visit ( DFType *df )
      {
        if( df )
        {
          if( conforming_ || (df->space().order() == 0) )
            vtkOut_.addCellData( *df );
          else
            vtkOut_.addVertexData( *df );
        }
      }

      template< class OutputTuple >
      void forEach ( OutputTuple &data )
      {
        ForEachValue< OutputTuple > forEach( data );
        forEach.apply( *this );
      }

    private:
      VTKOut &vtkOut_;
      bool conforming_;
    };
#endif // #if USE_VTKWRITER



    // DataOutput::VTKOutputerLagrange
    // -------------------------------

#if USE_VTKWRITER
#if ENABLE_VTXPROJECTION
    template< class GridImp, class DataImp >
    template< class VTKOut >
    struct DataOutput< GridImp, DataImp >::VTKOutputerLagrange
    {
      //! Constructor
      explicit VTKOutputerLagrange ( VTKOut &vtkOut )
      : vtkOut_( vtkOut ),
        vec_()
      {}

      //! destructor, delete entries of list
      ~VTKOutputerLagrange ()
      {
        for( auto& entry : vec_ )
        {
          delete entry;
          entry = nullptr;
        }
      }

      //! Applies the setting on every DiscreteFunction/LocalFunction pair.
      template< class DFType >
      void visit ( DFType *df )
      {
        if( df )
        {
          auto entry = new VTKFunc< VTKOut, DFType >( vtkOut_.gridPart(), *df );
          entry->add( vtkOut_ );
          vec_.push_back( entry );
        }
      }

      template< class OutputTuple >
      void forEach ( OutputTuple &data )
      {
        ForEachValue< OutputTuple > forEach( data );
        forEach.apply( *this );
      }

    private:
      VTKOut &vtkOut_;
      typedef VTKListEntry< VTKOut > VTKListEntryType;
      std::vector< VTKListEntryType * > vec_;
    };
#endif // #if ENABLE_VTXPROJECTION
#endif // #if USE_VTKWRITER



    // DataOutput::GnuplotOutputer
    // ---------------------------

    template< class GridImp, class DataImp >
    template< class GridPartType >
    class DataOutput< GridImp, DataImp >::GnuplotOutputer
    {
      typedef typename GridPartType::template Codim< 0 >::IteratorType::Entity Entity;
      std::ostream& out_;
      CachingQuadrature<GridPartType,0> &quad_;
      int i_;
      const Entity& en_;

    public:
      //! Constructor
      GnuplotOutputer ( std::ostream& out,
                        CachingQuadrature<GridPartType,0> &quad,
                        int i,
                        const Entity &en )
      : out_(out), quad_(quad), i_(i), en_(en)
      {}

      //! Applies the setting on every DiscreteFunction/LocalFunction pair.
      template <class DFType>
      void visit(DFType* df)
      {
        if( df )
        {
          auto lf = df->localFunction(en_);
          typename DFType::DiscreteFunctionSpaceType::RangeType u;
          lf.evaluate( quad_[ i_ ], u );

          DUNE_CONSTEXPR int dimRange = DFType::DiscreteFunctionSpaceType::dimRange;
          for( auto k = 0; k < dimRange; ++k )
            out_ << "  " << u[ k ];
        }
      }

      template< class OutputTuple >
      void forEach ( OutputTuple &data )
      {
        ForEachValue< OutputTuple > forEach( data );
        forEach.apply( *this );
      }
    };



    // Implementation of DataOutput
    // ----------------------------

    template< class GridImp, class DataImp >
    inline DataOutput< GridImp, DataImp >
      ::DataOutput ( const GridType &grid, OutPutDataType &data,
                     const DataOutputParameters& parameter )
    : grid_( grid ),
      data_( data ),
      writeStep_(0),
      writeCalls_(0),
      saveTime_(0),
      saveStep_(-1),
      saveCount_(-1),
      outputFormat_(vtkvtx),
      conformingOutput_( false ),
      param_(parameter.clone())
    {
      // initialize class
      init( parameter );
    }


    template< class GridImp, class DataImp >
    inline DataOutput< GridImp, DataImp >
      ::DataOutput ( const GridType &grid, OutPutDataType &data,
                     const TimeProviderBase &tp,
                     const DataOutputParameters &parameter )
    : grid_(grid),
      data_(data),
      writeStep_(0),
      writeCalls_(0),
      saveTime_(0),
      saveStep_(-1),
      saveCount_(-1),
      outputFormat_(vtkvtx),
      conformingOutput_( false ),
      param_(parameter.clone())
    {
      // initialize class
      init( parameter );

      // make save step consistent
      consistentSaveStep( tp );
    }

    template< class GridImp, class DataImp >
    inline void DataOutput< GridImp, DataImp >
      ::consistentSaveStep ( const TimeProviderBase &tp ) const
    {
      // set old values according to new time
      if( saveStep_ > 0 )
      {
        const auto oldTime = tp.time() - saveStep_;
        while( saveTime_ <= oldTime )
        {
          ++writeStep_;
          saveTime_ += saveStep_;
        }
      }
    }


    template< class GridImp, class DataImp >
    inline void DataOutput< GridImp, DataImp >::
    init ( const DataOutputParameters &parameter )
    {
      const bool writeMode = parameter.writeMode();
      path_ = parameter.absolutePath();

      // create path if not already exists
      if( writeMode )
        IOInterface :: createGlobalPath ( grid_.comm(), path_ );

      // add prefix for data file
      datapref_ += parameter.prefix();

      auto outputFormat = parameter.outputformat();
      switch( outputFormat )
      {
        case 0: outputFormat_ = vtk; break;
        case 1: outputFormat_ = vtkvtx; break;
        case 2: outputFormat_ = subvtk; break;
        case 3: outputFormat_ = binary; break;
        case 4: outputFormat_ = gnuplot; break;
        case 5: outputFormat_ = none; break;
        default:
          DUNE_THROW(NotImplemented,"DataOutput::init: wrong output format");
      }

      conformingOutput_ = parameter.conformingoutput();

      grapeDisplay_ = parameter.grapedisplay();

      // get parameters for data writing
      saveStep_ = parameter.savestep();
      saveCount_ = parameter.savecount();

      // set initial quantities
      writeStep_ = parameter.startcounter();
      writeCalls_ =  parameter.startcall();
      saveTime_ = parameter.startsavetime();

      if( writeMode )
      {
        // only write series file for VTK output
        if ( Parameter :: verbose() && outputFormat_ < binary )
        {
          {
            std::string name = path_ + "/" + datapref_;
            name += ".series";
            std::cout << "opening file: " << name << std::endl;
            sequence_.open(name.c_str());
            if ( ! sequence_ )
              std::cout << "could not write sequence file" << std::endl;
          }

          {
            std::string name = path_ + "/" + datapref_;
            name += ".pvd";
            std::cout << "opening file: " << name << std::endl;
            pvd_.open(name.c_str());
            if ( ! pvd_ )
              std::cout << "could not write sequence file" << std::endl;
            else
            {
              pvd_ << "<?xml version=\"1.0\"?>" << std::endl;
              pvd_ << "<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl;
              pvd_ << "  <Collection>" << std::endl;
            }
          }
        }

        // write parameter file
        Parameter::write("parameter.log");
      }
    }


    template< class GridImp, class DataImp >
    inline void DataOutput< GridImp, DataImp >
      ::writeData ( double sequenceStamp, const std::string &outstring ) const
    {
      std::string filename;
      // check online display
      display();
      switch( outputFormat_ )
      {
        // if no output was chosen just return
        case none: break ;
        case binary: writeBinaryData( sequenceStamp ); break;
        case vtk :
        case vtkvtx :
        case subvtk :
#if USE_VTKWRITER
          // write data in vtk output format
          filename = writeVTKOutput();
          break;
#else
          DUNE_THROW(NotImplemented,"DataOutput::write: VTKWriter was disabled by USE_VTKWRITER 0");
#endif // #if USE_VTKWRITER
        case gnuplot : filename = writeGnuPlotOutput(); break;
        default:
          DUNE_THROW(NotImplemented,"DataOutput::write: wrong output format = " << outputFormat_);
      }

      if( outputFormat_ != none )
      {
        if( sequence_ )
          sequence_ << writeStep_ << " "
                    << filename << " "
                    << sequenceStamp
                    << outstring
                    << std::endl;

        if( pvd_ )
        {
          // cH: only use the basename of filename, Paraview will
          // prepend the leading path correctly, if the pvd-file
          // resides in the same directory as the data files.
          std::string basefilename;
          auto pos = filename.find_last_of( '/' );
          if (pos == filename.npos)
            pos = -1;
          basefilename = filename.substr( pos+1, filename.npos );
          pvd_ << "    <DataSet timestep=\"" << sequenceStamp << "\" "
               << "group=\"\" part=\"0\" "
               << "file=\""<<basefilename<<"\"/>" << std::endl;
        }

        if( Parameter::verbose() )
        {
          // only write info for proc 0, otherwise on large number of procs
          // this is to much output
          std::cout << myClassName() << "[" << grid_.comm().rank() << "]::write data"
                    << " writestep=" << writeStep_
                    << " sequenceStamp=" << sequenceStamp
                    << outstring
                    << std::endl;
        }
      }

      saveTime_ += saveStep_;
      ++writeStep_;
    }


#if USE_VTKWRITER
    template< class GridImp, class DataImp >
    inline std::string DataOutput< GridImp, DataImp >::writeVTKOutput () const
    {
      std::string filename;
      // check whether to use vertex data of discontinuous data
      const bool vertexData = (outputFormat_ == vtkvtx);

      // check whether we have parallel run
      const bool parallel = (grid_.comm().size() > 1);

      // generate filename, with path only for serial run
      auto name = generateFilename( (parallel ? datapref_ : path_ + "/" + datapref_ ), writeStep_ );

      if( vertexData )
      {
#if ENABLE_VTXPROJECTION
        // get gridpart from first discrete function in tuple
        typedef GridPartGetter< GridType, OutPutDataType > GridPartGetterType;
        GridPartGetterType gp( grid_, data_ );
        const auto &gridPart = gp.gridPart();

        // create vtk output handler
        typedef VTKIO < typename GridPartGetterType::GridPartType > VTKIOType;
        VTKIOType vtkio ( gridPart, VTK::conforming, param_->parameter() );

        // add all functions
        VTKOutputerLagrange< VTKIOType > io( vtkio );
        io.forEach( data_ );

        if( parallel )
        {
          // write all data for parallel runs
          filename = vtkio.pwrite( name, path_, "." );
        }
        else
        {
          // write all data serial
          filename = vtkio.write( name );
        }
#endif
      }
      else if ( outputFormat_ == vtk )
      {
        typedef GridPartGetter< GridType, OutPutDataType> GridPartGetterType;
        GridPartGetterType gp( grid_, data_ );

        // create vtk output handler
        typedef VTKIO< typename GridPartGetterType::GridPartType > VTKIOType;
        VTKIOType vtkio( gp.gridPart(), conformingOutput_ ? VTK::conforming : VTK::nonconforming, param_->parameter() );

        // add all functions
        VTKOutputerDG< VTKIOType > io( vtkio, conformingOutput_ );
        io.forEach( data_ );

        // write all data
        if( parallel )
        {
          // write all data for parallel runs
          filename = vtkio.pwrite( name, path_, "." );
        }
        else
        {
          // write all data serial
          filename = vtkio.write( name );
        }
      }
      else if ( outputFormat_ == subvtk )
      {
        typedef GridPartGetter< GridType, OutPutDataType> GridPartGetterType;
        GridPartGetterType gp( grid_, data_ );

        // create vtk output handler
        typedef SubsamplingVTKIO < typename GridPartGetterType :: GridPartType > VTKIOType;
        VTKIOType vtkio ( gp.gridPart(), static_cast< unsigned int >( param_->subsamplingLevel() ), param_->parameter() );

        // add all functions
        VTKOutputerDG< VTKIOType > io( vtkio, conformingOutput_ );
        io.forEach( data_ );

        // write all data
        if( parallel )
        {
          // write all data for parallel runs
          filename = vtkio.pwrite( name, path_, "." );
        }
        else
        {
          // write all data serial
          filename = vtkio.write( name );
        }
      }
      return filename;
    }
#endif // #if USE_VTKWRITER


    template< class GridImp, class DataImp >
    inline std::string DataOutput< GridImp, DataImp >::writeGnuPlotOutput () const
    {
      // generate filename
      auto name = generateFilename( path_ + "/" + datapref_, writeStep_ );
      name += ".gnu";
      std::ofstream gnuout(name.c_str());
      gnuout << std::scientific << std::setprecision( 16 );

      typedef GridPartGetter< GridType, OutPutDataType > GridPartGetterType;
      GridPartGetterType gp( grid_, data_ );
      const auto &gridPart = gp.gridPart();

      // start iteration
      typedef typename GridPartGetterType::GridPartType GridPartType;
      typedef typename GridPartType::GridViewType GridViewType;
      for( const auto entity : elements( static_cast<GridViewType>( gridPart ) ) )
      {
        CachingQuadrature< GridPartType, 0 > quad( entity, 1 );
        for( unsigned int i = 0; i < quad.nop(); ++i )
        {
          const auto x = entity.geometry().global( quad.point( i ) );
          for( auto k = 0; k < x.dimension; ++k )
            gnuout << (k > 0 ? " " : "") << x[ k ];
          GnuplotOutputer< GridPartType > io( gnuout, quad, i, entity );
          io.forEach( data_ );
          gnuout << std::endl;
        }
      }
      return name;
    }


#if USE_GRAPE
    template< class GridImp, class DataImp >
    template< class OutputTupleType >
    inline void DataOutput< GridImp, DataImp >::grapeDisplay ( OutputTupleType &data ) const
    {
      GrapeDataDisplay<GridType> grape(grid_);
      IOTuple<OutPutDataType>::addToDisplay(grape,data);
      grape.display();
    }
#else // #if USE_GRAPE
    template< class GridImp, class DataImp >
    template< class OutputTupleType >
    inline void DataOutput< GridImp, DataImp >::grapeDisplay ( OutputTupleType &data ) const
    {
      std::cerr << "WARNING: HAVE_GRAPE == 0, but grapeDisplay == true, recompile with grape support for online display!" << std::endl;
    }
#endif // #else // #if USE_GRAPE

  } // end namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_DATAOUTPUT_HH
