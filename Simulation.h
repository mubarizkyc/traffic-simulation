#include <SFML/Graphics.hpp>
#include <vector>
#include "constants.h"
#include "Vehicle.h"
#include "TrafficControl.h"
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <shared_mutex>
using namespace std;
using namespace sf;
// two things to impl
// 1 peak hours
/*

if (rand() % 15 < 1)
{ // Emergency vehicle (higher priority)
    eastvehicels.emplace_back(emPaths[0], emergencyVehicelSpeed, eastX, eastY);
}
else
*/

std::mutex eastMutex, westMutex, northMutex, southMutex, heavyMutex, moveMutex;
static int eastIndex = 0, westIndex = 0, northIndex = 0, southIndex = 0;

class Line
{
public:
    RectangleShape shape;

    Line(Vector2f size, Vector2f position, Color color)
    {
        shape.setSize(size);
        shape.setPosition(position);
        shape.setFillColor(color);
    }

    Line(RectangleShape s)
    {
        shape = s;
    }
};
enum Direction
{
    EAST,
    WEST,
    NORTH,
    SOUTH
};

class Simulation
{

    struct DirectionData
    {
        float intersectionLimit;
        int positionMultiplier; // +1 for east/north, -1 for west/south
    };
    enum State
    {
        NORMAL,
        PEAK
    };
    State currentState;
    std::shared_mutex vehicleMutex;

public:
    TrafficController lightController;

    vector<Line> lines;
    Sprite background; // Game background sprite
    Texture bg_texture;
    struct TrafficInfo
    {
        mutable vector<Vehicle> vehicels1; // for lane 1 regular and emergency
        mutable vector<Vehicle> emergency;
        mutable vector<Vehicle> vehicels2; // for lane 2 heavy and regular
        sf::Clock vehicleTimer1;
        sf::Clock vehicleTimer2;
        sf::Clock vehicleTimerOutofOrder;
        sf::Clock vehicleTimerHeavy;
        string name;
        sf::Clock emergencyTimer;
        bool emergencyActive = false;
    };
    TrafficInfo east;
    TrafficInfo west;
    TrafficInfo north;
    TrafficInfo south;

    float INTERSECTION_LIMIT_EAST = 0;  // Left boundary of intersection
    float INTERSECTION_LIMIT_WEST = 0;  // Right boundary of intersection
    float INTERSECTION_LIMIT_SOUTH = 0; // Top boundary of intersection
    float INTERSECTION_LIMIT_NORTH = 0; // Bottom boundary of intersection
    sf::Clock deltaClock;
    std::atomic<bool> isSimulationRunning{true};
    Simulation(int width, int height) : lightController(Vector2f(960, 350), Vector2f(1100, 520))
    {
        background.setScale(static_cast<float>(width) / bg_texture.getSize().x, static_cast<float>(height) / bg_texture.getSize().y);
        drawRoads(width, height);

        // Initialize  vehicels
        east.vehicels1.emplace_back("img/car1_e.png", vehicelSpeed, eastX, eastY, REGULAR);
        west.vehicels1.emplace_back("img/car1_w.png", vehicelSpeed, westX, westY, REGULAR);
        north.vehicels1.emplace_back("img/car1_n.png", vehicelSpeed, northX, northY, REGULAR);
        south.vehicels1.emplace_back("img/car1_s.png", vehicelSpeed, southX, southY, REGULAR);
        currentState = NORMAL;
        east.name = "east";
        west.name = "west";
        north.name = "north";
        south.name = "south";
    }

    void drawRoads(const float WINDOW_WIDTH, const float WINDOW_HEIGHT)
    {
        // Road measurements
        float roadWidth = 600;     // Width of the road
        float dividerWidth = 8;    // Width of the yellow divider
        float laneMarkWidth = 4;   // Width of white lane markings
        float laneMarkLength = 30; // Length of each white dash
        float laneMarkGap = 20;    // Gap between dashes

        // Calculate center positions for the intersection
        int centerX = WINDOW_WIDTH / 2;  // 750 for 1500 width
        int centerY = WINDOW_HEIGHT / 2; // 500 for 1000 height

        // Calculate road positions
        int roadPosX = centerX - roadWidth / 2;
        int roadPosY = centerY - roadWidth / 2;

        // Main black roads (vertical and horizontal)
        // Vertical road (North-South)
        lines.emplace_back(Vector2f(roadWidth, WINDOW_HEIGHT),
                           Vector2f(roadPosX, 0), Color::Black);

        // Horizontal road (East-West)
        lines.emplace_back(Vector2f(WINDOW_WIDTH, roadWidth),
                           Vector2f(0, roadPosY), Color::Black);

        // Calculate divider center positions
        float dividerPosX = roadPosX + (roadWidth / 2) - (dividerWidth / 2);
        float dividerPosY = roadPosY + (roadWidth / 2) - (dividerWidth / 2);

        // Yellow dividers
        lines.emplace_back(Vector2f(dividerWidth, roadPosY),
                           Vector2f(dividerPosX, 0), Color::Yellow);

        lines.emplace_back(Vector2f(dividerWidth, WINDOW_HEIGHT - roadPosY),
                           Vector2f(dividerPosX, roadPosY), Color::Yellow);

        lines.emplace_back(Vector2f(roadPosX, dividerWidth),
                           Vector2f(0, dividerPosY), Color::Yellow);

        lines.emplace_back(Vector2f(WINDOW_WIDTH - roadPosX, dividerWidth),
                           Vector2f(roadPosX, dividerPosY), Color::Yellow);

        // Add lane markings (white dashed lines)
        float laneOffset = roadWidth / 4;

        for (float y = 0; y < WINDOW_HEIGHT; y += laneMarkLength + laneMarkGap)
        {
            lines.emplace_back(Vector2f(laneMarkWidth, laneMarkLength),
                               Vector2f(roadPosX + laneOffset - laneMarkWidth / 2, y),
                               Color::White);

            lines.emplace_back(Vector2f(laneMarkWidth, laneMarkLength),
                               Vector2f(roadPosX + roadWidth - laneOffset - laneMarkWidth / 2, y),
                               Color::White);
        }

        for (float x = 0; x < WINDOW_WIDTH; x += laneMarkLength + laneMarkGap)
        {
            lines.emplace_back(Vector2f(laneMarkLength, laneMarkWidth),
                               Vector2f(x, roadPosY + laneOffset - laneMarkWidth / 2),
                               Color::White);

            lines.emplace_back(Vector2f(laneMarkLength, laneMarkWidth),
                               Vector2f(x, roadPosY + roadWidth - laneOffset - laneMarkWidth / 2),
                               Color::White);
        }
        float factor = 150.0f;

        // Define intersection boundaries
        INTERSECTION_LIMIT_EAST = (roadPosX + roadWidth - laneOffset - laneMarkWidth / 2) + factor;
        INTERSECTION_LIMIT_WEST = (roadPosX + laneOffset - laneMarkWidth / 2) - factor;
        INTERSECTION_LIMIT_SOUTH = (roadPosY + roadWidth - laneOffset - laneMarkWidth / 2) + factor;
        INTERSECTION_LIMIT_NORTH = (roadPosY + laneOffset - laneMarkWidth / 2) - factor;

        // Draw critical area as a red box
        sf::RectangleShape criticalBox;
        criticalBox.setSize(Vector2f(INTERSECTION_LIMIT_EAST - INTERSECTION_LIMIT_WEST,
                                     INTERSECTION_LIMIT_SOUTH - INTERSECTION_LIMIT_NORTH));
        criticalBox.setPosition(INTERSECTION_LIMIT_WEST, INTERSECTION_LIMIT_NORTH);
        //   criticalBox.setFillColor(sf::Color(255, 0, 0, 128)); // Semi-transparent red
        criticalBox.setFillColor(sf::Color::Green);
        criticalBox.setOutlineThickness(2);

        // Add the critical box to a drawable list or render directly
        lines.emplace_back(criticalBox); // Assuming `sprites` is a container for sf::Drawable objects
    }
    bool shouldStopAtSignal(const Vehicle &vehicle, const string &direction,
                            const string &currentState)
    {

        if (currentState == "green" || vehicle.type == OutOfOrder)
            return false;

        constexpr float YELLOW_DECISION_DISTANCE = 5.0f;
        constexpr float RED_STOP_DISTANCE = 20.0f;

        // Get vehicle's current position and bounds
        Vector2f pos = vehicle.sprite.getPosition();
        FloatRect bounds = vehicle.sprite.getGlobalBounds();

        // Calculate vehicle's leading and trailing edges based on direction
        float leadingEdge, trailingEdge;
        if (direction == "east")
        {
            leadingEdge = pos.x + bounds.width;
            trailingEdge = pos.x;
        }
        else if (direction == "west")
        {
            leadingEdge = pos.x;
            trailingEdge = pos.x - bounds.width;
        }
        else if (direction == "north")
        {
            leadingEdge = pos.y;
            trailingEdge = pos.y - bounds.height;
        }
        else
        { // south
            leadingEdge = pos.y + bounds.height;
            trailingEdge = pos.y;
        }

        // Check if any part of the vehicle is inside intersection
        bool isInsideIntersection = false;
        if (direction == "east" || direction == "west")
        {
            isInsideIntersection = (leadingEdge > INTERSECTION_LIMIT_WEST &&
                                    trailingEdge < INTERSECTION_LIMIT_EAST);
        }
        else
        {
            isInsideIntersection = (leadingEdge > INTERSECTION_LIMIT_NORTH &&
                                    trailingEdge < INTERSECTION_LIMIT_SOUTH);
        }

        // Allow vehicels already inside to pass
        if (isInsideIntersection)
            return false;

        // Calculate distance to intersection considering vehicle length
        float distanceToIntersection;
        if (direction == "east")
        {
            distanceToIntersection = leadingEdge - INTERSECTION_LIMIT_EAST;
        }
        else if (direction == "west")
        {
            distanceToIntersection = INTERSECTION_LIMIT_WEST - leadingEdge;
        }
        else if (direction == "north")
        {
            distanceToIntersection = INTERSECTION_LIMIT_NORTH - leadingEdge;
        }
        else
        { // south
            distanceToIntersection = leadingEdge - INTERSECTION_LIMIT_SOUTH;
        }

        // Adjust stopping distances based on vehicle size
        float vehicleLength = (direction == "east" || direction == "west") ? bounds.width : bounds.height;
        float adjustedYellowDistance = YELLOW_DECISION_DISTANCE + vehicleLength;
        float adjustedRedDistance = RED_STOP_DISTANCE + vehicleLength;

        // Handle yellow light
        if (currentState == "yellow")
        {
            return distanceToIntersection > 0 &&
                   distanceToIntersection <= adjustedYellowDistance;
        }

        // Handle red light
        if (currentState == "red")
        {
            return distanceToIntersection > 0 &&
                   distanceToIntersection <= adjustedRedDistance;
        }

        return true;
    }
    void manageVehicleMemory()
    {
        for (auto &ti : {east, west, north, south}) // Iterate over traffic intersections
        {
            for (auto &vehicles : {std::ref(ti.vehicels1), std::ref(ti.vehicels2)}) // Use references
            {
                if (vehicles.get().size() > 20) // Access the actual container using `get()`
                {
                    // Remove the first 5 vehicles
                    vehicles.get().erase(vehicles.get().begin(), vehicles.get().begin() + 5);

                    // Optionally shrink memory allocation
                    vehicles.get().shrink_to_fit();
                }
            }

            // Handle emergency vehicles directly
            if (ti.emergency.size() > 5)
            {
                // Remove the first 5 vehicles
                ti.emergency.erase(ti.emergency.begin(), ti.emergency.begin() + 5);

                // Optionally shrink memory allocation
                ti.emergency.shrink_to_fit();
            }
        }
    }

    bool canMove(const Vehicle &vehicle, const Vehicle &prevVehicle, const string &direction)
    {
        if (vehicle.type == OutOfOrder)
            return true;
        constexpr float baseMinD = 10.0f; // Base minimum distance as a buffer

        const auto &vPos = vehicle.sprite.getPosition();
        const auto &pPos = prevVehicle.sprite.getPosition();
        const auto &vBounds = vehicle.sprite.getGlobalBounds();
        const auto &pBounds = prevVehicle.sprite.getGlobalBounds();

        float adjustedMinD = baseMinD;

        // Adjust minD based on the size of the vehicels
        if (direction == "east" || direction == "west")
        {
            adjustedMinD += vBounds.width / 2 + pBounds.width / 2;
        }
        else // "north" or "south"
        {
            adjustedMinD += vBounds.height / 2 + pBounds.height / 2;
        }

        // Direction-specific distance checks
        if (direction == "east")
            return (vPos.x - pPos.x) >= adjustedMinD;
        else if (direction == "west")
            return (pPos.x - vPos.x) >= adjustedMinD;
        else if (direction == "north")
            return (pPos.y - vPos.y) >= adjustedMinD;
        else // south
            return (vPos.y - pPos.y) >= adjustedMinD;
    }

    void moveVehicles(float deltaTime)
    {
        string eastWestState = lightController.getEastWestState();
        string northSouthState = lightController.getNorthSouthState();

        auto processVehicles = [this, deltaTime](
                                   vector<Vehicle> &vehicles,
                                   const string &direction,
                                   const string &state,
                                   std::mutex &mutex)
        {
            std::lock_guard<std::mutex> lock(mutex); // Protect the shared vector
            for (size_t i = 0; i < vehicles.size(); i++)
            {
                bool canProceed = (i == 0);
                if (!canProceed)
                {
                    canProceed = canMove(vehicles[i], vehicles[i - 1], direction) &&
                                 !shouldStopAtSignal(vehicles[i], direction, state);
                }

                if (canProceed)
                {
                    vehicles[i].move(direction, deltaTime);
                }
            }
        };

        struct VehicleGroup
        {
            vector<Vehicle> &vehicles;
            string direction;
            string state;
            std::mutex &mutex;
        };

        vector<VehicleGroup> vehicleGroups = {
            {east.vehicels1, "east", eastWestState, eastMutex},
            {west.vehicels1, "west", eastWestState, westMutex},
            {north.vehicels1, "north", northSouthState, northMutex},
            {south.vehicels1, "south", northSouthState, southMutex},
            {east.emergency, "east", eastWestState, eastMutex},
            {west.emergency, "west", eastWestState, westMutex},
            {north.emergency, "north", northSouthState, northMutex},
            {south.emergency, "south", northSouthState, southMutex},
            {east.vehicels2, "east", eastWestState, eastMutex},
            {west.vehicels2, "west", eastWestState, westMutex},
            {north.vehicels2, "north", northSouthState, northMutex},
            {south.vehicels2, "south", northSouthState, southMutex},
        };

        // Multithreading
        std::vector<std::thread> threads;

        for (auto &group : vehicleGroups)
        {
            threads.emplace_back([&, group]()
                                 { processVehicles(group.vehicles, group.direction, group.state, group.mutex); });
        }

        // Join threads
        for (auto &thread : threads)
        {
            thread.join();
        }
    }

    bool canSpawn(const std::vector<Vehicle> &vehicels, float spawnPos, const std::string &dir)
    {
        if (vehicels.empty())
            return true; // If no vehicels exist, spawning is always allowed.
        const float &minSpace = 10.0f;
        const Vehicle &lastVehicle = vehicels.back();
        const auto &lastPos = lastVehicle.sprite.getPosition();
        const auto &lastBounds = lastVehicle.sprite.getGlobalBounds();

        // Get the edge position of the last vehicle in the spawning direction
        float lastEdge = (dir == "east" || dir == "west") ? lastPos.x + (dir == "east" ? lastBounds.width : -lastBounds.width)
                                                          : lastPos.y + (dir == "south" ? lastBounds.height : -lastBounds.height);

        // Calculate the required spawn distance based on the direction
        if (dir == "east")
            return spawnPos - lastEdge >= minSpace;
        if (dir == "west")
            return lastEdge - spawnPos >= minSpace;
        if (dir == "north")
            return lastEdge - spawnPos >= minSpace;
        if (dir == "south")
            return spawnPos - lastEdge >= minSpace;

        return false; // Default to not spawning if direction is invalid.
    }

    bool isOnroad(Vehicle &vehicle, const std::string &direction)
    {
        float x = vehicle.sprite.getPosition().x;
        float y = vehicle.sprite.getPosition().y;

        if (direction == "west")
        {
            // Vehicle is moving west
            return (x > 0 && x < INTERSECTION_LIMIT_WEST && y > INTERSECTION_LIMIT_NORTH && y < INTERSECTION_LIMIT_SOUTH);
        }
        else if (direction == "north")
        {
            // Vehicle is moving north
            return (x > INTERSECTION_LIMIT_WEST && x < INTERSECTION_LIMIT_EAST && y > 0 && y < INTERSECTION_LIMIT_NORTH);
        }
        else if (direction == "east")
        {
            // Vehicle is moving east
            return (x > INTERSECTION_LIMIT_EAST && x < eastX && y > INTERSECTION_LIMIT_NORTH && y < INTERSECTION_LIMIT_SOUTH);
        }
        else if (direction == "south")
        {
            // Vehicle is moving south
            return (x > INTERSECTION_LIMIT_WEST && x < INTERSECTION_LIMIT_EAST && y > INTERSECTION_LIMIT_SOUTH && y < southY);
        }

        // If direction is invalid or unsupported, return false
        return false;
    }

    void spawnVehicles()
    {
        auto spawnRegularVehicle = [&](TrafficInfo &dir, auto &paths, int &index, float spawnInterval,
                                       float x1, float y1, float x2, float y2, float offset = 0)
        {
            if (dir.vehicleTimer1.getElapsedTime().asSeconds() >= spawnInterval &&
                canSpawn(dir.vehicels1, dir.name == "east" || dir.name == "west" ? x1 : y1, dir.name))
            {
                dir.vehicels1.emplace_back(paths[index], vehicelSpeed, x1, y1, REGULAR);
                index = (index + 1) % paths.size();
                dir.vehicleTimer1.restart();
            }

            if (currentState == State::PEAK &&
                dir.vehicleTimer2.getElapsedTime().asSeconds() >= spawnInterval &&
                canSpawn(dir.vehicels2, dir.name == "east" || dir.name == "west" ? x2 : y2, dir.name))
            {
                dir.vehicels2.emplace_back(paths[index], vehicelSpeed, x2, y2 + offset, REGULAR);
                index = (index + 1) % paths.size();
                dir.vehicleTimer2.restart();
            }
        };

        auto spawnHeavyVehicle = [&](TrafficInfo &dir, int index, float spawnInterval, float x, float y)
        {
            if (currentState == State::NORMAL &&
                dir.vehicleTimerHeavy.getElapsedTime().asSeconds() >= spawnInterval &&
                canSpawn(dir.vehicels2, x, dir.name))
            {
                dir.vehicels2.emplace_back(heavyPaths[index], heavyVehicelSpeed, x, y, HEAVY);
                dir.vehicleTimerHeavy.restart();
            }
        };

        auto spawnEmergencyVehicle = [&](TrafficInfo &dir, int pathIndex, float x, float y, int probability)
        {
            static constexpr float MIN_EMERGENCY_SPAWN_INTERVAL = 15.0f; // Minimum time between emergency vehicles

            if (dir.emergencyTimer.getElapsedTime().asSeconds() >= MIN_EMERGENCY_SPAWN_INTERVAL &&
                rand() % 100 < probability &&
                canSpawn(dir.emergency, x, dir.name))
            {
                dir.emergencyActive = true;
                dir.emergency.emplace_back(emPaths[pathIndex], emergencyVehicelSpeed, x, y, EMERGENCY);
                dir.emergencyTimer.restart();
            }
        };
        auto spawnOutOfOrderVehicle = [&](TrafficInfo &dir, auto &paths, int pathIndex, float x, float y, float spawnInterval)
        {
            if (dir.vehicleTimerOutofOrder.getElapsedTime().asSeconds() >= spawnInterval)
            {
                dir.emergency.emplace_back(paths[pathIndex], emergencyVehicelSpeed, x, y, OutOfOrder);
                dir.vehicleTimerOutofOrder.restart();
            }
        };

        // EASTBOUND
        {
            std::lock_guard<std::mutex> lock(eastMutex);
            std::unique_lock<std::shared_mutex> vehicleLock(vehicleMutex);
            spawnEmergencyVehicle(east, 0, eastX, emeastY, 10);
            spawnRegularVehicle(east, eastPaths, eastIndex, spawnIntervalEast, eastX, eastY, eastheavyX, eastheavyY, 50);
            spawnHeavyVehicle(east, 0, spawnIntervalHeavy, eastheavyX, eastheavyY);
            spawnOutOfOrderVehicle(east, eastPaths, 0, eastX, emeastY, spawnIntervalOutofOrderEast);
        }

        // WESTBOUND
        {
            std::lock_guard<std::mutex> lock(westMutex);
            std::unique_lock<std::shared_mutex> vehicleLock(vehicleMutex);
            spawnEmergencyVehicle(west, 1, westX, emwestY, 10);
            spawnRegularVehicle(west, westPaths, westIndex, spawnIntervalWest, westX, westY, westheavyX, westheavyY, 50);
            spawnHeavyVehicle(west, 1, spawnIntervalHeavy, westheavyX, westheavyY);
            spawnOutOfOrderVehicle(west, westPaths, 0, westX, emwestY, spawnIntervalOutofOrderWest);
        }

        // NORTHBOUND
        {
            std::lock_guard<std::mutex> lock(northMutex);
            std::unique_lock<std::shared_mutex> vehicleLock(vehicleMutex);
            spawnEmergencyVehicle(north, 2, emNorthX, northY, 10);
            spawnRegularVehicle(north, northPaths, northIndex, spawnIntervalNorth, northX, northY, northheavyX, northheavyY, 50);
            spawnHeavyVehicle(north, 2, spawnIntervalHeavy, northheavyX, northheavyY);
            spawnOutOfOrderVehicle(north, northPaths, 0, emNorthX, northY, spawnIntervalOutofOrderNorth);
        }

        // SOUTHBOUND
        {
            std::lock_guard<std::mutex> lock(southMutex);
            std::unique_lock<std::shared_mutex> vehicleLock(vehicleMutex);
            spawnEmergencyVehicle(south, 3, emsouthX, southY, 10);
            spawnRegularVehicle(south, southPaths, southIndex, spawnIntervalSouth, southX, southY, southheavyX, southheavyY, 50);
            spawnHeavyVehicle(south, 3, spawnIntervalHeavy, southheavyX, southheavyY);
            spawnOutOfOrderVehicle(south, southPaths, 0, emsouthX, southY, spawnIntervalOutofOrderSouth);
        }
    }

    void viewTrafficStatus(sf::RenderWindow &window)
    {
        static sf::Font font;
        static bool fontLoaded = false;

        // Load font only once
        if (!fontLoaded)
        {
            if (!font.loadFromFile("Arial.ttf"))
            {
                throw std::runtime_error("Failed to load Arial.ttf");
            }
            fontLoaded = true;
        }

        // Helper function to create and position text
        auto createText = [](const std::string &str, unsigned int size, sf::Color color, float x, float y)
        {
            sf::Text text(str, font, size);
            text.setFillColor(color);
            text.setPosition(x, y);
            return text;
        };

        // Create header texts
        auto title = createText("Traffic Statistics", 80, sf::Color::White,
                                window.getSize().x / 2 - 300, 20);

        // Create labels
        std::vector<sf::Text> labels = {
            createText("Vehicle Count", 50, sf::Color::White, 100, 300),
            createText("Regular Vehicle Count", 50, sf::Color::White, 100, 400),
            createText("Heavy Vehicle Count", 50, sf::Color::White, 100, 500),
            createText("Emergency Vehicle Count", 50, sf::Color::White, 100, 600),
            createText("Vehicles with Challan Pending", 50, sf::Color::White, 100, 700)};

        // Create direction headers
        std::vector<sf::Text> directions = {
            createText("East", 50, sf::Color::White, 800, 200),
            createText("West", 50, sf::Color::White, 1000, 200),
            createText("North", 50, sf::Color::White, 1200, 200),
            createText("South", 50, sf::Color::White, 1400, 200)};

        // Calculate statistics for each direction
        struct DirectionStats
        {
            int regular;
            int heavy;
            int emergency;
            int outOfOrder;
        };

        auto calculateStats = [](const TrafficInfo &dir) -> DirectionStats
        {
            DirectionStats stats{0, 0, 0, 0};
            for (const auto &v : dir.vehicels2)
            {
                if (v.type == REGULAR)
                    stats.regular++;
                else if (v.type == HEAVY)
                    stats.heavy++;
            }
            stats.emergency = dir.emergency.size();
            stats.outOfOrder = std::count_if(dir.emergency.begin(), dir.emergency.end(),
                                             [](const Vehicle &v)
                                             { return v.type == OutOfOrder; });
            stats.regular += dir.vehicels1.size();
            return stats;
        };

        DirectionStats eastStats = calculateStats(east);
        DirectionStats westStats = calculateStats(west);
        DirectionStats northStats = calculateStats(north);
        DirectionStats southStats = calculateStats(south);

        // Create and position count displays
        std::vector<sf::Text> regularCounts = {
            createText(std::to_string(eastStats.regular), 50, sf::Color::Cyan, 800, 400),
            createText(std::to_string(westStats.regular), 50, sf::Color::Blue, 1000, 400),
            createText(std::to_string(northStats.regular), 50, sf::Color::Magenta, 1200, 400),
            createText(std::to_string(southStats.regular), 50, sf::Color::Yellow, 1400, 400)};

        std::vector<sf::Text> heavyCounts = {
            createText(std::to_string(eastStats.heavy), 50, sf::Color::Cyan, 800, 500),
            createText(std::to_string(westStats.heavy), 50, sf::Color::Blue, 1000, 500),
            createText(std::to_string(northStats.heavy), 50, sf::Color::Magenta, 1200, 500),
            createText(std::to_string(southStats.heavy), 50, sf::Color::Yellow, 1400, 500)};

        std::vector<sf::Text> emergencyCounts = {
            createText(std::to_string(eastStats.emergency), 50, sf::Color::Cyan, 800, 600),
            createText(std::to_string(westStats.emergency), 50, sf::Color::Blue, 1000, 600),
            createText(std::to_string(northStats.emergency), 50, sf::Color::Magenta, 1200, 600),
            createText(std::to_string(southStats.emergency), 50, sf::Color::Yellow, 1400, 600)};

        std::vector<sf::Text> outOfOrderCounts = {
            createText(std::to_string(eastStats.outOfOrder), 50, sf::Color::Cyan, 800, 700),
            createText(std::to_string(westStats.outOfOrder), 50, sf::Color::Blue, 1000, 700),
            createText(std::to_string(northStats.outOfOrder), 50, sf::Color::Magenta, 1200, 700),
            createText(std::to_string(southStats.outOfOrder), 50, sf::Color::Yellow, 1400, 700)};

        // Instructions text
        auto instructions = createText("Press SPACE to return to simulation", 30, sf::Color::Green,
                                       window.getSize().x / 2 - 200, window.getSize().y - 50);

        // Draw all elements
        window.draw(title);

        for (const auto &label : labels)
            window.draw(label);
        for (const auto &dir : directions)
            window.draw(dir);
        for (const auto &count : regularCounts)
            window.draw(count);
        for (const auto &count : heavyCounts)
            window.draw(count);
        for (const auto &count : emergencyCounts)
            window.draw(count);
        for (const auto &count : outOfOrderCounts)
            window.draw(count);

        window.draw(instructions);
    }
    void start(sf::RenderWindow &window)
    {
        srand(static_cast<unsigned>(time(0)));

        std::atomic<bool> isPaused{false};
        std::atomic<bool> isLightControllerRunning{true};
        bool isViewingTrafficStatus = false; // Toggle to track the traffic status screen

        // Start simulation thread
        std::thread simulationThread(&Simulation::runSimulation, this, std::ref(isSimulationRunning));

        // Start light controller thread
        std::thread lightControllerThread([this, &isLightControllerRunning]()
                                          {
        sf::Clock lightClock;
        while (isLightControllerRunning)
        {
            float deltaTime = lightClock.restart().asSeconds();
            lightController.update(deltaTime); // Update lights
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        } });

        // Main render loop
        while (window.isOpen())
        {
            sf::Event event;
            while (window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                {
                    isSimulationRunning = false;      // Signal simulation thread to stop
                    isLightControllerRunning = false; // Signal light controller thread to stop
                    window.close();
                }
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)
                {
                    isViewingTrafficStatus = !isViewingTrafficStatus;
                }
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::P)
                {
                    currentState = State::PEAK;
                }
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::N)
                {
                    currentState = State::NORMAL;
                }
            }

            if (isViewingTrafficStatus)
            {
                window.clear(sf::Color::Black);

                viewTrafficStatus(window);
            }
            else
            {
                window.clear(sf::Color::Green);
                // Draw background and static elements
                window.draw(background);
                for (const auto &line : lines)
                {
                    window.draw(line.shape);
                }

                // Safely draw vehicles using shared lock
                {
                    std::shared_lock<std::shared_mutex> lock(vehicleMutex);
                    for (const auto &vehicle : east.vehicels1)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : west.vehicels1)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : north.vehicels1)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : south.vehicels1)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : east.vehicels2)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : west.vehicels2)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : north.vehicels2)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : south.vehicels2)
                        window.draw(vehicle.sprite);

                    for (const auto &vehicle : east.emergency)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : west.emergency)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : north.emergency)
                        window.draw(vehicle.sprite);
                    for (const auto &vehicle : south.emergency)
                        window.draw(vehicle.sprite);
                }

                lightController.draw(window);
            }
            window.display();

            // Frame rate limiting to prevent excessive CPU usage
            sf::sleep(sf::milliseconds(16)); // ~60 FPS
        }

        // Cleanup
        if (simulationThread.joinable())
        {
            simulationThread.join();
        }
        if (lightControllerThread.joinable())
        {
            isLightControllerRunning = false; // Ensure thread stops gracefully
            lightControllerThread.join();
        }
    }

private:
    void runSimulation(std::atomic<bool> &isRunning)
    {
        sf::Clock simulationClock;
        float spawnTimer = 0.0f;
        constexpr float SPAWN_INTERVAL = 1.0f;

        while (isRunning)
        {
            float deltaTime = simulationClock.restart().asSeconds();

            // Update vehicle positions with exclusive lock
            {
                std::unique_lock<std::shared_mutex> lock(vehicleMutex);
                moveVehicles(deltaTime);
            }

            // Handle spawning at regular intervals
            spawnTimer += deltaTime;
            if (spawnTimer >= SPAWN_INTERVAL)
            {
                spawnTimer = 0.0f;
                spawnVehicles();
            }

            // Cleanup with exclusive lock
            {
                std::unique_lock<std::shared_mutex> lock(vehicleMutex);
                manageVehicleMemory();
            }

            // Prevent tight loop
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
};