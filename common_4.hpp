//
// MAT201B Final Project
// Fall 2015
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

#include "allocore/io/al_App.hpp"
#include <deque>

#ifndef __COMMON_STUFF__
#define __COMMON_STUFF__

#define R 5
#define VERTEX_COUNT (3600)
#define N_SAMPLE_PLAYER (5)

struct FlatBolt {
	int numberOfPoints;
	Vec3f points[VERTEX_COUNT];
  Vec2f textCoe[VERTEX_COUNT];
  Color color[VERTEX_COUNT];
  Vec3f ending;
  Vec3f bulgePosition;
  Vec3f bulgeScale;
};

struct State {
  	Pose pose;
  	int frame;
  	int numberOfBolts;
  	FlatBolt flatBolt[5];
    Vec3f nucleusPose;
};


class Bolt {
  public:
  Mesh mesh;
  Color color;
  double strikeTime;
  double timer;
  Texture texture;
  Vec3f start;
  Vec3f ending;
  Mesh bulgeMesh;
  Vec3f bulgePosition;
  Vec3f bulgeScale;
  //float increment;
  // texture we will write our lightning sprite into
  // it only needs to be 1 pixel wide because we will repeat the texture side
  // by side
  Bolt(){
    strikeTime = 1.5;
    timer = strikeTime;
    color = Color(1, 0.7, 1, 1);
  }

  void makeTexture(){
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
        // draw lightning, white in the center, and fades out toward the edges
        // at two rates
        if (abs(x) < 0.2)
          lum.l = mapRange(abs(x), 0.f, .2f, 255.f, 60.f);
        else
          lum.l = mapRange(abs(x), 0.2f, 1.f, 60.f, 0.f);
        lum.a = lum.l;
        sprite.write(&lum.l, col, row);
      }
    }
  }

  // generate a bolt of lightning and add to our mesh
  // modified from Michael Hoffman's
  // (http://gamedevelopment.tutsplus.com/tutorials/how-to-generate-shockingly-good-2d-lightning-effects--gamedev-2681)
  void makeBolt(Vec3f source, Vec3f dest, int maxBranches = 0,
            float branchProb = 0.01, float wid = 0.05, int n = 80) {

    Vec3f tangent = (dest - source);  // direction of lightning
    Vec3f normal =
        tangent.cross(Vec3f(0, 0, -1)).normalize();  // normal to lightning
    float length = tangent.mag();

    // choose n random positions (0,1) along lightning vector and sort them
    vector<float> positions;
    positions.push_back(0);
    for (int i = 0; i < n; i++) positions.push_back(rnd::uniform());
    sort(positions.begin(), positions.end());

    float sway = 1;  // max random walk step of displacement along normal
    float jaggedness = 1 / sway;

    Vec3f prevPoint = source;
    float prevDisplacement = 0;
   
    //float width = 0.06; 
    float width = (wid > 0) ? wid : length / 25.0 + 0.01;
    //color = Color(1, 0.7, 1, 1);
    int branches = 0;

    mesh.primitive(Graphics::TRIANGLES);

    Vec3f point;

    for (int i = 1; i < n; i++) {
      float pos = positions[i];

      // used to prevent sharp angles by ensuring very close positions also have
      // small perpendicular variation.
      float scale = (length * jaggedness) * (pos - positions[i - 1]);

      // defines an envelope. Points near the middle of the bolt can be further
      // from the central line.
      float envelope = pos > 0.95f ? mapRange(pos, 0.95f, 1.0f, 1.0f, 0.0f) : 1;

      // displacement from prevDisplacement (random walk (brownian motion))
      float displacement = rnd::uniformS(sway) * scale + prevDisplacement;
      displacement *= envelope;

      // calculate point, source plus percentage along tangent, and displacement
      // along normal;
      point = source + pos * tangent + displacement * normal;

      // generate triangles (vertices and texture coordinates) from point and
      // prevPoint
//**********
      mesh.vertex(prevPoint + normal * width);
      mesh.vertex(prevPoint - normal * width);
      mesh.vertex(point + normal * width);
      mesh.vertex(point + normal * width);
      mesh.vertex(prevPoint - normal * width);
      mesh.vertex(point - normal * width);
      mesh.texCoord(0, (float)(i - 1) / n);
      mesh.texCoord(1, (float)(i - 1) / n);
      mesh.texCoord(0, (float)(i) / n);
      mesh.texCoord(0, (float)(i) / n);
      mesh.texCoord(1, (float)(i - 1) / n);
      mesh.texCoord(1, (float)(i) / n);
      mesh.color(color);
      mesh.color(color);
      mesh.color(color);
      mesh.color(color);
      mesh.color(color);
      mesh.color(color);
//**********
      if (branches < maxBranches && rnd::prob(branchProb)) {
        branches++;
        Vec3f dir(tangent);
        rotate(dir, Vec3f(0, 0, 1), rnd::uniformS(30) * M_DEG2RAD);
        dir.normalize();
        float len = (dest - point).mag();
        makeBolt(point, point + dir * len * 0.4, maxBranches - 1, branchProb, width * 0.6,
             n * 0.5);
      }
      // remember previous point and displacement for next iteration
      prevPoint = point;
      prevDisplacement = displacement;
    }
    //generate last piece
    mesh.vertex(point + normal * width);
    mesh.vertex(point - normal * width);
    mesh.vertex(dest + normal * width);
    mesh.vertex(dest + normal * width);
    mesh.vertex(point - normal * width);
    mesh.vertex(dest - normal * width);
    mesh.texCoord(0, (float)(n - 1) / n);
    mesh.texCoord(1, (float)(n - 1) / n);
    mesh.texCoord(0, (float)(n) / n); //0
    mesh.texCoord(0, (float)(n) / n); //0
    mesh.texCoord(1, (float)(n - 1) / n);
    mesh.texCoord(1, (float)(n) / n); //1
    mesh.color(color);
    mesh.color(color);
    mesh.color(color);
    mesh.color(color);
    mesh.color(color);
    mesh.color(color);
  }

  void fadeOut(){
    if(strikeTime - timer > 0.3f){
      for(int i=0;i<mesh.colors().size();i++){
        double coe = 0.94 * (i+1) / mesh.colors().size();
        if(coe < 0.45){
          coe += 0.45;
        }else if(coe > 0.97){
          coe = 0.97;
        }
        mesh.colors()[i] *= coe; 
      }
    }
  }
  void countDown(double dt){
    timer -= dt;
  }

  void createBulge(Vec3f nucleusP){
    bulgeScale = Vec3f(0.035, 0.035, 0.035);
    bulgePosition = (ending - nucleusP) * 0.6f + nucleusP;
    mesh.primitive(Graphics::TRIANGLES); 
    addSphere(bulgeMesh);
    for (int i = 0; i < bulgeMesh.vertices().size(); i++){
      bulgeMesh.color(Color(HSV(0.7, 0.5, 1.0), 0.6));
    }
    bulgeMesh.generateNormals();
  }

  void bulge(Vec3f nucleusP){
    Vec3f destination = 0.03f * (ending - nucleusP) + bulgePosition;
    if(timer > 1.15f) {//前0.25秒
      bulgePosition += 0.5f * (destination - bulgePosition);
    }else{//之后
      bulgePosition +=  0.007f * (nucleusP - ending);
      bulgeScale += Vec3f(0.0002, 0.0002, 0.0002); 
      if(bulgeScale.x > 0.07f){
        bulgeScale = Vec3f(0.07f, 0.07f, 0.07f);
      }
    }
  }

};

#endif
