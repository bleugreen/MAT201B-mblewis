#include "al/core.hpp"
using namespace al;

#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
using namespace gam;

#include <fstream>
#include <string>
using namespace std;

#include "vertex_shader.h"

string slurp(string fileName) {
  fstream file(fileName);
  string returnValue = "";
  while (file.good()) {
    string line;
    getline(file, line);
    returnValue += line + "\n";
  }
  return returnValue;
}

struct MyApp : App {
  gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> samplePlayer;
  gam::STFT stft;
  vector<float> spectrum;

  ShaderProgram shader;
  Texture texture;
  string fragmentCode;

  void onCreate() override {
    samplePlayer.load("../sound/6.wav");
      cout << samplePlayer.loop() << endl;
    Sync::master().spu(audioIO().fps());

    string code = slurp("../shader-fragment.glsl");
    ShaderProgram s;
    if (s.compile(vertexCode, code)) {
      fragmentCode = code;
      shader.compile(vertexCode, fragmentCode);
    }

    spectrum.resize(stft.numBins() * 1024);
    int n = stft.numBins();
    texture.create2D(n, 1024, Texture::R32F, Texture::RED, Texture::FLOAT);
  }

  void onDraw(Graphics& g) override {
    g.clear(0.5);
    g.camera(Viewpoint::ORTHO_FOR_2D);

    g.shader(shader);
    g.shader().uniform("time", time);
    g.shader().uniform("window", Vec2f(width(), height()));
    g.shader().uniform("mouse", Vec2f(mouseX, mouseY));

    Mesh m{Mesh::TRIANGLE_STRIP};
    m.vertex(0, 0);
    m.vertex(0, height());
    m.vertex(width(), 0);
    m.vertex(width(), height());
    g.draw(m);
  }

  float mouseX, mouseY;
  void onMouseMove(const Mouse& m) override {
    //mouseX = m.x();
    //mouseY = height() - m.y();
  }

  void onSound(AudioIOData& io) override {
    while (io()) {
      if (stft(samplePlayer())) {
          float mean = 0;
        for (int i = 0; i < stft.numBins(); ++i) {
            mean += stft.bin(i).norm() * pow(10, 6);
        }
          mean /= stft.numBins();
          cout << stft.numBins() << endl;
          if(mouseX < mean) mouseX += 1.5;
          if(mouseX > mean) mouseX -= 2.5;
          if(mouseY < mean) mouseY += 1.5;
          if(mouseY > mean) mouseY -= 2.5;
          //mouseY = mean;
      }
      float f = samplePlayer.read(0);
      io.out(0) = f;
      io.out(1) = f;
    }
  }
    

  double time = 0;
  double timer = 0;
  void onAnimate(double dt) override {
    time += dt;

    //
    timer += dt;
    if (timer > 1) {
      timer -= 1;
      string code = slurp("../shader-fragment.glsl");
      if (code != fragmentCode) {
        ShaderProgram s;
        if (s.compile(vertexCode, code)) {
          fragmentCode = code;
        }
        shader.compile(vertexCode, fragmentCode);
      }
    }
  }
};

int main() {
  MyApp app;
  app.initAudio();
  app.start();
}
