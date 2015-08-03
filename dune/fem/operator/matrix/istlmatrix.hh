#ifndef DUNE_FEM_ISTLMATRIXWRAPPER_HH
#define DUNE_FEM_ISTLMATRIXWRAPPER_HH

#if HAVE_DUNE_ISTL

//- system includes
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>

//- Dune common includes
#include <dune/common/exceptions.hh>

//- Dune istl includes
#include <dune/istl/bvector.hh>
#include <dune/istl/bcrsmatrix.hh>
#include <dune/istl/preconditioners.hh>

//- Dune fem includes
#include <dune/fem/function/blockvectorfunction.hh>
#include <dune/fem/operator/common/localmatrix.hh>
#include <dune/fem/operator/common/localmatrixwrapper.hh>
#include <dune/fem/operator/common/operator.hh>
#include <dune/fem/function/common/scalarproducts.hh>
#include <dune/fem/operator/matrix/preconditionerwrapper.hh>
#include <dune/fem/io/parameter.hh>
#include <dune/fem/operator/matrix/columnobject.hh>
#include <dune/fem/storage/objectstack.hh>

#include <dune/fem/operator/matrix/istlmatrixadapter.hh>

namespace Dune
{

  namespace Fem
  {

    ///////////////////////////////////////////////////////
    // --ISTLMatrixHandle
    //////////////////////////////////////////////////////
    template <class LittleBlockType,
              class RowDiscreteFunctionImp,
              class ColDiscreteFunctionImp = RowDiscreteFunctionImp>
    class ImprovedBCRSMatrix : public BCRSMatrix<LittleBlockType>
    {
      public:
        typedef RowDiscreteFunctionImp RowDiscreteFunctionType;
        typedef ColDiscreteFunctionImp ColDiscreteFunctionType;

        typedef BCRSMatrix<LittleBlockType> BaseType;
        //! type of the base matrix
        typedef BaseType MatrixBaseType;
        typedef typename BaseType :: RowIterator RowIteratorType ;
        typedef typename BaseType :: ColIterator ColIteratorType ;

        typedef ImprovedBCRSMatrix< LittleBlockType,
                RowDiscreteFunctionImp, ColDiscreteFunctionImp > ThisType;

        typedef typename BaseType :: size_type size_type;

        //===== type definitions and constants

        //! export the type representing the field
        typedef typename BaseType::field_type field_type;

        //! export the type representing the components
        typedef typename BaseType::block_type block_type;

        //! export the allocator type
        typedef typename BaseType:: allocator_type allocator_type;

        //! implement row_type with compressed vector
        typedef typename BaseType :: row_type row_type;

        //! increment block level counter
        enum {
          //! The number of blocklevels the matrix contains.
          blocklevel = BaseType :: blocklevel
        };

        /** \brief Iterator for the entries of each row */
        typedef typename BaseType :: ColIterator ColIterator;

        /** \brief Iterator for the entries of each row */
        typedef typename BaseType :: ConstColIterator ConstColIterator;

        /** \brief Const iterator over the matrix rows */
        typedef typename BaseType :: RowIterator RowIterator;

        /** \brief Const iterator over the matrix rows */
        typedef typename BaseType :: ConstRowIterator ConstRowIterator;

        //! type of discrete function space
        typedef typename ColDiscreteFunctionType :: DiscreteFunctionSpaceType RangeSpaceType;

        //! type of row block vector
        typedef typename RowDiscreteFunctionType :: DofStorageType  RowBlockVectorType;

        //! type of column block vector
        typedef typename ColDiscreteFunctionType :: DofStorageType  ColBlockVectorType;

        //! type of communication object
        typedef typename RangeSpaceType :: GridType :: Traits :: CollectiveCommunication   CollectiveCommunictionType ;

        typedef typename BaseType :: BuildMode BuildMode ;

      public:
        //! constructor used by ISTLMatrixObject to build matrix in implicit mode
        ImprovedBCRSMatrix(size_type rows, size_type cols,
                           size_type nnz, double overflowFraction)
          : BaseType (rows, cols, nnz, overflowFraction, BaseType::implicit)
        {
        }

        //! constuctor using old row_wise assembly (and used by ILU preconditioner)
        ImprovedBCRSMatrix(size_type rows, size_type cols, size_type nz = 0 )
          : BaseType (rows, cols, BaseType :: row_wise)
        {
        }

        //! copy constructor, needed by ISTL preconditioners
        ImprovedBCRSMatrix( )
          : BaseType ()
        {}

        //! copy constructor, needed by ISTL preconditioners
        ImprovedBCRSMatrix(const ImprovedBCRSMatrix& org)
          : BaseType(org)
        {}

        template <class RowKeyType, class ColKeyType>
        void createEntries(const std::map<RowKeyType , std::set<ColKeyType> >& indices)
        {
          // type of create interator
          typedef typename BaseType :: CreateIterator CreateIteratorType;
          // not insert map of indices into matrix
          CreateIteratorType endcreate = this->createend();
          for(CreateIteratorType create = this->createbegin();
              create != endcreate; ++create)
          {
            // set of column indices
            typedef typename std::map<RowKeyType , std::set<ColKeyType> >
              ::const_iterator StencilIterator;
            const StencilIterator it = indices.find( create.index() );
            if (it == indices.end() )
              continue;
            const std::set<ColKeyType>& localIndices = it->second;
            typedef typename std::set<ColKeyType>::const_iterator iterator;
            iterator end = localIndices.end();
            // insert all indices for this row
            for (iterator it = localIndices.begin(); it != end; ++it)
            {
              create.insert( *it );
            }
          }
        }

        //! clear Matrix, i.e. set all entires to 0
        void clear()
        {
          RowIteratorType endi=this->end();
          for (RowIteratorType i=this->begin(); i!=endi; ++i)
          {
            ColIteratorType endj = (*i).end();
            for (ColIteratorType j=(*i).begin(); j!=endj; ++j)
            {
              (*j) = 0;
            }
          }
        }

        //! setup like the old matrix but remove rows with hanging nodes
        template <class HangingNodesType>
        void setup(ThisType& oldMatrix,
                   const HangingNodesType& hangingNodes)
        {
          // necessary because element traversal not necessaryly is in
          // ascending order
          typedef std::set< std::pair<int, block_type> > LocalEntryType;
          typedef std::map< int , LocalEntryType > EntriesType;
          EntriesType entries;

          {
            // map of indices
            std::map< int , std::set<int> > indices;
            // not insert map of indices into matrix
            RowIteratorType rowend  = oldMatrix.end();
            for(RowIteratorType it  = oldMatrix.begin(); it != rowend; ++it)
            {
              const int row = it.index();
              std::set< int >& localIndices = indices[ row ];

              if( hangingNodes.isHangingNode( row ) )
              {
                // insert columns into other columns
                typedef typename HangingNodesType :: ColumnVectorType ColumnVectorType;
                const ColumnVectorType& cols = hangingNodes.associatedDofs( row );
                const size_t colSize = cols.size();
                for(size_t i=0; i<colSize; ++i)
                {
                  assert( ! hangingNodes.isHangingNode( cols[i].first ) );

                  // get local indices of col
                  std::set< int >& localColIndices = indices[ cols[i].first ];
                  LocalEntryType& localEntry = entries[  cols[i].first ];

                  // copy from old matrix
                  ColIteratorType endj = (*it).end();
                  for (ColIteratorType j= (*it).begin(); j!=endj; ++j)
                  {
                    localColIndices.insert( j.index () );
                    localEntry.insert( std::make_pair( j.index(), (cols[i].second * (*j)) ));
                  }
                }

                // insert diagonal and hanging columns
                localIndices.insert( row );
                for(size_t i=0; i<colSize; ++i)
                {
                  localIndices.insert( cols[i].first );
                }
              }
              else
              {
                // copy from old matrix
                ColIteratorType endj = (*it).end();
                for (ColIteratorType j= (*it).begin(); j!=endj; ++j)
                {
                  localIndices.insert( j.index () );
                }
              }
            }

            // create matrix from entry map
            createEntries( indices );

          } // end create, matrix is on delete of create iterator

          {
            // not insert map of indices into matrix
            RowIteratorType rowit  = oldMatrix.begin();

            RowIteratorType endcreate = this->end();
            for(RowIteratorType create = this->begin();
                create != endcreate; ++create, ++rowit )
            {
              assert( rowit != oldMatrix.end() );

              const int row = create.index();
              if( hangingNodes.isHangingNode( row ) )
              {
                typedef typename HangingNodesType :: ColumnVectorType ColumnVectorType;
                const ColumnVectorType& cols = hangingNodes.associatedDofs( row );

                std::map< const int , block_type > colMap;
                // only working for block size 1 ath the moment
                assert( block_type :: rows == 1 );
                // insert columns into map
                const size_t colSize = cols.size();
                for( size_t i=0; i<colSize; ++i)
                {
                  colMap[ cols[i].first ] = -cols[i].second;
                }
                // insert diagonal into map
                colMap[ row ] = 1;

                ColIteratorType endj = (*create).end();
                for (ColIteratorType j= (*create).begin(); j!=endj; ++j)
                {
                  assert( colMap.find( j.index() ) != colMap.end() );
                  (*j) = colMap[ j.index() ];
                }
              }
              // if entries are equal, just copy
              else if ( entries.find( row ) == entries.end() )
              {
                ColIteratorType colit = (*rowit).begin();
                ColIteratorType endj = (*create).end();
                for (ColIteratorType j= (*create).begin(); j!=endj; ++j, ++colit )
                {
                  assert( colit != (*rowit).end() );
                  (*j) = (*colit);
                }
              }
              else
              {
                typedef std::map< int , block_type > ColMapType;
                ColMapType oldCols;

                {
                  ColIteratorType colend = (*rowit).end();
                  for(ColIteratorType colit = (*rowit).begin(); colit !=
                      colend; ++colit)
                  {
                    oldCols[ colit.index() ] = 0;
                  }
                }

                typedef typename EntriesType :: iterator Entryiterator ;
                Entryiterator entry = entries.find( row );
                assert( entry  != entries.end ());

                {
                  typedef typename LocalEntryType :: iterator iterator;
                  iterator endcol = (*entry).second.end();
                  for( iterator co = (*entry).second.begin(); co != endcol; ++co)
                  {
                    oldCols[ (*co).first ] = 0;
                  }
                }

                {
                  ColIteratorType colend = (*rowit).end();
                  for(ColIteratorType colit = (*rowit).begin(); colit !=
                      colend; ++colit)
                  {
                    oldCols[ colit.index() ] += (*colit);
                  }
                }

                {
                  typedef typename LocalEntryType :: iterator iterator;
                  iterator endcol = (*entry).second.end();
                  for( iterator co = (*entry).second.begin(); co != endcol; ++co)
                  {
                    oldCols[ (*co).first ] += (*co).second;
                  }
                }

                ColIteratorType endj = (*create).end();
                for (ColIteratorType j= (*create).begin(); j!=endj; ++j )
                {
                  typedef typename ColMapType :: iterator iterator;
                  iterator colEntry = oldCols.find( j.index() );
                  if( colEntry != oldCols.end() )
                  {
                    (*j) = (*colEntry).second;
                  }
                  else
                  {
                    abort();
                  }
                }
              }
            }
          } // end create
        }

        //! extract diagonal of matrix to block vector
        void extractDiagonal( ColBlockVectorType& diag ) const
        {
          ConstRowIterator endi = this->end();
          for (ConstRowIterator i = this->begin(); i!=endi; ++i)
          {
            // get diagonal entry of matrix
            const size_t row = i.index();
            ConstColIterator entry = (*i).find( row );
            const LittleBlockType& block = (*entry);
            enum { blockSize = LittleBlockType :: rows };
            for( int l=0; l<blockSize; ++l )
            {
              diag[ row ][ l ] = block[ l ][ l ];
            }
          }
        }

        //! return value of entry (row,col) where row and col are global indices not block wise
        //! in order to be consistent with SparseRowMatrix.
        field_type operator()(const std::size_t row, const std::size_t col) const
        {
          const std::size_t blockRow(row/(LittleBlockType :: rows));
          const std::size_t localRowIdx(row%(LittleBlockType :: rows));
          const std::size_t blockCol(col/(LittleBlockType :: cols));
          const std::size_t localColIdx(col%(LittleBlockType :: cols));

          const row_type& matrixRow(this->operator[](blockRow));
          ConstColIterator entry = matrixRow.find( blockCol );
          const LittleBlockType& block = (*entry);
          return block[localRowIdx][localColIdx];
        }

        //! set entry to value (row,col) where row and col are global indices not block wise
        //! in order to be consistent with SparseRowMatrix.
        void set(const std::size_t row, const std::size_t col, field_type value)
        {
          const std::size_t blockRow(row/(LittleBlockType :: rows));
          const std::size_t localRowIdx(row%(LittleBlockType :: rows));
          const std::size_t blockCol(col/(LittleBlockType :: cols));
          const std::size_t localColIdx(col%(LittleBlockType :: cols));

          row_type& matrixRow(this->operator[](blockRow));
          ColIterator entry = matrixRow.find( blockCol );
          LittleBlockType& block = (*entry);
          block[localRowIdx][localColIdx] = value;
        }

        //! print matrix
        void print(std::ostream& s, unsigned int offset=0) const
        {
          s.precision( 6 );
          ConstRowIterator endi=this->end();
          for (ConstRowIterator i=this->begin(); i!=endi; ++i)
          {
            ConstColIterator endj = (*i).end();
            for (ConstColIterator j=(*i).begin(); j!=endj; ++j)
            {
              if( (*j).infinity_norm() > 1.e-15)
              {
                s << i.index()+offset << " " << j.index()+offset << " " << *j << std::endl;
              }
            }
          }
        }
    };

    template <class RowSpaceImp, class ColSpaceImp>
    class ISTLMatrixObject;

    template <class RowSpaceImp, class ColSpaceImp = RowSpaceImp>
    struct ISTLMatrixTraits
    {
      typedef RowSpaceImp RangeSpaceType;
      typedef ColSpaceImp DomainSpaceType;
      typedef ISTLMatrixTraits<DomainSpaceType,RangeSpaceType> ThisType;

      typedef ISTLMatrixObject<DomainSpaceType,RangeSpaceType> MatrixObjectType;
    };

    //! MatrixObject handling an istl matrix
    template <class DomainSpaceImp, class RangeSpaceImp>
    class ISTLMatrixObject
    {
    public:
      //! type of space defining row structure
      typedef DomainSpaceImp DomainSpaceType;
      //! type of space defining column structure
      typedef RangeSpaceImp RangeSpaceType;

      //! type of this pointer
      typedef ISTLMatrixObject<DomainSpaceType,RangeSpaceType> ThisType;

      typedef typename DomainSpaceType::GridType GridType;

      typedef typename RangeSpaceType :: EntityType  RowEntityType ;
      typedef typename DomainSpaceType :: EntityType ColumnEntityType ;

      enum { littleCols = DomainSpaceType :: localBlockSize };
      enum { littleRows = RangeSpaceType :: localBlockSize };

      typedef FieldMatrix<typename DomainSpaceType :: RangeFieldType, littleRows, littleCols> LittleBlockType;

      typedef ISTLBlockVectorDiscreteFunction< RangeSpaceType >      RowDiscreteFunctionType;
      //typedef typename RowDiscreteFunctionType :: LeakPointerType    RowLeakPointerType;
      typedef ISTLBlockVectorDiscreteFunction< DomainSpaceType >     ColumnDiscreteFunctionType;
      //typedef typename ColumnDiscreteFunctionType :: LeakPointerType ColumnLeakPointerType;

    protected:
      typedef typename RowDiscreteFunctionType :: DofStorageType    RowBlockVectorType;
      typedef typename ColumnDiscreteFunctionType :: DofStorageType ColumnBlockVectorType;

      typedef typename RangeSpaceType :: BlockMapperType  RowMapperType;
      typedef typename DomainSpaceType :: BlockMapperType ColMapperType;

    public:
      //! type of used matrix
      typedef ImprovedBCRSMatrix< LittleBlockType ,
                                  ColumnDiscreteFunctionType ,
                                  RowDiscreteFunctionType > MatrixType;
      typedef typename ISTLParallelMatrixAdapter<MatrixType,RangeSpaceType>::Type MatrixAdapterType;
      typedef typename MatrixAdapterType :: ParallelScalarProductType ParallelScalarProductType;

      template <class MatrixObjectImp>
      class LocalMatrix;

      struct LocalMatrixTraits
      {
        typedef DomainSpaceImp DomainSpaceType;
        typedef RangeSpaceImp  RangeSpaceType;
        typedef typename DomainSpaceType :: RangeFieldType RangeFieldType;
        typedef LocalMatrix<ThisType> LocalMatrixType;
        typedef typename MatrixType:: block_type LittleBlockType;
      };

      //! LocalMatrix
      template <class MatrixObjectImp>
      class LocalMatrix : public LocalMatrixDefault<LocalMatrixTraits>
      {
      public:
        //! type of base class
        typedef LocalMatrixDefault<LocalMatrixTraits> BaseType;

        //! type of matrix object
        typedef MatrixObjectImp MatrixObjectType;
        //! type of matrix
        typedef typename MatrixObjectImp :: MatrixType MatrixType;
        //! type of little blocks
        typedef typename MatrixType:: block_type LittleBlockType;
        //! type of entries of little blocks
        typedef typename DomainSpaceType :: RangeFieldType DofType;

        typedef typename MatrixType::row_type RowType;

        //! type of row mapper
        typedef typename MatrixObjectType :: RowMapperType RowMapperType;
        //! type of col mapper
        typedef typename MatrixObjectType :: ColMapperType ColMapperType;

      private:
        // special mapper omiting block size
        const RowMapperType& rowMapper_;
        const ColMapperType& colMapper_;

        // number of local matrices
        int numRows_;
        int numCols_;

        // vector with pointers to local matrices
        typedef std::vector< LittleBlockType* >            LittleMatrixRowStorageType ;
        typedef std::vector< LittleMatrixRowStorageType >  VecLittleMatrixRowStorageType;
        VecLittleMatrixRowStorageType matrices_;

        // matrix to build
        const MatrixObjectType& matrixObj_;

       template <class RowGlobalKey>
       struct ColFunctor
       {
         ColFunctor(LittleMatrixRowStorageType& localMatRow,
                    MatrixType& matrix,
                    const RowGlobalKey &globalRowKey)
         : localMatRow_(localMatRow),
           matRow_( matrix[ globalRowKey ] ),
           globalRowKey_(globalRowKey)
         {
         }
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           localMatRow_[ localDoF ] = &matRow_[ globalDoF ];
         }
         private:
         LittleMatrixRowStorageType& localMatRow_;
         RowType& matRow_;
         const RowGlobalKey &globalRowKey_;
       };

       template <class RowGlobalKey>
       struct ColFunctorImplicitBuildMode
       {
         ColFunctorImplicitBuildMode(LittleMatrixRowStorageType& localMatRow,
                                     MatrixType& matrix,
                                     const RowGlobalKey &globalRowKey)
         : localMatRow_(localMatRow),
           matrix_( matrix ),
           globalRowKey_(globalRowKey)
         {
         }
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           localMatRow_[ localDoF ] = &matrix_.entry( globalRowKey_, globalDoF );
         }
         private:
         LittleMatrixRowStorageType& localMatRow_;
         MatrixType& matrix_;
         const RowGlobalKey &globalRowKey_;
       };

       template <template <class> class ColFunctorImpl>
       struct RowFunctor
       {
         RowFunctor(const ColumnEntityType &colEntity,
                    const ColMapperType &colMapper,
                    MatrixType &matrix,
                    VecLittleMatrixRowStorageType &matrices,
                    int numCols)
         : colEntity_(colEntity),
           colMapper_(colMapper),
           matrix_(matrix),
           matrices_(matrices),
           numCols_(numCols)
         {}
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           LittleMatrixRowStorageType& localMatRow = matrices_[ localDoF ];
           localMatRow.resize( numCols_ );
           ColFunctorImpl< GlobalKey > colFunctor( localMatRow, matrix_, globalDoF );
           colMapper_.mapEach( colEntity_, colFunctor );
         }
         private:
         const ColumnEntityType & colEntity_;
         const ColMapperType & colMapper_;
         MatrixType &matrix_;
         VecLittleMatrixRowStorageType &matrices_;
         int numCols_;
       };

      public:
        LocalMatrix(const MatrixObjectType & mObj,
                    const DomainSpaceType & colSpace,
                    const RangeSpaceType & rowSpace)
          : BaseType( colSpace, rowSpace )
          , rowMapper_(mObj.rowMapper())
          , colMapper_(mObj.colMapper())
          , numRows_( rowMapper_.maxNumDofs() )
          , numCols_( colMapper_.maxNumDofs() )
          , matrixObj_(mObj)
        {
        }

        void init(const RowEntityType & rowEntity,
                  const ColumnEntityType & colEntity)
        {
          // initialize base functions sets
          BaseType :: init ( rowEntity , colEntity );

          numRows_  = rowMapper_.numDofs(colEntity);
          numCols_  = colMapper_.numDofs(rowEntity);
          matrices_.resize( numRows_ );

          if( matrixObj_.implicitModeActive() )
          {
            // implicit access via matrix.entry( i, j )
            RowFunctor< ColFunctorImplicitBuildMode >
              rowFunctor(colEntity, colMapper_, matrixObj_.matrix(), matrices_, numCols_);
            rowMapper_.mapEach(rowEntity, rowFunctor);
          }
          else
          {
            // normal access to matrix via operator [i][j]
            RowFunctor< ColFunctor > rowFunctor(rowEntity, rowMapper_, matrixObj_.matrix(), matrices_, numCols_);
            colMapper_.mapEach(colEntity, rowFunctor);
          }
        }

        LocalMatrix(const LocalMatrix& org)
          : BaseType( org )
          , rowMapper_(org.rowMapper_)
          , colMapper_(org.colMapper_)
          , numRows_( org.numRows_ )
          , numCols_( org.numCols_ )
          , matrices_(org.matrices_)
          , matrixObj_(org.matrixObj_)
        {
        }

      private:
        // check whether given (row,col) pair is valid
        void check(int localRow, int localCol) const
        {
          const size_t row = (int) localRow / littleRows;
          const size_t col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          assert( row < matrices_.size() ) ;
          assert( col < matrices_[row].size() );
          assert( lRow < littleRows );
          assert( lCol < littleCols );
        }

        DofType& getValue(const int localRow, const int localCol)
        {
          const int row = (int) localRow / littleRows;
          const int col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          return (*matrices_[row][col])[lRow][lCol];
        }

      public:
        const DofType get(const int localRow, const int localCol) const
        {
          const int row = (int) localRow / littleRows;
          const int col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          return (*matrices_[row][col])[lRow][lCol];
        }

        void scale (const DofType& scalar)
        {
          for(size_t i=0; i<matrices_.size(); ++i)
            for(size_t j=0; j<matrices_[i].size(); ++j)
              (*matrices_[i][j]) *= scalar;
        }

        void add(const int localRow, const int localCol , const DofType value)
        {
#ifndef NDEBUG
          check(localRow,localCol);
#endif
          getValue(localRow,localCol) += value;
        }

        void set(const int localRow, const int localCol , const DofType value)
        {
#ifndef NDEBUG
          check(localRow,localCol);
#endif
          getValue(localRow,localCol) = value;
        }

        //! make unit row (all zero, diagonal entry 1.0 )
        void unitRow(const int localRow)
        {
          const int row = (int) localRow / littleRows;
          const int lRow = localRow%littleRows;

          // clear row
          doClearRow( row, lRow );

          // set diagonal entry to 1
          (*matrices_[row][row])[lRow][lRow] = 1;
        }

        //! clear all entries belonging to local matrix
        void clear ()
        {
          for(int i=0; i<matrices_.size(); ++i)
            for(int j=0; j<matrices_[i].size(); ++j)
              (*matrices_[i][j]) = (DofType) 0;
        }

        //! set matrix row to zero
        void clearRow ( const int localRow )
        {
          const int row = (int) localRow / littleRows;
          const int lRow = localRow%littleRows;

          // clear the row
          doClearRow( row, lRow );
        }

        //! empty as the little matrices are already sorted
        void resort ()
        {}

    protected:
        //! set matrix row to zero
        void doClearRow ( const int row, const int lRow )
        {
          // get number of columns
          const int col = this->columns();
          for(int localCol=0; localCol<col; ++localCol)
          {
            const int col = (int) localCol / littleCols;
            const int lCol = localCol%littleCols;
            (*matrices_[row][col])[lRow][lCol] = 0;
          }
        }

      }; // end of class LocalMatrix

    public:
      //! type of local matrix
      typedef LocalMatrix<ThisType> ObjectType;
      typedef ThisType LocalMatrixFactoryType;
      typedef ObjectStack< LocalMatrixFactoryType > LocalMatrixStackType;
      //! type of local matrix
      typedef LocalMatrixWrapper< LocalMatrixStackType > LocalMatrixType;
      typedef ColumnObject< ThisType > LocalColumnObjectType;

    protected:
      const DomainSpaceType & domainSpace_;
      const RangeSpaceType & rangeSpace_;

      // sepcial row mapper
      RowMapperType& rowMapper_;
      // special col mapper
      ColMapperType& colMapper_;

      int size_;

      int sequence_;

      mutable MatrixType* matrix_;

      // ParallelScalarProductType scp_;

      mutable LocalMatrixStackType localMatrixStack_;

      mutable MatrixAdapterType* matrixAdap_;
      mutable ColumnBlockVectorType* Arg_;
      mutable RowBlockVectorType*    Dest_;
      // overflow fraction for implicit build mode
      const double overflowFraction_;

      // prohibit copy constructor
      ISTLMatrixObject(const ISTLMatrixObject&);
    public:
      //! constructor
      //! \param rowSpace space defining row structure
      //! \param colSpace space defining column structure
      ISTLMatrixObject ( const DomainSpaceType &domainSpace,
                         const RangeSpaceType &rangeSpace,
                         const std :: string &paramfile = "" )
        : domainSpace_(domainSpace)
        , rangeSpace_(rangeSpace)
        // create scp to have at least one instance
        // otherwise instance will be deleted during setup
        // get new mappers with number of dofs without considerung block size
        , rowMapper_( rangeSpace.blockMapper() )
        , colMapper_( domainSpace.blockMapper() )
        , size_(-1)
        , sequence_(-1)
        , matrix_(0)
        // , scp_(rangeSpace())
        , localMatrixStack_( *this )
        , matrixAdap_(0)
        , Arg_(0)
        , Dest_(0)
        , overflowFraction_( Parameter::getValue( "istl.matrix.overflowfraction", 1.0 ) )
      {
      }

      /** \copydoc Dune::Fem::Operator::assembled */
      static const bool assembled = true ;

      const ThisType& systemMatrix() const { return *this; }
      ThisType& systemMatrix() { return *this; }
    public:
      //! destructor
      ~ISTLMatrixObject()
      {
        removeObj( true );
      }

      //! return reference to system matrix
      MatrixType & matrix() const
      {
        assert( matrix_ );
        return *matrix_;
      }

      void printTexInfo(std::ostream& out) const
      {
        out << "ISTL MatrixObj: ";
        out  << "\\\\ \n";
      }

      //! return matrix adapter object
      std::string preconditionName() const
      {
        return "";
      }

      /*
      MatrixAdapterType createMatrixAdapter() const
      {
        // typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
        // PreConType preconAdapter(matrix(), numIterations, relaxFactor_, preconditioning );
        return MatrixAdapterType(matrix(), domainSpace(), rangeSpace() ); // , preconAdapter );
      }
      */
      void createMatrixAdapter () const
      {
        if( ! matrixAdap_ )
        {
          matrixAdap_ = new MatrixAdapterType(matrixAdapterObject());
        }
      }

      //! return matrix adapter object
      const MatrixAdapterType& matrixAdapter() const
      {
        if( matrixAdap_ == 0 )
          matrixAdap_ = new MatrixAdapterType( matrixAdapterObject() );
        return *matrixAdap_;
      }
      MatrixAdapterType& matrixAdapter()
      {
        if( matrixAdap_ == 0 )
          matrixAdap_ = new MatrixAdapterType( matrixAdapterObject() );
        return *matrixAdap_;
      }

    protected:
      MatrixAdapterType matrixAdapterObject() const
      {
        // need some precondition object - empty here
        typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
        return MatrixAdapterType(matrix(), domainSpace(), rangeSpace(), PreConType() );
      }

    public:
      bool implicitModeActive() const
      {
        // implicit build mode is only active when the
        // build mode of the matrix is implicit and the
        // matrix is currently being build
        if( matrix().buildMode()  == MatrixType::implicit &&
            matrix().buildStage() == MatrixType::building )
          return true;

        return false;
      }

      // compress matrix if not already done before and only in implicit build mode
      void communicate( )
      {
        if( implicitModeActive() )
          matrix().compress();
      }

      //! return true, because in case of no preconditioning we have empty
      //! preconditioner (used by OEM methods)
      bool hasPreconditionMatrix() const { return true; }

      //! set all matrix entries to zero
      void clear()
      {
        matrix().clear();
        // clean matrix adapter and other helper classes
        removeObj( false );
      }

      //! reserve memory for assemble based on the provided stencil
      template <class Stencil>
      void reserve(const Stencil &stencil,
                   const bool implicit = true )
      {
        // if grid sequence number changed, rebuild matrix
        if(sequence_ != domainSpace().sequence())
        {
          removeObj( true );

          if( implicit )
          {
            size_t nnz = stencil.maxNonZerosEstimate();
            if( nnz == 0 )
            {
              Stencil tmpStencil( stencil );
              tmpStencil.fill( *(domainSpace_.begin()), *(rangeSpace_.begin()) );
              nnz = tmpStencil.maxNonZerosEstimate();
            }
            matrix_ = new MatrixType( rowMapper_.size(), colMapper_.size(), nnz, overflowFraction_ );
          }
          else
          {
            matrix_ = new MatrixType( rowMapper_.size(), colMapper_.size() );
            matrix().createEntries( stencil.globalStencil() );
          }

          sequence_ = domainSpace().sequence();
        }
      }

      //! setup new matrix with hanging nodes taken into account
      template <class HangingNodesType>
      void changeHangingNodes(const HangingNodesType& hangingNodes)
      {
        // create new matrix
        MatrixType* newMatrix = new MatrixType(rowMapper_.size(), colMapper_.size());

        // setup with hanging rows
        newMatrix->setup( *matrix_ , hangingNodes );

        // remove old matrix
        removeObj( true );
        // store new matrix
        matrix_ = newMatrix;
      }

      //! extract diagonal entries of the matrix to a discrete function of type
      //! BlockVectorDiscreteFunction
      void extractDiagonal( ColumnDiscreteFunctionType& diag ) const
      {
        // extract diagonal entries
        matrix().extractDiagonal( diag.blockVector() );
      }

      //! we only have right precondition
      bool rightPrecondition() const { return false; }

      //! mult method for OEM Solver
      void multOEM(const double* arg, double* dest) const
      {
        createBlockVectors();

        assert( Arg_ );
        assert( Dest_ );

        RowBlockVectorType& Arg = *Arg_;
        ColumnBlockVectorType & Dest = *Dest_;

        std::copy( std::begin(arg), std::end(arg), Arg.dbegin() );

        // call mult of matrix adapter
        assert( matrixAdap_ );
        matrixAdap_->apply( Arg, Dest );

        std::copy( Dest.dbegin(), Dest.dend(), std::begin(dest) );
      }

      //! apply with discrete functions
      void apply(const RowDiscreteFunctionType& arg,
                 ColumnDiscreteFunctionType& dest) const
      {
        createMatrixAdapter();
        assert( matrixAdap_ );
        matrixAdap_->apply( arg.blockVector(), dest.blockVector() );
      }

      //! apply with arbitrary discrete functions calls multOEM
      template <class RowDFType, class ColDFType>
      void apply(const RowDFType& arg, ColDFType& dest) const
      {
        multOEM( arg.leakPointer(), dest.leakPointer ());
      }

      //! mult method of matrix object used by oem solver
      template <class ColumnLeakPointerType, class RowLeakPointerType>
      void multOEM(const ColumnLeakPointerType& arg, RowLeakPointerType& dest) const
      {
        DUNE_THROW(NotImplemented,"Method has been removed");
        /*
        createMatrixAdapter();
        assert( matrixAdap_ );
        matrixAdap_->apply( arg.blockVector(), dest.blockVector() );
        */
      }

      //! dot method for OEM Solver
      double ddotOEM(const double* v, const double* w) const
      {
        createBlockVectors();

        assert( Arg_ );
        assert( Dest_ );

        RowBlockVectorType&    V = *Arg_;
        ColumnBlockVectorType& W = *Dest_;

        std::copy( std::begin(v), std::end(v), V.dbegin() );
        std::copy( std::begin(w), std::end(w), W.dbegin() );

#if HAVE_MPI
        // in parallel use scalar product of discrete functions
        ISTLBlockVectorDiscreteFunction< DomainSpaceType > vF("ddotOEM:vF", domainSpace(), V );
        ISTLBlockVectorDiscreteFunction< RangeSpaceType  > wF("ddotOEM:wF", rangeSpace(), W );
        return vF.scalarProductDofs( wF );
#else
        return V * W;
#endif
      }

      //! resort row numbering in matrix to have ascending numbering
      void resort()
      {
      }

      //! create precondition matrix does nothing because preconditioner is
      //! created only when requested
      void createPreconditionMatrix()
      {
      }

      //! print matrix
      void print(std::ostream & s) const
      {
        matrix().print(std::cout);
      }

      const DomainSpaceType& domainSpace() const { return domainSpace_; }
      const RangeSpaceType&  rangeSpace() const  { return rangeSpace_; }

      const RowMapperType& rowMapper() const { return rowMapper_; }
      const ColMapperType& colMapper() const { return colMapper_; }

      //! interface method from LocalMatrixFactory
      ObjectType* newObject() const
      {
        return new ObjectType(*this,
                              domainSpace(),
                              rangeSpace());
      }

      //! return local matrix object
      LocalMatrixType localMatrix(const RowEntityType& rowEntity,
                                  const ColumnEntityType& colEntity) const
      {
        return LocalMatrixType( localMatrixStack_, rowEntity, colEntity );
      }
      LocalColumnObjectType localColumn( const ColumnEntityType &colEntity ) const
      {
        return LocalColumnObjectType ( *this, colEntity );
      }

    protected:
      void preConErrorMsg(int preCon) const
      {
        exit(1);
      }

      void removeObj( const bool alsoClearMatrix )
      {
        delete Dest_; Dest_ = 0;
        delete Arg_;  Arg_ = 0;
        delete matrixAdap_; matrixAdap_ = 0;

        if( alsoClearMatrix )
        {
          delete matrix_;
          matrix_ = 0;
        }
      }

      void createBlockVectors() const
      {
        if( ! Arg_ || ! Dest_ )
        {
          delete Arg_; delete Dest_;
          Arg_  = new RowBlockVectorType( rowMapper_.size() );
          Dest_ = new ColumnBlockVectorType( colMapper_.size() );
        }

        createMatrixAdapter ();
      }

    };

    //! MatrixObject handling an istl matrix
    template <class SpaceImp>
    class ISTLMatrixObject<SpaceImp,SpaceImp>
    {
    public:
      typedef SpaceImp DomainSpaceImp;
      typedef SpaceImp RangeSpaceImp;
      //! type of space defining row structure
      typedef DomainSpaceImp DomainSpaceType;
      //! type of space defining column structure
      typedef RangeSpaceImp RangeSpaceType;

      //! type of this pointer
      typedef ISTLMatrixObject<DomainSpaceType,RangeSpaceType> ThisType;

      typedef typename DomainSpaceType::GridType GridType;

      typedef typename RangeSpaceType :: EntityType ColumnEntityType ;
      typedef typename DomainSpaceType :: EntityType RowEntityType ;

      enum { littleRows = DomainSpaceType :: localBlockSize };
      enum { littleCols = RangeSpaceType :: localBlockSize };

      typedef FieldMatrix<typename DomainSpaceType :: RangeFieldType, littleRows, littleCols> LittleBlockType;

      typedef ISTLBlockVectorDiscreteFunction< DomainSpaceType >     RowDiscreteFunctionType;
      //typedef typename RowDiscreteFunctionType :: LeakPointerType  RowLeakPointerType;
      typedef ISTLBlockVectorDiscreteFunction< RangeSpaceType >  ColumnDiscreteFunctionType;
      //typedef typename ColumnDiscreteFunctionType :: LeakPointerType  ColumnLeakPointerType;

    protected:
      typedef typename RowDiscreteFunctionType    :: DofContainerType  RowBlockVectorType;
      typedef typename ColumnDiscreteFunctionType :: DofContainerType  ColumnBlockVectorType;

      typedef typename DomainSpaceType :: BlockMapperType RowMapperType;
      typedef typename RangeSpaceType :: BlockMapperType ColMapperType;

    public:
      //! type of used matrix
      typedef ImprovedBCRSMatrix< LittleBlockType ,
                                  RowDiscreteFunctionType ,
                                  ColumnDiscreteFunctionType > MatrixType;
      typedef typename ISTLParallelMatrixAdapter<MatrixType,DomainSpaceType>::Type MatrixAdapterType;
      // get preconditioner type from MatrixAdapterType
      typedef ThisType PreconditionMatrixType;
      typedef typename MatrixAdapterType :: ParallelScalarProductType ParallelScalarProductType;

      template <class MatrixObjectImp>
      class LocalMatrix;

      struct LocalMatrixTraits
      {
        typedef DomainSpaceImp DomainSpaceType ;
        typedef RangeSpaceImp  RangeSpaceType;
        typedef typename DomainSpaceType :: RangeFieldType RangeFieldType;
        typedef LocalMatrix<ThisType> LocalMatrixType;
        typedef typename MatrixType:: block_type LittleBlockType;
      };

      //! LocalMatrix
      template <class MatrixObjectImp>
      class LocalMatrix : public LocalMatrixDefault<LocalMatrixTraits>
      {
      public:
        //! type of base class
        typedef LocalMatrixDefault<LocalMatrixTraits> BaseType;

        //! type of matrix object
        typedef MatrixObjectImp MatrixObjectType;
        //! type of matrix
        typedef typename MatrixObjectImp :: MatrixType MatrixType;
        //! type of little blocks
        typedef typename MatrixType:: block_type LittleBlockType;
        //! type of entries of little blocks
        typedef typename DomainSpaceType :: RangeFieldType DofType;

        typedef typename MatrixType::row_type RowType;

        //! type of row mapper
        typedef typename MatrixObjectType :: RowMapperType RowMapperType;
        //! type of col mapper
        typedef typename MatrixObjectType :: ColMapperType ColMapperType;

      private:
        // special mapper omiting block size
        const RowMapperType& rowMapper_;
        const ColMapperType& colMapper_;

        // number of local matrices
        int numRows_;
        int numCols_;

        // vector with pointers to local matrices
        typedef std::vector< LittleBlockType* >            LittleMatrixRowStorageType ;
        typedef std::vector< LittleMatrixRowStorageType >  VecLittleMatrixRowStorageType;
        VecLittleMatrixRowStorageType matrices_;

        // matrix to build
        const MatrixObjectType& matrixObj_;

       template <class RowGlobalKey>
       struct ColFunctor
       {
         ColFunctor(LittleMatrixRowStorageType& localMatRow,
                    MatrixType& matrix,
                    const RowGlobalKey &globalRowKey)
         : localMatRow_(localMatRow),
           matRow_( matrix[ globalRowKey ] ),
           globalRowKey_(globalRowKey)
         {
         }
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           localMatRow_[ localDoF ] = &matRow_[ globalDoF ];
         }
         private:
         LittleMatrixRowStorageType& localMatRow_;
         RowType& matRow_;
         const RowGlobalKey &globalRowKey_;
       };

       template <class RowGlobalKey>
       struct ColFunctorImplicitBuildMode
       {
         ColFunctorImplicitBuildMode(LittleMatrixRowStorageType& localMatRow,
                                     MatrixType& matrix,
                                     const RowGlobalKey &globalRowKey)
         : localMatRow_(localMatRow),
           matrix_( matrix ),
           globalRowKey_(globalRowKey)
         {
         }
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           localMatRow_[ localDoF ] = &matrix_.entry( globalRowKey_, globalDoF );
         }
         private:
         LittleMatrixRowStorageType& localMatRow_;
         MatrixType& matrix_;
         const RowGlobalKey &globalRowKey_;
       };

       template <template <class> class ColFunctorImpl>
       struct RowFunctor
       {
         RowFunctor(const RowEntityType &rowEntity,
                    const RowMapperType &rowMapper,
                    MatrixType &matrix,
                    VecLittleMatrixRowStorageType &matrices,
                    int numCols)
         : rowEntity_(rowEntity),
           rowMapper_(rowMapper),
           matrix_(matrix),
           matrices_(matrices),
           numCols_(numCols)
         {}
         template< class GlobalKey >
         void operator() ( const int localDoF, const GlobalKey &globalDoF )
         {
           LittleMatrixRowStorageType& localMatRow = matrices_[ localDoF ];
           localMatRow.resize( numCols_ );
           ColFunctorImpl< GlobalKey > colFunctor( localMatRow, matrix_, globalDoF );
           rowMapper_.mapEach( rowEntity_, colFunctor );
         }
         private:
         const RowEntityType & rowEntity_;
         const RowMapperType & rowMapper_;
         MatrixType &matrix_;
         VecLittleMatrixRowStorageType &matrices_;
         int numCols_;
       };

      public:
        LocalMatrix(const MatrixObjectType & mObj,
                    const DomainSpaceType & rowSpace,
                    const RangeSpaceType & colSpace)
          : BaseType( rowSpace, colSpace )
          , rowMapper_(mObj.rowMapper())
          , colMapper_(mObj.colMapper())
          , numRows_( rowMapper_.maxNumDofs() )
          , numCols_( colMapper_.maxNumDofs() )
          , matrixObj_(mObj)
        {
        }

        void init(const RowEntityType & rowEntity,
                  const ColumnEntityType & colEntity)
        {
          // initialize base functions sets
          BaseType :: init ( rowEntity , colEntity );

          numRows_  = rowMapper_.numDofs(colEntity);
          numCols_  = colMapper_.numDofs(rowEntity);
          matrices_.resize( numRows_ );

          if( matrixObj_.implicitModeActive() )
          {
            // implicit access via matrix.entry( i, j )
            RowFunctor< ColFunctorImplicitBuildMode >
              rowFunctor(rowEntity, rowMapper_, matrixObj_.matrix(), matrices_, numCols_);
            colMapper_.mapEach(colEntity, rowFunctor);
          }
          else
          {
            // normal access to matrix via operator [i][j]
            RowFunctor< ColFunctor > rowFunctor(rowEntity, rowMapper_, matrixObj_.matrix(), matrices_, numCols_);
            colMapper_.mapEach(colEntity, rowFunctor);
          }
        }

        LocalMatrix(const LocalMatrix& org)
          : BaseType( org )
          , rowMapper_(org.rowMapper_)
          , colMapper_(org.colMapper_)
          , numRows_( org.numRows_ )
          , numCols_( org.numCols_ )
          , matrices_(org.matrices_)
          , matrixObj_(org.matrixObj_)
        {
        }

      private:
        // check whether given (row,col) pair is valid
        void check(int localRow, int localCol) const
        {
          const size_t row = (int) localRow / littleRows;
          const size_t col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          assert( row < matrices_.size() ) ;
          assert( col < matrices_[row].size() );
          assert( lRow < littleRows );
          assert( lCol < littleCols );
        }

        DofType& getValue(const int localRow, const int localCol)
        {
          const int row = (int) localRow / littleRows;
          const int col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          return (*matrices_[row][col])[lRow][lCol];
        }

      public:
        const DofType get(const int localRow, const int localCol) const
        {
          const int row = (int) localRow / littleRows;
          const int col = (int) localCol / littleCols;
          const int lRow = localRow%littleRows;
          const int lCol = localCol%littleCols;
          return (*matrices_[row][col])[lRow][lCol];
        }

        void scale (const DofType& scalar)
        {
          for(size_t i=0; i<matrices_.size(); ++i)
            for(size_t j=0; j<matrices_[i].size(); ++j)
              (*matrices_[i][j]) *= scalar;
        }

        void add(const int localRow, const int localCol , const DofType value)
        {
#ifndef NDEBUG
          check(localRow,localCol);
#endif
          getValue(localRow,localCol) += value;
        }

        void set(const int localRow, const int localCol , const DofType value)
        {
#ifndef NDEBUG
          check(localRow,localCol);
#endif
          getValue(localRow,localCol) = value;
        }

        //! make unit row (all zero, diagonal entry 1.0 )
        void unitRow(const int localRow)
        {
          const int row = (int) localRow / littleRows;
          const int lRow = localRow%littleRows;

          // clear row
          doClearRow( row, lRow );

          // set diagonal entry to 1
          (*matrices_[row][row])[lRow][lRow] = 1;
        }

        //! clear all entries belonging to local matrix
        void clear ()
        {
          for(int i=0; i<matrices_.size(); ++i)
            for(int j=0; j<matrices_[i].size(); ++j)
              (*matrices_[i][j]) = (DofType) 0;
        }

        //! set matrix row to zero
        void clearRow ( const int localRow )
        {
          const int row = (int) localRow / littleRows;
          const int lRow = localRow%littleRows;

          // clear the row
          doClearRow( row, lRow );
        }

        //! empty as the little matrices are already sorted
        void resort ()
        {}

    protected:
        //! set matrix row to zero
        void doClearRow ( const int row, const int lRow )
        {
          // get number of columns
          const int col = this->columns();
          for(int localCol=0; localCol<col; ++localCol)
          {
            const int col = (int) localCol / littleCols;
            const int lCol = localCol%littleCols;
            (*matrices_[row][col])[lRow][lCol] = 0;
          }
        }

      }; // end of class LocalMatrix

    public:
      //! type of local matrix
      typedef LocalMatrix<ThisType> ObjectType;
      typedef ThisType LocalMatrixFactoryType;
      typedef ObjectStack< LocalMatrixFactoryType > LocalMatrixStackType;
      //! type of local matrix
      typedef LocalMatrixWrapper< LocalMatrixStackType > LocalMatrixType;
      typedef ColumnObject< ThisType > LocalColumnObjectType;

    protected:
      const DomainSpaceType & domainSpace_;
      const RangeSpaceType & rangeSpace_;

      // sepcial row mapper
      RowMapperType& rowMapper_;
      // special col mapper
      ColMapperType& colMapper_;

      int size_;

      int sequence_;

      mutable MatrixType* matrix_;

      ParallelScalarProductType scp_;

      int numIterations_;
      double relaxFactor_;

      enum ISTLPreConder_Id { none  = 0 ,      // no preconditioner
                              ssor  = 1 ,      // SSOR preconditioner
                              sor   = 2 ,      // SOR preconditioner
                              ilu_0 = 3 ,      // ILU-0 preconditioner
                              ilu_n = 4 ,      // ILU-n preconditioner
                              gauss_seidel= 5, // Gauss-Seidel preconditioner
                              jacobi = 6,      // Jacobi preconditioner
                              amg_ilu_0 = 7,   // AMG with ILU-0 smoother
                              amg_ilu_n = 8,   // AMG with ILU-n smoother
                              amg_jacobi = 9   // AMG with Jacobi smoother
      };

      ISTLPreConder_Id preconditioning_;

      mutable LocalMatrixStackType localMatrixStack_;

      mutable MatrixAdapterType* matrixAdap_;
      mutable RowBlockVectorType* Arg_;
      mutable ColumnBlockVectorType* Dest_;
      // overflow fraction for implicit build mode
      const double overflowFraction_;

    public:
      ISTLMatrixObject(const ISTLMatrixObject&) = delete;

      //! constructor
      //! \param rowSpace space defining row structure
      //! \param colSpace space defining column structure
      //! \param paramfile parameter file to read variables
      //!         - Preconditioning: {0,1,2,3,4,5,6} put -1 to get info
      //!         - Pre-iteration: number of iteration of preconditioner
      //!         - Pre-relaxation: relaxation factor
      ISTLMatrixObject ( const DomainSpaceType &rowSpace,
                         const RangeSpaceType &colSpace,
                         const std :: string &paramfile = "" )
        : domainSpace_(rowSpace)
        , rangeSpace_(colSpace)
        // create scp to have at least one instance
        // otherwise instance will be deleted during setup
        // get new mappers with number of dofs without considerung block size
        , rowMapper_( rowSpace.blockMapper() )
        , colMapper_( colSpace.blockMapper() )
        , size_(-1)
        , sequence_(-1)
        , matrix_(0)
        , scp_(rangeSpace())
        , numIterations_(5)
        , relaxFactor_(1.1)
        , preconditioning_(none)
        , localMatrixStack_( *this )
        , matrixAdap_(0)
        , Arg_(0)
        , Dest_(0)
        , overflowFraction_( Parameter::getValue( "istl.matrix.overflowfraction", 1.0 ) )
      {
        int preCon = 0;
        if(paramfile != "")
        {
          DUNE_THROW(InvalidStateException,"ISTLMatrixObject: old parameter method disabled");
        }
        else
        {
          static const std::string preConTable[]
            = { "none", "ssor", "sor", "ilu-0", "ilu-n", "gauss-seidel", "jacobi",
              "amg-ilu-0", "amg-ilu-n", "amg-jacobi" };
          preCon         = Parameter::getEnum( "istl.preconditioning.method", preConTable, preCon );
          numIterations_ = Parameter::getValue( "istl.preconditioning.iterations", numIterations_ );
          relaxFactor_   = Parameter::getValue( "istl.preconditioning.relaxation", relaxFactor_ );
        }

        if( preCon >= 0 && preCon <= 9)
          preconditioning_ = (ISTLPreConder_Id) preCon;
        else
          preConErrorMsg(preCon);

        assert( rowMapper_.size() == colMapper_.size() );
      }

      /** \copydoc Dune::Fem::Operator::assembled */
      static const bool assembled = true ;

      const ThisType& systemMatrix() const { return *this; }
      ThisType& systemMatrix() { return *this; }
    public:
      //! destructor
      ~ISTLMatrixObject()
      {
        removeObj( true );
      }

      //! return reference to system matrix
      MatrixType & matrix() const
      {
        assert( matrix_ );
        return *matrix_;
      }

      void printTexInfo(std::ostream& out) const
      {
        out << "ISTL MatrixObj: ";
        out << " preconditioner = " << preconditionName() ;
        out  << "\\\\ \n";
      }

      //! return matrix adapter object
      std::string preconditionName() const
      {
        std::stringstream tmp ;
        // no preconditioner
        switch (preconditioning_)
        {
          case ssor : tmp << "SSOR"; break;
          case sor  : tmp << "SOR"; break;
          case ilu_0: tmp << "ILU-0"; break;
          case ilu_n: tmp << "ILU-n"; break;
          case gauss_seidel : tmp << "Gauss-Seidel"; break;
          case jacobi: tmp << "Jacobi"; break;
          default: tmp << "None"; break;
        }

        if( preconditioning_ != ilu_0 )
        {
          tmp << " n=" << numIterations_;
        }
        tmp << " relax=" << relaxFactor_ ;
        return tmp.str();
      }

      template <class PreconditionerType>
      MatrixAdapterType
      createMatrixAdapter(const PreconditionerType* preconditioning,
                          size_t numIterations) const
      {
        typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
        PreConType preconAdapter(matrix(), numIterations, relaxFactor_, preconditioning );
        return MatrixAdapterType(matrix(), domainSpace(), rangeSpace(), preconAdapter );
      }

      template <class PreconditionerType>
      MatrixAdapterType
      createAMGMatrixAdapter(const PreconditionerType* preconditioning,
                             size_t numIterations) const
      {
        typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
        PreConType preconAdapter(matrix(), numIterations, relaxFactor_, preconditioning, domainSpace().gridPart().comm() );
        return MatrixAdapterType(matrix(), domainSpace(), rangeSpace(), preconAdapter );
      }

      //! return matrix adapter object
      const MatrixAdapterType& matrixAdapter() const
      {
        if( matrixAdap_ == 0 )
          matrixAdap_ = new MatrixAdapterType( matrixAdapterObject() );
        return *matrixAdap_;
      }
      MatrixAdapterType& matrixAdapter()
      {
        if( matrixAdap_ == 0 )
          matrixAdap_ = new MatrixAdapterType( matrixAdapterObject() );
        return *matrixAdap_;
      }

    protected:
      MatrixAdapterType matrixAdapterObject() const
      {
#ifndef DISABLE_ISTL_PRECONDITIONING
        const size_t procs = domainSpace().gridPart().comm().size();

        typedef typename MatrixType :: BaseType ISTLMatrixType ;
        typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
        // no preconditioner
        if( preconditioning_ == none )
        {
          return MatrixAdapterType(matrix(), domainSpace(),rangeSpace(), PreConType() );
        }
        // SSOR
        else if( preconditioning_ == ssor )
        {
          if( procs > 1 )
            DUNE_THROW(InvalidStateException,"ISTL::SeqSSOR not working in parallel computations");

          typedef SeqSSOR<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // SOR
        else if(preconditioning_ == sor )
        {
          if( procs > 1 )
            DUNE_THROW(InvalidStateException,"ISTL::SeqSOR not working in parallel computations");

          typedef SeqSOR<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // ILU-0
        else if(preconditioning_ == ilu_0)
        {
          if( procs > 1 )
            DUNE_THROW(InvalidStateException,"ISTL::SeqILU0 not working in parallel computations");

          typedef FemSeqILU0<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // ILU-n
        else if(preconditioning_ == ilu_n)
        {
          if( procs > 1 )
            DUNE_THROW(InvalidStateException,"ISTL::SeqILUn not working in parallel computations");

          typedef SeqILUn<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // Gauss-Seidel
        else if(preconditioning_ == gauss_seidel)
        {
          if( procs > 1 )
            DUNE_THROW(InvalidStateException,"ISTL::SeqGS not working in parallel computations");

          typedef SeqGS<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // Jacobi
        else if(preconditioning_ == jacobi)
        {
          if( numIterations_ == 1 ) // diagonal preconditioning
          {
            typedef FemDiagonalPreconditioner< ThisType, RowBlockVectorType, ColumnBlockVectorType > PreconditionerType;
            typedef typename MatrixAdapterType :: PreconditionAdapterType PreConType;
            PreConType preconAdapter( matrix(), new PreconditionerType( *this ) );
            return MatrixAdapterType( matrix(), domainSpace(), rangeSpace(), preconAdapter );
          }
          else if ( procs == 1 )
          {
            typedef SeqJac<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
            return createMatrixAdapter( (PreconditionerType*)0, numIterations_ );
          }
          else
          {
            DUNE_THROW(InvalidStateException,"ISTL::SeqJac(Jacobi) only working with istl.preconditioning.iterations: 1 in parallel computations");
          }
        }
        // AMG ILU-0
        else if(preconditioning_ == amg_ilu_0)
        {
          // use original SeqILU0 because of some AMG traits classes.
          typedef SeqILU0<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createAMGMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // AMG ILU-n
        else if(preconditioning_ == amg_ilu_n)
        {
          typedef SeqILUn<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType> PreconditionerType;
          return createAMGMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        // AMG Jacobi
        else if(preconditioning_ == amg_jacobi)
        {
          typedef SeqJac<ISTLMatrixType,RowBlockVectorType,ColumnBlockVectorType,1> PreconditionerType;
          return createAMGMatrixAdapter( (PreconditionerType*)0, numIterations_ );
        }
        else
        {
          preConErrorMsg(preconditioning_);
        }
#endif

        return MatrixAdapterType(matrix(), domainSpace(), rangeSpace(), PreConType() );
      }

    public:
      bool implicitModeActive() const
      {
        // implicit build mode is only active when the
        // build mode of the matrix is implicit and the
        // matrix is currently being build
        if( matrix().buildMode()  == MatrixType::implicit &&
            matrix().buildStage() == MatrixType::building )
          return true;

        return false;
      }

      // compress matrix if not already done before and only in implicit build mode
      void communicate( )
      {
        if( implicitModeActive() )
          matrix().compress();
      }

      //! return true, because in case of no preconditioning we have empty
      //! preconditioner (used by OEM methods)
      bool hasPreconditionMatrix() const { return (preconditioning_ != none); }

      //! return reference to preconditioner object (used by OEM methods)
      const PreconditionMatrixType& preconditionMatrix() const { return *this; }

      //! set all matrix entries to zero
      void clear()
      {
        matrix().clear();
        // clean matrix adapter and other helper classes
        removeObj( false );
      }

      //! reserve memory for assemble based on the provided stencil
      template <class Stencil>
      void reserve(const Stencil &stencil,
                   const bool implicit = true )
      {
        // if grid sequence number changed, rebuild matrix
        if(sequence_ != domainSpace().sequence())
        {
          removeObj( true );

          if( implicit )
          {
            size_t nnz = stencil.maxNonZerosEstimate();
            if( nnz == 0 )
            {
              Stencil tmpStencil( stencil );
              tmpStencil.fill( *(domainSpace_.begin()), *(rangeSpace_.begin()) );
              nnz = tmpStencil.maxNonZerosEstimate();
            }
            matrix_ = new MatrixType( rowMapper_.size(), colMapper_.size(), nnz, overflowFraction_ );
          }
          else
          {
            matrix_ = new MatrixType( rowMapper_.size(), colMapper_.size() );
            matrix().createEntries( stencil.globalStencil() );
          }

          sequence_ = domainSpace().sequence();
        }
      }

      //! setup new matrix with hanging nodes taken into account
      template <class HangingNodesType>
      void changeHangingNodes(const HangingNodesType& hangingNodes)
      {
        // create new matrix
        MatrixType* newMatrix = new MatrixType(rowMapper_.size(), colMapper_.size());

        // setup with hanging rows
        newMatrix->setup( *matrix_ , hangingNodes );

        // remove old matrix
        removeObj( true );
        // store new matrix
        matrix_ = newMatrix;
      }

      //! extract diagonal entries of the matrix to a discrete function of type
      //! BlockVectorDiscreteFunction
      void extractDiagonal( ColumnDiscreteFunctionType& diag ) const
      {
        // extract diagonal entries
        matrix().extractDiagonal( diag.blockVector() );
      }

      //! we only have right precondition
      bool rightPrecondition() const { return true; }

      //! precondition method for OEM Solvers
      //! not fast but works, double is copied to block vector
      //! and after application copied back
      void precondition(const double* arg, double* dest) const
      {
        createBlockVectors();

        assert( Arg_ );
        assert( Dest_ );

        RowBlockVectorType& Arg = *Arg_;
        ColumnBlockVectorType & Dest = *Dest_;

        std::copy( std::begin(arg), std::end(arg), Arg.dbegin());

        // set Dest to zero
        Dest = 0;

        assert( matrixAdap_ );
        // not parameter swaped for preconditioner
        matrixAdap_->preconditionAdapter().apply(Dest , Arg);

        std::copy( Dest.dbegin(), Dest.dend(), std::begin(dest) );
      }

      //! mult method for OEM Solver
      void multOEM(const double* arg, double* dest) const
      {
        createBlockVectors();

        assert( Arg_ );
        assert( Dest_ );

        RowBlockVectorType& Arg = *Arg_;
        ColumnBlockVectorType & Dest = *Dest_;

        std::copy( std::begin(arg), std::end(arg), Arg.dbegin() );

        // call mult of matrix adapter
        assert( matrixAdap_ );
        matrixAdap_->apply( Arg, Dest );

        std::copy( Dest.dbegin(), Dest.dend(), std::begin(dest) );
      }

      //! apply with discrete functions
      void apply(const RowDiscreteFunctionType& arg,
                 ColumnDiscreteFunctionType& dest) const
      {
        createMatrixAdapter();
        assert( matrixAdap_ );
        matrixAdap_->apply( arg.blockVector(), dest.blockVector() );
      }

      //! apply with arbitrary discrete functions calls multOEM
      template <class RowDFType, class ColDFType>
      void apply(const RowDFType& arg, ColDFType& dest) const
      {
        multOEM( arg.leakPointer(), dest.leakPointer ());
      }

      //! mult method of matrix object used by oem solver
      template <class RowLeakPointerType, class ColumnLeakPointerType>
      void multOEM(const RowLeakPointerType& arg, ColumnLeakPointerType& dest) const
      {
        DUNE_THROW(NotImplemented,"Method has been removed");
        /*
        createMatrixAdapter();
        assert( matrixAdap_ );
        matrixAdap_->apply( arg.blockVector(), dest.blockVector() );
        */
      }

      //! dot method for OEM Solver
      double ddotOEM(const double* v, const double* w) const
      {
        createBlockVectors();

        assert( Arg_ );
        assert( Dest_ );

        RowBlockVectorType&    V = *Arg_;
        ColumnBlockVectorType& W = *Dest_;

        std::copy( std::begin(v), std::end(v), V.dbegin() );
        std::copy( std::begin(w), std::end(w), W.dbegin() );

#if HAVE_MPI
        // in parallel use scalar product of discrete functions
        ISTLBlockVectorDiscreteFunction< DomainSpaceType > vF("ddotOEM:vF", domainSpace(), V );
        ISTLBlockVectorDiscreteFunction< RangeSpaceType  > wF("ddotOEM:wF", rangeSpace(), W );
        return vF.scalarProductDofs( wF );
#else
        return V * W;
#endif
      }

      //! resort row numbering in matrix to have ascending numbering
      void resort()
      {
      }

      //! create precondition matrix does nothing because preconditioner is
      //! created only when requested
      void createPreconditionMatrix()
      {
      }

      //! print matrix
      void print(std::ostream & s) const
      {
        matrix().print(std::cout);
      }

      const DomainSpaceType& domainSpace() const { return domainSpace_; }
      const RangeSpaceType&  rangeSpace() const  { return rangeSpace_; }

      const RowMapperType& rowMapper() const { return rowMapper_; }
      const ColMapperType& colMapper() const { return colMapper_; }

      //! interface method from LocalMatrixFactory
      ObjectType* newObject() const
      {
        return new ObjectType(*this,
                              domainSpace(),
                              rangeSpace());
      }

      //! return local matrix object
      LocalMatrixType localMatrix(const RowEntityType& rowEntity,
                                  const ColumnEntityType& colEntity) const
      {
        return LocalMatrixType( localMatrixStack_, rowEntity, colEntity );
      }
      LocalColumnObjectType localColumn( const ColumnEntityType &colEntity ) const
      {
        return LocalColumnObjectType ( *this, colEntity );
      }

    protected:
      void preConErrorMsg(int preCon) const
      {
        std::cerr << "ERROR: Wrong precoditioning number (p = " << preCon;
        std::cerr <<") in ISTLMatrixObject! \n";
        std::cerr <<"Valid values are: \n";
        std::cerr <<"0 == no \n";
        std::cerr <<"1 == SSOR \n";
        std::cerr <<"2 == SOR \n";
        std::cerr <<"3 == ILU-0 \n";
        std::cerr <<"4 == ILU-n \n";
        std::cerr <<"5 == Gauss-Seidel \n";
        std::cerr <<"6 == Jacobi \n";
        assert(false);
        exit(1);
      }

      void removeObj( const bool alsoClearMatrix )
      {
        delete Dest_; Dest_ = 0;
        delete Arg_;  Arg_ = 0;
        delete matrixAdap_; matrixAdap_ = 0;

        if( alsoClearMatrix )
        {
          delete matrix_;
          matrix_ = 0;
        }
      }

      void createBlockVectors() const
      {
        if( ! Arg_ || ! Dest_ )
        {
          delete Arg_; delete Dest_;
          Arg_  = new RowBlockVectorType( rowMapper_.size() );
          Dest_ = new ColumnBlockVectorType( colMapper_.size() );
        }

        createMatrixAdapter ();
      }

      void createMatrixAdapter () const
      {
        if( ! matrixAdap_ )
        {
          matrixAdap_ = new MatrixAdapterType(matrixAdapterObject());
        }
      }

    };

  } // namespace Fem

} // namespace Dune

#endif // #if HAVE_DUNE_ISTL

#endif // #ifndef DUNE_FEM_ISTLMATRIXWRAPPER_HH
