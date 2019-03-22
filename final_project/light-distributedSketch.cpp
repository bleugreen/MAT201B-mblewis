/*
 Seeing Sound
 
 Final Project for MAT201B
 by Mitchell Lewis
 
 Adapted from DistributedApp by Andres Cabrera
 
 */



#include <stdio.h>
#include "al/core.hpp"
#include "al/core/math/al_Random.hpp"
#include "al/core/app/al_DistributedApp.hpp"
using namespace al;

#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
using namespace gam;

#include <fstream>
#include <string>
#include <vector>
using namespace std;

// Number of bands in state() storage
#define BAND_N (26)

// Number of historical sound values to store
#define HIST_N (1000)

// -----------------------------------------------------------------------
//  Variables
// -----------------------------------------------------------------------

// Plays and sends sound file data
gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> samplePlayer;

// Analyzes sound file data
gam::STFT stft;

// Hard-coded band widths for each band of Bark scale (with a split in the last band)
// Specifies number of FFT bands to include in each Bark sum
std::vector<int> bandWidths{
    2, 2,  2,  2,  2,  3,  3,  3,  3,  4,  4,  5, 6, 7,
    8, 10, 12, 15, 19, 23, 28, 38, 53, 75, 90, 94};

// Stores max value of each band for normalization
std::vector<float> bandMax;

// Stores location and color of each pillar
std::vector<Mesh> pillars;

// Inner radius of pillars, used to determine length of pillars as well
float pillarRadius = 2.0;

// Number of values to display across wave circle
float waveWidth = HIST_N;

// Maximum height of each wave
// Each wave is also incremented by this height as they move outwards
float waveHeight = 4.0;

// Inner radius of waves
float waveRadius = 3.0;

// Maximum radius for clouds to move within
float maxRadius = 10.0;

// Total number of clouds to render
int numClouds = BAND_N*5;

// Number of vertices within each cloud
int cloudSize = HIST_N/4;

// Stores cloud meshes
vector<Mesh> clouds;

// Stores cloud positions
vector<Vec3f> cloudPos;

// Stores cloud velocities
vector<Vec2f> cloudVelocity;

// Random number object to synchronize random number generation across distributed system
rnd::Random<> rng;

// Background value / lightness
float background = 0.0;

// Control: if true, nothing is drawn/animated/analyzed; controlled by space bar
bool paused = true;


// -----------------------------------------------------------------------
//  Shared State
// -----------------------------------------------------------------------

struct SharedState {
    
    // Stores/shares the last HIST_N sound values for each Bark band
    float soundVals[BAND_N][HIST_N];
    
    // Stores/shares current index to write
    int soundIndex;
    
    // Stores/shares current pose of nav()
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
        
        // Updates saved pose
        state().pose = nav();
    }
    
// -----------------------------------------------------------------------
//  Create
// -----------------------------------------------------------------------
    
    virtual void onCreate() override {

        // Loads song, sets it to loop on completion
        samplePlayer.load("../sound/10.wav");
        samplePlayer.loop();
        
        Sync::master().spu(audioIO().fps());
        
        createPillars();
        createClouds();
        
        // Init analysis arrays / index
        state().soundIndex = 0;
        for(int i=0; i<BAND_N; i++){
            bandMax.push_back(0.1);
            for(int j=0; j<HIST_N; j++){
                state().soundVals[i][j] = 0;
            }
        }
        
        // Set initial pose
        nav().pos(0.000000, 1.5, 0);
        
        // Seed random num generator
        rng.seed(12345);
        
        
    }
    
    void createPillars() {
        // Each band has 2 pillars, directly across from each other
        // This ensures that the full spectrum of sound is visible from any vantage point
        for (int i = 0; i < 2 * BAND_N; i++) {
            float angle = i * (M_PI / BAND_N);
            
            // Calculates center of each pillar and a vector orthogonal to origin->center for finding sides
            Vec3f bandCenter(pillarRadius * cos(angle), 0, pillarRadius * sin(angle));
            Vec3f bandCross = bandCenter.cross(Vec3f(0, 1, 0));
            
            // Four bottom corners
            Vec3f vertexA = (bandCenter + bandCenter / 10) - (bandCross / 20);
            Vec3f vertexB = (bandCenter + bandCenter / 10) + (bandCross / 20);
            Vec3f vertexC = (bandCenter - bandCenter / 10) - (bandCross / 20);
            Vec3f vertexD = (bandCenter - bandCenter / 10) + (bandCross / 20);
            
            // Four top corners
            Vec3f vertexE = vertexA + Vec3f(0, pillarRadius, 0);
            Vec3f vertexF = vertexB + Vec3f(0, pillarRadius, 0);
            Vec3f vertexG = vertexC + Vec3f(0, pillarRadius, 0);
            Vec3f vertexH = vertexD + Vec3f(0, pillarRadius, 0);
            
            // Creates Mesh that draws a solid rectangular prism
            Mesh m{Mesh::TRIANGLE_STRIP};
            m.vertex(vertexA);  // 0
            m.vertex(vertexE);  // 1 -- Each highlighted value is changed over time to control height
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
        // Initializes cloud mesh, random position (with controlled height), color according to band, and random velocity
        for(int i=0; i<numClouds; i++){
            
            Mesh cloud{Mesh::TRIANGLE_FAN};
            Vec3f center(rng.normal() * (maxRadius/3), (float(i) * 0.0005) + 2*(1+waveHeight), rng.normal() * (maxRadius/3));
            
            // for each point in cloud
            for(int j=0; j<cloudSize; j++){
                cloud.color(HSV(float(i%BAND_N)/BAND_N, 0.25, 1));
            }
            
            clouds.push_back(cloud);
            cloudPos.push_back(center);
            cloudVelocity.push_back(Vec2f(rng.normal()*2, rng.normal()*2));
        }
    }
    
    // -----------------------------------------------------------------------
    //  Draw
    // -----------------------------------------------------------------------
    
    virtual void onDraw(Graphics& g) override {
        if(paused) return;
        
        if (hasRole(ROLE_RENDERER) || hasRole(ROLE_DESKTOP) ||
            hasRole(ROLE_SIMULATOR)) {
            
            g.clear(0);
            g.depthTesting(true);
            g.blending(true);
            g.blendModeTrans();
            
            drawWaves(g);
            drawPillars(g);
            drawClouds(g);
        }
        
    }
    
    void drawPillars(Graphics& g) {
        
        // For each pillar
        for (int i = 0; i < 2 * BAND_N; i++) {
            float height, color;
            
            // Finds height / color for pillars in first half of circle
            if (i < BAND_N) {
                color = (float)i / BAND_N;
                height = state().soundVals[i][state().soundIndex];
            }
            
            // Finds height / color for pillars in second half of circle
            else {
                color = (float)(i - BAND_N) / BAND_N;
                height = state().soundVals[i - BAND_N][state().soundIndex];
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
            
            // Updates color:
            //  hue range is from 0.33 - 0.667 (according to band)
            //  saturation range is from 0.5 - 1.0 (according to height)
            //  value range is from 0.667 - 1.0 (according to height)
            g.color(HSV(0.33+color*0.667, (height+5)/10, (height+2)/3));
            
            g.draw(pillars[i]);
        }
    }
    
    void drawWaves(Graphics& g) {
        
        // For each band, draws one wave
        for (int i = 0; i < BAND_N; i++) {
            
            Mesh m{Mesh::TRIANGLE_STRIP};
            
            // Inner wave is 100%, outer wave is 0%
            float wavePercent = 1 - (float)(i + 1) / BAND_N;
            
            // For each stored sound value, one vertex is added
            for (int j = 0; j < HIST_N; j++) {
                
                // Percentage through circle
                float valPercent = (float)j / HIST_N;
                
                // Finds corresponding sound value
                float val = state().soundVals[i][(j+state().soundIndex)%HIST_N];
                
                // Polar coordinates to determine place in circle
                float x = (waveRadius + (wavePercent) * waveRadius) * cos(valPercent * 2 * M_PI);
                float z = (waveRadius + (wavePercent) * waveRadius) * sin(valPercent * 2 * M_PI);
                
                // Height according to sound value
                float y = (val * waveHeight) + (wavePercent * waveHeight);
                
                // Pushes a vertex at height and 0 to create solid wave
                m.vertices().push_back(Vec3f(x, y, z));
                m.vertices().push_back(Vec3f(x, 0, z));
                
                // Color:
                //  hue is from 0 - 0.33
                //  saturation is from 0.5 - 1.0
                //  value is from 0.3 - 0.7
                m.color(HSV(wavePercent*0.33, (val+1)/2, (val*0.7)+0.3));
                
                // Lower value is darker to create gradient that highlights the waves in front
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
        if(paused) return;
        
        if (hasRole(ROLE_RENDERER)) {
            
            // Sets pose to saved pose
            pose() = state().pose;
            
        }
        
        
        animateClouds();
        
    }
    
    
    virtual void animateClouds(){
        // Separate cloud iterator since it loops according to band
        int cloudIterator = 0;
        
        // for each band
        for(int i=1; i<BAND_N; i++){
            
            // for each cloud in band[i]
            for(int j=0; j<(numClouds/BAND_N); j++){
                
                // Finds distance from center to determine if it is within radius
                float dist = sqrt((cloudPos[cloudIterator].x * cloudPos[cloudIterator].x)+(cloudPos[cloudIterator].z * cloudPos[cloudIterator].z));
                
                // Bounces if it reaches max radius
                if(dist >= maxRadius) cloudVelocity[cloudIterator] *= -1;
                
                // Places clouds slightly above max wave height, and each "band" of clouds is slightly above the last
                float height = (float(cloudIterator) * 0.005) + 2*(waveHeight);
                
                // hue ranges from 0.5 - 1.0 (according to band)
                float hue = 0.5 + 0.5*float(i)/BAND_N;
                
                
                // get last center point
                Vec3f center = cloudPos[cloudIterator];
                
                // Moves center based on velocity * current sound value
                // This causes them to jump and dance during loud sounds
                center.x += cloudVelocity[cloudIterator].x * 0.01 * state().soundVals[i][state().soundIndex]*2;
                center.z += cloudVelocity[cloudIterator].y * 0.01 * state().soundVals[i][state().soundIndex]*2;
                
                // Update height
                center.y = height;
                
                // Update stored center position
                cloudPos[cloudIterator] = center;
                
                // Clear positions and colors for update
                clouds[cloudIterator].vertices().clear();
                clouds[cloudIterator].colors().clear();
                
                // Init cloud at new center with darker color to create gradient moving outwards
                clouds[cloudIterator].vertex(center);
                clouds[cloudIterator].color(HSV(hue, 0.1, 0.5));
                
                Vec3f soundPoint;
                float soundVal;
                
                //for each point in cloud[cloudIterator]
                for(int k=0; k<cloudSize; k++){

                    // find sound value and clamp to 0-1 (just in case)
                    float rawVal =  state().soundVals[i][(state().soundIndex - (k+HIST_N))%HIST_N];
                    soundVal = rawVal <= 0 ? 0 : rawVal >= 1 ? 1 : rawVal;
                    
                    float angle = (float(k)/cloudSize) * M_2PI;
                    
                    // find point based on sound val and position in circle
                    soundPoint = Vec3f(center.x+(soundVal*cos(angle)), height, center.z+(soundVal*sin(angle)));
                    
                    // Add new vertex to mesh
                    clouds[cloudIterator].vertex(soundPoint);
                    
                    // Add color:
                    //  Hue according to band
                    //  Saturation between 0.125 - 0.625
                    //  Value between 0.667 - 1.0
                    clouds[cloudIterator].color(HSV(hue, (0.25+soundVal)/2, (2+soundVal)/3));
                    
                }
                
                // Add point averaged between first and last edge points to smooth updating edge
                Vec3f avePoint = (clouds[cloudIterator].vertices()[1] + clouds[cloudIterator].vertices()[cloudSize]) / 2;
                clouds[cloudIterator].vertex(avePoint);
                clouds[cloudIterator].color(clouds[cloudIterator].colors()[1]);
                
                cloudIterator++;
                
            }
        }
        
        
    }
    
    // -----------------------------------------------------------------------
    //  Sound
    // -----------------------------------------------------------------------
    
    virtual void onSound(AudioIOData& io) override {
        // Audio will recieve state from simulator
        while (io() && !paused) {
            if (stft(samplePlayer())) {
                int currentBand = 0;
                
                // Update soundVal index so that it iterates circularly
                state().soundIndex = (1+state().soundIndex)%HIST_N;
                
                // for each band in Bark scale
                for (int i = 0; i < BAND_N; i++) {
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
                    
                    // find difference between last value and current value
                    // divide by 20 to smooth changes
                    float changeVal = (distRatio - state().soundVals[i][(state().soundIndex-1)%HIST_N]) / 20;
                    
                    // set current value to last value + change
                    state().soundVals[i][state().soundIndex] = state().soundVals[i][(state().soundIndex-1)%HIST_N] + changeVal;
                }
            }
            
            // output current sound frame to all channels such that they add up to original amplitude/volume
            float f = samplePlayer.read(0) / io.channelsOut();
            for(int i=0; i<io.channelsOut(); i++){
                io.out(i) = f;
            }
        }
    }
    
    // -----------------------------------------------------------------------
    //  Control
    // -----------------------------------------------------------------------
    
    void onKeyDown(const Keyboard& k) override {
        
        // on spacebar press, pause output and reset sample player
        if (k.key() == ' ') {
            paused = !paused;
            samplePlayer.reset();
        }
    }
    
};

int main() {
    DistributedExampleApp app;
    app.start();
    return 0;
}
