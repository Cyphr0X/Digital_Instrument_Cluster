#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "Shader.h"
#include "Gauge.h"

// Window dimensions and called also aspect ratio
const unsigned int WIDTH = 1360;
const unsigned int HEIGHT = 768;

// Vehicle state
struct VehicleState {

	// speed of the vehicle in km/h set to 0.0f initially which 0 km/h 
    float speed = 0.0f;
	// RPM of the engine in revolutions per minute set to 800.0f initially which is idle RPM
    float rpm = 800.0f; // Idle RPM
	// Fuel level in percentage set to 85.0f initially which is 85% fuel level
    float fuel = 85.0f;
	// Engine temperature in degrees Celsius set to 90.0f initially which is normal operating temperature which is 90 degrees Celsius
    float engineTemp = 90.0f;
	// oil pressure in PSI set to 45.0f initially which is normal oil pressure
    float oilPressure = 45.0f;
	// Battery voltage in volts set to 12.6f initially which is normal battery voltage
    float batteryVoltage = 12.6f;
    bool engineRunning = false;
    bool acOn = false;
    bool lightsOn = false;
    bool turnSignalLeft = false;
    bool turnSignalRight = false;
    bool hazardsOn = false;
    bool parkingBrake = true;
    bool seatbelt = false;
	// Doors status, false means closed, true means open
    bool doors[4] = { false, false, false, false }; // FL, FR, RL, RR
    int gear = 0; // P=0, R=-1, N=0, D=1-8
    int displayMode = 0; // 0=Normal, 1=Sport, 2=Eco, 3=Comfort
	// Odometer in kilometers set to 45672.8f initially which is 45672.8 km
    float odometer = 45672.8f;
	// Trip A in kilometers set to 0.0f initially which is 0 km
    float tripA = 0.0f;
	// Trip B in kilometers set to 158.3f initially which is 158.3 km
    float tripB = 158.3f;
	// Average fuel consumption in liters per 100 km set to 7.2f initially which is 7.2 L/100km
    float avgFuelConsumption = 7.2f;
    float outsideTemp = 22.5f;
    int timeHour = 14;
    int timeMinute = 23;
    bool throttlePressed = false;
    float targetSpeed = 0.0f;
	// Target revolutions per minute (RPM) for the engine, set to 800.0f initially which is idle RPM
    float targetRPM = 800.0f;
};

VehicleState vehicle;
// Last time the frame was updated
double lastTime = 0.0;
// Blink timer for turn signals and hazards
float blinkTimer = 0.0f;

// Vertex shader with text rendering support
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform float rotation;
uniform vec2 offset;
uniform vec2 scale;

void main()
{
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec2 rotatedPos = vec2(
        aPos.x * cosR - aPos.y * sinR,
        aPos.x * sinR + aPos.y * cosR
    );

    vec2 scaledPos = rotatedPos * scale;
    vec2 finalPos = scaledPos + offset;

    float x = finalPos.x / 500.0;
    float y = finalPos.y / 300.0;

    gl_Position = vec4(x, y, 0.0, 1.0);
}
)";

// Fragment shader
const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;
uniform float alpha;

void main()
{
    FragColor = vec4(color, alpha);
}
)";

void processInput(GLFWwindow* window, float deltaTime) {
	// Check if the ESC key was pressed to close the window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Throttle control
    bool currentThrottle = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    vehicle.throttlePressed = currentThrottle;

    if (currentThrottle && vehicle.engineRunning) {
        vehicle.targetSpeed = std::min(vehicle.targetSpeed + 50.0f * deltaTime, 250.0f);
        vehicle.targetRPM = std::min(vehicle.targetRPM + 2000.0f * deltaTime, 7000.0f);
    }
    else {
        vehicle.targetSpeed = std::max(vehicle.targetSpeed - 30.0f * deltaTime, 0.0f);
        if (vehicle.engineRunning) {
            vehicle.targetRPM = std::max(vehicle.targetRPM - 1500.0f * deltaTime, 800.0f);
        }
        else {
            vehicle.targetRPM = 0.0f;
        }
    }

    // Smooth transitions
    vehicle.speed += (vehicle.targetSpeed - vehicle.speed) * 5.0f * deltaTime;
    vehicle.rpm += (vehicle.targetRPM - vehicle.rpm) * 3.0f * deltaTime;

    // Mode switching
    static bool qPressed = false, ePressed = false;
    bool qCurrent = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
    bool eCurrent = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (qCurrent && !qPressed) {
        vehicle.displayMode = (vehicle.displayMode - 1 + 4) % 4;
    }
    if (eCurrent && !ePressed) {
        vehicle.displayMode = (vehicle.displayMode + 1) % 4;
    }
    qPressed = qCurrent;
    ePressed = eCurrent;

    // Vehicle controls
    static bool keyStates[256] = { false };

    // Engine start/stop
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS && !keyStates[GLFW_KEY_I]) {
        vehicle.engineRunning = !vehicle.engineRunning;
        if (!vehicle.engineRunning) {
            vehicle.targetRPM = 0.0f;
            vehicle.targetSpeed = 0.0f;
        }
        else {
            vehicle.targetRPM = 800.0f;
        }
    }

    // AC toggle
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !keyStates[GLFW_KEY_A]) {
        vehicle.acOn = !vehicle.acOn;
    }

    // Lights toggle
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !keyStates[GLFW_KEY_L]) {
        vehicle.lightsOn = !vehicle.lightsOn;
    }

    // Turn signals
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS && !keyStates[GLFW_KEY_LEFT]) {
        vehicle.turnSignalLeft = !vehicle.turnSignalLeft;
        if (vehicle.turnSignalLeft) vehicle.turnSignalRight = false;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS && !keyStates[GLFW_KEY_RIGHT]) {
        vehicle.turnSignalRight = !vehicle.turnSignalRight;
        if (vehicle.turnSignalRight) vehicle.turnSignalLeft = false;
    }

    // Hazards
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !keyStates[GLFW_KEY_H]) {
        vehicle.hazardsOn = !vehicle.hazardsOn;
        if (vehicle.hazardsOn) {
            vehicle.turnSignalLeft = false;
            vehicle.turnSignalRight = false;
        }
    }

    // Parking brake
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !keyStates[GLFW_KEY_P]) {
        vehicle.parkingBrake = !vehicle.parkingBrake;
    }

    // Seatbelt
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !keyStates[GLFW_KEY_B]) {
        vehicle.seatbelt = !vehicle.seatbelt;
    }

    // Update key states
    for (int i = 0; i < 256; i++) {
        keyStates[i] = glfwGetKey(window, i) == GLFW_PRESS;
    }

    // Update timers and dynamic values
    blinkTimer += deltaTime;
    if (blinkTimer >= 1.0f) blinkTimer = 0.0f;

    // Simulate fuel consumption
    if (vehicle.engineRunning && vehicle.speed > 0) {
        vehicle.fuel -= 0.5f * deltaTime * (vehicle.speed / 100.0f);
        vehicle.fuel = std::max(vehicle.fuel, 0.0f);

        vehicle.tripA += vehicle.speed * deltaTime / 3600.0f; // km/h to km
    }

    // Engine temperature simulation
    if (vehicle.engineRunning) {
        float targetTemp = 90.0f + (vehicle.rpm - 800.0f) / 100.0f;
        vehicle.engineTemp += (targetTemp - vehicle.engineTemp) * 0.5f * deltaTime;
    }
    else {
        vehicle.engineTemp += (20.0f - vehicle.engineTemp) * 0.1f * deltaTime;
    }
}

void drawRectangle(const Shader& shader, float x, float y, float width, float height, float r, float g, float b, float a = 1.0f) {
    float vertices[] = {
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    shader.use();
    shader.setFloat("rotation", 0.0f);
    shader.setVec2("offset", 0.0f, 0.0f);
    shader.setVec2("scale", 1.0f, 1.0f);
    shader.setVec3("color", r, g, b);
    shader.setFloat("alpha", a);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void drawWarningLight(const Shader& shader, float x, float y, float size, bool active, float r, float g, float b) {
    if (active) {
        float alpha = (blinkTimer < 0.5f) ? 1.0f : 0.3f;
        drawRectangle(shader, x, y, size, size, r, g, b, alpha);
    }
    else {
        drawRectangle(shader, x, y, size, size, 0.2f, 0.2f, 0.2f, 0.3f);
    }
}

void drawDigitalDisplay(const Shader& shader) {
    // Main display background
    drawRectangle(shader, -200, 150, 400, 100, 0.1f, 0.1f, 0.1f);

    // Mode indicator
    const char* modes[] = { "COMFORT", "SPORT", "ECO", "INDIVIDUAL" };
    float modeColors[][3] = { {0.0f, 0.8f, 1.0f}, {1.0f, 0.3f, 0.0f}, {0.0f, 1.0f, 0.3f}, {0.8f, 0.0f, 1.0f} };

    drawRectangle(shader, -180, 180, 80, 30, modeColors[vehicle.displayMode][0],
        modeColors[vehicle.displayMode][1], modeColors[vehicle.displayMode][2]);

    // Gear indicator
    drawRectangle(shader, -50, 180, 60, 40, 0.2f, 0.2f, 0.2f);
    if (vehicle.gear == 0) {
        drawRectangle(shader, -40, 190, 40, 20, 0.0f, 1.0f, 0.0f); // P
    }
    else if (vehicle.gear == -1) {
        drawRectangle(shader, -40, 190, 40, 20, 1.0f, 1.0f, 0.0f); // R
    }
    else if (vehicle.gear > 0) {
        drawRectangle(shader, -40, 190, 40, 20, 0.0f, 0.8f, 1.0f); // D
    }

    // Time display
    drawRectangle(shader, 80, 180, 100, 30, 0.0f, 0.5f, 1.0f);

    // Temperature and other info
    drawRectangle(shader, -150, 120, 60, 20, vehicle.outsideTemp < 5 ? 0.0f : 0.8f,
        vehicle.outsideTemp < 5 ? 0.8f : 1.0f,
        vehicle.outsideTemp < 5 ? 1.0f : 0.0f);
}

void drawWarningPanel(const Shader& shader) {
    float y = -150;
    float size = 25;
    float spacing = 35;
    float x = -400;

    // Engine warning
    drawWarningLight(shader, x, y, size, !vehicle.engineRunning && vehicle.speed > 0, 1.0f, 0.0f, 0.0f);
    x += spacing;

    // Oil pressure
    drawWarningLight(shader, x, y, size, vehicle.oilPressure < 20, 1.0f, 0.5f, 0.0f);
    x += spacing;

    // Engine temperature
    drawWarningLight(shader, x, y, size, vehicle.engineTemp > 110, 1.0f, 0.0f, 0.0f);
    x += spacing;

    // Battery
    drawWarningLight(shader, x, y, size, vehicle.batteryVoltage < 12.0f, 1.0f, 1.0f, 0.0f);
    x += spacing;

    // Fuel
    drawWarningLight(shader, x, y, size, vehicle.fuel < 10, 1.0f, 0.5f, 0.0f);
    x += spacing;

    // AC indicator
    drawWarningLight(shader, x, y, size, vehicle.acOn, 0.0f, 0.8f, 1.0f);
    x += spacing;

    // Lights
    drawWarningLight(shader, x, y, size, vehicle.lightsOn, 0.0f, 1.0f, 0.0f);
    x += spacing;

    // Turn signals
    bool leftBlink = vehicle.turnSignalLeft || vehicle.hazardsOn;
    bool rightBlink = vehicle.turnSignalRight || vehicle.hazardsOn;
    drawWarningLight(shader, x, y, size, leftBlink && blinkTimer < 0.5f, 0.0f, 1.0f, 0.0f);
    x += spacing;
    drawWarningLight(shader, x, y, size, rightBlink && blinkTimer < 0.5f, 0.0f, 1.0f, 0.0f);
    x += spacing;

    // Parking brake
    drawWarningLight(shader, x, y, size, vehicle.parkingBrake, 1.0f, 0.0f, 0.0f);
    x += spacing;

    // Seatbelt
    drawWarningLight(shader, x, y, size, !vehicle.seatbelt && vehicle.speed > 0, 1.0f, 0.0f, 0.0f);
    x += spacing;

    // ABS (always off in this simulation)
    drawWarningLight(shader, x, y, size, false, 1.0f, 1.0f, 0.0f);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Mercedes-Benz Instrument Cluster", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader shader(vertexShaderSrc, fragmentShaderSrc);

    // Create gauges with different positions and sizes
    Gauge speedometer(-250.0f, -50.0f, 120.0f);
    Gauge tachometer(250.0f, -50.0f, 120.0f);
    Gauge fuelGauge(-400.0f, 50.0f, 60.0f);
    Gauge tempGauge(400.0f, 50.0f, 60.0f);

    glLineWidth(3.0f);
    lastTime = glfwGetTime();

    std::cout << "Mercedes-Benz Instrument Cluster Controls:\n";
    std::cout << "SPACE - Throttle\n";
    std::cout << "Q/E - Switch display modes\n";
    std::cout << "I - Engine start/stop\n";
    std::cout << "A - AC toggle\n";
    std::cout << "L - Lights toggle\n";
    std::cout << "LEFT/RIGHT - Turn signals\n";
    std::cout << "H - Hazard lights\n";
    std::cout << "P - Parking brake\n";
    std::cout << "B - Seatbelt\n";
    std::cout << "ESC - Exit\n\n";

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = float(currentTime - lastTime);
        lastTime = currentTime;

        processInput(window, deltaTime);

        // Background color based on mode
        float bgColors[][3] = { {0.02f, 0.02f, 0.05f}, {0.05f, 0.02f, 0.02f}, {0.02f, 0.05f, 0.02f}, {0.05f, 0.02f, 0.05f} };
        glClearColor(bgColors[vehicle.displayMode][0], bgColors[vehicle.displayMode][1], bgColors[vehicle.displayMode][2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Calculate gauge angles
        float speedAngle = (-135.0f + (vehicle.speed / 250.0f) * 270.0f) * M_PI / 180.0f;
        float rpmAngle = (-135.0f + (vehicle.rpm / 8000.0f) * 270.0f) * M_PI / 180.0f;
        float fuelAngle = (-135.0f + (vehicle.fuel / 100.0f) * 270.0f) * M_PI / 180.0f;
        float tempAngle = (-135.0f + ((vehicle.engineTemp - 60.0f) / 80.0f) * 270.0f) * M_PI / 180.0f;

        // Draw main gauges
        speedometer.draw(shader, speedAngle);
        tachometer.draw(shader, rpmAngle);
        fuelGauge.draw(shader, fuelAngle);
        tempGauge.draw(shader, tempAngle);

        // Draw digital displays and warning lights
        drawDigitalDisplay(shader);
        drawWarningPanel(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}