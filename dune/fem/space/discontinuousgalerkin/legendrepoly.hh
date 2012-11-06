#ifndef DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LEGENDREPOLY_HH
#define DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LEGENDREPOLY_HH

// C++ inlcudes
#include <cassert>
#include <iostream>


namespace Dune
{

  namespace Fem 
  {  

    class LegendrePoly
    {
    protected:
      static const int maxPol = 11;

      static const double factor[ maxPol ][ maxPol ];
      static const double weight[ maxPol ];

    public:
      static double evaluate ( const int num, const double x )
      {
        assert( 0 <= num && num < maxPol );

        double phi = factor[ num ][ num ];
        for( int i = num-1; i >= 0; --i )
          phi = phi * x + factor[ num ][ i ];
        return weight[ num ] * phi;
      }
      
      static double jacobian ( const int num, const double x )
      { 
        assert( 0 <= num && num < maxPol );

        double phi = 0.;
        if( num >= 1 )
        {
          phi = factor[ num ][ num ] * num;
          for( int i = num-1; i >= 1; --i )
            phi = phi * x + factor[ num ][ i ] * i;
        }
        return weight[ num ] * phi;
      }
         
      static double hessian ( const int num, const double x )
      { 
        assert( 0 <= num && num < maxPol );

        double phi=0.;
        if( num >= 2 )
        {
          phi = factor[ num ][ num ] * num * ( num-1 );
          for( int i = num-1; i >= 2; --i )
            phi = phi * x + factor[ num ][ i ] * i * (i-1);
        }
        return weight[ num ] * phi;
      }
    };

  } // namespace Fem 

} // namespace Dune 

#endif // #ifndef DUNE_FEM_SPACE_DISCONTINUOUSGALERKIN_LEGENDREPOLY_HH
