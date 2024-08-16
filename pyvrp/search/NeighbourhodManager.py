class NeighbourhoodManager:
    def __init__(
        self,
        data: ProblemData,
        full_neighbourhood: list[list[int]],
        neighbourhood: list[list[int]],
        params: NeighbourhoodParams = NeighbourhoodParams(),
    ):
        self._data = data
        self._full_neighbourhood = full_neighbourhood
        self._neighbourhood = neighbourhood
        self._params = params

    def __call__(
        self, solution1: Solution, solution2: Solution
    ) -> list[list[int]]:
        """
        Finds client candidates for search. The candidates are those
        who are in routes that are different between solution1 and solution2.
        """
        modified = set()

        neighbours1 = solution1.neighbours()
        neighbours2 = solution2.neighbours()

        for idx in range(self._data.num_depots, self._data.num_locations):
            pred1, succ1 = neighbours1[idx]
            pred2, succ2 = neighbours2[idx]

            if neighbours1[idx] != neighbours2[idx]:
                modified.add(idx)
                modified.add(pred1)
                modified.add(succ1)
                modified.add(pred2)
                modified.add(succ2)

        nbhd = []
        for idx, clients in enumerate(self._full_neighbourhood):
            if idx in modified:
                nbhd.append(clients)
            else:
                nbhd.append([])
                # restricted = [idx for idx in clients if idx in modified]
                # nbhd.append(restricted)

        return nbhd

    def update(self):
        pass

    @property
    def neighbourhood(self) -> list[list[int]]:
        return self._neighbourhood
