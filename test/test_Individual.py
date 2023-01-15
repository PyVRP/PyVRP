import unittest

from test.tools import get_hgspy


class TestIndividual(unittest.TestCase):

    def setUp(self) -> None:
        self.hgspy = get_hgspy("debug/lib/hgspy*.so")

        data_loc = "hgs/test/data/OkSmall.txt"
        self.data = self.hgspy.ProblemData.from_file(data_loc)

    def test_can_create_from_routes(self):
        # This test largely replicates the distance unit test from
        # test_Individual.cpp
        pen_manager = self.hgspy.PenaltyManager(self.data.vehicle_capacity())

        routes = [[1, 2], [3], [4]]
        individual = self.hgspy.Individual(self.data, pen_manager, routes)

        self.assertEqual(individual.get_routes(), routes)
        self.assertTrue(individual.is_feasible())

        cost = sum([  # solution is feasible, so only costs are due to distance
            self.data.dist(0, 1),
            self.data.dist(1, 2),
            self.data.dist(2, 0),

            self.data.dist(0, 3),
            self.data.dist(3, 0),

            self.data.dist(0, 4),
            self.data.dist(4, 0)
        ])

        self.assertEqual(individual.cost(), cost)
