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
#include <vector>
#include <algorithm>
#include <cmath>
#include <deque>
#include "Cuttlebone/Cuttlebone.hpp"
#include "alloutil/al_Simulator.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include <Gamma/SamplePlayer.h>

string fullPathOrDie(string fileName, string whereToLook = ".") {
  SearchPaths searchPaths;
  searchPaths.addSearchPath(".");
  string filePath = searchPaths.find(fileName).filepath();
  assert(filePath != "");
  return filePath;
}

using namespace al;
using namespace std;

#include "common_4.hpp"
#include "phasespace_interaction.hpp"

void printFactsAboutState(int size);

struct AlloApp : App, AlloSphereAudioSpatializer, InterfaceServerClient {
  double time;
  double pace;
  double upperbound;
  Material material;
  Light light;
  Vec3f center;
  Vec3f nucleusPose;
  Vec3f affection;
  Mesh nucleus;
  Mesh shell;
  std::deque<Bolt*> boltQ;
  SoundSource soundSource;
  gam::SamplePlayer<> samplePlayer[N_SAMPLE_PLAYER];
  int currentPlayer = 0;

  cuttlebone::Maker<State> maker;  // XXX
  State state;                     // XXX
  PS ps;

  AlloApp() 
    : maker(Simulator::defaultBroadcastIP()),                        // XXX
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()) // XXX
        {

    printFactsAboutState(sizeof(State));

    time = 0;
    pace = 1.0;
    upperbound = 2.5;
    center = Vec3f(0, 0.6, -1);
    nucleusPose = center;
    state.frame = 0;  // XXX

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

    // set interface server nav/lens to App's nav/lens
    InterfaceServerClient::setNav(nav());    // XXX
    InterfaceServerClient::setLens(lens());  // XXX

    // Audio
     for (int i = 0; i < N_SAMPLE_PLAYER; i++) {
      samplePlayer[i].load(fullPathOrDie("electric_spark.wav").c_str());
      samplePlayer[i].phase(0.99999);
    }
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    scene()->addSource(soundSource);
    scene()->usePerSampleProcessing(true);

    // Setup settings
    light.pos(10, 10, 10);
    navControl().useMouse(false);
    initWindow();
    background(0.12);
    nav().set(Pose(Vec3d(0.000000, 0.454005, -0.011147), Quatd(0.999998, -0.001988, 0.000000, 0.000000)));
  
    //initiate phasespace
    if(Simulator::sim()) ps.init(&nav(), &state, &boltQ, samplePlayer, &currentPlayer); // Use in sphere
    else ps.initTest(&nav(), &state, &boltQ, samplePlayer, &currentPlayer); // Use on laptop
  }

  virtual void onDraw (Graphics& g, const Viewpoint& v) {
    //add lighting specular 
    material.specular(light.diffuse() * 0.2);  // Specular highlight, "shine"
    material.shininess(50);  // Concentration of specular component [0,128]

    material();
    light();

    // draw behind lightnings
    g.blendMode(g.ONE, g.ONE);
    for(std::deque<Bolt*> :: iterator it = boltQ.begin() ; it!= boltQ.end();it++){
      Bolt* bolt = *it;
      if ((nav().pos() - nucleusPose).dot(bolt->ending - nucleusPose) < 0){
        g.depthTesting(false);
        g.blending(true);
        bolt->texture.bind();
        g.draw(bolt->mesh);
        bolt->texture.unbind();
        //bulge
        g.blending(false);
        g.depthTesting(true);
        g.pushMatrix();
          g.translate(bolt->bulgePosition);
          g.scale(bolt->bulgeScale);
          g.draw(bolt->bulgeMesh);
        g.popMatrix();
      }
    }
    
    //draw newcleus
    g.blending(false);
    g.depthTesting(true);
    g.pushMatrix();
      g.translate(nucleusPose);
      g.draw(nucleus);
    g.popMatrix();
    
    // draw front lightnings
    g.blendMode(g.ONE, g.ONE);
    for(std::deque<Bolt*> :: iterator it = boltQ.begin() ; it!= boltQ.end();it++){
      Bolt* bolt = *it;
      if ((nav().pos() - nucleusPose).dot(bolt->ending - nucleusPose) >= 0){
        if(bolt->ending == Vec3f(0.1f, 0.6f, -1.f)){
          cout<<"starting point: "<<bolt->start<<endl;
        }
        g.depthTesting(false);
        g.blending(true);
        bolt->texture.bind();
        g.draw(bolt->mesh);
        bolt->texture.unbind();
        //bulge
        g.blending(false);
        g.depthTesting(true);
        g.pushMatrix();
          g.translate(bolt->bulgePosition);
          g.scale(bolt->bulgeScale);
          g.draw(bolt->bulgeMesh);
        g.popMatrix();
      }  
    }

    //draw shell
    g.depthTesting(true);
    g.blending(true);
    g.blendModeTrans();
    g.draw(shell);
  }

  virtual void onAnimate(double dt) {
    // accumulate time steps (dt) into our time variable.
    time += dt;

    if (time > pace) {
      // trigger a lightning to start
      //Audio 
      samplePlayer[currentPlayer].reset(); // reset the phase == start playing the sound
      currentPlayer++;
      if (currentPlayer == N_SAMPLE_PLAYER)
        currentPlayer = 0;

      //wiggle 
      nucleusPose += Vec3f(rnd::uniformS(0.006),rnd::uniformS(0.006),rnd::uniformS(0.006)); 
      
      //generate starting and ending point
      float z = 4.7f * rnd::uniformS();
      float y = R * rnd::uniformS();
      float x;
      if(R*R - z*z - y*y<=0){
        x = 0.f;
      }else {
        x = sqrt(R*R - z*z - y*y);
      }
      float sign = rnd::uniformS();
      if(sign <= 0){
        sign = -1;
      }else{
        sign = 1;
      }
      Vec3f source = Vec3f(sign * x, y, z);
      Vec3f dest = (source - center).normalize() * 0.1f + center;
      affection = (source - center).normalize() * 0.1;
      Bolt* newBolt = new Bolt();
      newBolt->makeTexture();
      newBolt->start = source;
      newBolt->ending = dest;
      newBolt->makeBolt(source, dest, 4, 0.06);
      newBolt->createBulge(nucleusPose);
      boltQ.push_back(newBolt); // XXX
      //reset time and pace
      time = 0;
      pace = rnd::uniform(upperbound, 0.0);
    } else {
      nucleusPose = center;
    }
    //call phasespace
    ps.step(dt);
    //fade out all the bolt in vector
    for(std::deque<Bolt*> :: iterator it = boltQ.begin() ; it!= boltQ.end();it++){
      Bolt* bolt = *it;
      bolt->fadeOut();
      bolt->countDown(dt);
      bolt->bulge(nucleusPose);
      //delete the one already disappear
      if(bolt->timer < 0.002 && it == boltQ.begin()){
        Bolt* delete_me = *boltQ.begin();
        boltQ.pop_front();
        delete delete_me;
      }
    }

    //simulator setting
    state.numberOfBolts = boltQ.size(); // XXX
    int i = 0;
    for(std::deque<Bolt*> :: iterator it = boltQ.begin() ; it!= boltQ.end();it++){
      Bolt* bolt = *it;
      if(i< boltQ.size() && i< 5){
        state.flatBolt[i].numberOfPoints = bolt->mesh.vertices().size(); // XXX
        state.flatBolt[i].ending = bolt->ending;
        state.flatBolt[i].bulgePosition = bolt->bulgePosition;
        state.flatBolt[i].bulgeScale = bolt->bulgeScale;
        for(int j = 0; j < bolt->mesh.vertices().size() && j<VERTEX_COUNT; j++){
          state.flatBolt[i].points[j] = bolt->mesh.vertices()[j];  // XXX
          state.flatBolt[i].textCoe[j] = bolt->mesh.texCoord2s()[j];
          state.flatBolt[i].color[j] = bolt->mesh.colors()[j];
        }
      }else{
        break;
      }
      i++;
    }
    state.nucleusPose = nucleusPose;
    state.pose = nav(); // XXX
    maker.set(state);  // XXX
    state.frame++; // XXX
  }

  virtual void onSound(AudioIOData& io) {
    soundSource.pose(Pose(Vec3f(0,0.6,-1), Quatf()));
    listener()->pose(nav());

    while (io()) {
      float s = 0;
      for (int i = 0; i < N_SAMPLE_PLAYER; i++)
        s += samplePlayer[i]();
      s /= N_SAMPLE_PLAYER;
      soundSource.writeSample(s);
    }

    scene()->render(io);
  }

  virtual void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    if (k.key() == 'p') {
      cout << "Pace of the lightning is: "<< pace << endl;
    }else if( k.key() == '='){
      upperbound -= 0.5;
      if(upperbound <= 0){
        upperbound = 0.5;
      }
      cout << "Increase the pace by 0.5s:  pace = "<< upperbound << endl;
    }else if(k.key() == '-'){
      upperbound += 0.5;
      if(upperbound >= 30){
        upperbound = 30;
      }
      cout << "Decrease the pace by 0.5s:  pace = "<< upperbound << endl;
    }
  }

  /*
  virtual void onMouseDown(const ViewpointWindow& w, const Mouse& m) {
    //take a coordinate from the mouse 
    Rayd r = getPickRay(w, m.x(), m.y());
    float t = r.intersectPlane(Vec3f(), (Vec3f(0, 0, 1)));
    Vec3f mouseP = r(t);
    Vec3f src = (float)R * mouseP.normalize();
    Vec3f center = Vec3f(0, 0.6, -1);
    Vec3f dest = 0.1f * (src - center).normalize() + center;
    //create new bolt
    Bolt* newMouseBolt = new Bolt();
    newMouseBolt->makeTexture();
    newMouseBolt->makeBolt(src, dest, 2, 0.03);
    boltQ.push_back(newMouseBolt);
    //animate nucleus
    //affection = (src - center).normalize() * 0.1;
    //nucleusPose += affection;
  }
  */

};

int main() {
  AlloApp app;
  app.AlloSphereAudioSpatializer::audioIO().start();  // start audio
  app.InterfaceServerClient::connect();  // handshake with interface server
  app.maker.start();  // XXX
  app.start();
}


void printFactsAboutState(int size) {
  cout << "==================================================" << endl;
  cout << "Your state type takes " << size << " bytes in memory." << endl;

  if (size > 1024.0f * 1024.0f * 1024.0f) {
    cout << "That is " << size / (1024.0f * 1024.0f * 1024.0f) << " gigabytes."
         << endl;
    cout << "That's too big." << endl;
  } else if (size > 1024.0f * 1024.0f * 10) {
    cout << "That is " << size / (1024.0f * 1024.0f) << " megabytes." << endl;
    cout << "That's a very big state." << endl;
  } else if (size > 1024.0f * 1024.0f) {
    cout << "That is " << size / (1024.0f * 1024.0f) << " megabytes." << endl;
  } else if (size > 1024.0f) {
    cout << "That is " << size / 1024.0f << " kilobytes." << endl;
  } else {
  }

  // Standard 1Gb Ethernet effective bandwidth
  //
  float gbe = 1.18e+8f;

  cout << "On a 1Gb Ethernet LAN (like the MAT network), ";
  if (gbe / size > 60.0f)
    cout << "you will use \n" << 100.0f * (size * 60.0f) / gbe
         << "\% of the effective bandwidth with a framerate of 60 Hz." << endl;
  else
    cout << "your framerate will be *network limited* to " << gbe / size
         << " Hz." << endl;

  cout << "On a 10Gb Ethernet LAN (like the AlloSphere network), ";
  if (10 * gbe / size > 60.0f)
    cout << "you will use \n" << 100.0f * (size * 60.0f) / (10 * gbe)
         << "\% of the effective bandwidth with a framerate of 60 Hz." << endl;
  else {
    cout << "your framerate will be *network limited* to " << 10 * gbe / size
         << " Hz." << endl;
    cout << "Your state is very large." << endl;
  }
  cout << "==================================================" << endl;
}

