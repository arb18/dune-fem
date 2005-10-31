#ifndef DUNE_DGPASS_HH
#define DUNE_DGPASS_HH

#include "pass.hh"
#include "selection.hh"
#include "problem.hh"
#include "problemcaller.hh"

//! \TODO Move the boundary stuff to a common base class for all discrete
//! operators
#include <dune/fem/common/boundary.hh>
#include <dune/common/fvector.hh>
#include <dune/grid/common/grid.hh>

namespace Dune {

  //! Concrete implementation of Pass for LDG.
  template <class ProblemImp, class PreviousPassImp>
  class LocalDGPass :
    public LocalPass<ProblemImp, PreviousPassImp> 
  {
  public:
    //- Typedefs and enums
    // Base class
    typedef LocalPass<ProblemImp, PreviousPassImp> BaseType;

    // Repetition of template arguments
    typedef ProblemImp ProblemType;
    typedef PreviousPassImp PreviousPassType;

    // Types from the base class
    typedef typename BaseType::Entity EntityType;
    typedef typename BaseType::ArgumentType ArgumentType;

    // Types from the traits
    typedef typename ProblemType::Traits::DestinationType DestinationType;
    typedef typename ProblemType::Traits::VolumeQuadratureType VolumeQuadratureType;
    typedef typename ProblemType::Traits::FaceQuadratureType FaceQuadratureType;
    typedef typename ProblemType::Traits::SpaceType SpaceType;

    // Types extracted from the discrete function space type
    typedef typename SpaceType::GridType GridType;
    typedef typename SpaceType::DomainType DomainType;
    typedef typename SpaceType::RangeType RangeType;
    typedef typename SpaceType::JacobianRangeType JacobianRangeType;

    // Types extracted from the underlying grids
    typedef typename GridType::Traits::IntersectionIterator IntersectionIterator;
    typedef typename GridType::template Codim<0>::Geometry Geometry;


    // Various other types
    typedef typename DestinationType::LocalFunctionType LocalFunctionType;
    typedef typename ProblemType::SelectorType SelectorType;
    typedef ProblemCaller<
      ProblemType, ArgumentType, SelectorType> ProblemCallerType;
    typedef BoundaryManager<SpaceType> BoundaryManagerType;
    typedef typename BoundaryManagerType::BoundaryType BoundaryType;

    // Range of the destination
    enum { dimRange = SpaceType::DimRange };
  public:
    //- Public methods
    //! Constructor
    //! \param problem Actual problem definition (see problem.hh)
    //! \param pass Previous pass
    //! \param spc Space belonging to the discrete function local to this pass
    //! \param bc Container for boundary conditions for this operator
    LocalDGPass(ProblemType& problem, 
                PreviousPassType& pass, 
                SpaceType& spc,
                BoundaryManagerType& bc) :
      BaseType(pass, spc),
      problem_(problem),
      caller_(0),
      arg_(0),
      dest_(0),
      spc_(spc),
      boundaryManager_(bc),
      dtMin_(std::numeric_limits<double>::max()),
      dtOld_(std::numeric_limits<double>::max()),
      fMat_(0.0),
      valEn_(0.0),
      valNeigh_(0.0),
      tmp_(0.0),
      grads_(0.0),
      diffVar_()
    {}
   
    virtual ~LocalDGPass() {
      //delete caller_;
    }

  private:
    //! In the preparations, store pointers to the actual arguments and 
    //! destinations. Filter out the "right" arguments for this pass.
    virtual void prepare(const ArgumentType& arg, DestinationType& dest) const
    {
      arg_ = const_cast<ArgumentType*>(&arg);
      dest_ = &dest;

      if (caller_) {
        caller_->setArgument(*arg_);
      }
      else {
        // * Move this initialisation garbage into the ProblemCaller
        caller_ = new ProblemCallerType(problem_, *arg_);
      }

      // dt initialisation
      dtMin_ = std::numeric_limits<double>::max();
    }

    //! Some timestep size management.
    virtual void finalize(const ArgumentType& arg, DestinationType& dest) const
    {
      dtOld_ = dtMin_;
    }

    //! Perform the (volume and surface) integration on all elements.
    virtual void applyLocal(EntityType& en) const
    {
      std::cout << "DGLocalPass::applyLocal()" << std::endl;
    
      //- Initialise quadratures and stuff
      caller_->setEntity(en);
      LocalFunctionType updEn = dest_->localFunction(en);
      GeometryType geom = en.geometry().type();
      
      VolumeQuadratureType volQuad(geom);

      double vol = volumeElement(en, volQuad);
      double dtLocal;

      const typename SpaceType::IndexSetType& iset = spc_.indexSet();

      // Volumetric integral part
      for (int l = 0; l < volQuad.nop(); ++l) {
        JacobianRangeType flux(0.0);
        caller_->analyticalFlux(en, volQuad, l, flux);

        RangeType source(0.0);
        caller_->source(en, volQuad, l, source);

        for (int k = 0; k < spc_.getBaseFunctionSet(en).numBaseFunctions(); 
             ++k) {
          JacobianRangeType gradients(0.0);
          RangeType values(0.0);
          spc_.getBaseFunctionSet(en).jacobian(k, volQuad, l, gradients);
          spc_.getBaseFunctionSet(en).eval(k, volQuad, l, values);


          // Update vector
          for (int i = 0; i < dimRange; ++i) {
            grads_ = 0.0;
            en.geometry().jacobianInverseTransposed(volQuad.point(l)).
              umv(gradients[i], grads_);
       
            double update =
              (flux[i]*grads_+source[i]*values[i])*volQuad.weight(l)*
              en.geometry().integrationElement(volQuad.point(l));

            // * is (i + k*dimRange) right?
            updEn[i + k*dimRange] += update;
          } // end for i (dimRange)
        } // end for k (baseFunctions)
      } // end for l (quadraturePoints)

      // Surface integral part
      IntersectionIterator endnit = en.iend();
      IntersectionIterator nit = en.ibegin();
      FaceQuadratureType faceQuad(nit.intersectionGlobal().type());
      
      for (; nit != endnit; ++nit) {
        if (nit.neighbor()) {
          if (iset.index(*nit.outside()) > iset.index(en)
              || nit.outside()->partitionType() == GhostEntity) {
            
            caller_->setNeighbor(*nit.outside());
            LocalFunctionType updNeigh = 
              dest_->localFunction(*(nit.outside()));

            for (int l = 0; l < faceQuad.nop(); ++l) {
              double h = 
                nit.intersectionGlobal().integrationElement(faceQuad.point(l));
              // * might be improved by using quadrature points directly
              // * (how to deal with quadrature points on neighbor?)
              DomainType xGlobal =
                nit.intersectionGlobal().global(faceQuad.point(l));
              DomainType xLocalEn = 
                nit.intersectionSelfLocal().global(faceQuad.point(l));
              DomainType xLocalNeigh = 
                nit.intersectionNeighborLocal().global(faceQuad.point(l));
              
              // Evaluate flux
              dtLocal = 
                caller_->numericalFlux(nit, faceQuad, l, valEn_, valNeigh_);

              dtLocal =  (dtLocal < std::numeric_limits<double>::min()) ?
                dtMin_ : vol/(dtLocal*h);
              if(dtLocal < dtMin_) dtMin_ = dtLocal;
              
              // * Assumption: all elements have same number of base functions
              for (int k = 0; 
                   k < spc_.getBaseFunctionSet(en).numBaseFunctions(); ++k) {
                RangeType baseEn;
                spc_.getBaseFunctionSet(en).evaluate(k,
                                                     diffVar_,
                                                     xLocalEn,
                                                     baseEn);

                RangeType baseNeigh;
                spc_.getBaseFunctionSet(en).evaluate(k,
                                                     diffVar_,
                                                     xLocalNeigh,
                                                     baseNeigh);

                for (int i = 0; i < dimRange; ++i) {
                  updEn[i + dimRange*k] -=
                    tmp_[i]*baseEn[0]*faceQuad.weight(l)*h;

                  updNeigh[i + dimRange*k] += 
                    tmp_[i]*baseNeigh[0]*faceQuad.weight(l)*h;

                } // end loop dimRange
              } // end loop base functions
            } // end loop quadrature points
            
          } // end if outside etc.
        } // end if neighbor

        if (nit.boundary()) {
          const BoundaryType& bc = 
            boundaryManager_.getBoundaryCondition(nit.boundaryId());

          if (bc.boundaryType() == BoundaryType::Dirichlet) {
            for (int l = 0; l < faceQuad.nop(); ++l) {
              double integrationElement =
                nit.intersectionGlobal().integrationElement(faceQuad.point(l));
              
              DomainType xLocalEn = 
                nit.intersectionSelfLocal().global(faceQuad.point(l));
              DomainType xGlobal =
                nit.intersectionGlobal().global(faceQuad.point(l));

              DomainType n = nit.integrationOuterNormal(faceQuad.point(l));

              RangeType boundaryValue;
              bc.evaluate(valEn_, xGlobal, n, boundaryValue);
     
              // * How to deal with boundary?
              //dtLocal = 
              //  caller_->boundaryFlux(nit, faceQuad, l, boundaryValue, valEn_);
              dtLocal = (dtLocal < std::numeric_limits<double>::min()) ?
                dtMin_ : vol/(dtLocal*integrationElement);
              if (dtLocal < dtMin_) dtMin_ = dtLocal;
                    
              for (int k = 0; 
                   k < spc_.getBaseFunctionSet(en).numBaseFunctions(); ++k) {
                RangeType baseEn;
                spc_.getBaseFunctionSet(en).evaluate(k,
                                                     diffVar_,
                                                     xLocalEn,
                                                     baseEn);
                for (int i = 0; i < dimRange; ++i) {
                  updEn[i + dimRange*k] -=
                    tmp_[i]*faceQuad.weight(l)*baseEn[0]*
                    integrationElement;
                }
              }
            }
          }
          else if (bc.boundaryType() == BoundaryType::Neumann) {
            
            //assert(false); // Not the way to treat Neumann, I suppose...
            
            for (int l = 0; l < faceQuad.nop(); ++l) {
              double integrationElement =
                nit.intersectionGlobal().integrationElement(faceQuad.point(l));
              
              DomainType xLocalEn =
                nit.intersectionSelfLocal().global(faceQuad.point(l));
              DomainType xGlobal =
                nit.intersectionGlobal().global(faceQuad.point(l));
              
              DomainType n = nit.integrationOuterNormal(faceQuad.point(l));

              RangeType boundaryValue;
              bc.evaluate(valEn_, xGlobal, n, tmp_);
                           
              for (int k = 0; 
                   k < spc_.getBaseFunctionSet(en).numBaseFunctions(); ++k) {
                RangeType baseEn(0.0);
                spc_.getBaseFunctionSet(en).eval(k, xLocalEn, baseEn);

                for (int i = 0; i < dimRange; ++i) {
                  updEn[i + dimRange*k] -=
                    tmp_[i]*faceQuad.weight(l)*baseEn[0]*
                    integrationElement;
                }
              }
            }
          } else {
            assert(false); // I don't know how to handle anything else
          }
          
        } // end if boundary
      }
     
    }
    
  private:
    LocalDGPass();
    LocalDGPass(const LocalDGPass&);
    LocalDGPass& operator=(const LocalDGPass&);

  private:
    double volumeElement(const EntityType& en,
                         const VolumeQuadratureType& quad) const {
      double result = 0.0;
      for (int qp = 0; qp < quad.nop(); ++qp) {
        result += 
          quad.weight(qp) * en.geometry().integrationElement(quad.point(qp));
      }
      return result;
    }
    
  private:
    ProblemType& problem_;
    mutable ProblemCallerType* caller_;
    
    mutable ArgumentType* arg_;
    mutable DestinationType* dest_;

    SpaceType& spc_;
    const BoundaryManagerType& boundaryManager_;
    mutable double dtMin_;
    mutable double dtOld_;

    //! Some helper variables
    mutable JacobianRangeType fMat_;
    mutable RangeType valEn_;
    mutable RangeType valNeigh_;
    mutable RangeType tmp_;
    mutable DomainType grads_;
    FieldVector<int, 0> diffVar_;
  };
  
} // end namespace Dune

#endif
