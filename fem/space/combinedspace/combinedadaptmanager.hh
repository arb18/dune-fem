#ifndef DUNE_COMBINEDADAPTMANAGERIMP_HH
#define DUNE_COMBINEDADAPTMANAGERIMP_HH

//- local includes  
#include <dune/fem/space/common/adaptmanager.hh>
#include <dune/fem/quadrature/cachequad.hh>
#include <dune/fem/space/common/restrictprolonginterface.hh>

//- local includes 
#include "combinedspace.hh"

//----------------------------------------------------------------------
//-   1) Gewichte zwischen Vater/Sohn default Implementieren auf Gitter
//-      (father.weight(son) oder so
//-   2) Caching der Basisfunktionen fuer Vater-BasisFunktionen fuer
//-      Kinderquadraturen
//----------------------------------------------------------------------

namespace Dune{
/** @ingroup RestrictProlongImpl
    @{
**/

//***********************************************************************
/** \brief This is a restriction/prolongation operator for combined DG data. 
 */
template <class DiscreteFunctionImp, int polOrd> 
class RestrictProlongCombinedSpace
: public RestrictProlongInterface< 
  RestrictProlongTraits< RestrictProlongCombinedSpace<DiscreteFunctionImp,polOrd> 
                                                     > >
{
public:
  typedef DiscreteFunctionImp DiscreteFunctionType;
  typedef typename DiscreteFunctionType::FunctionSpaceType FunctionSpaceType;
  typedef typename FunctionSpaceType :: GridPartType GridPartType;
  typedef typename FunctionSpaceType :: GridType GridType;
  typedef typename DiscreteFunctionType::LocalFunctionType LocalFunctionType;

  typedef typename DiscreteFunctionType::RangeFieldType RangeFieldType;
  typedef typename DiscreteFunctionType::DomainType DomainType;
  typedef CachingQuadrature<GridPartType,0> QuadratureType;
  typedef typename GridType::template Codim<0>::Entity::Geometry Geometry;

  enum { dimRange = FunctionSpaceType :: DimRange };
public:  
  //! Constructor
  RestrictProlongCombinedSpace( DiscreteFunctionType & df ) : 
    df_ (df) , quadord_(2*df.space().order()),
    weight_(-1.0)
  {
    // make sure that index set is used that can handle adaptivity 
    assert( (Capabilities::IsUnstructured<GridType>::v) ? (df.space().indexSet().adaptive()) : true );
  }
  //! if weight is set, then ists assumend that we have always the same
  //! proportion between fahter and son volume 
  void setFatherChildWeight (const RangeFieldType& val) const
  {
    // volume of son / volume of father  
    weight_ = val; 
  }

  //! restrict data to father 
  template <class EntityType>
  void restrictLocal ( EntityType &father, EntityType &son, 
           bool initialize ) const
  {
    typename FunctionSpaceType::RangeType ret (0.0);
    typename FunctionSpaceType::ContainedRangeType phi (0.0);
    assert( !father.isLeaf() );
    const RangeFieldType weight = 
      (weight_ < 0.0) ? (this->calcWeight(father,son)) : weight_; 

    LocalFunctionType vati_ = df_.localFunction( father);
    LocalFunctionType sohn_ = df_.localFunction( son   );

    QuadratureType quad(son,quadord_);
    const typename FunctionSpaceType::BaseFunctionSetType & baseset =
      vati_.baseFunctionSet();
    const int nop=quad.nop();
    const Geometry& geometryInFather = son.geometryInFather();

    const int diff_numDofs = vati_.baseFunctionSet().numDifferentBaseFunctions();
    const int vati_numDofs = vati_.numDofs(); 
    if(initialize) 
    {
      for(int i=0; i<vati_numDofs ; ++i) 
      {
        vati_[i] = 0.0;
      }
    }
    
    for(int qP = 0; qP < nop; ++qP) 
    {
      sohn_.evaluate(quad,qP,ret);
      const double intel = quad.weight(qP) * weight;
      for(int i=0; i<diff_numDofs; ++i) 
      {
        baseset.evaluateScalar(i,geometryInFather.global(quad.point(qP)),phi);
        int idx = i * dimRange;
        for(int k=0; k<dimRange; ++k, ++idx)
        {
          vati_[idx] += intel * (ret[k] * phi[0]) ;
        }
      }
    }
  }

  //! prolong data to children 
  template <class EntityType>
  void prolongLocal ( EntityType &father, EntityType &son, bool initialize ) const
  {
    //assert( son.state() == REFINED );
    typename FunctionSpaceType::RangeType ret (0.0);
    typename FunctionSpaceType::ContainedRangeType phi (0.0);
    // get local functions 
    LocalFunctionType vati_ = df_.localFunction( father);
    LocalFunctionType sohn_ = df_.localFunction( son   );

    // get number of dofs 
    const int sohn_numDofs = sohn_.numDofs();
    const int diff_numDofs = sohn_.baseFunctionSet().numDifferentBaseFunctions();
    // set sohn to zero
    for(int i=0; i<sohn_numDofs; ++i) sohn_[i] = 0.;

    // get quadrature 
    QuadratureType quad(son,quadord_);
    const typename FunctionSpaceType::BaseFunctionSetType & baseset =
      sohn_.baseFunctionSet();
    const Geometry& geometryInFather = son.geometryInFather();
    const int nop=quad.nop();
    for(int qP = 0; qP < nop; ++qP) 
    {
      // evaluate father 
      vati_.evaluate(geometryInFather.global(quad.point(qP)),ret);
      // make projection 
      for(int i=0; i<diff_numDofs; ++i) 
      {
        baseset.evaluateScalar(i,quad,qP,phi);
        int idx = i * dimRange;
        for(int k=0; k<dimRange; ++k, ++idx)
        {
          sohn_[idx] += quad.weight(qP) * (ret[k] * phi[0]) ;
        }
      }
    }
  }

  //! add discrete function to communicator 
  template <class CommunicatorImp>
  void addToList(CommunicatorImp& comm)
  {
    comm.addToList(df_);
  }

private:
  mutable DiscreteFunctionType & df_;
  int quadord_;
  mutable RangeFieldType weight_;
};

/** \brief This is a restriction/prolongation operator for 
    combined DG data with polynomial order 0. 
 */
template <class DiscreteFunctionImp> 
class RestrictProlongCombinedSpace<DiscreteFunctionImp,0>
: public RestrictProlongPieceWiseConstantData<DiscreteFunctionImp> 
{
public:
  typedef DiscreteFunctionImp DiscreteFunctionType;
  typedef RestrictProlongPieceWiseConstantData<DiscreteFunctionImp>  BaseType; 
public:  
  //! Constructor
  RestrictProlongCombinedSpace( DiscreteFunctionType & df ) :
    BaseType(df)
  {}
};

/** \brief specialization of RestrictProlongDefault for
    CombinedSpace.
*/
template <template <class> class DiscFunc,
          class DiscreteFunctionSpaceImp, 
          int N, 
          DofStoragePolicy policy> 
class RestrictProlongDefault< 
        DiscFunc<CombinedSpace<DiscreteFunctionSpaceImp,N,policy> >
                            > 
: public RestrictProlongCombinedSpace<
        DiscFunc<CombinedSpace<DiscreteFunctionSpaceImp,N,policy> >,
        DiscreteFunctionSpaceImp :: polynomialOrder
                                     >
{
public:
  //! type of discrete function 
  typedef DiscFunc<CombinedSpace<DiscreteFunctionSpaceImp,N,policy> > DiscreteFunctionType;
  //! type of base class  
  typedef RestrictProlongCombinedSpace<
      DiscFunc<CombinedSpace<DiscreteFunctionSpaceImp,N,policy> >, 
        DiscreteFunctionSpaceImp :: polynomialOrder > BaseType;
public:  
  //! Constructor
  RestrictProlongDefault ( DiscreteFunctionType & df ) : 
    BaseType(df) 
  {
  }
};

///@}
} // end namespace Dune 
#endif
