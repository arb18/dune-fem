#ifndef DUNE_FEM_FUNCTION_LOCALFUNCTION_BINDABLE_HH
#define DUNE_FEM_FUNCTION_LOCALFUNCTION_BINDABLE_HH

#include <dune/fem/space/common/discretefunctionspace.hh>
#include <dune/fem/function/common/discretefunction.hh>
#include <dune/fem/common/coordinate.hh>
#include <dune/fem/quadrature/quadrature.hh> // shouldn't be here (but otherwise the coordinate method doesn't work)

namespace Dune
{
  namespace Fem
  {
    struct BindableFunction : public HasLocalFunction {};

    template <class GridPart, class Range>
    struct BindableGridFunction : public BindableFunction
    {
      typedef GridPart GridPartType;
      typedef typename GridPart::template Codim<0>::EntityType EntityType;
      typedef typename EntityType::Geometry::GlobalCoordinate DomainType;
      typedef Dune::Fem::GridFunctionSpace<GridPartType, Range> FunctionSpaceType;
      typedef typename FunctionSpaceType::RangeFieldType RangeFieldType;
      typedef typename FunctionSpaceType::RangeType RangeType;
      typedef typename FunctionSpaceType::JacobianRangeType JacobianRangeType;
      typedef typename FunctionSpaceType::HessianRangeType HessianRangeType;
      BindableGridFunction(const GridPart &gridPart)
      : gridPart_(gridPart) {}
      void bind(const EntityType &entity) { entity_ = entity; }
      void unbind() {}
      bool continuous() const { return true; }
      template <class Point>
      DomainType global(const Point &x) const
      {
        return entity_.geometry().global( Dune::Fem::coordinate(x) );
      }
      const GridPart& gridPart() const { return gridPart_; }
      const EntityType &entity() const { return entity_; }
      private:
      EntityType entity_;
      const GridPart &gridPart_;
    };

    template <class GridPart, class Range>
    struct BindableGridFunctionWithSpace : public BindableGridFunction<GridPart,Range>
    {
      typedef BindableGridFunction<GridPart,Range> Base;
      typedef GridPart GridPartType;
      typedef typename GridPart::template Codim<0>::EntityType EntityType;
      typedef typename EntityType::Geometry::GlobalCoordinate DomainType;
      typedef Dune::Fem::GridFunctionSpace<GridPartType, Range> FunctionSpaceType;
      typedef typename FunctionSpaceType::RangeFieldType RangeFieldType;
      typedef typename FunctionSpaceType::RangeType RangeType;
      typedef typename FunctionSpaceType::JacobianRangeType JacobianRangeType;
      typedef typename FunctionSpaceType::HessianRangeType HessianRangeType;
      typedef DiscreteFunctionSpaceAdapter< FunctionSpaceType, GridPartType > DiscreteFunctionSpaceType;
      BindableGridFunctionWithSpace(const GridPart &gridPart, const std::string &name, int order)
      : Base(gridPart),
        space_( gridPart, order ),
        name_(name)
      {}
      //! return the order of the space
      unsigned int order() const
      {
        return space().order();
      }
      const std::string &name() const
      {
        return name_;
      }
      const DiscreteFunctionSpaceType &space () const
      {
        return space_;
      }
      private:
      DiscreteFunctionSpaceType space_;
      const std::string name_;
    };
  } // namespace Fem
} // namespace Dune
#endif // DUNE_FEM_FUNCTION_LOCALFUNCTION_BINDABLE_HH
