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
    vector<float> pointDist;
    vector<float> max;

    
    void onCreate() override {
        
        samplePlayer.load("../sound/9.wav");
        samplePlayer.loop();
        Sync::master().spu(audioIO().fps());
        
        spectrum.resize(stft.numBins() * 1024);
        int n = stft.numBins();
        
        
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
        pointMesh.primitive(Mesh::LINE_STRIP);
        for (int i = 0; i < N; i++) {
            
            //plots points from -5 to 5
            float xCoord = -5 + ((float)i / N)*10;
            pointMesh.vertex(Vec3f(xCoord, 0, 0));
            pointDist.push_back(0);
            max.push_back(0);

            pointMesh.color(HSV(rnd::uniform(), 1.0, 1.0));
        }
    }
    

    
    void onSound(AudioIOData& io) override {
        vector<Vec3f>& vertex(pointMesh.vertices());
        while (io()) {
            if (stft(samplePlayer())) {
                for (int i = 0; i < pointDist.size(); ++i) {
                    // record magnitude of band
                    float dist = stft.bin(i).norm() * pow(10, 4);
                    
                    // adjust distance by pre-set values to facilitate smooth motion
                    if(dist > pointDist[i]) pointDist[i] += 0.1;
                    else pointDist[i] -= 0.05;
                    
                    // update max if necessary
                    if(dist > max[i]) max[i]  = dist;
                    
                    
                }
            }
            
            // output the current audio frame
            float f = samplePlayer.read(0);
            io.out(0) = f;
            io.out(1) = f;
        }
    }
    
    void onAnimate(double dt) override {
        
        // plots band levels along x-axis
        vector<Vec3f>& vertex(pointMesh.vertices());  // make an alias
        for(int i=0; i<vertex.size(); i++){
            vertex[i].y =  10 * (pointDist[i] / max[i]);
        }
        
        
    }
    
    void onDraw(Graphics& g) override {
        // you don't have to change anything in here
        //
        g.clear(0.23);
        
        g.depthTesting(false);
        g.blending(true);
        g.blendModeTrans();
        
        // removed point shader to use lines
        
        //texture.bind();
        //g.shader(shader);
        //g.shader().uniform("halfSize", 0.15);
        g.draw(pointMesh);
        //texture.unbind();
    }
};



int main() {
    AlloApp app;
    app.initAudio();
    app.start();
    
}
