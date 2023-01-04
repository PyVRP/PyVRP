#include "CommandLine.h"
#include "Exchange.h"
#include "GeneticAlgorithm.h"
#include "LocalSearch.h"
#include "MaxRuntime.h"
#include "MoveTwoClientsReversed.h"
#include "Population.h"
#include "ProblemData.h"
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
    ProblemData data = ProblemData::fromFile(config, args.instPath());
    Population pop(data, rng);

    LocalSearch ls(data, rng);

    auto exchange10 = Exchange<1, 0>(data);
    ls.addNodeOperator(exchange10);

    auto exchange20 = Exchange<2, 0>(data);
    ls.addNodeOperator(exchange20);

    auto reverse20 = MoveTwoClientsReversed(data);
    ls.addNodeOperator(reverse20);

    auto exchange22 = Exchange<2, 2>(data);
    ls.addNodeOperator(exchange22);

    auto exchange21 = Exchange<2, 1>(data);
    ls.addNodeOperator(exchange21);

    auto exchange11 = Exchange<1, 1>(data);
    ls.addNodeOperator(exchange11);

    auto twoOpt = TwoOpt(data);
    ls.addNodeOperator(twoOpt);

    auto relocateStar = RelocateStar(data);
    ls.addRouteOperator(relocateStar);

    auto swapStar = SwapStar(data);
    ls.addRouteOperator(swapStar);

    GeneticAlgorithm solver(data, rng, pop, ls);
    solver.addCrossoverOperator(selectiveRouteExchange);

    MaxRuntime stop(config.timeLimit);
    auto const res = solver.run(stop);

    std::chrono::duration<double> const timeDelta = clock::now() - start;
    auto const &bestSol = res.getBestFound();
    bestSol.toFile(args.solPath(), timeDelta.count());
}
catch (std::exception const &e)
{
    std::cerr << "EXCEPTION | " << e.what() << '\n';
}
catch (...)
{
    std::cerr << "UNKNOWN EXCEPTION\n";
}
