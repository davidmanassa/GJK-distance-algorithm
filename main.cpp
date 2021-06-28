// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// GLM header file
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// shaders header file
#include "common/shader.hpp"

#include "shape.hpp"

//-----------------------------------------------------------------------------
// Basic vector arithmetic operations

glm::vec2 subtract (glm::vec2 a, glm::vec2 b) { a.x -= b.x; a.y -= b.y; return a; }
glm::vec2 negate (glm::vec2 v) { v.x = -v.x; v.y = -v.y; return v; }
glm::vec2 perpendicular (glm::vec2 v) { glm::vec2 p = { v.y, -v.x }; return p; }
float dotProduct (glm::vec2 a, glm::vec2 b) { return a.x * b.x + a.y * b.y; }
float lengthSquared (glm::vec2 v) { return v.x * v.x + v.y * v.y; }

//-----------------------------------------------------------------------------
// Triple product expansion is used to calculate perpendicular normal vectors 
// which kinda 'prefer' pointing towards the Origin in Minkowski space
glm::vec2 tripleProduct (glm::vec2 a, glm::vec2 b, glm::vec2 c) {
    
    glm::vec2 r;
    
    float ac = a.x * c.x + a.y * c.y; // perform a.dot(c)
    float bc = b.x * c.x + b.y * c.y; // perform b.dot(c)
    
    // perform b * a.dot(c) - a * b.dot(c)
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;
}

//-----------------------------------------------------------------------------
// Get furthest vertex along a certain direction
int indexOfFurthestPoint (const glm::vec2 * vertices, int count, vec2 d) {
    
    float maxProduct = dotProduct (d, vertices[0]);
    int index = 0;
    for (int i = 1; i < count; i++) {
        float product = dotProduct (d, vertices[i]);
        if (product > maxProduct) {
            maxProduct = product;
            index = i;
        }
    }
    return index;
}

//-----------------------------------------------------------------------------
// Minkowski sum support function for GJK
glm::vec2 support (const glm::vec2 * vertices1, int count1,
              const glm::vec2 * vertices2, int count2, glm::vec2 d) {

    // get furthest point of first body along an arbitrary direction
    int i = indexOfFurthestPoint (vertices1, count1, d);
    
    // get furthest point of second body along the opposite direction
    int j = indexOfFurthestPoint (vertices2, count2, negate (d));

    // subtract (Minkowski sum) the two points to see if bodies 'overlap'
    return subtract (vertices1[i], vertices2[j]);
}

//-----------------------------------------------------------------------------
// GJK test
int gjk (const glm::vec2 * vertices1, int count1,
         const glm::vec2 * vertices2, int count2) {
    
    int index = 0; // index of current vertex of simplex
    glm::vec2 a, b, c, d, ao, ab, ac, abperp, acperp, simplex[3];
    
    d = glm::vec2(1, 0); // direção inicial

    // primeiro ponto inicial do simplex
    a = simplex[0] = support (vertices1, count1, vertices2, count2, d);
    
    if (dotProduct (a, d) <= 0)
        return 0; // no collision
    
    d = negate (a); // A próxima procura é em direção à origem

    while (1) {
        
        a = simplex[++index] = support (vertices1, count1, vertices2, count2, d);
        
        if (dotProduct (a, d) <= 0)
            return 0; // no collision
        
        ao = negate (a); // de A para a origem é o negativo de A
        
        // o simplex têm 2 pontos (line segment)
        if (index < 2) {
            b = simplex[0];
            ab = subtract (b, a); // from point A to B
            d = tripleProduct (ab, ao, ab); // normal to AB towards Origin
            if (lengthSquared (d) == 0)
                d = perpendicular (ab);
            continue; // skip to next iteration
        }
        
        b = simplex[1];
        c = simplex[0];
        ab = subtract (b, a); // from point A to B
        ac = subtract (c, a); // from point A to C
        
        acperp = tripleProduct (ab, ac, ac); // perpendicular a ac
        
        if (dotProduct (acperp, ao) >= 0) { 
            // a origem está fora da linha ac
            
            d = acperp; // A nova direção: normal a AC que aponta para a origem
            
        } else {
            
            abperp = tripleProduct (ac, ab, ab);
            
            if (dotProduct (abperp, ao) < 0) // origem entre AB e AC
                return 1; // collision
            
            simplex[0] = simplex[1]; // swap first element (point C)

            d = abperp; // A nova direção: normal a AB que aponta para a origem
        }
        
        simplex[1] = simplex[2]; // swap element in the middle (point B)
        --index;
    }
    
    return 0;
}

// Vertex array object (VAO)
GLuint VertexArrayID;

// GLSL program from the shaders
GLuint programID;

GLint WindowWidth = 800;
GLint WindowHeight = 600;

float delta = 0.1f;

float x = 0.0f;
float y = 0.0f;
const float size = 10;
float xstep = delta;
float ystep = delta;

static vector<GLuint> vertexbuffer, colorbuffer;
static vector<Shape> shapes;

int loadShape( int index, Shape shape ) {

    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

	int length = shape.points.size() + (2 * (shape.points.size() - 3));
    length *= 3; // 3 valores por vetor (x, y, z)

	static GLfloat* g_vertex_buffer_data = (GLfloat*) malloc(length * sizeof(GLfloat)); 
	static GLfloat* g_color_buffer_data = (GLfloat*) malloc(length * sizeof(GLfloat));
	
    int i = 0;
    int numberOfEdgesInTheFace = shape.points.size() + (2 * (shape.points.size() - 3));
	int numberOfTriangles = numberOfEdgesInTheFace / 3;

    int nTriangle = 1, nPoint = 1;

    for (int p = 0; p < shape.points.size(); p++) {

        glm::vec2 point = shape.points[p];

        g_vertex_buffer_data[i] = point.x;
        g_vertex_buffer_data[i + 1] = point.y;
        g_vertex_buffer_data[i + 2] = 0;
        g_color_buffer_data[i] = 1.0f;
        g_color_buffer_data[i + 1] = 1.0f;
        g_color_buffer_data[i + 2] = 1.0f;
        i += 3;
        nPoint += 1;

        // std::cout << " N " << point.x << " " << point.y << std::endl;
        if ((3 * nTriangle) == nPoint && nTriangle != 1) { // 6, 9, 12, ... (ultimo ponto dos triangulos depois do primeiro)
            g_vertex_buffer_data[i] = shape.points[0].x;
            g_vertex_buffer_data[i + 1] = shape.points[0].y;
            g_vertex_buffer_data[i + 2] = 0;
            g_color_buffer_data[i] = 1.0f;
            g_color_buffer_data[i + 1] = 1.0f;
            g_color_buffer_data[i + 2] = 1.0f;
            i += 3;
            nPoint += 1;

            // std::cout << " F " << shape.points[0].x << " " << shape.points[0].y << std::endl;
        }
        if (nPoint == ((3 * nTriangle) + 1) && numberOfEdgesInTheFace != 3 && nTriangle < numberOfTriangles) { // 4, 7, 10, ... (primeiro de cada triangulo depois do primeiro)
            g_vertex_buffer_data[i] = point.x;
            g_vertex_buffer_data[i + 1] = point.y;
            g_vertex_buffer_data[i + 2] = 0;
            g_color_buffer_data[i] = 1.0f;
            g_color_buffer_data[i + 1] = 1.0f;
            g_color_buffer_data[i + 2] = 1.0f;
            i += 3;
            nPoint += 1;
            nTriangle += 1;

            // std::cout << " I " << point.x << " " << point.y << std::endl;
        }

    }

    vertexbuffer.push_back(NULL);
    colorbuffer.push_back(NULL);

    glGenBuffers(1, &vertexbuffer[index]);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[index]);
	glBufferData(GL_ARRAY_BUFFER, length * sizeof(GLfloat), g_vertex_buffer_data, GL_STATIC_DRAW); // float 4 bits

	glGenBuffers(1, &colorbuffer[index]);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer[index]);
	glBufferData(GL_ARRAY_BUFFER, length * sizeof(GLfloat), g_color_buffer_data, GL_STATIC_DRAW);

	return length / 3;

}

void setMVP() {

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glm::mat4 mvp = glm::ortho(-40.0f, 40.0f, -30.0f, 30.0f);
    unsigned int matrix = glGetUniformLocation(programID, "mvp");
    glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

}

void drawShape(int index, int nPoints, glm::vec2 transform) {

    glm::mat4 trans = glm::translate(glm::mat4(1.0), glm::vec3(transform.x, transform.y, 0.0f));

    unsigned int m = glGetUniformLocation(programID, "trans");
    glUniformMatrix4fv(m, 1, GL_FALSE, &trans[0][0]);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[index]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

    // 2nd attribute buffer : colors
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer[index]);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

    // Draw the triangles
    glDrawArrays(GL_TRIANGLES, 0, nPoints * 3);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

}

void cleanupDataFromGPU()
{
    glDeleteBuffers(1, &vertexbuffer[0]);
    glDeleteBuffers(1, &colorbuffer[0]);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
}

GLfloat rotation = 0.0f;

int main( void ) {

    // Initialise GLFW
    glewExperimental = true; // Needed for core profile
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Open a window
    window = glfwCreateWindow( WindowWidth, WindowHeight, "2D GJK", NULL, NULL);
    if( window == NULL ) {
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        glfwTerminate();
        return -1;
    }

    // Create window context
    glfwMakeContextCurrent(window);
    
    // Initialize GLEW
    glewExperimental=true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    
    // Create and compile our GLSL program from the shaders
    programID = LoadShaders( "SimpleVertexShader.vert", "SimpleFragmentShader.frag" );

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    glUseProgram(programID);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glm::vec2 p1(20, 20);
	glm::vec2 p2(0, 17);
	glm::vec2 p3(17, 0);

	glm::vec2 p4(10, 1);
    glm::vec2 p5(-5, -5);
	glm::vec2 p6(-10, 1);
	glm::vec2 p7(0, 4);

	Shape s1;
    s1.points.push_back(p1);
    s1.points.push_back(p2);
    s1.points.push_back(p3);

	Shape s2;
    s2.points.push_back(p4);
    s2.points.push_back(p5);
    s2.points.push_back(p6);
    s2.points.push_back(p7);

	int nPoints0 = loadShape(0, s1);
    int nPoints1 = loadShape(1, s2);

    shapes.push_back(s1);
    shapes.push_back(s2);

    do {

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        
        setMVP();
        s1.translate(glm::vec2(xstep,0));
        s2.translate(glm::vec2(0,ystep));
        drawShape(0, nPoints0, glm::vec2(x,0));
        drawShape(1, nPoints1, glm::vec2(0,y));

        glm::vec2 *s1Vecs = NULL;
        int tot1 = 0;
        for (int i = 0; i < s1.points.size(); i++) {
            s1Vecs = (glm::vec2*) realloc(s1Vecs, ++tot1 * sizeof(glm::vec2));
            s1Vecs[tot1 - 1] = glm::vec2(s1.points[i].x, s1.points[i].y);
        }

        glm::vec2 *s2Vecs = NULL;
        int tot2 = 0;
        for (int i = 0; i < s2.points.size(); i++) {
            s2Vecs = (glm::vec2*) realloc(s2Vecs, ++tot2 * sizeof(glm::vec2));
            s2Vecs[tot2 - 1] = glm::vec2(s2.points[i].x, s2.points[i].y);
        }

        int collision = gjk(s1Vecs, tot1, s2Vecs, tot2);

        if (collision) {

            int r = rand() % 2;

            if (r == 1) {
                xstep = -xstep;
            } else {
                ystep = -ystep;
            }

        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        if (x + size > 40.0f  || x < -40.0f)
            xstep = -xstep;
        
        if (y + size > 30.0f || y < -30.0f)
            ystep = -ystep;

        x += xstep;
        y += ystep;
        
    } while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
          glfwWindowShouldClose(window) == 0 );
    
    
    // Cleanup VAO, VBOs, and shaders from GPU
    cleanupDataFromGPU();
    
    // Close OpenGL window and terminate GLFW
    glfwTerminate();
    
    return 0;
}

