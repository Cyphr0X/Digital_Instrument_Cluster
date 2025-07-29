#ifndef GAUGE_H
#define GAUGE_H

#include <glad/glad.h>
#include "Shader.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Gauge {
public:
    Gauge(float xOffset, float yOffset, float radius);
    ~Gauge();

    void draw(const Shader& shader, float needleRotationRadians);

private:
    GLuint circleVAO, circleVBO;
    GLuint needleVAO, needleVBO;
    GLuint ticksVAO, ticksVBO;
    float offsetX, offsetY;
    float radius;
    int tickCount;

    void setupCircle();
    void setupNeedle();
    void setupTicks();
};

#endif