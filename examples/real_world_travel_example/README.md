# Summary

In this tutorial, we formulate and solve a Prize-Collecting Vehicle Routing Problem (VRP) for a traveler who has between 2 and 7 days to visit popular European cities and must return to a depot. We use PyVRP to model the problem with integer costs (durations in minutes, distances in meters, prices in EUR), two parallel edges (fast‐expensive vs. slow‐cheap trains) between each city pair, and integer prizes for visiting each city. We walk through (1) data setup, (2) PyVRP model creation, (3) solving with PyVRP’s heuristic solver, and (4) mapping the solution back to real‐world city names and chosen trains.

# Problem Description

A Vehicle Routing Problem with Profits (VRPP) is a maximization variant in which visiting each client yields a prize, and it is not mandatory to visit all clients; the goal is to maximize total collected prize under resource constraints (e.g., time) Wikipedia. PyVRP supports optional clients with prizes, making it ideal for this “visit as many popular destinations as possible” scenario.

# Trip Constraints

- **Time budget**: between 2 days (2 × 24 × 60 = 2880 minutes) and 7 days (10080 minutes).
- **Start/end**: fixed depot city.
- **Transport modes**: between any two cities,
  - **Fast train**: shorter duration, higher price.
  - **Slow train**: longer duration, lower price.
- **Objective**: maximize sum of visited city popularities under the time budget, including travel durations and a fixed 60 minutes “visit time” at each city.
