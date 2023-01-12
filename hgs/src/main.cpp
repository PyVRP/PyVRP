#include "Exchange.h"
#include "GeneticAlgorithm.h"
#include "LocalSearch.h"
#include "MaxRuntime.h"
#include "MoveTwoClientsReversed.h"
#include "PenaltyManager.h"
#include "Population.h"
#include "ProblemData.h"
#include "RelocateStar.h"
#include "SwapStar.h"
#include "TwoOpt.h"
#include "XorShift128.h"
#include "crossover.h"
#include "diversity.h"

#include <chrono>
#include <iostream>

int main(int argc, char **argv)
try
{
    using clock = std::chrono::system_clock;
    auto start = clock::now();

    if (argc < 3)
        throw std::runtime_error("Expected at least two arguments");

    // Hardcoded since we're only using this for profiling.
    XorShift128 rng(4);
    MaxRuntime stop(30);

    ProblemData data = ProblemData::fromFile(argv[1]);
    PenaltyManager pMngr(data.vehicleCapacity());
    Population pop(data, pMngr, rng, brokenPairsDistance);
    LocalSearch ls(data, pMngr, rng);

    auto exchange10 = Exchange<1, 0>(data, pMngr);
    ls.addNodeOperator(exchange10);

    auto exchange20 = Exchange<2, 0>(data, pMngr);
    ls.addNodeOperator(exchange20);

    auto reverse20 = MoveTwoClientsReversed(data, pMngr);
    ls.addNodeOperator(reverse20);

    auto exchange22 = Exchange<2, 2>(data, pMngr);
    ls.addNodeOperator(exchange22);

    auto exchange21 = Exchange<2, 1>(data, pMngr);
    ls.addNodeOperator(exchange21);

    auto exchange11 = Exchange<1, 1>(data, pMngr);
    ls.addNodeOperator(exchange11);

    auto twoOpt = TwoOpt(data, pMngr);
    ls.addNodeOperator(twoOpt);

    auto relocateStar = RelocateStar(data, pMngr);
    ls.addRouteOperator(relocateStar);

    auto swapStar = SwapStar(data, pMngr);
    ls.addRouteOperator(swapStar);

    GeneticAlgorithm solver(data, pMngr, rng, pop, ls, selectiveRouteExchange);
    auto const res = solver.run(stop);

    std::chrono::duration<double> const timeDelta = clock::now() - start;
    auto const &bestSol = res.getBestFound();
    bestSol.toFile(argv[2], timeDelta.count());
}
catch (std::exception const &e)
{
    std::cerr << "EXCEPTION | " << e.what() << '\n';
}
catch (...)
{
    std::cerr << "UNKNOWN EXCEPTION\n";
}
