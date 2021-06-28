#include "shape.hpp"

void Shape::translate(glm::vec2 translation) {

    for (int i = 0; i < points.size(); i++) {
        points[i].x += translation.x;
        points[i].y += translation.y;
    }

}
