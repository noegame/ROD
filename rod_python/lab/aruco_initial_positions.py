# Define the initial positions for the ArUco markers
# [possible id, zone, x, y, z, angle]
# x,y,z unit : mm
initial_position = [
    [[41], "ELEMENTS_COLLECTING_ZONE_1", 325, 750, 30, 0],
    [[41], "ELEMENTS_COLLECTING_ZONE_1", 325, 800, 30, 0],
    [[41], "ELEMENTS_COLLECTING_ZONE_1", 325, 850, 30, 0],
    [[41], "ELEMENTS_COLLECTING_ZONE_2", 325, 2150, 30, 0],
    [[41], "ELEMENTS_COLLECTING_ZONE_2", 325, 2200, 30, 0],
    [[41], "ELEMENTS_COLLECTING_ZONE_2", 325, 2250, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_3", 725, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_3", 775, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_3", 825, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_3", 875, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_4", 725, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_4", 775, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_4", 825, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_4", 875, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_5", 1200, 1075, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_5", 1200, 1125, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_5", 1200, 1175, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_5", 1200, 1225, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_6", 1200, 1775, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_6", 1200, 1825, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_6", 1200, 1875, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_6", 1200, 1925, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_7", 1525, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_7", 1575, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_7", 1625, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_7", 1675, 200, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_8", 1525, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_8", 1575, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_8", 1625, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_8", 1675, 2800, 30, 90],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_9", 1800, 1025, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_9", 1800, 1075, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_9", 1800, 1125, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_9", 1800, 1175, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_10", 1800, 1825, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_10", 1800, 1875, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_10", 1800, 1925, 30, 0],
    [[36, 47], "ELEMENTS_COLLECTING_ZONE_10", 1800, 1975, 30, 0],
]


def get_expected_position():
    """
    Convert the initial_position list into a dictionary mapping tag IDs to their expected positions.
    Returns:
        dict: A dictionary where keys are tag IDs and values are lists of (x, y, z) tuples representing expected positions.
    """
    expected_positions = {}
    for entry in initial_position:
        possible_ids = entry[0]
        x, y, z = entry[2], entry[3], entry[4]
        for tag_id in possible_ids:
            if tag_id not in expected_positions:
                expected_positions[tag_id] = []
            # Store only X,Y for proximity search (Z added dynamically)
            expected_positions[tag_id].append((x, y, z))

    return expected_positions
