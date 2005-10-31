#ifndef DUNE_DGPASSSTUB_HH
#define DUNE_DGPASSSTUB_HH

#include <string>

#include "../dgpass.hh"
#include "../problem.hh"
#include "../selection.hh"

#include <dune/fem/lagrangebase.hh>
#include <dune/grid/common/gridpart.hh>
#include <dune/grid/alu3dgrid.hh>
#include <dune/fem/dfadapt.hh>
#include <dune/fem/discretefunction/adaptivefunction.hh>
#include <dune/quadrature/fixedorder.hh>

namespace Dune {

 struct DGStubTraits {
   typedef FunctionSpace<double, double, 3, 1> FunctionSpaceType;
   typedef ALU3dGrid<3, 3, tetra> GridType;
   typedef LeafGridPart<GridType> GridPartType;
   typedef LagrangeDiscreteFunctionSpace<
     FunctionSpaceType, GridPartType, 1> DiscreteFunctionSpaceType;
   typedef DiscreteFunctionSpaceType SpaceType;
   //typedef DFAdapt<DiscreteFunctionSpaceType> DestinationType;
   typedef AdaptiveDiscreteFunction<DiscreteFunctionSpaceType> DestinationType;
   typedef FixedOrderQuad<double, FieldVector<double, 3>, 1> VolumeQuadratureType;
   typedef FixedOrderQuad<double, FieldVector<double, 2>, 1> FaceQuadratureType;
  };
  
  class ProblemStub : 
    public ProblemDefault<ProblemStub, DGStubTraits::FunctionSpaceType>
  {
  public:
    typedef Selector<0>::Base SelectorType;

    typedef DGStubTraits Traits;

    typedef Traits::FunctionSpaceType FunctionSpaceType;
    typedef Traits::GridType GridType;
    typedef Traits::GridPartType GridPartType;
    typedef Traits::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;
    // * find common notation
    typedef Traits::SpaceType SpaceType;
    typedef Traits::DestinationType DestinationType;
    typedef Traits::VolumeQuadratureType VolumeQuadratureType;
    typedef Traits::FaceQuadratureType FaceQuadratureType;

    typedef FieldVector<double, 3> DomainType;
    typedef FieldVector<double, 2> FaceDomainType;
  public:
    bool hasFlux() const { return true; }
    bool hasSource() const { return true; }

    template <
      class IntersectionIterator, class ArgumentTuple, class ResultType>
    double numericalFlux(IntersectionIterator& it,
                         double time, const FaceDomainType& x,
                         const ArgumentTuple& uLeft, 
                         const ArgumentTuple& uRight,
                         ResultType& gLeft,
                         ResultType& gRight)
    { 
      std::cout << "f()" << std::endl; 
      return 0.0;
    }

    template <class Entity, class ArgumentTuple, class ResultType>
    void analyticalFlux(Entity& en,
                        double time, const DomainType& x,
                        const ArgumentTuple& u, ResultType& f) 
    { std::cout << "g()" << std::endl; }

    template <class Entity, class ArgumentTuple, class JacobianTuple, class ResultType>
    void source(Entity& en, 
                double time, const DomainType& x,
                const ArgumentTuple& u, 
                const JacobianTuple& jac, 
                ResultType& s)
    { std::cout << "S()" << std::endl; }
  };

} // end namespace Dune

#endif
