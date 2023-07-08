#ifndef PYVRP_CIRCLESECTOR_H
#define PYVRP_CIRCLESECTOR_H

namespace pyvrp::search
{
// Data structure to represent circle sectors
// Angles are measured in [0,65535] instead of [0,359], in such a way that
// modulo operations are much faster (since 2^16 = 65536) Credit to Fabian
// Giesen at "https://web.archive.org/web/20200912191950" and
// "https://fgiesen.wordpress.com/2015/09/24/intervals-in-modular-arithmetic/"
// for useful implementation tips regarding interval overlaps in modular
// arithmetics
struct CircleSector
{
    int start;  // The angle where the circle sector starts
    int end;    // The angle where the circle sector ends

    // Calculate the positive modulo 65536 of i
    // Example: positive_mod(-6) returns 65530
    // Example: positive_mod(10) returns 10
    // Example: positive_mod(65538) returns 2
    static int positive_mod(int i)
    {
        // Using the formula positive_mod(i,x) = (i % x + x) % x
        // Remark that "i % 65536" should be automatically compiled in an
        // optimized form as "i & 0xffff" for faster calculations
        return (i % 65536 + 65536) % 65536;
    }

    static int positive_mod(CircleSector const &sector)
    {
        return positive_mod(sector.end - sector.start);
    }

    // Initialize a circle sector from a single point
    // The function extend(int point) can be used to create a circle sector that
    // is not only one point
    void initialize(int point)
    {
        start = point;
        end = point;
    }

    // Tests if a point is enclosed in the circle sector
    [[nodiscard]] bool isEnclosed(int point) const
    {
        return positive_mod(point - start) <= positive_mod(end - start);
    }

    // Tests overlap of two circle sectors with tolerance
    // Note, this effectively is sector1.isEnclosed(sector2.start) ||
    // sector2.isEnclosed(sector1.start), while also taking into account
    // tolerance
    static bool overlap(const CircleSector &sector1,
                        const CircleSector &sector2,
                        const int tolerance)
    {
        // the RHS is the size of the sector, by adding the tolerance outside
        // the positive_mod, we avoid overflow beyond a full circle
        return ((positive_mod(sector2.start - sector1.start)
                 <= positive_mod(sector1) + tolerance)
                || (positive_mod(sector1.start - sector2.start)
                    <= positive_mod(sector2) + tolerance));
    }

    // Extends the circle sector to include an additional point
    // Done in a "greedy" way, such that the resulting circle sector is the
    // smallest
    void extend(int point)
    {
        // Check if the point is in the circle sector
        if (!isEnclosed(point))
        {
            // If the point is outside the circle sector, extend the circle
            // sector in a greedy way
            if (positive_mod(point - end) <= positive_mod(start - point))
                end = point;
            else
                start = point;
        }
    }
};
}  // namespace pyvrp::search

#endif  // PYVRP_CIRCLESECTOR_H
