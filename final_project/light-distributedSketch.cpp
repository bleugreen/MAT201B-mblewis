/*
 Distributed Sketch 1
 
 Demo of final project for MAT201B
 by Mitchell Lewis
 
 Adapted from DistributedApp by Andres Cabrera
 
 
 
 TODO:
 
 make lines pop more
 add spiral clouds
 sqaush lines down
 
 chanegd vertice number
 
 */



#include <stdio.h>
#include "al/core.hpp"
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
float pillarRadius = 2.0;

vector<vector<float>> waves;
float waveWidth = 500;
float waveHeight = 4.0;
float waveRadius = 3.0;
float waveInnerRadius = 3.0;

float maxRadius = 10.0;
int numClouds = N*5;
int cloudSize = 100;
vector<Mesh> clouds;
vector<Vec3f> cloudPos;
vector<Vec2f> cloudVelocity;

float background = 0.0;


bool analysisOn = true;


// -----------------------------------------------------------------------
//  Shared State
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
    
    DistributedExampleApp(){
        if(hasRole(ROLE_RENDERER)){
            displayMode(displayMode() | Window::STEREO_BUF);
        }
        if(hasRole(ROLE_DESKTOP)){
            initAudio();
        }
        else{
            initAudio(44100, 1024, 60, 60, 10);
        }
        
        
    }
    
    // The simulate function is only run for the simulator
    virtual void simulate(double dt) override {
        //
        state().pose = nav();
    }
    
    // -----------------------------------------------------------------------
    //  Create
    // -----------------------------------------------------------------------
    
    virtual void onCreate() override {

        samplePlayer.load("../sound/10.wav");
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
        createClouds();
        
        nav().pos(0.000000, 1.5, 0);
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
    
    void createClouds(){
        // for each cloud in list
        for(int i=0; i<numClouds; i++){
            Mesh cloud{Mesh::TRIANGLE_FAN};
            Vec3f center(rnd::normal() * (maxRadius/3), (float(i) * 0.0005) + 2*(1+waveHeight), rnd::normal() * (maxRadius/3));
            // for each point in cloud
            for(int j=0; j<cloudSize; j++){
                cloud.color(HSV(float(i%N)/N, 0.25, 1));
            }
            clouds.push_back(cloud);
            cloudPos.push_back(center);
            cloudVelocity.push_back(Vec2f(rnd::normal()*2, rnd::normal()*2));
        }
    }
    
    // -----------------------------------------------------------------------
    //  Draw
    // -----------------------------------------------------------------------
    
    virtual void onDraw(Graphics& g) override {
        if (hasRole(ROLE_RENDERER) || hasRole(ROLE_DESKTOP) ||
            hasRole(ROLE_SIMULATOR)) {
            
            g.clear(0);
            g.pointSize(30);
            g.depthTesting(true);
            g.blending(true);
            g.blendModeTrans();
            
            drawWaves(g);
            drawPillars(g, state().soundVals);
            drawClouds(g);
        }
        
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
            
            g.color(HSV(0.33+color*0.667, (height+5)/10, (height+2)/3));
            g.pushMatrix();
            // g.rotate(180, 0, 0, 1); // translates/rotates s.t. pillars are above
            // and extend down g.translate(0, -10, 0);
            g.draw(pillars[i]);
            g.popMatrix();
        }
    }
    
    void drawWaves(Graphics& g) {
        for (int i = 0; i < waves.size(); i++) {
            //Mesh m{Mesh::LINE_LOOP};
            Mesh m{Mesh::TRIANGLE_STRIP};
            float wavePercent = 1 - (float)(i + 1) / waves.size();
            for (int j = 0; j < waves[i].size(); j++) {
                float valPercent = (float)j / waves[i].size();
                float val = waves[i][j];
                
                float x = (waveInnerRadius + (wavePercent) * waveRadius) *
                cos(valPercent * 2 * M_PI);
                float z = (waveInnerRadius + (wavePercent) * waveRadius) *
                sin(valPercent * 2 * M_PI);
                
                float y = (val * waveHeight) + (wavePercent * waveHeight);
                
                m.vertices().push_back(Vec3f(x, y, z));
                m.vertices().push_back(Vec3f(x, 0, z));
                m.color(HSV(wavePercent*0.33, (val+1)/2, (val*0.7)+0.3));
                m.color(HSV(wavePercent*0.33, (val+1)/2, (val*0.7)));
            }
            
            g.meshColor();
            g.draw(m);
        }
    }
    
    void drawClouds(Graphics& g){
        for(int i=0; i<clouds.size(); i++){
            g.meshColor();
            g.draw(clouds[i]);
        }
    }
    
    // -----------------------------------------------------------------------
    //  Animate
    // -----------------------------------------------------------------------
    
    void onAnimate(double dt) override {
        if (hasRole(ROLE_RENDERER)) {
            // if (role() == ROLE_RENDERER) {
            pose() = state().pose;
        }
        
        for (int i = 0; i < waves.size(); i++) {
            waves[i].pop_back();
            waves[i].insert(waves[i].begin(), state().soundVals[i]);
        }
        
        animateClouds();
        
        
    }
    
    
    virtual void animateClouds(){
        // for each cloud in band[i]
        for(int i=0; i<numClouds; i++){
            float dist = sqrt((cloudPos[i].x * cloudPos[i].x)+(cloudPos[i].z * cloudPos[i].z));
            float height = (float(i) * 0.005) + 2*(waveHeight); // place clouds close together, slightly above waves
            float hue = 0.5 + 0.5*float(i%N)/N;
            if(dist >= maxRadius) cloudVelocity[i] *= -1;
            
            // get last center point
            Vec3f center = cloudPos[i];
            center.x += cloudVelocity[i].x * 0.01 * waves[i%N][0]*2;
            center.y = height;
            center.z += cloudVelocity[i].y * 0.01 * waves[i%N][0]*2;
            cloudPos[i] = center;
            clouds[i].vertices().clear();
            clouds[i].colors().clear();
            
            clouds[i].vertex(center);
            clouds[i].color(HSV(hue, 0.1, 0.5));
            
            Vec3f soundPoint;
            
            
            float soundVal;
            
            
            
            //for each point in cloud (- center
            for(int k=0; k<cloudSize; k++){
                
                // sound val point

                    
                    // find sound-based point and next sound-based point for interpolation
                    soundVal = 2*waves[i%N][k];
                    float angle = (float(k)/cloudSize) * M_2PI;
                    
                    
                    soundPoint = Vec3f(center.x+(soundVal*cos(angle)), height, center.z+(soundVal*sin(angle)));
                    
                    clouds[i].vertex(soundPoint);
                    clouds[i].color(HSV(hue, (0.25+soundVal)/2, (2+soundVal)/3));
                
                // interpolated point
                /*else{
                    float value = (float(k%10)/10)*(nextSoundVal)+(1-float(k%10)/10)*soundVal;
                    clouds[i].vertex(cosInterpolate(soundPoint, nextSoundPoint, float(k%10)/10));
                    clouds[i].color(HSV(hue, (0.25+value)/2, value));
                }*/
                
            }
            
            clouds[i].vertex(clouds[i].vertices()[1]);
            clouds[i].color(clouds[i].colors()[1]);
            
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
            
            float f = samplePlayer.read(0) / io.channelsOut();
            for(int i=0; i<io.channelsOut(); i++){
                io.out(i) = f;
            }
        }
    }
    
    // -----------------------------------------------------------------------
    //  Utility
    // -----------------------------------------------------------------------
    
    void onKeyDown(const Keyboard& k) override {
        if (k.key() == ' ') {
            analysisOn = !analysisOn;
            samplePlayer.reset();
        }
    }
    
    Vec3f cosInterpolate(Vec3f last, Vec3f target, double progress){
        double mu;
        mu = (1-cos(progress*M_PI))/2;
        
        return last*(1-mu)+target*mu;
        
        // update from class 1/29: easier to use vector operations than split into x,y,z
    }
    
    
};

int main() {
    DistributedExampleApp app;
    app.start();
    return 0;
}
