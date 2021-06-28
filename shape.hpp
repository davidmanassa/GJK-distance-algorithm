#ifndef SHAPE
#define SHAPE

#include <stdio.h>
#include <stdlib.h>
#include <vector>
using std::vector;

// GLM header file
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

class Shape {

    public:

        vector<glm::vec2> points; // lista ordenada dos pontos da shape

        void translate(glm::vec2 traslation);

};

#endif /* SHAPE_H */