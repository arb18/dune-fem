dune_install(cginverseoperator.hh diagonalpreconditioner.hh istlinverseoperators.hh
             istlsolver.hh krylovinverseoperators.hh multistep.hh newtoninverseoperator.hh
             odesolver.hh odesolverinterface.hh oemsolver.hh
             parameter.hh pardg.hh pardginverseoperators.hh
             petscsolver.hh petscinverseoperators.hh preconditionedinverseoperator.hh
             timeprovider.hh umfpacksolver.hh spqrsolver.hh ldlsolver.hh eigen.hh)

dune_add_subdirs(linear rungekutta test)

dune_add_library(solver OBJECT
    rungekutta/butchertable.cc )
