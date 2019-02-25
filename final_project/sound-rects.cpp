// sound-lines - a simple spectrogram-like output of a sampled audio clip
// Mitchell Lewis, 2-20-19


#include "al/core.hpp"
using namespace al;

#include "Gamma/DFT.h"
#include "Gamma/SamplePlayer.h"
using namespace gam;

#include <vector>
using namespace std;

#define N (513)  // number of particles
#define DISP_WIDTH (10.0)


// --------------------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------
//                                          APP START
// --------------------------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------------------------


struct AlloApp : App {
    ShaderProgram shader;
    Texture texture;
    
    Mesh pointMesh;          // holds positions

    gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> samplePlayer;
    gam::STFT stft;
    AudioIO audio;

    vector<float> spectrum;
    
    // measures magnitude of each band and records it as a distance\
    // also keeps track of max value per band for normalization
    vector<float> bandDist;
    vector<float> max;
    
    vector<float> pointDist;
    vector<float> pointMax;
    
    vector<int> bandWidths{ 2,2,2,2,2,3,3,3,3,4,4,5,6,7,8,10,12,15,19,23,28,38,53,75,90,94 }; // Bark Scale - # of stft bands per Bark band
    float incAmount = 0.5;
    float decAmount = 0.25;
    float heightScale = 10;
    float rectWidth, eqRectWidth;
    bool analyzeInput = false; // if true: analyze input channel, else: analyze sample
    bool displayMode = false;  // true: show equal-width rectangle bands, false: show all bands
    
    void onCreate() override {
        if(!analyzeInput){
            samplePlayer.load("../sound/9.wav");
            samplePlayer.loop();
        }
        Sync::master().spu(audioIO().fps());
        
        int n = bandWidths.size();
        rectWidth = DISP_WIDTH / stft.numBins();
        eqRectWidth = DISP_WIDTH / bandWidths.size();
        
        for(int i=0; i<n; i++){
            bandDist.push_back(0);
            max.push_back(0.1);
        }
        
        for(int i=0; i<stft.numBins(); i++){
            pointDist.push_back(0);
            pointMax.push_back(0.1);
            
        }
        
        nav().pos(0, 0, 40); // place the viewer at z=40
        

    }
    
    void rect(Graphics& g, float x1, float x2, float height, float progress){
        if(height == 0) return;
        
        float centerX1 = x1 - DISP_WIDTH/2;
        float centerX2 = x2 - DISP_WIDTH/2;
        
        Mesh m{Mesh::TRIANGLE_FAN};
        m.vertex(Vec2f(centerX1, -height));
        m.vertex(Vec2f(centerX2, -height));
        m.vertex(Vec2f(centerX2, height));
        m.vertex(Vec2f(centerX1, height));
        g.color( HSV(progress, abs(2*height/heightScale), abs(2*height/heightScale)) );
        g.draw(m);
    }
    
    void line(Graphics& g, float x, float height){
        float centerX = x - DISP_WIDTH/2;
        
        Mesh m{Mesh::LINES};
        m.vertex(Vec2f(centerX, -height));
        m.vertex(Vec2f(centerX, height));
        g.color( HSV((x/DISP_WIDTH), abs(4*height/heightScale), abs(4*height/heightScale)) );
        g.draw(m);
    }
    

    
    void onSound(AudioIOData& io) override {
        vector<Vec3f>& vertex(pointMesh.vertices());
        while (io()) {
            
            // only works on my laptop, uses virtual input device to analyze output
            if(analyzeInput){
                if (stft(io.in(0))) {
                    int currentBand = 0;
                    
                    // for each band in Bark scale
                    for(int i=0; i<bandWidths.size(); i++){
                        float barkSum = 0;
                        
                        // for each STFT band
                        for(int j=0; j<bandWidths[i]; j++){
                            barkSum += stft.bin(currentBand).norm() * 1000;
                            
                            // calculate height of band
                            float pDist = stft.bin(currentBand).norm() * 1000;
                            
                            // update max if necessary
                            if(pDist > pointMax[currentBand]) pointMax[currentBand] = pDist;
                            
                            float pDistRatio = heightScale * pDist/pointMax[currentBand];
                            
                            // set point distance to normalized band amplitude
                            pointDist[currentBand] += (pDistRatio - pointDist[currentBand])/(heightScale*2);
                            
                            currentBand++;
                        }
                        
                        // calculate mean height of Bark band
                        float dist = barkSum / bandWidths[i];
                        
                        // update max if necessary
                        if(dist > max[i]) max[i] = dist;
                        float distRatio = heightScale * dist/max[i];
                        
                        // set point distance to normalized band amplitude
                         bandDist[i] += (distRatio - bandDist[i])/(heightScale*2);
                        
                    }
                }
            }
            else{
                if (stft(samplePlayer())) {
                    int currentBand = 0;
                    
                    // for each band in Bark scale
                    for(int i=0; i<bandWidths.size(); i++){
                        float barkSum = 0;
                        
                        // for each STFT band
                        for(int j=0; j<bandWidths[i]; j++){
                            barkSum += stft.bin(currentBand).norm() * 1000;
                            
                            // calculate height of band
                            float pDist = stft.bin(currentBand).norm() * 1000;
                            
                            // update max if necessary
                            if(pDist > pointMax[currentBand]) pointMax[currentBand] = pDist;
                            
                            float pDistRatio = heightScale * pDist/pointMax[currentBand];
                            
                            // set point distance to normalized band amplitude
                            pointDist[currentBand] += (pDistRatio - pointDist[currentBand])/(heightScale*2);
                            
                            currentBand++;
                        }
                        
                        // calculate mean height of Bark band
                        float dist = barkSum / bandWidths[i];
                        
                        // update max if necessary
                        if(dist > max[i]) max[i] = dist;
                        float distRatio = heightScale * dist/max[i];
                        
                        // set point distance to normalized band amplitude
                        bandDist[i] += (distRatio - bandDist[i])/(heightScale*2);
                        
                    }
                }
                // output the current audio frame
                float f = samplePlayer.read(0);
                io.out(0) = f;
                io.out(1) = f;
            }
            
            
            
            
        }
    }
    
    void onAnimate(double dt) override {
        
        
    }
    
    void onDraw(Graphics& g) override {
        g.clear(0.23);
        
        g.depthTesting(false);
        g.blending(true);
        g.blendModeTrans();

        // shows all bands, with Bark scale rectangles behind
        if(displayMode){
            float x = 0;
            for(int i=0; i<bandWidths.size(); i++){
                float progress = (float)i /bandWidths.size();
                float x2 = x+(rectWidth*bandWidths[i]);
                rect(g, x, x2, bandDist[i], progress);
                x = x2;
            }
            for(int i=0; i<pointDist.size(); i++){
                float x = ((float) i / pointDist.size()) * DISP_WIDTH;
                line(g, x, pointDist[i]/2);
            }
        }
        
        // shows only Bark Scale rectangles
        else{
            float x = 0;
            for(int i=0; i<bandWidths.size(); i++){
                float progress = (float)i /bandWidths.size();
                float x2 = x+(eqRectWidth);
                rect(g, x, x2, bandDist[i], progress);
                x = x2;
            }
        }
    }
    
    void onKeyDown(const Keyboard& k) override {
        switch (k.key()) {
            case 'o':
                displayMode = !displayMode;
                
            
        }
        //
    }
    
    bool getAnalysisMode() { return analyzeInput; }
};



int main() {
    AlloApp app;
    if(app.getAnalysisMode()){
        app.initAudio(44100, 512, -1,-1,3);
    }
    else{
        app.initAudio();
    }
    app.start();
    
}
