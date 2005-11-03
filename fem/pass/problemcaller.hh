#ifndef DUNE_PROBLEMCALLER_HH
#define DUNE_PROBLEMCALLER_HH

#include <utility>

#include <dune/common/fvector.hh> 

#include "caller.hh"
#include "problem.hh"

namespace Dune {

  /**
   * @brief Wrapper class for all the template magic used to call the problem
   * methods.
   */
  template <class ProblemImp, class ArgumentImp, class SelectorImp>
  class ProblemCaller {
  public:
    typedef ProblemImp ProblemType;
    typedef ArgumentImp TotalArgumentType;
    typedef SelectorImp SelectorType;

    typedef typename ProblemType::Traits Traits;
    typedef typename Traits::DomainType DomainType;
    typedef typename Traits::RangeType RangeType;
    typedef typename Traits::JacobianRangeType JacobianRangeType;
    typedef typename Traits::GridType GridType;
    typedef typename GridType::Traits::IntersectionIterator IntersectionIterator;
    typedef typename GridType::template Codim<0>::Entity Entity;

    typedef typename Traits::FaceQuadratureType FaceQuadratureType;
    typedef typename Traits::VolumeQuadratureType VolumeQuadratureType;

    typedef Filter<TotalArgumentType, SelectorType> FilterType;
    typedef typename FilterType::ResultType DiscreteFunctionTupleType;
    typedef typename LocalFunctionCreator<
      DiscreteFunctionTupleType>::ResultType LocalFunctionTupleType;
    typedef typename Creator<
      RangeTypeEvaluator,
      LocalFunctionTupleType>::ResultType RangeTupleType;
    typedef typename Creator<
      JacobianRangeTypeEvaluator,
      LocalFunctionTupleType>::ResultType JacobianRangeTupleType;
    //typedef typename Caller<RangeTupleType> CallerType;

    typedef LocalFunctionCreator<DiscreteFunctionTupleType> LFCreator;
    typedef Creator<
      RangeTypeEvaluator, LocalFunctionTupleType> RangeCreator;
    typedef Creator<
      JacobianRangeTypeEvaluator, LocalFunctionTupleType> JacobianCreator;

    typedef std::pair<LocalFunctionTupleType, RangeTupleType> ValuePair;
  public:
    ProblemCaller(ProblemType& problem, TotalArgumentType& arg) :
      problem_(problem),
      arg_(&arg),
      discreteFunctions_(FilterType::apply(arg)),
      valuesEn_(LFCreator::apply(discreteFunctions_), RangeCreator::apply()),
      valuesNeigh_(LFCreator::apply(discreteFunctions_),RangeCreator::apply()),
      jacobians_(JacobianCreator::apply())
    {}

    void setArgument(TotalArgumentType& arg) 
    {
      if (*arg_ != arg) {

        // Set pointer
        arg_ = &arg;
      
        // Filter the argument
        discreteFunctions_ = FilterType::apply(arg);

        // Build up new local function tuples
        valuesEn_.first = LFCreator::apply(discreteFunctions_);
        valuesNeigh_.first = LFCreator::apply(discreteFunctions_);
      }
    }

    void setEntity(Entity& en) 
    {
      setter(en, valuesEn_.first);
    }

    void setNeighbor(Entity& en) 
    {
      setter(en, valuesNeigh_.first);
    }

    // Here, the interface of the problem is replicated and the Caller
    // is used to do the actual work
    void analyticalFlux(Entity& en, const DomainType& x, 
                        JacobianRangeType& res) 
    {
      evaluateLocal(en, x, valuesEn_);
      problem_.analyticalFlux(en, 0.0, x, valuesEn_.second, res);
      //CallerType::analyticalFlux(problem_, en, x, valuesEn_.second, res);
    }
    
    void analyticalFlux(Entity& en, VolumeQuadratureType& quad, int quadPoint,
                        JacobianRangeType& res) 
    {
      // * temporary
      ForEachValuePair<
        LocalFunctionTupleType, RangeTupleType> forEach(valuesEn_.first,
                                                        valuesEn_.second);
      LocalFunctionEvaluateQuad<
        Entity, VolumeQuadratureType> eval(en, 
                                           quad, 
                                           quadPoint);
      forEach.apply(eval);


      //evaluateQuad(en, quad, quadPoint, valuesEn_);
      problem_.analyticalFlux(en, 0.0, quad.point(quadPoint), 
                              valuesEn_.second, res);
      //CallerType::analyticalFlux(problem_, en, quad, quadPoint, valuesEn_.second, res);
    }

    template <class FaceDomainType>
    double numericalFlux(IntersectionIterator& nit, const FaceDomainType& x,
                         RangeType& resEn, RangeType& resNeigh) 
    {
      typedef typename IntersectionIterator::LocalGeometry Geometry;

      const Geometry& selfLocal = nit.intersectionSelfLocal();
      const Geometry& neighLocal = nit.intersectionNeighborLocal();
      evaluateLocal(*nit.inside(), selfLocal.global(x),
                    valuesEn_);
      evaluateLocal(*nit.outside(), neighLocal.global(x),
                    valuesNeigh_);
      return problem_.numericalFlux(nit, 0.0, x,
                                    valuesEn_.second, valuesNeigh_.second,
                                    resEn, resNeigh);
      //CallerType::numericalFlux(problem_, nit, x, 
      //                      valuesEn_.second, valuesNeigh_.second,
      //                      res_en, res_neigh);
    }

    // Ensure: entities set correctly before call
    double numericalFlux(IntersectionIterator& nit,
                         FaceQuadratureType& quad, int quadPoint,
                         RangeType& resEn, RangeType& resNeigh)
    {
      typedef typename IntersectionIterator::LocalGeometry Geometry;
    
      const Geometry& selfLocal = nit.intersectionSelfLocal();
      const Geometry& neighLocal = nit.intersectionNeighborLocal();
      evaluateLocal(*nit.inside(), selfLocal.global(quad.point(quadPoint)),
                    valuesEn_);
      evaluateLocal(*nit.outside(), neighLocal.global(quad.point(quadPoint)),
                    valuesNeigh_);
      return problem_.numericalFlux(nit, 0.0, 
                                    quad.point(quadPoint),
                                    valuesEn_.second, valuesNeigh_.second,
                                    resEn, resNeigh);
      //CallerType::numericalFlux(problem_, nit, quad, quadPoint,
      //                      valuesEn_.second, valuesNeigh_.second,
      //                      res_en, res_neigh);
    }


    template <class IntersectionIterator, class FaceDomainType>
    double boundaryFlux(IntersectionIterator& nit,
                        const FaceDomainType& x,
                        RangeType& boundaryFlux) 
    {
      typedef typename IntersectionIterator::LocalGeometry Geometry;
      const Geometry& selfLocal = nit.intersectionSelfLocal();
      evaluateLocal(*nit.inside(), selfLocal.global(x),
                    valuesEn_);
      return problem_.boundaryFlux(nit, 0.0, x,
                                   valuesEn_.second,
                                   boundaryFlux);
    }

    template <class IntersectionIterator>
    double boundaryFlux(IntersectionIterator& nit,
                        FaceQuadratureType& quad,
                        int quadPoint,
                        RangeType& boundaryFlux) 
    {
      typedef typename IntersectionIterator::LocalGeometry Geometry;

      const Geometry& selfLocal = nit.intersectionSelfLocal();
      evaluateLocal(*nit.inside(), selfLocal.global(quad.point(quadPoint)),
                    valuesEn_);
      return problem_.boundaryFlux(nit, 0.0, quad.point(quadPoint),
                                   valuesEn_.second,
                                   boundaryFlux);
    }

    void source(Entity& en, const DomainType& x, RangeType& res) 
    {
      evaluateLocal(en, x, valuesEn_);
      evaluateJacobianLocal(en, x);
      problem_.source(en, 0.0, x, valuesEn_.second, jacobians_, res);
      //CallerType::source(problem_, en, x, valuesEn_.second, jacobians_, res);
    }

    void source(Entity& en, VolumeQuadratureType& quad, int quadPoint, 
                RangeType& res) 
    {
      evaluateQuad(en, quad, quadPoint, valuesEn_);
      evaluateJacobianQuad(en, quad, quadPoint);
      problem_.source(en, 0.0, quad.point(quadPoint), valuesEn_.second,
                      jacobians_, res);
      //CallerType::source(problem_, en, quad, quadPoint,
      //valuesEn_.second, jacobians_, res);
    }

  private:
    void setter(Entity& en, LocalFunctionTupleType& tuple) 
    {
      ForEachValuePair<DiscreteFunctionTupleType, 
        LocalFunctionTupleType> forEach(discreteFunctions_, tuple);
      LocalFunctionSetter<Entity> setter(en);
      forEach.apply(setter);
    }

    void evaluateLocal(Entity& en, const DomainType& x, ValuePair& p) 
    {
      ForEachValuePair<
        LocalFunctionTupleType, RangeTupleType> forEach(p.first,
                                                        p.second);
      
      LocalFunctionEvaluateLocal<Entity, DomainType> eval(en, x);
      forEach.apply(eval);
    }

    void evaluateQuad(Entity& en, VolumeQuadratureType& quad, int quadPoint, 
                      ValuePair& p) 
    {
      ForEachValuePair<
        LocalFunctionTupleType, RangeTupleType> forEach(p.first,
                                                        p.second);
      LocalFunctionEvaluateQuad<
        Entity, VolumeQuadratureType> eval(en, quad, quadPoint);
      
      forEach.apply(eval);
    }

    void evaluateJacobianLocal(Entity& en, const DomainType& x)
    {
      ForEachValuePair<
        LocalFunctionTupleType,JacobianRangeTupleType> forEach(valuesEn_.first,
                                                               jacobians_);
      
      LocalFunctionEvaluateJacobianLocal<Entity, DomainType> eval(en, x);
      forEach.apply(eval);
    }

    void evaluateJacobianQuad(Entity& en, VolumeQuadratureType& quad, 
                              int quadPoint) 
    {
      ForEachValuePair<
        LocalFunctionTupleType,JacobianRangeTupleType> forEach(valuesEn_.first,
                                                               jacobians_);
      LocalFunctionEvaluateJacobianQuad<
        Entity, VolumeQuadratureType> eval(en, quad, quadPoint);
      
      forEach.apply(eval);
    }

  private:
    ProblemCaller(const ProblemCaller&);
    ProblemCaller& operator=(const ProblemCaller&);

  private:
    ProblemType& problem_;
    TotalArgumentType* arg_;

    DiscreteFunctionTupleType discreteFunctions_;
    ValuePair valuesEn_;
    ValuePair valuesNeigh_;
    JacobianRangeTupleType jacobians_;
  };

}

#endif
