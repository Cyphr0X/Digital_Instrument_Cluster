#ifndef GAUGE_H
#define GAUGE_H

#include <glad/glad.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


class Shader; // Forward declaration

enum class GaugeType {
    FULL_CIRCLE,    // 270° sweep from -135° to +135°
    QUADRANT_1,     // 90° sweep from 0° to 90° (fuel)
    QUADRANT_4      // 90° sweep from 270° to 360° (temp)
};

class Gauge {
public:
    Gauge(float xOffset, float yOffset, float radius, GaugeType type = GaugeType::FULL_CIRCLE);
    ~Gauge();

    void draw(const Shader& shader, float needleRotationRadians, bool isMainGauge = false);

    // Get the correct angle for a value (0.0 to 1.0 normalized)
    float getAngleForValue(float normalizedValue) const;

private:
    void setupCircle();
    void setupNeedle();
    void setupTicks();
    void setupGlow();

    // OpenGL objects
    GLuint circleVAO, circleVBO;
    GLuint needleVAO, needleVBO;
    GLuint ticksVAO, ticksVBO;
    GLuint glowVAO, glowVBO;

    // Gauge properties
    float offsetX, offsetY, radius;
    GaugeType gaugeType;
    int tickCount;
    int glowVertexCount;

    // Angle parameters based on gauge type
    float startAngle, endAngle, sweep;

    void calculateAngleParams();
};

#endif