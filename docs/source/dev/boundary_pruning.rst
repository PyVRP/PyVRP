Conservative boundary prechecks
==============================

``BoundaryPruning`` attempts an inexpensive rejection before constructing
inter-route proposals in Relocate1/2, Swap11/21/22 and SwapTails. This is the
existing lower-bound principle applied earlier, not a new optimisation theorem.
If the directed boundary-edge change exceeds all current penalties, even
removing every penalty cannot make the move improve. Internal preserved edges
cancel; symmetry and the triangle inequality are unnecessary.

Scope
-----

This first implementation deliberately supports a narrow model: at most 1,000
clients and locations, one depot/profile/vehicle type, required delivery clients,
zero fixed/duration/overtime vehicle costs, unit distance cost exactly one, no
pickups, shipments, groups or reloads, and a finite nonnegative distance matrix
with zero diagonal and entries no larger than 1e12. Unsupported models retain
the original evaluator. Model guards use raw values, not tolerant equality.
Per-operator state avoids cross-problem/global mutable configuration. The
distance matrix is scanned during construction; this cost must be included in
any performance comparison.

Numerical envelope
------------------

Assume IEEE binary64 round-to-nearest, without fast-math or arithmetic
reassociation. The rounding mode is checked at construction; callers must not
change it while searching. Let M denote the largest edge and Cu,Cv the current
host penalised costs; Du,Dv denote stored route distances. Compute the boundary
change B and P=(Cu-Du)+(Cv-Dv). Reject only if P is nonnegative and

.. math::

   B > P + 10^{-6} + (1+M)2^{-24} + (1+C_u+C_v)2^{-40}.

Nonfinite arithmetic and ambiguous zero/tiny differences fall back. No integer
conversion is involved.

With unit roundoff u=2^-53, each prefix contains at most 1,001 nonnegative edges.
Its absolute error is below gamma(1001)*1001*M < 1.12e-10*M, where
gamma(n)=nu/(1-nu). Six proposal segments involve at most twelve prefix endpoints;
adding two old distances and a pessimistic 64 additional boundary/distance
operations gives an envelope below 1e-8*M. Using the actual stored Cu,Cv, a
further loose envelope 1e-13*(Cu+Cv) covers the host's two cost subtractions and
the old-penalty computation. The chosen coefficients strictly exceed these
envelopes, including rounding when computing the threshold. The absolute term
covers subnormal arithmetic and the host's absolute comparison tolerance.

New penalties are nonnegative under the supported valid model. Dropping them
cannot increase a rounded result: round-to-nearest arithmetic is monotone. This
also applies to source removal followed by target insertion, even though the
target's old cost is subtracted later. Therefore a strictly positive lower bound
cannot be accepted as a negative move by the host. This argument does not rely
on future penalties having the same magnitude as present penalties.

This is a conservative forward-error argument, not formal verification. Tests
should compare against complete route reconstruction, include fractional and
large-prefix cases, and retain original evaluation for unsupported variants.
When rejected early, the returned zero cost is an incomplete nonnegative value
as allowed by the local-search operator contract, not the exact move cost.

Performance claims must specify the model, workload, hardware, comparison
revision and full timing boundary. Faster evaluation of the same trajectory
does not demonstrate shorter driving routes or general performance superiority.
