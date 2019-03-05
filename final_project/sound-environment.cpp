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
#define DISP_WIDTH (5.0)


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
    
    vector<vector<float>> waves;
    float waveWidth = 1000;
    float waveHeight = 5;
    float xWidth = 10;
    float zWidth = 10;
    float waveRadius = 5;
    
    
    vector<int> bandWidths{ 2,2,2,2,2,3,3,3,3,4,4,5,6,7,8,10,12,15,19,23,28,38,53,75,90,94 }; // Bark Scale - # of stft bands per Bark band
    float heightScale = 10;
    bool analyzeInput = false; // if true: analyze input channel, else: analyze sample
    bool displayMode = false;  // true: show equal-width rectangle bands, false: show all bands
    
    void onCreate() override {
        if(!analyzeInput){
            samplePlayer.load("../sound/9.wav");
            samplePlayer.loop();
        }
        Sync::master().spu(audioIO().fps());
        
        int n = bandWidths.size();

        
        for(int i=0; i<N; i++){
            bandDist.push_back(0);
            max.push_back(0.1);
            
            vector<float> wave;
            for(int j=0; j<waveWidth; j++){
                wave.push_back(0);
            }
            waves.push_back(wave);
        }
        
        //Vec3d(0.000000, 17.495046, 46.419401);
        //Quatd(0.987681, -0.156482, 0.000000, 0.000000);
        nav().pos(0.000000, 17.495046, 46.419401);
        nav().quat(Quatd(0.987681, -0.156482, 0.000000, 0.000000));

    }

    void drawWaves(Graphics& g, vector<vector<float>> waves){
        
        
        for(int i=0; i<waves.size(); i++){
            Mesh m{Mesh::LINE_STRIP};
            float wavePercent = 1 - (float) (i+1) / waves.size();  //
            for(int j=0; j<waves[i].size(); j++){
                float valPercent = (float) j / waves[i].size();
                float val = waves[i][j];
                
                float x = (waveRadius + wavePercent*xWidth)*cos(valPercent*2*M_PI);
                float z = (waveRadius + wavePercent*xWidth)*sin(valPercent*2*M_PI);
                
                float y = val*waveHeight + (wavePercent*5);
                
                m.vertices().push_back( Vec3f(x, val*waveHeight, z) );
                m.color( HSV(wavePercent, val, val) );
                
            }
            g.meshColor();
            g.draw(m);
        }
        
    }
    
    
    float radius = 4.0;
    void pillar(Graphics& g, float height, float angle){
        if(height == 0) return;
        
        float color = abs(angle/M_PI);
        if(angle < 0) color = 1 - color;
        
        Vec3f bandCenter(radius*cos(angle), 0, radius*sin(angle));
        Vec3f bandCross = bandCenter.cross(Vec3f(0, 1, 0));
        
        Vec3f vertexA = (bandCenter + bandCenter/10) - (bandCross/20);
        Vec3f vertexB = (bandCenter + bandCenter/10) + (bandCross/20);
        Vec3f vertexC = (bandCenter - bandCenter/10) - (bandCross/20);
        Vec3f vertexD = (bandCenter - bandCenter/10) + (bandCross/20);
        
        Vec3f vertexE = vertexA + Vec3f(0, height*radius, 0);
        Vec3f vertexF = vertexB + Vec3f(0, height*radius, 0);
        Vec3f vertexG = vertexC + Vec3f(0, height*radius, 0);
        Vec3f vertexH = vertexD + Vec3f(0, height*radius, 0);
        //cout << vertexA  << vertexD << endl;
        
        Mesh m{Mesh::TRIANGLE_STRIP};
        m.vertex(vertexA);
        m.vertex(vertexE);
        m.vertex(vertexF);
        m.vertex(vertexB);
        m.vertex(vertexA);
        
        m.vertex(vertexE);
        m.vertex(vertexG);
        m.vertex(vertexC);
        
        m.vertex(vertexA);
        m.vertex(vertexB);
        m.vertex(vertexD);
        
        m.vertex(vertexH);
        m.vertex(vertexF);
        m.vertex(vertexB);
        
        m.vertex(vertexD);
        m.vertex(vertexH);
        m.vertex(vertexG);
        
        m.vertex(vertexC);
        m.vertex(vertexD);
        m.vertex(vertexA);
        
        
        
        g.color( HSV(color, abs(height), abs(height)) );
        g.draw(m);
    }

    
    void onSound(AudioIOData& io) override {
        vector<Vec3f>& vertex(pointMesh.vertices());
        while (io()) {
            float s;
            if(analyzeInput) s = io.in(0);
            else {
                s = samplePlayer();
                float f = samplePlayer.read(0);
                io.out(0) = f;
                io.out(1) = f;
            }
            if (stft(s)){
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
                    if(dist > max[i]) max[i] = dist;
                    float distRatio = dist/max[i];
                    
                    // set point distance to normalized band amplitude
                    float changeVal = (distRatio - bandDist[i]);
                    bandDist[i] += changeVal/20;
                    
                }
            }
        }
    }
    
    void onAnimate(double dt) override {
        for(int i=0; i<waves.size(); i++){
            waves[i].pop_back();
            waves[i].insert(waves[i].begin(), bandDist[i]);
        }
        
        //nav().print();
        
    }
    
    void onDraw(Graphics& g) override {
        g.clear(0);
        
        g.depthTesting(false);
        g.blending(true);
        g.blendModeTrans();
        
        //drawWaves(g, waves);
        
        for(int i=0; i<bandWidths.size(); i++){
            float angle = i * (M_PI / bandWidths.size());
            pillar(g,bandDist[i], angle);
            pillar(g,bandDist[bandDist.size()-1-i], -angle);
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
