#ifndef DUNE_FEM_LDLSOLVER_HH
#define DUNE_FEM_LDLSOLVER_HH

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <dune/common/exceptions.hh>
#include <dune/common/hybridutilities.hh>
#include <dune/common/std/utility.hh>
#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/function/blockvectorfunction.hh>
#include <dune/fem/function/tuplediscretefunction.hh>
#include <dune/fem/io/parameter.hh>
#include <dune/fem/operator/common/operator.hh>
#include <dune/fem/operator/matrix/colcompspmatrix.hh>

#if HAVE_SUITESPARSE_LDL

#ifdef __cplusplus
extern "C"
{
#include "ldl.h"
#include "amd.h"
}
#endif

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

/** \class LDLOp
 *  \ingroup DirectSolver
 *  \brief The %LDL direct sparse solver
 *   Details on %LDL can be found on
 *   http://www.cise.ufl.edu/research/sparse/ldl/
 *  \note This will only work if dune-fem has been configured to use LDL
 */
template<class DF, class Op, bool symmetric=false>
class LDLOp:public Operator<DF, DF>
{
  public:
  typedef DF DiscreteFunctionType;
  typedef Op OperatorType;

  // \brief The column-compressed matrix type.
  typedef ColCompMatrix<typename OperatorType::MatrixType::MatrixBaseType> CCSMatrixType;
  typedef typename DiscreteFunctionType::DofType DofType;
  typedef typename DiscreteFunctionType::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;

  /** \brief Constructor.
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   *  \param[in] verbose verbosity
   */
  LDLOp(const double& redEps, const double& absLimit, const int& maxIter, const bool& verbose,
        const ParameterReader &parameter = Parameter::container() ) :
    verbose_(verbose), ccsmat_()
  {}

  /** \brief Constructor.
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   */
  LDLOp(const double& redEps, const double& absLimit, const int& maxIter,
        const ParameterReader &parameter = Parameter::container() ) :
    verbose_(parameter.getValue<bool>("fem.solver.verbose",false)), ccsmat_()
  {}

  LDLOp(const ParameterReader &parameter = Parameter::container() ) :
    verbose_(parameter.getValue<bool>("fem.solver.verbose",false)), ccsmat_()
  {}

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   *  \param[in] verbose verbosity
   */
  LDLOp(const OperatorType& op, const double& redEps, const double& absLimit, const int& maxIter, const bool& verbose,
        const ParameterReader &parameter = Parameter::container() ) :
    verbose_(verbose), ccsmat_(), isloaded_(false)
  {
    bind(op);
  }

  /** \brief Constructor.
   *  \param[in] op Operator to invert
   *  \param[in] redEps relative tolerance for residual (not used here)
   *  \param[in] absLimit absolut solving tolerance for residual (not used here)
   *  \param[in] maxIter maximal number of iterations performed (not used here)
   */
  LDLOp(const OperatorType& op, const double& redEps, const double& absLimit, const int& maxIter,
        const ParameterReader &parameter = Parameter::container() ) :
    verbose_(parameter.getValue<bool>("fem.solver.verbose",false)), ccsmat_(), isloaded_(false)
  {
    bind(op);
  }

  LDLOp(const OperatorType& op, const ParameterReader &parameter = Parameter::container() ) :
    verbose_(parameter.getValue<bool>("fem.solver.verbose",false)), ccsmat_(), isloaded_(false)
  {
    bind(op);
  }

  // \brief Destructor.
  ~LDLOp()
  {
    finalize();
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
      delete [] D_;
      delete [] Y_;
      delete [] Lp_;
      delete [] Lx_;
      delete [] Li_;
      delete [] P_;
      delete [] Pinv_;
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
    ldl_perm(dimMat, Y_, const_cast<DofType*>(arg), P_);
    ldl_lsolve(dimMat, Y_, Lp_, Li_, Lx_);
    ldl_dsolve(dimMat, Y_, D_);
    ldl_ltsolve(dimMat, Y_, Lp_, Li_, Lx_);
    ldl_permt(dimMat, dest, Y_, P_);
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
    out<<"Solver: LDL direct solver"<<std::endl;
  }

  // \brief Print some statistics about the LDL decomposition.
  void printDecompositionInfo() const
  {
    amd_info(info_);
  }

  double averageCommTime() const
  {
    return 0.0;
  }

  int iterations() const
  {
    return 0;
  }
  void setMaxIterations ( int ) {}

  /** \brief Get factorization diagonal matrix D.
   *  \warning It is up to the user to preserve consistency.
   */
  DofType* getD()
  {
    return D_;
  }

  /** \brief Get factorization Lp.
   *  \warning It is up to the user to preserve consistency.
   */
  int* getLp()
  {
    return Lp_;
  }

  /** \brief Get factorization Li.
   *  \warning It is up to the user to preserve consistency.
   */
  int* getLi()
  {
    return Li_;
  }

  /** \brief Get factorization Lx.
   *  \warning It is up to the user to preserve consistency.
   */
  DofType* getLx()
  {
    return Lx_;
  }

  /** \brief Get CCS matrix of the operator to solve.
   *  \warning It is up to the user to preserve consistency.
   */
  CCSMatrixType& getCCSMatrix()
  {
    return ccsmat_;
  }

  private:
  const OperatorType* op_;
  const bool verbose_;
  mutable CCSMatrixType ccsmat_;
  mutable bool isloaded_ = false;
  mutable int* Lp_;
  mutable int* Parent_;
  mutable int* Lnz_;
  mutable int* Flag_;
  mutable int* Pattern_;
  mutable int* P_;
  mutable int* Pinv_;
  mutable DofType* D_;
  mutable DofType* Y_;
  mutable DofType* Lx_;
  mutable int* Li_;
  mutable double info_[AMD_INFO];

  // \brief Computes the LDL decomposition.
  void decompose() const
  {
    // allocate vectors
    const std::size_t dimMat(ccsmat_.N());
    D_ = new DofType [dimMat];
    Y_ = new DofType [dimMat];
    Lp_ = new int [dimMat + 1];
    Parent_ = new int [dimMat];
    Lnz_ = new int [dimMat];
    Flag_ = new int [dimMat];
    Pattern_ = new int [dimMat];
    P_ = new int [dimMat];
    Pinv_ = new int [dimMat];

    if(amd_order (dimMat, ccsmat_.getColStart(), ccsmat_.getRowIndex(), P_, (DofType *) NULL, info_) < AMD_OK)
      DUNE_THROW(InvalidStateException,"LDL Error: AMD failed!");
    if(verbose_)
      printDecompositionInfo();
    // compute the symbolic factorisation
    ldl_symbolic(dimMat, ccsmat_.getColStart(), ccsmat_.getRowIndex(), Lp_, Parent_, Lnz_, Flag_, P_, Pinv_);
    // initialise those entries of additionalVectors_ whose dimension is known only now
    Lx_ = new DofType [Lp_[dimMat]];
    Li_ = new int [Lp_[dimMat]];
    // compute the numeric factorisation
    const std::size_t k(ldl_numeric(dimMat, ccsmat_.getColStart(), ccsmat_.getRowIndex(), ccsmat_.getValues(),
                                    Lp_, Parent_, Lnz_, Li_, Lx_, D_, Y_, Pattern_, Flag_, P_, Pinv_));
    // free temporary vectors
    delete [] Flag_;
    delete [] Pattern_;
    delete [] Parent_;
    delete [] Lnz_;

    if(k!=dimMat)
    {
      std::cerr<<"LDL Error: D("<<k<<","<<k<<") is zero!"<<std::endl;
      DUNE_THROW(InvalidStateException,"LDL Error: factorisation failed!");
    }
  }
};

}
}

#endif // #if HAVE_SUITESPARSE_LDL

#endif // #ifndef DUNE_FEM_LDLSOLVER_HH
