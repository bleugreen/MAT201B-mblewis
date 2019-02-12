#include "al/core.hpp"
using namespace al;

#include <vector>
using namespace std;

#define N (1000)  // number of particles
#define CLOUD_WIDTH (5.0)

const char* vertex = R"(
#version 400

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec4 vertexColor;

uniform mat4 al_ModelViewMatrix;
uniform mat4 al_ProjectionMatrix;

out Vertex {
  vec4 color;
} vertex;

void main() {
  gl_Position = al_ModelViewMatrix * vec4(vertexPosition, 1.0);
  vertex.color = vertexColor;
}
)";

const char* fragment = R"(
#version 400

in Fragment {
  vec4 color;
  vec2 textureCoordinate;
} fragment;

uniform sampler2D alphaTexture;

layout (location = 0) out vec4 fragmentColor;

void main() {
  // use the first 3 components of the color (xyz is rgb), but take the alpha value from the texture
  //
  fragmentColor = vec4(fragment.color.xyz, texture(alphaTexture, fragment.textureCoordinate));
}
)";

const char* geometry = R"(
#version 400

// take in a point and output a triangle strip with 4 vertices (aka a "quad")
//
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform mat4 al_ProjectionMatrix;

// this uniform is *not* passed in automatically by AlloLib; do it manually
//
uniform float halfSize;

in Vertex {
  vec4 color;
} vertex[];

out Fragment {
  vec4 color;
  vec2 textureCoordinate;
} fragment;

void main() {
  mat4 m = al_ProjectionMatrix; // rename to make lines shorter
  vec4 v = gl_in[0].gl_Position; // al_ModelViewMatrix * gl_Position

  gl_Position = m * (v + vec4(-halfSize, -halfSize, 0.0, 0.0));
  fragment.textureCoordinate = vec2(0.0, 0.0);
  fragment.color = vertex[0].color;
  EmitVertex();

  gl_Position = m * (v + vec4(halfSize, -halfSize, 0.0, 0.0));
  fragment.textureCoordinate = vec2(1.0, 0.0);
  fragment.color = vertex[0].color;
  EmitVertex();

  gl_Position = m * (v + vec4(-halfSize, halfSize, 0.0, 0.0));
  fragment.textureCoordinate = vec2(0.0, 1.0);
  fragment.color = vertex[0].color;
  EmitVertex();

  gl_Position = m * (v + vec4(halfSize, halfSize, 0.0, 0.0));
  fragment.textureCoordinate = vec2(1.0, 1.0);
  fragment.color = vertex[0].color;
  EmitVertex();

  EndPrimitive();
}
)";

struct AlloApp : App {
  ShaderProgram shader;
  Texture texture;

  Mesh pointMesh;          // holds positions
  vector<float> mass;      // holds masses
  vector<Vec3f> velocity;  // holds velocities

  void onCreate() override {
    // use a texture to control the alpha channel of each particle
    //
    texture.create2D(256, 256, Texture::R8, Texture::RED, Texture::SHORT);
    int Nx = texture.width();
    int Ny = texture.height();
    std::vector<short> alpha;
    alpha.resize(Nx * Ny);
    for (int j = 0; j < Ny; ++j) {
      float y = float(j) / (Ny - 1) * 2 - 1;
      for (int i = 0; i < Nx; ++i) {
        float x = float(i) / (Nx - 1) * 2 - 1;
        float m = exp(-13 * (x * x + y * y));
        m *= pow(2, 15) - 1;  // scale by the largest positive short int
        alpha[j * Nx + i] = m;
      }
    }
    texture.submit(&alpha[0]);

    // compile and link the three shaders
    //
    shader.compile(vertex, fragment, geometry);

    // create a mesh of points scattered randomly with random colors
    //
    pointMesh.primitive(Mesh::POINTS);
    for (int i = 0; i < N; i++) {
      pointMesh.vertex(
          Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) *
          CLOUD_WIDTH);
        velocity.push_back(Vec3f(0,0,0));
        mass.push_back(1); // I initialized here so i can connect color to mass (later)
        pointMesh.color(HSV(rnd::uniform(), 1.0, 1.0));
    }
  }

  void onAnimate(double dt) override {
    // put your simulation code here
    //

    vector<Vec3f>& vertex(pointMesh.vertices());  // make an alias

    // for each pair (i,j)...
    for (int i = 0; i < vertex.size(); ++i) {
      for (int j = 1 + i; j < vertex.size(); ++j) {
        Vec3f& a(vertex[i]);  // alias
        Vec3f& b(vertex[j]);  // alias
        Vec3f uAB(b-a);      // uAB = vector between a and b
        Vec3f normAB = uAB.normalize();     // make it a unit vector
          if(uAB.mag()> 0) {
              float coeff = ((0.000001*mass[i]*mass[j])/pow(uAB.mag(),2));
              Vec3f gravityA = coeff*normAB;
              velocity[i] += gravityA / mass[i];
              velocity[j] += gravityA*-1 / mass[j];
          }
          else{
              if(velocity[i].mag() > 5 || velocity[j].mag() > 5){ // combine on fast collision
                  if(mass[i] < mass[j]){
                      mass[j] += mass[i];
                      mass.erase(mass.begin()+i);
                      velocity.erase(velocity.begin()+i);
                      vertex.erase(vertex.begin()+i);
                  }
                  else{
                      mass[i] += mass[j];
                      mass.erase(mass.begin()+j);
                      velocity.erase(velocity.begin()+j);
                      vertex.erase(vertex.begin()+j);
                  }
              }
          }
        
        // Fij =    G * Mi * Mj
        //       ---------------- * Uij
        //        distance(ij)^2
        // where Uij is the unit vector from i to j
        //
      }
    }
      for(int i = 0; i<vertex.size(); i++){
          // calc drag
          float dragX = -1/2*(velocity[i].x * velocity[i].x)*1.5;
          float dragY = -1/2*(velocity[i].y * velocity[i].y)*1.5;
          float dragZ = -1/2*(velocity[i].z * velocity[i].z)*1.5;
          Vec3f drag(dragX, dragY, dragZ);
          velocity[i] += drag / mass[i];
          vertex[i] += velocity[i];
          
      }

    // F = ma
    // 1. calculate gravitational forces
    // 2. accumulate accellerations into velocities
    // 3. add drag forces to velocities
    // 4. accumulate velocity into position
  }

  void onDraw(Graphics& g) override {
    // you don't have to change anything in here
    //
    g.clear(0.23);

    g.depthTesting(false);
    g.blending(true);
    g.blendModeTrans();

    texture.bind();
    g.shader(shader);
    g.shader().uniform("halfSize", 0.15);
    g.draw(pointMesh);
    texture.unbind();
  }
};

int main() { AlloApp().start(); }
