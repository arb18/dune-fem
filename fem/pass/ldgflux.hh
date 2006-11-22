#ifndef DUNE_LDGFLUX_CC
#define DUNE_LDGFLUX_CC

// Numerical Upwind-Flux
template <class ModelImp>
class LDGFlux
{
public:
  typedef ModelImp ModelType;
  typedef typename ModelType::Traits::DomainType DomainType;
public:
  LDGFlux(const ModelType& mod, 
          const double beta, 
          const double power,
          const double eta) 
    : model_(mod)
    , beta_(beta)
    , power_(power)
    , eta_(eta)
    , betaNotZero_( std::abs( beta_ ) > 0.0 )
  {
  }

  const ModelType& model() const {return model_;}

  //! evaluates { sigma } = 0.5 ( sigmaLeft + sigmaRight )
  template <class SigmaFluxType, class UFluxType>
  double sigmaFluxBetaZero(
                       const DomainType & unitNormal,
                       const double faceVol, 
                       const UFluxType & uLeft,
                       const UFluxType & uRight, 
                       SigmaFluxType & sigmaLeft,
                       SigmaFluxType & sigmaRight) const
  {
    sigmaLeft  = unitNormal;
    sigmaRight = unitNormal;

    sigmaLeft  *= 0.5*uLeft[0];
    sigmaRight *= 0.5*uRight[0];

    return 0.0;
  }
  
  //! evaluates sigmaBetaZero + stabilization 
  template <class UFluxType>
  double sigmaFluxStability(const DomainType & unitNormal,
                            const double faceVol, 
                            const UFluxType & uLeft,
                            const UFluxType & uRight, 
                            UFluxType & gLeft,
                            UFluxType & gRight) const
  {
    // stabilization term 
    const double factor = eta_ / faceVol;
    gLeft   =  uLeft;
    gRight  = -uRight;

    gLeft  *= factor;
    gRight *= factor;

    return 0.0;
  }

  //! evaluates sigmaBetaZero + stabilization 
  template <class SigmaFluxType, class UFluxType>
  double sigmaFlux(const DomainType & unitNormal,
                   const double faceVol, 
                   const UFluxType & uLeft,
                   const UFluxType & uRight, 
                   SigmaFluxType & sigmaLeft,
                   SigmaFluxType & sigmaRight,
                   UFluxType & gLeft,
                   UFluxType & gRight) const
  {
    // call part of flux for beta == 0
    sigmaFluxBetaZero(unitNormal,faceVol,uLeft,uRight,sigmaLeft,sigmaRight);

    if( betaNotZero_ )
    {
      // add part wiht beta != 0 
      DomainType scaling(unitNormal);
      scaling *= beta_ * std::pow(faceVol,power_);

      DomainType jumpLeft(scaling);
      DomainType jumpRight(scaling);

      /*
      DomainType valLeft(unitNormal);
      valLeft  *= uLeft[0];
      DomainType valRight(unitNormal);
      valRight *= uRight[0];
      */

      //jumpLeft  *= (unitNormal * valLeft);
      //jumpRight *= (unitNormal * valRight);

      jumpLeft  *= (uLeft[0]);//nitNormal * valLeft);
      jumpRight *= (uRight[0]);//nitNormal * valRight);

      sigmaLeft  -= jumpLeft;
      sigmaRight += jumpRight;
    }

    sigmaFluxStability(unitNormal,faceVol,uLeft,uRight,gLeft,gRight);
    return 0.0;
  }

  //! evaluates { u } = 0.5 * ( uLeft + uRight )
  template <class SigmaFluxType, class UFluxType>
  double uFluxBetaZero(const DomainType & unitNormal,
               const double faceVol, 
               const UFluxType & uLeft,
               const UFluxType & uRight, 
               SigmaFluxType & sigmaLeft,
               SigmaFluxType & sigmaRight,
               UFluxType & gLeft,
               UFluxType & gRight) const
  {
    // this flux has not sigma parts 
    sigmaLeft  = 0.0;
    sigmaRight = 0.0;
    
    // call part of flux for beta == 0
    gLeft  = uLeft;
    gRight = uRight;

    gLeft  *= 0.5;
    gRight *= 0.5;
    
    return 0.0;
  }


  //! evaluates uFLuxBetaZero + stabilization 
  template <class SigmaFluxType, class UFluxType>
  double uFlux(const DomainType & unitNormal,
               const double faceVol, 
               const UFluxType & uLeft,
               const UFluxType & uRight, 
               SigmaFluxType & sigmaLeft,
               SigmaFluxType & sigmaRight,
               UFluxType & gLeft,
               UFluxType & gRight) const
  {
    // call part of flux for beta == 0
    uFluxBetaZero(unitNormal,faceVol,uLeft,uRight,
                  sigmaLeft,sigmaRight,gLeft,gRight);

    if( betaNotZero_ )
    {
      DomainType left(unitNormal);
      DomainType right(unitNormal);

      left  *= uLeft[0];
      right *= -uRight[0];

      DomainType scaling(unitNormal);
      //scaling *= beta_ * faceVol;
      scaling *= beta_ * std::pow(faceVol,power_);

      gLeft  += scaling * left;
      gRight += scaling * right;
    }
    return 0.0;
  }

private:
  const ModelType& model_;
  const double beta_;
  const double power_;
  const double eta_;
  const bool betaNotZero_;
};

#endif
