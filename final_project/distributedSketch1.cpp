/*
Example: DistributedApp

Description:
Demontration of basic usage of the DistributedApp class

Author:
Andres Cabrera 2/2018
*/

// To run this example on a distributed cluster, you first need to copy the
// built binary to the cluster's shared file system, and then run it with mpirun.
// You can use a command like:
// mpirun -n 4 -f host_file ./distributedApp
// -n 4 determines the number of precesses launched, and -f host_file
// lists the machines where things will run. It should look like:
//
// Look at http://mpitutorial.com/tutorials/mpi-hello-world/ for more details on
// running MPI applications.


#include <stdio.h>
#include "al/core/app/al_DistributedApp.hpp"
using namespace al;

#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
using namespace gam;

#include <vector>
using namespace std;

#define N (26)

gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> samplePlayer;
gam::STFT stft;

std::vector<int> bandWidths{ 2,2,2,2,2,3,3,3,3,4,4,5,6,7,8,10,12,15,19,23,28,38,53,75,90,94 }; // Bark Scale - # of stft bands per Bark band

std::vector<float> bandDist;
std::vector<float> bandMax;

std::vector<Mesh> pillars;

struct SharedState {
    float soundVals[N];
};

class DistributedExampleApp : public DistributedApp<SharedState> {
public:

    // The simulate function is only run for the simulator
    virtual void simulate(double dt) override {
        
#ifdef AL_BUILD_MPI
       
#else
   
#endif
      
    }
    
    virtual void onCreate() override {
        samplePlayer.load("../sound/9.wav");
        samplePlayer.loop();
        Sync::master().spu(audioIO().fps());
        
        for(int i=0; i<N; i++){
            bandDist.push_back(0);
            bandMax.push_back(0.1);
        }
        
        createPillars();
        
    }

    virtual void onDraw(Graphics &g) override {
        if (role() == ROLE_RENDERER || role() == ROLE_DESKTOP) {
            g.clear(0);
            
            g.depthTesting(false);
            g.blending(true);
            g.blendModeTrans();
            
            //drawWaves(g, waves);
            
            drawPillars(g, state().soundVals);
        }
    }
    
    float radius = 4.0;
    
    void createPillars(){
        for(int i=0; i<N; i++){
            float angle = i * (M_PI / N);
            float color = abs(angle/M_PI);
            Vec3f bandCenter(radius*cos(angle), 0, radius*sin(angle));
            Vec3f bandCross = bandCenter.cross(Vec3f(0, 1, 0));
            
            Vec3f vertexA = (bandCenter + bandCenter/10) - (bandCross/20);
            Vec3f vertexB = (bandCenter + bandCenter/10) + (bandCross/20);
            Vec3f vertexC = (bandCenter - bandCenter/10) - (bandCross/20);
            Vec3f vertexD = (bandCenter - bandCenter/10) + (bandCross/20);
            
            Vec3f vertexE = vertexA + Vec3f(0, radius, 0);
            Vec3f vertexF = vertexB + Vec3f(0, radius, 0);
            Vec3f vertexG = vertexC + Vec3f(0, radius, 0);
            Vec3f vertexH = vertexD + Vec3f(0, radius, 0);
            
            Mesh m{Mesh::TRIANGLE_STRIP};
            m.vertex(vertexA); // 0
            m.vertex(vertexE); // 1 -- vals that need to be modulated
            m.vertex(vertexF); // 2 --
            m.vertex(vertexB); // 3
            m.vertex(vertexA); // 4
            
            m.vertex(vertexE); // 5 --
            m.vertex(vertexG); // 6 --
            m.vertex(vertexC); // 7
            
            m.vertex(vertexA); // 8
            m.vertex(vertexB); // 9
            m.vertex(vertexD); // 10
            
            m.vertex(vertexH); // 11 --
            m.vertex(vertexF); // 12 --
            m.vertex(vertexB); // 13
            
            m.vertex(vertexD); // 14
            m.vertex(vertexH); // 15 --
            m.vertex(vertexG); // 16 --
            
            m.vertex(vertexC); // 17
            m.vertex(vertexD); // 18
            m.vertex(vertexA); // 19
            

            
            pillars.push_back(m);
            
        }
    }
    
    void drawPillars(Graphics& g, float soundVals[]){
        for(int i=0; i<N; i++){
            float color = abs(i / (M_PI * N));
            float height = soundVals[i];
            
            // update height
            pillars[i].vertices()[1].y = radius*height;
            pillars[i].vertices()[2].y = radius*height;
            pillars[i].vertices()[5].y = radius*height;
            pillars[i].vertices()[6].y = radius*height;
            pillars[i].vertices()[11].y = radius*height;
            pillars[i].vertices()[12].y = radius*height;
            pillars[i].vertices()[15].y = radius*height;
            pillars[i].vertices()[16].y = radius*height;
            
            g.color( HSV(color, abs(height), abs(height)) );
            g.draw(pillars[i]);
            
        }
    }
    
    
    virtual void onSound(AudioIOData &io) override {
        if (role() == ROLE_AUDIO || role() == ROLE_DESKTOP) {
            // Audio will recieve state from simulator
            while (io()) {

                if (stft(samplePlayer())){
                    int currentBand = 0;
                    
                    // for each band in Bark scale
                    for(int i=0; i<bandWidths.size(); i++){
                        float barkSum = 0;
                        
                        // for each STFT band
                        for(int j=0; j<bandWidths[i]; j++){
                            barkSum += stft.bin(currentBand).norm() * 1000;
                            currentBand++;
                        }
                        
                        // calculate mean height of Bark band
                        float dist = barkSum / bandWidths[i];
                        
                        // update max if necessary
                        if(dist > bandMax[i]) bandMax[i] = dist;
                        float distRatio = dist/bandMax[i];
                        
                        // set point distance to normalized band amplitude
                        float changeVal = (distRatio - bandDist[i]);
                        bandDist[i] += changeVal/20;
                        
                        state().soundVals[i] = bandDist[i];
                        
                        
                    }
                    
                    
                }
                
                float f = samplePlayer.read(0);
                io.out(0) = f;
                io.out(1) = f;
            }
        }
    }

};

int main(){
    DistributedExampleApp app;
    //app.fps(1);
    app.startFPS();
    app.print();
    app.initAudio();
    for (int i = 0; i < 10; i++) {
        if (app.isPrimary()) {
            std::cout << " Run " << i << " ---------------" <<std::endl;
        }
        app.simulate(0);
    }
    app.start();
    return 0;
}
