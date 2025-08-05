#include "Gauge.h"
#include "Shader.h"

#include <vector>

Gauge::Gauge(float xOffset, float yOffset, float radius)
    : offsetX(xOffset), offsetY(yOffset), radius(radius)
{
    setupCircle();
    setupNeedle();
    setupTicks();
}

Gauge::~Gauge() {
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &needleVAO);
    glDeleteBuffers(1, &needleVBO);
    glDeleteVertexArrays(1, &ticksVAO);
    glDeleteBuffers(1, &ticksVBO);
}

void Gauge::setupCircle() {
    const int segments = 100;
    std::vector<float> vertices;

    // Outer circle
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        vertices.push_back(radius * cos(angle));
        vertices.push_back(radius * sin(angle));
    }

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Gauge::setupNeedle() {
    float needleVertices[] = {
        0.0f, 0.0f,
        radius * 0.85f, 0.0f,  // Main needle
        radius * 0.85f, 0.0f,
        radius * 0.75f, radius * 0.05f,  // Needle tip
        radius * 0.85f, 0.0f,
        radius * 0.75f, -radius * 0.05f,  // Needle tip
        0.0f, 0.0f,
        -radius * 0.15f, 0.0f  // Counter weight
    };

    glGenVertexArrays(1, &needleVAO);
    glGenBuffers(1, &needleVBO);

    glBindVertexArray(needleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, needleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(needleVertices), needleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Gauge::setupTicks() {
    std::vector<float> tickVertices;
    const int majorTicks = 10;   // Number of major ticks
    const int minorTicks = 30;   // Total minor tick steps
    const float startAngle = -135.0f;  // Start of sweep
    const float endAngle = 135.0f;     // End of sweep
    const float sweep = endAngle - startAngle;

    // ---- Major tick marks ----
    for (int i = 0; i <= majorTicks; i++) {
        float angleDeg = startAngle - (sweep * i / majorTicks);
        float angle = angleDeg * M_PI / 180.0f;

        float innerRadius = radius * 0.85f;
        float outerRadius = radius * 0.95f;

        float cosA = cos(angle);
        float sinA = sin(angle);

        tickVertices.push_back(innerRadius * cosA);
        tickVertices.push_back(innerRadius * sinA);
        tickVertices.push_back(outerRadius * cosA);
        tickVertices.push_back(outerRadius * sinA);
    }

    // ---- Minor tick marks ----
    for (int i = 0; i <= minorTicks; i++) {
        float angleDeg = startAngle - (sweep * i / minorTicks);

        // Skip if it's a major tick position
        bool isMajorTick = false;
        for (int j = 0; j <= majorTicks; j++) {
            float majorAngle = startAngle + (sweep * j / majorTicks);
            if (fabs(angleDeg - majorAngle) < 0.5f) {  // tolerance
                isMajorTick = true;
                break;
            }
        }

        if (!isMajorTick) {
            float angle = angleDeg * M_PI / 180.0f;
            float innerRadius = radius * 0.88f;
            float outerRadius = radius * 0.93f;

            float cosA = cos(angle);
            float sinA = sin(angle);

            tickVertices.push_back(innerRadius * cosA);
            tickVertices.push_back(innerRadius * sinA);
            tickVertices.push_back(outerRadius * cosA);
            tickVertices.push_back(outerRadius * sinA);
        }
    }

    // ---- Send to OpenGL ----
    glGenVertexArrays(1, &ticksVAO);
    glGenBuffers(1, &ticksVBO);

    glBindVertexArray(ticksVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ticksVBO);
    glBufferData(GL_ARRAY_BUFFER, tickVertices.size() * sizeof(float), tickVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    tickCount = tickVertices.size() / 2;
}


void Gauge::draw(const Shader& shader, float needleRotationRadians) {
    shader.use();
    shader.setVec2("scale", 1.0f, 1.0f);

    // Draw outer ring (chrome effect)
    shader.setFloat("rotation", 0.0f);
    shader.setVec2("offset", offsetX, offsetY);
    shader.setVec3("color", 0.7f, 0.7f, 0.8f);
    shader.setFloat("alpha", 1.0f);

    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 102);

    // Draw inner circle (black background)
    shader.setVec2("scale", 0.9f, 0.9f);
    shader.setVec3("color", 0.05f, 0.05f, 0.08f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 102);

    // Draw tick marks
    shader.setVec2("scale", 1.0f, 1.0f);
    shader.setVec3("color", 0.9f, 0.9f, 0.9f);
    glBindVertexArray(ticksVAO);
    glDrawArrays(GL_LINES, 0, tickCount);

    // Draw needle
    shader.setFloat("rotation", needleRotationRadians);
    shader.setVec3("color", 1.0f, 0.2f, 0.1f);
    glBindVertexArray(needleVAO);
    glDrawArrays(GL_LINES, 0, 8);

    // Draw center hub
    shader.setFloat("rotation", 0.0f);
    shader.setVec2("scale", 0.08f, 0.08f);
    shader.setVec3("color", 0.3f, 0.3f, 0.35f);
    glBindVertexArray(circleVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
}