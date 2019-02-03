// added 4th mass and adjusted constants to make it a bit springier


#include "al/core.hpp"
using namespace al;

#include <vector>
using namespace std;

// draw a circle, given a 2D point and a radius
void circle(Graphics& g, Vec2f center, float radius) {
  Mesh m{Mesh::TRIANGLE_FAN};
  m.vertex(center);
  for (int i = 0; i < 101; ++i)
    m.vertex(center.x + radius * sin(M_2PI * i / 100),
             center.y + radius * cos(M_2PI * i / 100));
  g.color(1);
  g.draw(m);
}

// draw a line, given two points, a and b
void line(Graphics& g, Vec2f a, Vec2f b) {
  Mesh m{Mesh::LINES};
  m.vertex(a);
  m.vertex(b);
  g.color(1);
  g.draw(m);
}

// Earlier, I defined Mass like this:
//
// struct Mass {
//   Vec2f position;
//   Vec2f velocity;
//   float mass;
// };
//
// I changed the definition of Mass to "inherit" from Vec2f so that we can treat
// it as its own position. Below, we say that Mass *is* a Vec2f which represents
// its position. As well, it has a Vec2f for velocity.
//
struct Mass : Vec2f {
  Vec2f velocity;
  float mass;
};

struct Spring {
  Mass& a;  // a reference to a Mass
  Mass& b;
  float restLength;
  float stiffness;
};

struct MyApp : App {
  // simulation parameters
  double timeStep = 0.001;
  double dragFactor = 0.05;

  vector<Mass> mass;
  vector<Spring> spring;

  void onCreate() override {
    mass.resize(4, Mass{});
    mass[0].set(0, 0);
    mass[0].mass = 0.11;
    mass[1].set(1.5, 0);
    mass[1].mass = 0.19;
    mass[2].set(1.5, 1.2);
    mass[2].mass = 0.27;
    mass[3].set(0, 1.2);
    mass[3].mass = 0.17;

    spring.push_back({mass[0], mass[1], 1, 50});
    spring.push_back({mass[1], mass[2], 1, 50});
    spring.push_back({mass[2], mass[3], 1, 50});
    spring.push_back({mass[3], mass[0], 1, 50});
  }

  void onAnimate(double dt) override {
    // For each Spring...
    for (Spring& s : spring) {
      // Make a vector that points from a to b.
      Vec2f a2b = s.b - s.a;

      // Use Hooke's Law (Fs = -kx) to calculate the amount of force this
      // Spring will exert on each Mass. Know that .mag() computes the length (a
      // scalar) of a vector.
      float forceMagnitude = (a2b.mag() - s.restLength) * s.stiffness;

      // create a "unit vector" (a vector with length 1.0) that points in the
      // direction of the Spring.
      Vec2f unitVector = a2b / a2b.mag();

      Vec2f force = unitVector * forceMagnitude;

      // equal and opposite forces
      s.a.velocity += force;
      s.b.velocity -= force;
    }

    // add drag
    for (Mass& m : mass) {
      m.velocity += -m.velocity * dragFactor;
    }

    // integrate velocity
    for (Mass& m : mass) {
      m += m.velocity * timeStep;
    }
  }

  void onDraw(Graphics& g) override {
    g.clear(0.25);

    // Establish a coordinate system where the window goes from (-1, -1) on the
    // bottom left corner and (1, 1) on the top right corner.
    g.camera(Viewpoint::IDENTITY);

    // scale the x dimension to correct the aspect ratio
    g.scale((float)height() / width(), 1, 1);

    // scale *all* dimensions; this will "zoom out".
    g.scale(0.5);

    for (Mass& m : mass) {
      circle(g, m, m.mass);
    }
    for (Spring& s : spring) {
      line(g, s.a, s.b);
    }
  }

  void onKeyDown(const Keyboard& k) override {
    // Kick Mass 0 in a random direction when the space bar is pressed
    if (k.key() == ' ') {
      float angle = rnd::uniform(M_2PI);
      mass[0].velocity.x = 40 * sin(angle);
      mass[0].velocity.y = 40 * cos(angle);
    }
  }
};

int main() {
  MyApp app;
  app.initAudio();
  app.start();
}
