#include "gfx_mesh.h"
#include "lin_time.h"
static void AvLights (float target[4], const float  other[4]) {
  target[0] = .5*(target[0] + other[0]);
  target[1] = .5*(target[1] + other[1]);
  target[2] = .5*(target[2] + other[2]);
  target[3] = .5*(target[3] + other[3]);
}
MeshFX::MeshFX (const float TTL, const float delta, const bool enabled, const GFXColor &vect, const GFXColor &diffuse, const GFXColor &specular, const GFXColor &ambient, const GFXColor&attenuate) 	
  :GFXLight(enabled,vect,diffuse,specular,ambient,attenuate){
  this->TTL = TTL; 
  this->TTD = this->TTL; this->delta = delta;
}
void MeshFX::MergeLights (const MeshFX & other) {
  if (TTL>0) {
    delta = (other.delta + this->delta) *.5;
    /*  TTL = (TTL>other.TTL)
	?
	(.666667*TTL+.33333*other.TTL)
	:
	(.666667*other.TTL+.333333*TTL);*/
    TTL = .5*(TTL+other.TTL);
    TTD = .5*(TTD+other.TTD);
    Vector vec (vect[0],vect[1],vect[2]);
    vec*=.5;
    Vector othervec (other.vect[0],other.vect[1],other.vect[2]);
    othervec*=.5;
    float distsqr = ((vec-othervec)).Dot ((vec-othervec));
    options |= other.options;
    vec = vec+othervec;
    vect[0]=vec.i;
    vect[1]=vec.j;
    vect[2]=vec.k;
    AvLights (diffuse,other.diffuse);
    AvLights (specular,other.specular);
    AvLights (ambient,other.ambient);
    /*    attenuate[2]=1./attenuate[2];
    attenuate[2]+=1./other.attenuate[2]+distsqr;
    attenuate[2]= 1./attenuate[2];
  
    attenuate[1]=1./attenuate[1];
    attenuate[1]+=1./other.attenuate[1]+sqrtf(distsqr);
    attenuate[1]= 1./attenuate[1];*/
  } else {
    memcpy(this, &other, sizeof (MeshFX));
  }
}
bool MeshFX::Update() {
  TTL -= GetElapsedTime();
  if (TTL <0) {
    TTL = 0;
    TTD -= GetElapsedTime();
    attenuate[2]+=2*delta*GetElapsedTime()
;
    attenuate[1]+=2*delta*GetElapsedTime();
    
    //    attenuate[2]*=1+2*delta*GetElapsedTime();
    //    attenuate[1]*=1+2*delta*GetElapsedTime();
  } else {
    attenuate[2]-=1.25*delta*GetElapsedTime();
    attenuate[1]-=1.25*delta*GetElapsedTime();
    //    attenuate[2]*=1- .5*delta*GetElapsedTime();
    //    attenuate[1]*=1- .5*delta*GetElapsedTime();
  }
  return TTD>0;
}

void Mesh::AddDamageFX(const Vector & pnt, const Vector &norm,  const float damage, const GFXColor &col) {

  Vector loc(pnt+norm);
  if (!(norm.i||norm.j||norm.k)) {
    loc = (pnt*(rSize()*rSize()/pnt.Dot(pnt)));
  }

  GFXColor tmp (col.r,col.g,col.b,col.a);
  float numsec = 5*damage;
  MeshFX newFX (numsec,.2*numsec   ,true,GFXColor(loc.i,loc.j,loc.k,1),tmp,GFXColor (0,0,0,1),tmp,GFXColor (1,0,10/(rSize()*rSize())));
  if (LocalFX.size()>2) {
    LocalFX[(rand()%(LocalFX.size()))].MergeLights (newFX);
  } else {
    LocalFX.push_back (newFX);
  }
}
void Mesh::UpdateFX() {
  //adjusts lights by TTL, eventually removing them
  for (int i=0;i<LocalFX.size();i++) {
    if (!LocalFX[i].Update()) {
      vector <MeshFX>::iterator er = LocalFX.begin();
      er+=i;
      LocalFX.erase (er);
      i--;
    }
  }
}

void Mesh::EnableSpecialFX(){
  draw_sequence=MESH_SPECIAL_FX_ONLY;
  setEnvMap(GFXFALSE);
  orig->envMap = GFXFALSE;
  blendSrc=ONE;
  blendDst=ONE;
}
