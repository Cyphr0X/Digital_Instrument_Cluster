#include "Gauge.h"
#include "Shader.h"
#include <vector>
#include <algorithm>

Gauge::Gauge(float xOffset, float yOffset, float radius, GaugeType type)
    : offsetX(xOffset), offsetY(yOffset), radius(radius), gaugeType(type)
{
    calculateAngleParams();
    setupCircle();
    setupNeedle();
    setupTicks();
    setupGlow();
}

Gauge::~Gauge() {
    glDeleteVertexArrays(1, &circleVAO);
    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &needleVAO);
    glDeleteBuffers(1, &needleVBO);
    glDeleteVertexArrays(1, &ticksVAO);
    glDeleteBuffers(1, &ticksVBO);
    glDeleteVertexArrays(1, &glowVAO);
    glDeleteBuffers(1, &glowVBO);
}

void Gauge::calculateAngleParams() {
    switch (gaugeType) {
        case GaugeType::FULL_CIRCLE:
            startAngle = -135.0f;  // Start position
            endAngle = 135.0f;     // End position
            break;
        case GaugeType::QUADRANT_1:
            startAngle = 0.0f;     // Right side
            endAngle = 90.0f;      // Top
            break;
        case GaugeType::QUADRANT_4:
            startAngle = 270.0f;   // Bottom
            endAngle = 360.0f;     // Right side
            break;
    }
    sweep = endAngle - startAngle;
}

float Gauge::getAngleForValue(float normalizedValue) const {
    // Clamp value between 0 and 1
    normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
    
    float angleDeg;
    if (gaugeType == GaugeType::FULL_CIRCLE) {
        // For main gauges (speed/RPM): clockwise - mirrored
        angleDeg = startAngle - (sweep * normalizedValue);
    } else {
        // For partial gauges (fuel/temp): keep original behavior
        angleDeg = startAngle + (sweep * normalizedValue);
    }
    
    return angleDeg * M_PI / 180.0f;
}

void Gauge::setupCircle() {
    const int segments = 100;
    std::vector<float> vertices;

    // Center point
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    if (gaugeType == GaugeType::FULL_CIRCLE) {
        // Full circle for main gauges
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            vertices.push_back(radius * cos(angle));
            vertices.push_back(radius * sin(angle));
        }
    } else {
        // Arc for partial gauges
        int arcSegments = (int)(segments * (sweep / 360.0f));
        for (int i = 0; i <= arcSegments; i++) {
            float angleDeg = startAngle + (sweep * i / arcSegments);
            float angle = angleDeg * M_PI / 180.0f;
            vertices.push_back(radius * cos(angle));
            vertices.push_back(radius * sin(angle));
        }
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
    float needleLength = radius * 0.85f;
    float needleWidth = radius * 0.02f;
    float hubRadius = radius * 0.05f;
    
    std::vector<float> needleVertices;
    
    // Needle line (main part)
    needleVertices.push_back(0.0f);
    needleVertices.push_back(0.0f);
    needleVertices.push_back(needleLength);
    needleVertices.push_back(0.0f);
    
    // Needle triangle tip
    needleVertices.push_back(needleLength);
    needleVertices.push_back(0.0f);
    needleVertices.push_back(needleLength * 0.9f);
    needleVertices.push_back(needleWidth);
    
    needleVertices.push_back(needleLength);
    needleVertices.push_back(0.0f);
    needleVertices.push_back(needleLength * 0.9f);
    needleVertices.push_back(-needleWidth);
    
    // Counter weight
    needleVertices.push_back(0.0f);
    needleVertices.push_back(0.0f);
    needleVertices.push_back(-hubRadius);
    needleVertices.push_back(0.0f);

    glGenVertexArrays(1, &needleVAO);
    glGenBuffers(1, &needleVBO);

    glBindVertexArray(needleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, needleVBO);
    glBufferData(GL_ARRAY_BUFFER, needleVertices.size() * sizeof(float), needleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void Gauge::setupTicks() {
    std::vector<float> tickVertices;
    
    if (gaugeType == GaugeType::FULL_CIRCLE) {
        // Main gauge ticks (speed/RPM) - clockwise (mirrored)
        const int majorTicks = 10;
        const int minorTicksPerMajor = 5;
        const int totalMinorTicks = majorTicks * minorTicksPerMajor;

        // Major ticks - mirrored for clockwise
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

        // Minor ticks - mirrored for clockwise
        for (int i = 0; i <= totalMinorTicks; i++) {
            float angleDeg = startAngle - (sweep * i / totalMinorTicks);
            
            // Skip if it's a major tick position
            bool isMajorTick = (i % minorTicksPerMajor == 0);
            
            if (!isMajorTick) {
                float angle = angleDeg * M_PI / 180.0f;
                float innerRadius = radius * 0.88f;
                float outerRadius = radius * 0.92f;

                float cosA = cos(angle);
                float sinA = sin(angle);

                tickVertices.push_back(innerRadius * cosA);
                tickVertices.push_back(innerRadius * sinA);
                tickVertices.push_back(outerRadius * cosA);
                tickVertices.push_back(outerRadius * sinA);
            }
        }
    } else {
        // Smaller gauge ticks (fuel/temp) - keep original behavior
        const int totalTicks = 6;
        
        for (int i = 0; i <= totalTicks; i++) {
            float angleDeg = startAngle + (sweep * i / totalTicks);
            float angle = angleDeg * M_PI / 180.0f;

            float innerRadius = radius * 0.80f;
            float outerRadius = radius * 0.95f;

            float cosA = cos(angle);
            float sinA = sin(angle);

            tickVertices.push_back(innerRadius * cosA);
            tickVertices.push_back(innerRadius * sinA);
            tickVertices.push_back(outerRadius * cosA);
            tickVertices.push_back(outerRadius * sinA);
        }
    }

    glGenVertexArrays(1, &ticksVAO);
    glGenBuffers(1, &ticksVBO);

    glBindVertexArray(ticksVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ticksVBO);
    glBufferData(GL_ARRAY_BUFFER, tickVertices.size() * sizeof(float), tickVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    tickCount = tickVertices.size() / 2;
}

void Gauge::setupGlow() {
    std::vector<float> glowVertices;
    const int segments = 50;
    
    if (gaugeType == GaugeType::FULL_CIRCLE) {
        // Create glow arc for active portion of the gauge - mirrored for clockwise
        for (int i = 0; i <= segments; i++) {
            float angleDeg = startAngle - (sweep * i / segments);
            float angle = angleDeg * M_PI / 180.0f;
            
            float innerRadius = radius * 0.75f;
            float outerRadius = radius * 0.85f;
            
            float cosA = cos(angle);
            float sinA = sin(angle);
            
            // Inner vertex
            glowVertices.push_back(innerRadius * cosA);
            glowVertices.push_back(innerRadius * sinA);
            
            // Outer vertex
            glowVertices.push_back(outerRadius * cosA);
            glowVertices.push_back(outerRadius * sinA);
        }
    }
    
    glGenVertexArrays(1, &glowVAO);
    glGenBuffers(1, &glowVBO);

    glBindVertexArray(glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, glowVBO);
    glBufferData(GL_ARRAY_BUFFER, glowVertices.size() * sizeof(float), glowVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glowVertexCount = glowVertices.size() / 2;
}

void Gauge::draw(const Shader& shader, float needleRotationRadians, bool isMainGauge) {
    shader.use();
    shader.setVec2("scale", 1.0f, 1.0f);
    shader.setVec2("offset", offsetX, offsetY);
    shader.setFloat("rotation", 0.0f);

    if (isMainGauge) {
        // Draw outer bezel (chrome/silver effect)
        shader.setVec3("color", 0.8f, 0.8f, 0.9f);
        shader.setFloat("alpha", 1.0f);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 102);

        // Draw dark background
        shader.setVec2("scale", 0.92f, 0.92f);
        shader.setVec3("color", 0.02f, 0.02f, 0.08f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
        
        // Draw glow effect for active area
        shader.setVec2("scale", 1.0f, 1.0f);
        shader.setVec3("color", 0.0f, 0.4f, 1.0f); // Blue glow
        shader.setFloat("alpha", 0.6f);
        glBindVertexArray(glowVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, glowVertexCount);
        
        // Draw tick marks
        shader.setFloat("alpha", 1.0f);
        shader.setVec3("color", 0.7f, 0.8f, 1.0f);
        glBindVertexArray(ticksVAO);
        glDrawArrays(GL_LINES, 0, tickCount);

        // Draw needle
        shader.setFloat("rotation", needleRotationRadians);
        shader.setVec3("color", 0.9f, 0.9f, 1.0f); // Bright white/blue
        glBindVertexArray(needleVAO);
        glDrawArrays(GL_LINES, 0, 8);

        // Draw center hub
        shader.setFloat("rotation", 0.0f);
        shader.setVec2("scale", 0.06f, 0.06f);
        shader.setVec3("color", 0.2f, 0.3f, 0.4f);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
        
    } else {
        // Smaller gauges (fuel/temp)
        // Draw outer ring
        shader.setVec3("color", 0.6f, 0.6f, 0.7f);
        shader.setFloat("alpha", 1.0f);
        glBindVertexArray(circleVAO);
        
        if (gaugeType == GaugeType::QUADRANT_1 || gaugeType == GaugeType::QUADRANT_4) {
            int arcVertices = (int)(102 * (sweep / 360.0f)) + 2;
            glDrawArrays(GL_TRIANGLE_FAN, 0, arcVertices);
            
            // Draw background
            shader.setVec2("scale", 0.85f, 0.85f);
            shader.setVec3("color", 0.02f, 0.02f, 0.08f);
            glDrawArrays(GL_TRIANGLE_FAN, 0, arcVertices);
        }

        // Draw tick marks
        shader.setVec2("scale", 1.0f, 1.0f);
        shader.setVec3("color", 0.6f, 0.7f, 0.8f);
        glBindVertexArray(ticksVAO);
        glDrawArrays(GL_LINES, 0, tickCount);

        // Draw needle
        shader.setFloat("rotation", needleRotationRadians);
        shader.setVec3("color", 1.0f, 0.3f, 0.0f); // Orange/red for smaller gauges
        glBindVertexArray(needleVAO);
        glDrawArrays(GL_LINES, 0, 8);

        // Draw center hub
        shader.setFloat("rotation", 0.0f);
        shader.setVec2("scale", 0.08f, 0.08f);
        shader.setVec3("color", 0.15f, 0.2f, 0.25f);
        glBindVertexArray(circleVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 102);
    }
}