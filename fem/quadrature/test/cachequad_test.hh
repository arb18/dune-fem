#ifndef DUNE_CACHEQUAD_TEST_HH
#define DUNE_CACHEQUAD_TEST_HH

#include <dune/fem/misc/test.hh>
#include <dune/fem/quadrature/cachingquadrature.hh>

namespace Dune {

  class CachingQuadrature_Test : public Test 
  {
  public:
    CachingQuadrature_Test(std::string albertaGridFile,
                           std::string aluGridHexaFile,
                           std::string aluGridTetraFile,
                           std::string dgf2DGridFile,
                           std::string dgf3DGridFile) :
      albertaGridFile_(albertaGridFile),
      aluGridHexaFile_(aluGridHexaFile),
      aluGridTetraFile_(aluGridTetraFile),
      dgf2DGridFile_(dgf2DGridFile),
      dgf3DGridFile_(dgf3DGridFile)
    {}

    virtual void run();

    void codim0Test();
    void codim1AlbertaTest();
    void codim1PrismTest();
    void codim1UGTest();
    void codim1ALUHexaTest();
    void codim1ALUTetraTest();
    void codim1ALUSimplexTest ();
    void codim1SGridTest();
    void codim1YaspGridTest();

    template <class GridPartType>
    void checkLeafsCodim1(GridPartType& gridPart, 
                    const int quadOrd);

    template <class EntityType, class LocalGeometryType>
    int  checkLocalIntersectionConsistency(
              const EntityType& en, const LocalGeometryType& localGeom,
              const int face, const bool neighbor, const bool output = false) const;

  private:
    std::string albertaGridFile_;
    std::string aluGridHexaFile_;
    std::string aluGridTetraFile_;
    std::string dgf2DGridFile_;
    std::string dgf3DGridFile_;
  };

} // end namespace Dune

#endif
