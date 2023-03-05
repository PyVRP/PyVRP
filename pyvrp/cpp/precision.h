#ifndef PRECISION_H
#define PRECISION_H

#ifdef INT_PRECISION
using TCost = int;
using TDist = int;
using TTime = int;
#else
using TCost = double;
using TDist = double;
using TTime = double;
#endif

#endif  // PRECISION_H
