#ifndef DUNE_FEM_SPQRSOLVER_HH
#define DUNE_FEM_SPQRSOLVER_HH

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <dune/common/hybridutilities.hh>
#include <dune/common/std/utility.hh>
#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/function/blockvectorfunction.hh>
#include <dune/fem/function/tuplediscretefunction.hh>
#include <dune/fem/io/parameter.hh>
#include <dune/fem/operator/common/operator.hh>
#include <dune/fem/operator/matrix/colcompspmatrix.hh>

#if HAVE_SUITESPARSE_SPQR
#include <SuiteSparseQR.hpp>

namespace Dune
{
namespace Fem
{

/** @addtogroup DirectSolver
 *
 *  In this section implementations of direct solvers
 *  for solving linear systems of the from
 *  \f$A x = b\f$, where \f$A\f$ is a Mapping or
 *  Operator and \f$x\f$ and \f$b\f$ are discrete functions
 *  (see DiscreteFunctionInterface) can be found.
 */

/** \class SPQROp
 *  \ingroup DirectSolver
 *  \brief The %SPQR direct sparse solver
 *  %SPQR will always go double precision and supports complex numbers.
 *  Details on SPQR can be found on http://www.cise.ufl.edu/research/sparse/spqr/
 *  \note This will only work if dune-fem has been configured to use SPQR
 */
template<class DF, class Op, bool symmetric=false>
class SPQROp:public Operator<DF, DF>
{
  public:
  typedef DF DiscreteFunctionType;
  typedef Op OperatorType;

  // \brief The column-compressed matrix type.
  typedef ColCompMatrix<typename OperatorType::MatrixType::MatrixBaseType> CCSMatrixType;
  typedef typename DiscreteFunctionType::DofType DofType;
  typedef typename DiscreteFunctionType::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   *  \param[in] verbose verbosity
   */
  SPQROp(const double& redEps, const double& absLimit, const int& maxIter, const bool& verbose,
         const ParameterReader &paramter = Parameter::container() ) :
    SPQROp(verbose)
  {}

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   */
  SPQROp(const double& redEps, const double& absLimit, const int& maxIter,
         const ParameterReader &parameter = Parameter::container() ) :
    SPQROp(parameter.getValue<bool>("fem.solver.verbose",false))
  {}

  SPQROp(const ParameterReader &parameter = Parameter::container() ) :
    SPQROp(parameter.getValue<bool>("fem.solver.verbose",false))
  {}

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   *  \param[in] verbose verbosity
   */
  SPQROp(const OperatorType& op, const double& redEps, const double& absLimit, const int& maxIter, const bool& verbose,
      const ParameterReader &paramter = Parameter::container() ) :
    SPQROp(verbose)
  {
    bind(op);
  }

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   */
  SPQROp(const OperatorType& op, const double& redEps, const double& absLimit, const int& maxIter,
         const ParameterReader &parameter = Parameter::container() ) :
    SPQROp(parameter.getValue<bool>("fem.solver.verbose",false))
  {
    bind(op);
  }

  SPQROp(const OperatorType& op, const ParameterReader &parameter = Parameter::container() ) :
    SPQROp(parameter.getValue<bool>("fem.solver.verbose",false))
  {
    bind(op);
  }

  // \brief Destructor.
  ~SPQROp()
  {
    finalize();
    cholmod_l_finish(cc_);
    delete cc_;
  }

  void bind (const OperatorType& op) { op_ = &op; }
  void unbind () { op_ = nullptr; finalize(); }

  /** \brief Solve the system
   *  \param[in] arg right hand side
   *  \param[out] dest solution
   */
  void operator()(const DiscreteFunctionType& arg, DiscreteFunctionType& dest) const
  {
    prepare();
    apply(arg,dest);
    finalize();
  }

  // \brief Decompose matrix.
  template<typename... A>
  void prepare(A... ) const
  {
    if(op_ && !isloaded_)
    {
      ccsmat_ = op_->systemMatrix().matrix();
      decompose();
      isloaded_ = true;
    }
  }

  // \brief Free allocated memory.
  void finalize() const
  {
    if(isloaded_)
    {
      ccsmat_.free();
      cholmod_l_free_sparse(&A_, cc_);
      cholmod_l_free_dense(&B_, cc_);
      SuiteSparseQR_free<DofType>(&spqrfactorization_, cc_);
      isloaded_ = false;
    }
  }

  /** \brief Solve the system.
   *  \param[in] arg right hand side
   *  \param[out] dest solution
   *  \warning You have to decompose the matrix before calling the apply (using the method prepare)
   *   and you have free the decompistion when is not needed anymore (using the method finalize).
   */
  void apply(const DofType* arg, DofType* dest) const
  {
    const std::size_t dimMat(ccsmat_.N());
    // fill B
    for(std::size_t k = 0; k != dimMat; ++k)
      (static_cast<DofType*>(B_->x))[k] = arg[k];
    cholmod_dense* BTemp = B_;
    B_ = SuiteSparseQR_qmult<DofType>(0, spqrfactorization_, B_, cc_);
    cholmod_dense* X = SuiteSparseQR_solve<DofType>(1, spqrfactorization_, B_, cc_);
    cholmod_l_free_dense(&BTemp, cc_);
    // fill x
    for(std::size_t k = 0; k != dimMat; ++k)
      dest[k] = (static_cast<DofType*>(X->x))[k];
    cholmod_l_free_dense(&X, cc_);
    // output some statistics
    if(verbose_ > 0)
      printDecompositionInfo();
  }

  /** \brief Solve the system.
   *  \param[in] arg right hand side
   *  \param[out] dest solution
   *  \warning You have to decompose the matrix before calling the apply (using the method prepare)
   *   and you have free the decompistion when is not needed anymore (using the method finalize).
   */
  void apply(const AdaptiveDiscreteFunction<DiscreteFunctionSpaceType>& arg,
             AdaptiveDiscreteFunction<DiscreteFunctionSpaceType>& dest) const
  {
    apply(arg.leakPointer(),dest.leakPointer());
  }

  /** \brief Solve the system.
   *  \param[in] arg right hand side
   *  \param[out] dest solution
   *  \warning You have to decompose the matrix before calling the apply (using the method prepare)
   *   and you have free the decompistion when is not needed anymore (using the method finalize).
   */
  void apply(const ISTLBlockVectorDiscreteFunction<DiscreteFunctionSpaceType>& arg,
             ISTLBlockVectorDiscreteFunction<DiscreteFunctionSpaceType>& dest) const
  {
    // copy DOF's arg into a consecutive vector
    std::vector<DofType> vecArg(arg.size());
    std::copy(arg.dbegin(),arg.dend(),vecArg.begin());
    std::vector<DofType> vecDest(dest.size());
    // apply operator
    apply(vecArg.data(),vecDest.data());
    // copy back solution into dest
    std::copy(vecDest.begin(),vecDest.end(),dest.dbegin());
  }

  /** \brief Solve the system.
   *  \param[in] arg right hand side
   *  \param[out] dest solution
   *  \warning You have to decompose the matrix before calling the apply (using the method prepare)
   *   and you have free the decompistion when is not needed anymore (using the method finalize).
   */
  template<typename... DFs>
  void apply(const TupleDiscreteFunction<DFs...>& arg,TupleDiscreteFunction<DFs...>& dest) const
  {
    // copy DOF's arg into a consecutive vector
    std::vector<DofType> vecArg(arg.size());
    auto vecArgIt(vecArg.begin());
    Hybrid::forEach(Std::make_index_sequence<sizeof...(DFs)>{},
      [&](auto i){vecArgIt=std::copy(std::get<i>(arg).dbegin(),std::get<i>(arg).dend(),vecArgIt);});
    std::vector<DofType> vecDest(dest.size());
    // apply operator
    apply(vecArg.data(),vecDest.data());
    // copy back solution into dest
    auto vecDestIt(vecDest.begin());
    Hybrid::forEach(Std::make_index_sequence<sizeof...(DFs)>{},[&](auto i){for(auto& dof:dofs(std::get<i>(dest))) dof=(*(vecDestIt++));});
  }

  void printTexInfo(std::ostream& out) const
  {
    out<<"Solver: SPQR direct solver"<<std::endl;
  }

  // \brief Print some statistics about the SPQR decomposition.
  void printDecompositionInfo() const
  {
    if(isloaded_)
    {
      std::cout<<std::endl<<"Solving with SuiteSparseQR"<<std::endl;
      std::cout<<"Flops Taken: "<<cc_->SPQR_flopcount<<std::endl;
      std::cout<<"Analysis Time: "<<cc_->SPQR_analyze_time<<" s"<<std::endl;
      std::cout<<"Factorize Time: "<<cc_->SPQR_factorize_time<<" s"<<std::endl;
      std::cout<<"Backsolve Time: "<<cc_->SPQR_solve_time<<" s"<<std::endl;
      std::cout<<"Peak Memory Usage: "<<cc_->memory_usage<<" bytes"<<std::endl;
      std::cout<<"Rank Estimate: "<<cc_->SPQR_istat[4]<<std::endl<<std::endl;
    }
  }

  double averageCommTime() const
  {
    return 0.0;
  }

  int iterations() const
  {
    return 0;
  }
  void setMaxIterations( int ) {}

  /** \brief Get QR factorization.
   *  \warning It is up to the user to preserve consistency when modifyng it.
   */
  SuiteSparseQR_factorization<DofType>* getFactorization()
  {
    return spqrfactorization_;
  }

  /** \brief Get CCS matrix of the operator to solve.
   *  \warning It is up to the user to preserve consistency.
   */
  CCSMatrixType& getCCSMatrix()
  {
    return ccsmat_;
  }

  private:
  explicit SPQROp(const bool& verbose) :
    verbose_(verbose), ccsmat_(), cc_(new cholmod_common())
  {
    cholmod_l_start(cc_);
  }

  const OperatorType* op_ = nullptr;
  const bool verbose_;
  mutable CCSMatrixType ccsmat_;
  mutable bool isloaded_ = false;
  mutable cholmod_common* cc_;
  mutable cholmod_sparse* A_;
  mutable cholmod_dense* B_;
  mutable SuiteSparseQR_factorization<DofType>* spqrfactorization_;

  // \brief Computes the SPQR decomposition.
  void decompose() const
  {
    const std::size_t dimMat(ccsmat_.N());
    const std::size_t nnz(ccsmat_.getColStart()[dimMat]);
    // initialise the matrix A
    bool sorted(true);
    bool packed(true);
    bool real(std::is_same<DofType,double>::value);
    A_ = cholmod_l_allocate_sparse(dimMat, dimMat, nnz, sorted, packed, symmetric, real, cc_);
    // copy all the entries of Ap, Ai, Ax
    for(std::size_t k = 0; k != (dimMat+1); ++k)
      (static_cast<long int *>(A_->p))[k] = ccsmat_.getColStart()[k];
    for(std::size_t k = 0; k != nnz; ++k)
    {
      (static_cast<long int*>(A_->i))[k] = ccsmat_.getRowIndex()[k];
      (static_cast<DofType*>(A_->x))[k] = ccsmat_.getValues()[k];
    }
    // initialise the vector B
    B_ = cholmod_l_allocate_dense(dimMat, 1, dimMat, A_->xtype, cc_);
    // compute factorization of A
    spqrfactorization_=SuiteSparseQR_factorize<DofType>(SPQR_ORDERING_DEFAULT,SPQR_DEFAULT_TOL,A_,cc_);
  }
};

}
}

#endif // #if HAVE_SUITESPARSE_SPQR

#endif // #ifndef DUNE_FEM_SPQRSOLVER_HH
