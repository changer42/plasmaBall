#ifndef __PHASESPACE_INTERACTION__
#define __PHASESPACE_INTERACTION__

#include "phasespace/Phasespace.hpp"
#include "phasespace/Glove.hpp"

#include "common_4.hpp"

struct PS {
  Phasespace* tracker;
  Glove left, right; 
  Nav oldNav;
  Nav *mNav;
  float sensitivity;

  std::deque<Bolt*> *boltQ;
  State *state;
  gam::SamplePlayer<> *samplePlayer;
  int *currentPlayer;

  PS(): sensitivity(10.0) {}

  void init(Nav *nav, State *s, std::deque<Bolt*> *importBoltQ, gam::SamplePlayer<> *samplePlayerN, int *currentPlayerC){
    tracker = Phasespace::master();
    tracker->start();
    right.monitorMarkers(tracker->markers,0);
    left.monitorMarkers(tracker->markers,8);
    state = s;
    mNav = nav;
    boltQ = importBoltQ;
    samplePlayer = samplePlayerN;
    currentPlayer = currentPlayerC;
  }

  void initTest(Nav *nav, State *s, std::deque<Bolt*> *importBoltQ, gam::SamplePlayer<> *samplePlayerN, int *currentPlayerC){
    tracker = Phasespace::master();
    tracker->startPlaybackFile("phasespace/marker-data/gloves.txt");
    right.monitorMarkers(tracker->markers,0);
    left.monitorMarkers(tracker->markers,8);
    state = s;
    mNav = nav;
    boltQ = importBoltQ;
    samplePlayer = samplePlayerN;
    currentPlayer = currentPlayerC;
  }

  const Nav& nav() const { return *mNav; }
  Nav& nav() { return *mNav; }

  void step(float dt){
    // update glove pinch detection based on current marker positions
    left.step(dt);
    right.step(dt);
    
    // do glove interaction
    doInteraction(dt);
  }

  void doInteraction(float dt){
    Glove *l = &left;
    Glove *r = &right;

    Vec3f headPos = tracker->markerPositions[17]; // right(0-7) left(8-15) A B C (16 17 18)
    if(headPos.mag() == 0) return;

    Rayd rayS(headPos, r->centroid - headPos); // ray pointing in direction of head to right hand
    
    float t = rayS.intersectAllosphere(); // get t on surface of allosphere screen
    Vec3f pos = nav().quat().rotate( rayS(t) ); // rotate point on allosphere to match current nav orientation (check this)
    Rayd ray( nav().pos(), pos); // ray from sphere center (camera location) to intersected location

    // Use ray to intesect with plasma shell on pinch
    if( r->pinchOn[eIndex]){
      cout<<"right index finger pinched, create lightning!"<<endl; 
      //audio
      //cout << currentPlayer << endl;
      //cout << *currentPlayer << endl;
      samplePlayer[*currentPlayer].reset(); // reset the phase == start playing the sound

      *currentPlayer = *currentPlayer + 1;
      if (*currentPlayer == N_SAMPLE_PLAYER){
        *currentPlayer = 0;
        // currentPlayer = 0; // this was a horrible BUG! XXX
      }
      //visual
      Vec3f center = Vec3f(0, 0.6, -1);
      float t = ray.intersectSphere(Vec3f(0,0,0), R);
      Vec3f src = ray(t);
      Vec3f dest = 0.1f * (src - center).normalize() + center;
      Bolt* newPSBolt = new Bolt();
      newPSBolt->makeTexture();
      newPSBolt->start = src;
      newPSBolt->ending = dest;
      newPSBolt->makeBolt(src, dest, 2, 0.03);
      newPSBolt->createBulge(center);
      boltQ->push_back(newPSBolt); 
    }

    // state->cursor.set(nav().pos() + pos);

    // Navigation joystick mode
    // Translation done with the left index finger pinch gesture
    if( l->pinchOn[eIndex]){
    } else if( l->pinched[eIndex]){
      Vec3f translate = sensitivity * l->getPinchTranslate(eIndex);
      for(int i=0; i<3; i++){
        nav().pos()[i] = nav().pos()[i] * 0.9 + 
          (nav().pos()[i] + translate.dot(Vec3d(nav().ur()[i], nav().uu()[i], -nav().uf()[i]))) * 0.1;
      }
      // nav().pos().lerp( nav().pos() + translate, 0.01f);
    } else if( l->pinchOff[eIndex] ){
    } else {
    }
    //ePinky
    //eRing

    // change navigation sensitivity
    if( l->pinched[eMiddle]){
      Vec3f v = l->getPinchTranslate(eMiddle);
      sensitivity = abs(v.y*10);
    }

  }

  
};



#endif
