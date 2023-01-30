from numpy.testing import assert_raises
from pytest import mark

from pyvrp.tests.helpers import read


@mark.parametrize(
    "where, exception",
    [
        ("data/UnknownEdgeWeightFmt.txt", RuntimeError),
        ("data/UnknownEdgeWeightType.txt", RuntimeError),
        ("somewhere that does not exist", ValueError),
        ("data/FileWithUnknownSection.txt", RuntimeError),
        ("data/DepotNotOne.txt", RuntimeError),
        ("data/DepotSectionDoesNotEndInMinusOne.txt", RuntimeError),
        ("data/MoreThanOneDepot.txt", RuntimeError),
        ("data/NonZeroDepotServiceDuration.txt", RuntimeError),
        ("data/NonZeroDepotReleaseTime.txt", RuntimeError),
        ("data/NonZeroDepotOpenTimeWindow.txt", RuntimeError),
        ("data/NonZeroDepotDemand.txt", RuntimeError),
        ("data/TimeWindowOpenEqualToClose.txt", RuntimeError),
        ("data/TimeWindowOpenLargerThanClose.txt", RuntimeError),
        ("data/EdgeWeightsNoExplicit.txt", RuntimeError),
        ("data/EdgeWeightsNotFullMatrix.txt", RuntimeError),
    ],
)
def test_raises_invalid_file(where: str, exception: Exception):
    with assert_raises(exception):
        read(where)
