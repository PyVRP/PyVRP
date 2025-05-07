# Five large European cities, with integer popularity scores.
cities = {
    0: {"name": "Groningen", "popularity": 1100, "x": 0, "y": 0},  # Depot.
    1: {"name": "London", "popularity": 1000, "x": 100, "y": 300},
    2: {"name": "Paris", "popularity": 900, "x": 150, "y": 150},
    3: {"name": "Madrid", "popularity": 700, "x": 50, "y": -50},
    4: {"name": "Rome", "popularity": 600, "x": 200, "y": -100},
}

edges = {
    (0, 1): [
        {'duration': 240, 'price': 140, 'distance': 600_000},  # fast
        {'duration': 360, 'price': 80,  'distance': 600_000},  # slow
    ],
    (0, 2): [
        {'duration': 180, 'price': 120, 'distance': 500_000},
        {'duration': 300, 'price': 60,  'distance': 500_000},
    ],
    (0, 3): [
        {'duration': 300, 'price': 100, 'distance': 650_000},
        {'duration': 420, 'price': 50,  'distance': 650_000},
    ],
    (0, 4): [
        {'duration': 420, 'price': 160, 'distance': 850_000},
        {'duration': 600, 'price': 90,  'distance': 850_000},
    ],
    (1, 0): [
        {'duration': 240, 'price': 140, 'distance': 600_000},
        {'duration': 360, 'price': 80,  'distance': 600_000},
    ],
    (1, 2): [
        {'duration': 150, 'price': 110, 'distance': 450_000},
        {'duration': 240, 'price': 60,  'distance': 450_000},
    ],
    (1, 3): [
        {'duration': 360, 'price': 130, 'distance': 700_000},
        {'duration': 540, 'price': 70,  'distance': 700_000},
    ],
    (1, 4): [
        {'duration': 400, 'price': 150, 'distance': 850_000},
        {'duration': 600, 'price': 80,  'distance': 850_000},
    ],
    (2, 0): [
        {'duration': 180, 'price': 120, 'distance': 500_000},
        {'duration': 300, 'price': 60,  'distance': 500_000},
    ],
    (2, 1): [
        {'duration': 150, 'price': 110, 'distance': 450_000},
        {'duration': 240, 'price': 60,  'distance': 450_000},
    ],
    (2, 3): [
        {'duration': 270, 'price': 90,  'distance': 550_000},
        {'duration': 420, 'price': 50,  'distance': 550_000},
    ],
    (2, 4): [
        {'duration': 300, 'price': 100, 'distance': 600_000},
        {'duration': 450, 'price': 60,  'distance': 600_000},
    ],
    (3, 0): [
        {'duration': 300, 'price': 100, 'distance': 650_000},
        {'duration': 420, 'price': 50,  'distance': 650_000},
    ],
    (3, 1): [
        {'duration': 360, 'price': 130, 'distance': 700_000},
        {'duration': 540, 'price': 70,  'distance': 700_000},
    ],
    (3, 2): [
        {'duration': 270, 'price': 90,  'distance': 550_000},
        {'duration': 420, 'price': 50,  'distance': 550_000},
    ],
    (3, 4): [
        {'duration': 240, 'price': 80,  'distance': 500_000},
        {'duration': 360, 'price': 40,  'distance': 500_000},
    ],
    (4, 0): [
        {'duration': 420, 'price': 160, 'distance': 850_000},
        {'duration': 600, 'price': 90,  'distance': 850_000},
    ],
    (4, 1): [
        {'duration': 400, 'price': 150, 'distance': 850_000},
        {'duration': 600, 'price': 80,  'distance': 850_000},
    ],
    (4, 2): [
        {'duration': 300, 'price': 100, 'distance': 600_000},
        {'duration': 450, 'price': 60,  'distance': 600_000},
    ],
    (4, 3): [
        {'duration': 240, 'price': 80,  'distance': 500_000},
        {'duration': 360, 'price': 40,  'distance': 500_000},
    ],
}
