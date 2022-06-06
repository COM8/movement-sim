import geojson, json
from typing import Dict, List, Tuple, Union, Any
from haversine import haversine
from sys import float_info

class Coordinate:
    lat: float
    long: float

    distLat: float
    distLong: float

    def __init__(self, lat: float, long: float):
        self.lat = lat
        self.long = long
        
        self.distLat = 0
        self.distLong = 0
    
    def to_json(self) -> Dict[str, float]:
        return {
            "lat": self.lat,
            "long": self.long,
            "distLat": self.distLat,
            "distLong": self.distLong,
        }
    
    def __get_lat_dis_meters(self, lat1: float, lat2: float) -> float:
        return haversine((lat1, 0), (lat2, 0)) * 1000

    def __get_long_dis_meters(self, long1: float, long2: float) -> float:
        return haversine((0, long1), (0, long2)) * 1000

    def calc_dist(self, refLat: float, refLong) -> None:
        self.distLat = self.__get_lat_dis_meters(refLat, self.lat)
        self.distLong = self.__get_long_dis_meters(refLong, self.long)

    def __eq__(self, other):
        return isinstance(other, Coordinate) and self.lat == other.lat and self.long == other.long
    
    def __ne__(self, other):
        return not self.__eq__(other)


class Road:
    start: Coordinate
    end: Coordinate

    def __init__(self, start: Coordinate, end: Coordinate):
        self.start = start
        self.end = end

    def to_json(self) -> Dict[str, Any]:
        return {
            "start": self.start.to_json(),
            "end": self.end.to_json()
        }
    
    def __eq__(self, other):
        return isinstance(other, Road) and self.start == other.start and self.end == other.end
    
    def __ne__(self, other):
        return not self.__eq__(other)


class Map:
    roads: List[Road]
    
    minDistLat: float
    maxDistLat: float

    minDistLong: float
    maxDistLong: float

    def __init__(self):
        self.roads = list()
        
        self.minDistLat = float_info.max
        self.maxDistLat = 0

        self.minDistLong = float_info.max
        self.maxDistLong = 0
    
    def update_min_max_dist(self):
        road: Road
        for road in self.roads:
            # Start:
            if road.start.distLat > self.maxDistLat:
                self.maxDistLat = road.start.distLat
            if road.start.distLat < self.minDistLat:
                self.minDistLat = road.start.distLat
            
            if road.start.distLong > self.maxDistLong:
                self.maxDistLong = road.start.distLong
            if road.start.distLong < self.minDistLong:
                self.minDistLong = road.start.distLong

            # End:
            if road.end.distLat > self.maxDistLat:
                self.maxDistLat = road.end.distLat
            if road.end.distLat < self.minDistLat:
                self.minDistLat = road.end.distLat
            
            if road.end.distLong > self.maxDistLong:
                self.maxDistLong = road.end.distLong
            if road.end.distLong < self.minDistLong:
                self.minDistLong = road.end.distLong
    
    def get_min_lat_long(self) -> Tuple[float, float]:
        minLat = float_info.max
        minLong = float_info.max

        road: Road
        for road in self.roads:
            # Start:
            if road.start.lat < minLat:
                minLat = road.start.lat
            if road.start.long < minLong:
                minLong = road.start.long

            # Start:
            if road.end.lat < minLat:
                minLat = road.end.lat
            if road.end.long < minLong:
                minLong = road.end.long

        return (minLat, minLong)

    def to_json(self) -> Dict[str, Any]:
        return {
            "minDistLat": self.minDistLat,
            "maxDistLat": self.maxDistLat,
            "minDistLong": self.minDistLong,
            "maxDistLong": self.maxDistLong,
            "roads": [r.to_json() for r in self.roads]
        }


def build_map(features: Dict[str, Any]) -> Map:
    map: Map = Map()
    feature: Dict[str, Any]
    for feature in features:
        geometry: Dict[str, Any] = feature["geometry"]
        if geometry["type"] != "LineString":
            continue
        roadPointList: List[Tuple[float, float]] = geometry["coordinates"]
        if(len(roadPointList) < 2):
            print("Found road with only one point. Ignoring.")
            continue
        i: int
        for i in range(1, len(roadPointList)):
            road: Road = Road(Coordinate(roadPointList[i-1][0], roadPointList[i-1][1]), Coordinate(roadPointList[i][0], roadPointList[i][1]))
            map.roads.append(road)
    return map
    
def remove_not_connected(map: Map, refPoint: Tuple[float, float]) -> None:
    roads: List[Road] = map.roads
    result: List[Road] = list()
    next: List[Road] = list()
    next.append(roads[0])
    roads.remove(roads[0])

    next[0].start.calc_dist(refPoint[0], refPoint[1])
    next[0].end.calc_dist(refPoint[0], refPoint[1])
    
    while next:
        curRoad: Road = next.pop()
        result.append(curRoad)
        road: Road
        for road in roads:
            if curRoad.end == road.start:
                road.end.calc_dist(refPoint[0], refPoint[1])
                # Ensure we don't have hay gaps between roads due to floating point inaccuracies:
                road.start = curRoad.end
                next.append(road)
                roads.remove(road)
            elif curRoad.end == road.end:
                road.end = road.start
                # Ensure we don't have hay gaps between roads due to floating point inaccuracies:
                road.start = curRoad.end
                road.end.calc_dist(refPoint[0], refPoint[1])
                next.append(road)
                roads.remove(road)
                
        if len(result) % 10 == 0:
            print(f"Status: {len(result)}/{len(roads)} - {len(next)}")
        if len(result) % 10000 == 0:
            break
    
    map.roads = result

print("Loading map from file...")
with open("map/munich.geojson") as f:
    map: Dict[str, Union[str, List[Dict[str, Any]]]] = geojson.load(f)

print("Converting map...")
map: Map = build_map(map["features"])
print(f"Found {len(map.roads)} road pieces.")

print("Removing not connected roads...")
refPoint: Tuple[float, float] = map.get_min_lat_long()
remove_not_connected(map, refPoint)
print(f"Reduced to {len(map.roads)} connected road pieces.")

print("Converting coordinates...")
map.update_min_max_dist()
print(f"Suggested map size at least: {map.maxDistLat}x{map.maxDistLong} meters.")

print("Exporting results...")
with open('munich.json', 'w') as outfile:
    outfile.write(json.dumps(map.to_json()))
print("Done.")