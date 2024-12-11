#include <SFML/Graphics.hpp>
#include <vector>
#include <map>

using namespace sf;
using namespace std;
enum State
{
    RED,
    YELLOW,
    GREEN
};
struct LightCircle
{
    CircleShape circle;
    State state;
};

struct TrafficLight
{

    vector<LightCircle> lights;
    State currentState;
    RectangleShape frame; // Optional frame for visual enhancement

    TrafficLight(Vector2f position, bool horizontal = false, float lightSpacing = 50.0f)
        : currentState(RED)
    {
        const Color defaultColor(50, 50, 50); // Default "off" color
        const float rad = lightSpacing / 2.0f;

        // Create frame for traffic light
        float width = horizontal ? 3 * lightSpacing : lightSpacing;
        float height = horizontal ? lightSpacing : 3 * lightSpacing;
        frame.setSize(Vector2f(width, height));
        frame.setFillColor(Color(30, 30, 30));
        frame.setOutlineThickness(2);
        frame.setOutlineColor(Color::Black);
        frame.setOrigin(width / 2, height / 2);
        frame.setPosition(position);

        // Create lights
        for (int i = 0; i < 3; ++i)
        {
            LightCircle light;
            light.circle.setRadius(rad);
            light.circle.setOrigin(rad, rad);
            light.circle.setFillColor(defaultColor);

            Vector2f lightPos = horizontal
                                    ? Vector2f(position.x + i * lightSpacing - width / 2 + rad, position.y)
                                    : Vector2f(position.x, position.y + i * lightSpacing - height / 2 + rad);
            light.circle.setPosition(lightPos);

            // Assign states
            switch (i)
            {
            case 0:
                light.state = RED;
                break;
            case 1:
                light.state = YELLOW;
                break;
            case 2:
                light.state = GREEN;
                break;
            }
            lights.push_back(light);
        }

        updateLightColors(RED); // Set initial light state
    }

    void updateLightColors(State activeState)
    {
        currentState = activeState; // Update current state
        const Color offColor(50, 50, 50);

        for (auto &light : lights)
        {
            if (light.state == activeState)
            {
                switch (light.state)
                {
                case RED:
                    light.circle.setFillColor(Color::Red);
                    break;
                case YELLOW:
                    light.circle.setFillColor(Color::Yellow);
                    break;
                case GREEN:
                    light.circle.setFillColor(Color::Green);
                    break;
                }
            }
            else
            {
                light.circle.setFillColor(offColor);
            }
        }
    }

    void draw(RenderWindow &window) const
    {
        window.draw(frame); // Draw frame
        for (const auto &light : lights)
        {
            window.draw(light.circle);
        }
    }
};
class TrafficController
{
public:
    // static constexpr float EMERGENCY_ADJUSTMENT = 1.0f;
    // static constexpr float YELLOW_EMERGENCY_FACTOR = 0.5f;

    enum class LightPhase
    {
        NORTH_SOUTH_GREEN,
        NORTH_SOUTH_YELLOW,
        EAST_WEST_GREEN,
        EAST_WEST_YELLOW
    };

    struct PhaseConfig
    {
        float duration;
        State northSouthState;
        State eastWestState;
    };

private:
    TrafficLight northSouthLight;
    TrafficLight eastWestLight;
    LightPhase currentPhase;
    float phaseTimer;

    const std::map<LightPhase, PhaseConfig> phaseMap = {
        {LightPhase::NORTH_SOUTH_GREEN, {6.0f, State::GREEN, State::RED}},
        {LightPhase::NORTH_SOUTH_YELLOW, {3.0f, State::YELLOW, State::RED}},
        {LightPhase::EAST_WEST_GREEN, {6.0f, State::RED, State::GREEN}},
        {LightPhase::EAST_WEST_YELLOW, {3.0f, State::RED, State::YELLOW}},
    };

public:
    TrafficController(const Vector2f &northSouthPosition, const Vector2f &eastWestPosition)
        : northSouthLight(northSouthPosition, false),
          eastWestLight(eastWestPosition, true),
          currentPhase(LightPhase::EAST_WEST_GREEN),
          phaseTimer(0.0f)
    {
    }

    std::string getEastWestState() const
    {
        return stateToString(phaseMap.at(currentPhase).eastWestState);
    }

    std::string getNorthSouthState() const
    {
        return stateToString(phaseMap.at(currentPhase).northSouthState);
    }

    void update(float deltaTime)
    {
        phaseTimer += deltaTime;

        const auto &config = phaseMap.at(currentPhase);

        if (phaseTimer >= config.duration)
        {
            // Transition to the next phase
            phaseTimer = 0.0f;
            switch (currentPhase)
            {
            case LightPhase::NORTH_SOUTH_GREEN:
                currentPhase = LightPhase::NORTH_SOUTH_YELLOW;
                break;
            case LightPhase::NORTH_SOUTH_YELLOW:
                currentPhase = LightPhase::EAST_WEST_GREEN;
                break;
            case LightPhase::EAST_WEST_GREEN:
                currentPhase = LightPhase::EAST_WEST_YELLOW;
                break;
            case LightPhase::EAST_WEST_YELLOW:
                currentPhase = LightPhase::NORTH_SOUTH_GREEN;
                break;
            }
        }

        // Update lights based on the current phase
        northSouthLight.updateLightColors(config.northSouthState);
        eastWestLight.updateLightColors(config.eastWestState);
    }

    void draw(RenderWindow &window)
    {
        northSouthLight.draw(window);
        eastWestLight.draw(window);
    }

private:
    static std::string stateToString(State state)
    {
        switch (state)
        {
        case State::GREEN:
            return "green";
        case State::YELLOW:
            return "yellow";
        case State::RED:
            return "red";
        default:
            return "unknown";
        }
    }

    void transitionToNextPhase()
    {
        switch (currentPhase)
        {
        case LightPhase::NORTH_SOUTH_GREEN:
            currentPhase = LightPhase::NORTH_SOUTH_YELLOW;
            break;
        case LightPhase::NORTH_SOUTH_YELLOW:
            currentPhase = LightPhase::EAST_WEST_GREEN;
            break;
        case LightPhase::EAST_WEST_GREEN:
            currentPhase = LightPhase::EAST_WEST_YELLOW;
            break;
        case LightPhase::EAST_WEST_YELLOW:
            currentPhase = LightPhase::NORTH_SOUTH_GREEN;
            break;
        }
    }
};