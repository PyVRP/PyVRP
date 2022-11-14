#include "CommandLine.h"
#include "Exchange.h"
#include "GeneticAlgorithm.h"
#include "LocalSearch.h"
#include "MaxRuntime.h"
#include "MoveTwoClientsReversed.h"
#include "Params.h"
#include "Population.h"
#include "RelocateStar.h"
#include "SwapStar.h"
#include "TwoOpt.h"
#include "XorShift128.h"
#include "crossover.h"

#include <chrono>
#include <iostream>

int main(int argc, char **argv)
try
{
    using clock = std::chrono::system_clock;
    auto start = clock::now();

    CommandLine args(argc, argv);
    auto config = args.parse();

    XorShift128 rng(config.seed);
    Params params(config, args.instPath());
    Population pop(params, rng);

    LocalSearch ls(params, rng);

    auto exchange10 = Exchange<1, 0>(params);
    ls.addNodeOperator(exchange10);

    auto exchange20 = Exchange<2, 0>(params);
    ls.addNodeOperator(exchange20);

    auto reverse20 = MoveTwoClientsReversed(params);
    ls.addNodeOperator(reverse20);

    auto exchange22 = Exchange<2, 2>(params);
    ls.addNodeOperator(exchange22);

    auto exchange21 = Exchange<2, 1>(params);
    ls.addNodeOperator(exchange21);

    auto exchange11 = Exchange<1, 1>(params);
    ls.addNodeOperator(exchange11);

    auto twoOpt = TwoOpt(params);
    ls.addNodeOperator(twoOpt);

    auto relocateStar = RelocateStar(params);
    ls.addRouteOperator(relocateStar);

    auto swapStar = SwapStar(params);
    ls.addRouteOperator(swapStar);

    GeneticAlgorithm solver(params, rng, pop, ls);
    solver.addCrossoverOperator(selectiveRouteExchange);

    MaxRuntime stop(config.timeLimit);
    auto const res = solver.run(stop);

    std::chrono::duration<double> const timeDelta = clock::now() - start;
    auto const &bestSol = res.getBestFound();
    bestSol.exportCVRPLibFormat(args.solPath(), timeDelta.count());
}
catch (std::exception const &e)
{
    std::cerr << "EXCEPTION | " << e.what() << '\n';
}
catch (...)
{
    std::cerr << "UNKNOWN EXCEPTION\n";
}
