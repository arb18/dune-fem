#ifndef DUNE_FEM_DGPASSSTUB_HH
#define DUNE_FEM_DGPASSSTUB_HH

#include <string>

#include <dune/fem/space/lagrange.hh>
#include <dune/fem/gridpart/leafgridpart.hh>
#include <dune/fem/function/adaptivefunction.hh>
#include <dune/fem/quadrature/elementquadrature.hh>

#include <dune/fem/pass/dgpass.hh>
#include <dune/fem/pass/dgdiscretemodel.hh>
#include <dune/fem/pass/selection.hh>

namespace Dune
{

  namespace Fem
  {
  
    class DiscreteModelStub;

    struct DGStubTraits
    {
      enum { dim = GridSelector::GridType::dimension };
      typedef LeafGridPart< GridSelector::GridType > GridPartType;
      typedef FunctionSpace<double, double, dim , 1> FunctionSpaceType;
      typedef LagrangeDiscreteFunctionSpace<
        FunctionSpaceType, GridPartType, 1> DiscreteFunctionSpaceType;
      typedef DiscreteFunctionSpaceType SpaceType;
      typedef AdaptiveDiscreteFunction<DiscreteFunctionSpaceType> DestinationType;
      typedef ElementQuadrature< GridPartType, 0> VolumeQuadratureType;
      typedef ElementQuadrature< GridPartType, 1>  FaceQuadratureType;
      typedef FieldVector<double, dim > DomainType;
      typedef FieldVector<double, dim > FaceDomainType;
      typedef DiscreteFunctionSpaceType::RangeType RangeType;
      typedef DiscreteFunctionSpaceType::JacobianRangeType JacobianRangeType;

      typedef DiscreteModelStub DGDiscreteModelType;
    };
    
    class DiscreteModelStub
    : public Fem::DGDiscreteModelDefault< DGStubTraits >
    {
      typedef Fem::DGDiscreteModelDefault< DGStubTraits > BaseType;

    public:
      typedef Selector< 0 >::Base SelectorType;

      typedef DGStubTraits Traits;

      typedef Traits::FunctionSpaceType FunctionSpaceType;
      typedef Traits::GridPartType GridPartType;
      typedef GridPartType::GridType GridType;
      typedef Traits::DiscreteFunctionSpaceType DiscreteFunctionSpaceType;
      // * find common notation
      typedef Traits::SpaceType SpaceType;
      typedef Traits::DestinationType DestinationType;
      typedef Traits::VolumeQuadratureType VolumeQuadratureType;
      typedef Traits::FaceQuadratureType FaceQuadratureType;
      typedef Traits::DiscreteFunctionSpaceType::RangeType RangeType;
      typedef Traits::DiscreteFunctionSpaceType::JacobianRangeType JacobianRangeType;
      typedef GridPartType :: IntersectionIteratorType IntersectionIterator;
      typedef IntersectionIterator :: Intersection  Intersection;
      typedef GridType::Codim<0>::Entity EntityType;

      typedef BaseType :: MassFactorType MassFactorType; 
   
      typedef FieldVector<double, 3> DomainType;
    
    public:
      bool hasFlux() const
      {
        return true;
      }

      bool hasSource() const
      {
        return true;
      }

      bool hasMass () const
      {
        return true;
      }

      template <class ArgumentTuple, class FaceDomainType>
      double numericalFlux(const Intersection& it,
                           double time, const FaceDomainType& x,
                           const ArgumentTuple& uLeft, 
                           const ArgumentTuple& uRight,
                           RangeType& gLeft,
                           RangeType& gRight)
      { 
        std::cout << "numericalFlux()" << std::endl; 
        return 0.0;
      }

      template <class ArgumentTuple, class FaceDomainType>
      double boundaryFlux(const Intersection& it,
                          double time, const FaceDomainType& x,
                          const ArgumentTuple& uLeft,
                          RangeType& boundaryFlux)
      {
        std::cout << "boundaryFlux()" << std::endl;
        return 0.0;
      }

      template <class ArgumentTuple>
      void analyticalFlux(EntityType& en,
                          double time, const DomainType& x,
                          const ArgumentTuple& u, JacobianRangeType& f) 
      {
        std::cout << "analyticalFlux()" << std::endl;
      }

      template <class ArgumentTuple, class JacobianTuple>
      double source(const EntityType& en, 
                    const double time, 
                    const DomainType& x,
                    const ArgumentTuple& u, 
                    const JacobianTuple& jac, 
                    RangeType& s)
      {
        std::cout << "S()" << std::endl;
        return 0.0;
      }

      template <class ArgumentTuple>
      void mass(const EntityType& en,
                const double time,
                const DomainType& x,
                const ArgumentTuple& u,
                MassFactorType& m)
      {
        std::cout << "M()" << std::endl;
      }
    };

  } // namespace Fem

} // namespace Dune

#endif // #ifndef DUNE_FEM_DGPASSSTUB_HH
