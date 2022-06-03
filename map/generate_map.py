import geojson, json
from typing import Dict, List, Tuple, Union, Any, Optional, Set
from haversine import haversine

def get_min_lat_long(features: Dict[str, Any]) -> Tuple[Optional[float], Optional[float]]:
    minLat: Optional[float] = None
    minLong: Optional[float] = None
    feature: Dict[str, Any]
    for feature in features:
        geometry: Dict[str, Any] = feature["geometry"]
        if geometry["type"] != "LineString":
            continue
        coordinates: List[Tuple[float, float]] = geometry["coordinates"]
        coordinate: Tuple[float, float]
        for coordinate in coordinates:
            if not minLat or minLat > coordinate[0]:
                minLat = coordinate[0]
                
            if not minLong or minLong > coordinate[1]:
                minLong = coordinate[1]
    return (minLat, minLong)

def get_lat_dis_meters(lat1: float, lat2: float) -> float:
    return haversine((lat1, 0), (lat2, 0))

def get_long_dis_meters(long1: float, long2: float) -> float:
    return haversine((0, long1), (0, long2))

def build_road_list(features: Dict[str, Any]) -> List[Tuple[Tuple[float, float], Tuple[float, float]]]:
    roads: List[Tuple[Tuple[float, float], Tuple[float, float]]] = list()
    feature: Dict[str, Any]
    for feature in features:
        geometry: Dict[str, Any] = feature["geometry"]
        if geometry["type"] != "LineString":
            continue
        road: List[Tuple[float, float]] = geometry["coordinates"]
        if(len(road) < 2):
            print("Found road with only one point. Ignoring.")
            continue
        i: int
        for i in range(1, len(road)):
            roads.append((road[i-1], road[i]))
    return roads

def convert_coordinates(refPoint: Tuple[float, float], roads: List[Tuple[Tuple[float, float], Tuple[float, float]]]) -> Tuple[float, float]:
    latMax: float = 0
    longMax: float = 0
    road: Tuple[Tuple[float, float], Tuple[float, float]]
    for road in roads:
        point: Tuple[float, float]
        for point in road:
            point[0] = get_long_dis_meters(refPoint[0], point[0])
            point[1] = get_lat_dis_meters(refPoint[1], point[1])
            
            if point[0] > latMax:
                latMax = point[0]
                
            if point[0] > longMax:
                longMax = point[0]
    
    return (latMax, longMax)
    
def remove_not_connected(roads: List[Tuple[Tuple[float, float], Tuple[float, float]]]) -> List[Tuple[Tuple[float, float], Tuple[float, float]]]:
    result: List[Tuple[Tuple[float, float], Tuple[float, float]]] = list()
    next: List[Tuple[float, float], Tuple[float, float]] = list()
    next.append(roads[0])
    roads.remove(roads[0])
    
    while next:
        curRoad: Tuple[Tuple[float, float], Tuple[float, float]] = next.pop()
        result.append(curRoad)
        road: Tuple[Tuple[float, float], Tuple[float, float]]
        for road in roads:
            if curRoad[1] == road[0]:
                next.append(road)
                roads.remove(road)
            elif curRoad[1] == road[1]:
                next.append((road[1], road[0]))
                roads.remove(road)
                
        if len(result) % 100 == 0:
            print(f"Status: {len(result)}/{len(roads)} - {len(next)}")
        if len(result) % 1000 == 0:
            break
    return result

print("Loading map from file...")
with open("munich.geojson") as f:
    map: Dict[str, Union[str, List[Dict[str, Any]]]] = geojson.load(f)

print("Converting map...")
roads: List[Tuple[Tuple[float, float], Tuple[float, float]]] = build_road_list(map["features"])
print(f"Found {len(roads)} road pieces.")

print("Removing not connected roads...")
if len(roads) > 0:
    roadsFiltered: List[Tuple[Tuple[float, float], Tuple[float, float]]] = remove_not_connected(roads)
else:
    roadsFiltered: List[Tuple[Tuple[float, float], Tuple[float, float]]] = roads
print(f"Reduced to {len(roadsFiltered)} connected road pieces.")

print("Converting coordinates...")
minLatLong: Tuple[Optional[float], Optional[float]] = get_min_lat_long(map["features"])
maxLatLong: Tuple[float, float] = convert_coordinates(minLatLong, roadsFiltered)
print(f"Suggested map size at least: {maxLatLong} meters.")

result: Dict[str, Any] = {"maxLat": maxLatLong[0],
                          "maxLong": maxLatLong[1],
                          "points": roadsFiltered}

print("Exporting results...")
with open('munich.json', 'w') as outfile:
    outfile.write(json.dumps(result))

print("Done.")