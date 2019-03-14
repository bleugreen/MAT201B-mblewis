/*
Distributed Sketch 1
 
Demo of final project for MAT201B
 by Mitchell Lewis
 
Adapted from DistributedApp by Andres Cabrera
*/

#include <stdio.h>
#include "al/core/app/al_DistributedApp.hpp"
using namespace al;

#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
using namespace gam;

#include <fstream>
#include <string>
#include <vector>
using namespace std;

#define N (26)

// -----------------------------------------------------------------------
//  Variables
// -----------------------------------------------------------------------
gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> samplePlayer;
gam::STFT stft;

std::vector<int> bandWidths{
    2, 2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  5, 6, 7,
    8, 10, 12, 15, 19, 23, 28, 38, 53, 75, 90, 94};  // Bark Scale - # of stft
                                                     // bands per Bark band
std::vector<float> bandMax;

std::vector<Mesh> pillars;
float pillarRadius = 4.0;

vector<vector<float>> waves;
float waveWidth = 2000;
float waveHeight = 5.0;
float waveRadius = 30.0;
float waveInnerRadius = 5.0;

bool analysisOn = true;


// -----------------------------------------------------------------------
//  Utility
// -----------------------------------------------------------------------

struct SharedState {
  float soundVals[N];
    Pose pose;
};

// -----------------------------------------------------------------------
//  App Start
// -----------------------------------------------------------------------

class DistributedExampleApp : public DistributedApp<SharedState> {
 public:
  // The simulate function is only run for the simulator
  virtual void simulate(double dt) override {
    
      state().pose = nav();

  }

  // -----------------------------------------------------------------------
  //  Create
  // -----------------------------------------------------------------------

  virtual void onCreate() override {
    // if (role() == ROLE_RENDERER) {
    //   parameterServer().verbose();
    //   load_perprojection_configuration();
    //   cursorHide(true);
    // }

    samplePlayer.load("../sound/BigSmoke.wav");
    // samplePlayer.loop();
    Sync::master().spu(audioIO().fps());

    for (int i = 0; i < N; i++) {
      state().soundVals[i] = 0;
      bandMax.push_back(0.1);

      vector<float> wave;
      for (int j = 0; j < waveWidth; j++) {
        wave.push_back(0.1);
      }
      waves.push_back(wave);
    }

    createPillars();

    // nav().pos(0.000000, 17.495046, 46.419401);
    // nav().quat(Quatd(0.987681, -0.156482, 0.000000, 0.000000));
  }

  void createPillars() {
    for (int i = 0; i < 2 * N; i++) {
      float angle = i * (M_PI / N);
      Vec3f bandCenter(pillarRadius * cos(angle), 0, pillarRadius * sin(angle));
      Vec3f bandCross = bandCenter.cross(Vec3f(0, 1, 0));

      Vec3f vertexA = (bandCenter + bandCenter / 10) - (bandCross / 20);
      Vec3f vertexB = (bandCenter + bandCenter / 10) + (bandCross / 20);
      Vec3f vertexC = (bandCenter - bandCenter / 10) - (bandCross / 20);
      Vec3f vertexD = (bandCenter - bandCenter / 10) + (bandCross / 20);

      Vec3f vertexE = vertexA + Vec3f(0, pillarRadius, 0);
      Vec3f vertexF = vertexB + Vec3f(0, pillarRadius, 0);
      Vec3f vertexG = vertexC + Vec3f(0, pillarRadius, 0);
      Vec3f vertexH = vertexD + Vec3f(0, pillarRadius, 0);

      Mesh m{Mesh::TRIANGLE_STRIP};
      m.vertex(vertexA);  // 0
      m.vertex(vertexE);  // 1 -- vals that need to be modulated
      m.vertex(vertexF);  // 2 --
      m.vertex(vertexB);  // 3
      m.vertex(vertexA);  // 4

      m.vertex(vertexE);  // 5 --
      m.vertex(vertexG);  // 6 --
      m.vertex(vertexC);  // 7

      m.vertex(vertexA);  // 8
      m.vertex(vertexB);  // 9
      m.vertex(vertexD);  // 10

      m.vertex(vertexH);  // 11 --
      m.vertex(vertexF);  // 12 --
      m.vertex(vertexB);  // 13

      m.vertex(vertexD);  // 14
      m.vertex(vertexH);  // 15 --
      m.vertex(vertexG);  // 16 --

      m.vertex(vertexC);  // 17
      m.vertex(vertexD);  // 18
      m.vertex(vertexA);  // 19

      pillars.push_back(m);
    }
  }

  // -----------------------------------------------------------------------
  //  Draw
  // -----------------------------------------------------------------------

  virtual void onDraw(Graphics& g) override {
    if (hasRole(ROLE_RENDERER) || hasRole(ROLE_DESKTOP) || hasRole(ROLE_SIMULATOR)) {
    //if (role() == ROLE_DESKTOP) {
      g.clear(0);

      g.depthTesting(true);
      g.blending(true);
      g.blendModeTrans();

      drawWaves(g, waves);
      drawPillars(g, state().soundVals);
    }
      
      
       g.clear(0);
       
       g.depthTesting(true);
       g.blending(true);
       g.blendModeTrans();
       
       drawWaves(g, waves);
       drawPillars(g, state().soundVals);
       }
       

      
      
  void drawPillars(Graphics& g, float soundVals[]) {
    for (int i = 0; i < 2 * N; i++) {
      float height, color;

      // draws first half of circle
      if (i < N) {
        color = (float)i / N;
        height = soundVals[i];
      }

      // draws second half of circle, flipped so bands correspond to the one
      // directly across
      else {
        color = (float)(i - N) / N;
        height = soundVals[i - N];
      }

      // updates height
      pillars[i].vertices()[1].y = pillarRadius * height;
      pillars[i].vertices()[2].y = pillarRadius * height;

      pillars[i].vertices()[5].y = pillarRadius * height;
      pillars[i].vertices()[6].y = pillarRadius * height;

      pillars[i].vertices()[11].y = pillarRadius * height;
      pillars[i].vertices()[12].y = pillarRadius * height;

      pillars[i].vertices()[15].y = pillarRadius * height;
      pillars[i].vertices()[16].y = pillarRadius * height;

      g.color(HSV(color, height, height));
      g.pushMatrix();
      // g.rotate(180, 0, 0, 1); // translates/rotates s.t. pillars are above
      // and extend down g.translate(0, -10, 0);
      g.draw(pillars[i]);
      g.popMatrix();
    }
  }

  void drawWaves(Graphics& g, vector<vector<float>> waves) {
    for (int i = 0; i < waves.size(); i++) {
      Mesh m{Mesh::LINE_LOOP};
      float wavePercent = 1 - (float)(i + 1) / waves.size();
      for (int j = 0; j < waves[i].size(); j++) {
        float valPercent = (float)j / waves[i].size();
        float val = waves[i][j];

        float x = (waveInnerRadius + wavePercent * waveRadius) *
                  cos(valPercent * 2 * M_PI);
        float z = (waveInnerRadius + wavePercent * waveRadius) *
                  sin(valPercent * 2 * M_PI);

        float y = val * waveHeight + (5 * wavePercent * waveHeight);

        m.vertices().push_back(Vec3f(x, y, z));
        m.color(HSV(wavePercent, val, val));
      }

      g.meshColor();
      g.draw(m);
    }
  }

  // -----------------------------------------------------------------------
  //  Animate
  // -----------------------------------------------------------------------

  void onAnimate(double dt) override {
    
      if (hasRole(ROLE_RENDERER)) {
      //if (role() == ROLE_RENDERER) {
          pose() = state().pose;
      }
      
      for (int i = 0; i < waves.size(); i++) {
      waves[i].pop_back();
      waves[i].insert(waves[i].begin(), state().soundVals[i]);
    }
  }

  // -----------------------------------------------------------------------
  //  Sound
  // -----------------------------------------------------------------------

  virtual void onSound(AudioIOData& io) override {
    // Audio will recieve state from simulator
    while (io() && analysisOn) {
      if (stft(samplePlayer())) {
        int currentBand = 0;

        // for each band in Bark scale
        for (int i = 0; i < bandWidths.size(); i++) {
          float barkSum = 0;

          // sums each FFT band [j] within Bark band [i]
          for (int j = 0; j < bandWidths[i]; j++) {
            barkSum += stft.bin(currentBand).norm() * 1000;
            currentBand++;
          }

          // calculate mean amplitude of Bark band
          float dist = barkSum / bandWidths[i];

          // update max if necessary
          if (dist > bandMax[i]) bandMax[i] = dist;

          // create ratio of current amplitude to max amplitude
          float distRatio = dist / bandMax[i];

          // move soundVals[i] incrementally according to distance from
          // current band amplitude
          float changeVal = (distRatio - state().soundVals[i]);
          if (changeVal > -2 && changeVal < 2) {
            state().soundVals[i] += changeVal / 20;
          }
        }
      }

      float f = samplePlayer.read(0);
      io.out(0) = f;
      io.out(1) = f;
    }
  }

  void onKeyDown(const Keyboard& k) override {
    if (k.key() == ' ') {
      analysisOn = !analysisOn;
      samplePlayer.reset();
    }
  }
};

int main() {
  DistributedExampleApp app;
  // app.fps(1);
  app.startFPS();
  app.print();
  AudioDevice::printAll();
  app.audioIO().device(AudioDevice("ECHO XS"));
  app.initAudio();
  app.start();
  return 0;
}
