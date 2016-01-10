//
// MAT201B
// Fall 2015 Final Project
// Original Author(s): Tim, Karl
// New Author: Chang HE (Hilda)
// 
// Description:
// The program randomly generates lightning which is the simulation of plasma and shoots the nucleus on its surface,
// and the nucleus will bulge in direction of the lightning to absorb the plasma. 
// Audience can interact with Plasma to generate new lightning by pointing the direction they want and pinch using PhaseSpace. 
// There’s also electric sound when every plasma is generated.
//  
// Interaction:
//    1. you can use PhaseSpace
//    2. Press "+" key to accelerate the speed of lightnings 
//       Press "-" key tp slpow down the speed of lightnings
//
// Future Developments:
//    1. Snowflake effect at the starting point of the lightning
//    2. Volume rendering 
//    3. 3D lighting algorithms
//    4. GPU threshold in allo_render
//    5. Spatial sound
//
//
// Reference:
// http://gamedevelopment.tutsplus.com/tutorials/how-to-generate-shockingly-good-2d-lightning-effects--gamedev-2681
// For more realistic lightning see
// http://gamma.cs.unc.edu/FAST_LIGHTNING/lightning_tvcg_2007.pdf
//
//
// Thanks for the huge help from Karl Yerkes and Tim Wood. 
// Also thank to Zenghui Yan, Qiaodong Cui, Jingxiang Liu, Kurt Kaminski and my mom and dad. 
//
//


#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"
#include "allocore/io/al_App.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include "common_4.hpp"                 // XXX
#include "Cuttlebone/Cuttlebone.hpp"  // XXX
#include "alloutil/al_Simulator.hpp"


using namespace al;
using namespace std;

//struct AlloApp : App {
struct AlloApp : OmniStereoGraphicsRenderer { 
  Material material;
  Light light;
  Vec3f nucleusPose;
  //spheres
  Mesh nucleus;
  Mesh shell;
  //lightning bolts and bulges
  Mesh bolts[5];
  Mesh bulge[5];
  Vec3f endingPositions[5];
  Vec3f bulgeScale[5];
  Vec3f bulgePosition[5];
  Texture texture;

  cuttlebone::Taker<State> taker;  // XXX
  State state;                     // XXX

  AlloApp() {

    texture = Texture(256, 1, Graphics::LUMINANCE_ALPHA, Graphics::UBYTE, true);
    Array& sprite(texture.array());
    struct{
      uint8_t l, a;
    }lum;
    //texture fading out at the edges   
    for (size_t row = 0; row < sprite.height(); ++row) {
      float y = float(row) / (sprite.height() - 1) * 2 - 1;
      for (size_t col = 0; col < sprite.width(); ++col) {
        float x = float(col) / (sprite.width() - 1) * 2 - 1;
        // draw lightning, white in the center, and fades out toward the edges at two rates
        if (abs(x) < 0.2)
          lum.l = mapRange(abs(x), 0.f, .2f, 255.f, 60.f);
        else
          lum.l = mapRange(abs(x), 0.2f, 1.f, 60.f, 0.f);
        lum.a = lum.l;
        sprite.write(&lum.l, col, row);
      }
    }
    for(int i = 0; i < 5; i++){
      bolts[i].primitive(Graphics::TRIANGLES);
    }

    //add nucleus and shell
    addSphere(nucleus, 0.1, 64, 64);
    for (int i = 0; i < nucleus.vertices().size(); i++){
      nucleus.color(Color(HSV(0.7, 0.5, 1.0), 0.6));
    }
    nucleus.generateNormals();
    addSphere(shell, R, 64, 64);
    for (int i = 0; i < shell.vertices().size(); i++){
      shell.color(Color(HSV(0.9, 0.5, 1.0), 0.2));
    }
    shell.generateNormals();

    //add bulges
    for(int i = 0; i < 5; i++){
      bulge[i].primitive(Graphics::TRIANGLES); 
      addSphere(bulge[i], 1, 24, 24);
      for (int j = 0; j < bulge[i].vertices().size(); i++){
        bulge[j].color(Color(HSV(0.7, 0.5, 1.0), 0.6));
      }
      bulge[i].generateNormals();
    }

    //settings
    //nav().pos(0, 0.5, 2);
    light.pos(10, 10, 10);
    material.specular(light.diffuse() * 0.2);  // Specular highlight, "shine"
    material.shininess(50);  // Concentration of specular component [0,128]

    lens().near(0.05);
    lens().far(8);

    //initWindow();
    omni().clearColor() = Color(0.12);
  }

  virtual void onDraw(Graphics& g) {

    //draw behind lightnings 
    g.depthTesting(false);
    g.blending(true); 
    g.blendModeTrans();
    shader().uniform("texture", 1.0);
    for(int i = 0; i < 5 && i < state.numberOfBolts ; i++){
      if ((pose.pos() - nucleusPose).dot(endingPositions[i] - nucleusPose) < 0) {
        texture.bind();
        g.draw(bolts[i]);
        texture.unbind();
      }
    }
    shader().uniform("texture", 0.0);

    material();
    light();

    //nucleus
    g.depthTesting(true);
    g.blending(false);
    shader().uniform("lighting", 0.7);
    g.pushMatrix();
    g.translate(nucleusPose);
    g.draw(nucleus);
    g.popMatrix();
    shader().uniform("lighting", 0.0);

    //draw front lightnings
    g.depthTesting(false);
    g.blending(true);
    g.blendModeTrans();
    shader().uniform("texture", 1.0);
    for(int i = 0; i < 5 && i < state.numberOfBolts ; i++){
      if ((pose.pos() - nucleusPose).dot(endingPositions[i] - nucleusPose) >= 0) { 
        texture.bind();
        g.draw(bolts[i]);
        texture.unbind();
      }
    }
    shader().uniform("texture", 0.0);

    //bulge
    g.depthTesting(true);
    g.blending(false);
    shader().uniform("lighting", 0.7);
    for(int i = 0; i < 5 && i < state.numberOfBolts ; i++){
      g.pushMatrix();
        g.translate(bulgePosition[i]);
        g.scale(bulgeScale[i]);
        g.draw(bulge[i]);
      g.popMatrix();
    }
    shader().uniform("lighting", 0.0);

    //draw shell
    // g.depthTesting(true);
    // g.blending(true);
    // g.blendModeTrans();
    // g.draw(shell);
  }

  virtual void onAnimate(double dt) {
    taker.get(state); // XXX

    //nucleus
    nucleusPose = state.nucleusPose;
    pose = state.pose;
  
    //lightning
    for(int i = 0; i <state.numberOfBolts && i < 5; i++){
      bolts[i].reset();
      endingPositions[i] = Vec3f(0,0,0);
      for(int j = 0; j< state.flatBolt[i].numberOfPoints && j < VERTEX_COUNT; j++){
        bolts[i].vertex(state.flatBolt[i].points[j]);
        bolts[i].texCoord(state.flatBolt[i].textCoe[j]);
        bolts[i].color(state.flatBolt[i].color[j]);
      }
      if(state.flatBolt[i].numberOfPoints > VERTEX_COUNT){
          cout<< "bigger number appear for numberOfPoints: " << state.flatBolt[i].numberOfPoints << endl;
      }
      endingPositions[i] = state.flatBolt[i].ending;
    }
    //bulges
    for(int i = 0 ; i < state.numberOfBolts && i < 5; i++){
      bulgeScale[i] = state.flatBolt[i].bulgeScale;
      bulgePosition[i] = state.flatBolt[i].bulgePosition;
    }
  }

inline std::string vertexCode() {
  // XXX use c++11 string literals
  return R"(
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  color = gl_Color;
  vec4 vertex = gl_ModelViewMatrix * gl_Vertex;
  normal = gl_NormalMatrix * gl_Normal;
  vec3 V = vertex.xyz;
  eyeVec = normalize(-V);
  lightDir = normalize(vec3(gl_LightSource[0].position.xyz - V));
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = omni_render(vertex);
}
)";
}

inline std::string fragmentCode() {
  return R"(
uniform float lighting;
uniform float texture;
uniform sampler2D texture0;
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  vec4 colorMixed;
  if (texture > 0.0) {
    vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
    //colorMixed = mix(color, textureColor, texture);
    colorMixed = color * textureColor;
  } else {
    colorMixed = color;
  }
  vec4 final_color = colorMixed * gl_LightSource[0].ambient;
  vec3 N = normalize(normal);
  vec3 L = lightDir;
  float lambertTerm = max(dot(N, L), 0.0);
  final_color += gl_LightSource[0].diffuse * colorMixed * lambertTerm;
  vec3 E = eyeVec;
  vec3 R = reflect(-L, N);
  float spec = pow(max(dot(R, E), 0.0), 0.9 + 1e-20);
  final_color += gl_LightSource[0].specular * spec;
  gl_FragColor = mix(colorMixed, final_color, lighting);
}
)";
}


};

int main() {
  AlloApp app;
  app.taker.start();  // XXX
  app.start();
}
