#include <iostream>
#include <vector>
using namespace std;

// const float INTERSECTION_WIDTH = 100.0f;
// const float INTERSECTION_HEIGHT = 100.0f;
float spawnIntervalEast = 1.5f;  // 1.5 sec
float spawnIntervalWest = 2.0f;  // 2 sec
float spawnIntervalNorth = 1.0f; // 1 sec
float spawnIntervalSouth = 2.0f; // 2 sec
float spawnIntervalHeavy = 1.0f; // 3 sec

float spawnIntervalOutofOrderEast = 5.0f;   // 1.5 sec
float spawnIntervalOutofOrderWest = 5.0f;   // 2 sec
float spawnIntervalOutofOrderNorth = 10.0f; // 1 sec
float spawnIntervalOutofOrderSouth = 13.0f; // 2 sec

float speedFactor = 5.0f;

float vehicelSpeed = speedFactor * 1.0f;
float heavyVehicelSpeed = speedFactor / 2.0f;
float emergencyVehicelSpeed = speedFactor * 2.0f;

const int eastX = 1850, eastY = 370, westX = 20, westY = 540, southX = 1050, southY = 1000, northX = 800, northY = 0;
const int eastheavyX = 1850, eastheavyY = 180, westheavyX = 0, westheavyY = 700, southheavyX = 1150, southheavyY = 1000, northheavyX = 650, northheavyY = 0;
const int emeastY = 420, emsouthX = 940, emNorthX = 850, emwestY = 600;
// Add intersection boundary constants
const float INTERSECTION_LIMIT_EAST = 800;  // Left boundary of intersection
const float INTERSECTION_LIMIT_WEST = 700;  // Right boundary of intersection
const float INTERSECTION_LIMIT_SOUTH = 600; // Top boundary of intersection
const float INTERSECTION_LIMIT_NORTH = 400; // Bottom boundary of intersection

string pathE = "img/car1_e.png";
string pathW = "img/car1_w.png";
string pathN = "img/car1_n.png";
string pathS = "img/car1_s.png";

vector<string> eastPaths = {"img/car1_e.png", "img/car2_e.png", "img/car3_e.png"};
vector<string> westPaths = {"img/car1_w.png", "img/car2_w.png", "img/car3_w.png"};
vector<string> northPaths = {"img/car1_n.png", "img/car2_n.png", "img/car3_n.png"};
vector<string> southPaths = {"img/car1_s.png", "img/car2_s.png", "img/car3_s.png"};
vector<string> heavyPaths = {"img/truck_e.png", "img/truck_w.png", "img/truck_n.png", "img/truck_s.png"};
vector<string> emPaths = {"img/emergency_e.png", "img/emergency_w.png", "img/emergency_n.png", "img/emergency_s.png"};