/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if defined(HAVE_SDL)
#include <SDL/SDL.h>
#endif
#include "vs_globals.h"
#include "gfxlib.h"
#include "in_kb.h"
#include "lin_time.h"
#include "main_loop.h"
#include "config_xml.h"
#include "cmd/script/mission.h"
#include "audiolib.h"
#include "vs_path.h"
#include "gfx/animation.h"
#include "cmd/unit.h"
#include "gfx/cockpit.h"
#include "python/init.h"
#include "savegame.h"
/*
 * Globals 
 */
game_data_t g_game;

Universe *_Universe;
FILE * fpread=NULL;
/* 
 * Function definitions
 */

void setup_game_data ( ){ //pass in config file l8r??
  g_game.audio_frequency_mode=4;//22050/16
  g_game.sound_enabled =1;
  g_game.music_enabled=1;
  g_game.sound_volume=1;
  g_game.music_volume=1;
  g_game.warning_level=20;
  g_game.capture_mouse=GFXFALSE;
  g_game.y_resolution = 768;
  g_game.x_resolution = 1024;
  g_game.fov=78;
  g_game.MouseSensitivityX=2;
  g_game.MouseSensitivityY=4;

}
void ParseCommandLine(int argc, char ** CmdLine);
void cleanup(void)
{
  printf ("Thank you for playing!\n");
  if (_Universe->AccessCockpit()) {
    if (_Universe->AccessCockpit()->GetParent()) {
      if(_Universe->AccessCockpit()->GetParent()->GetHull()>0) {
	WriteSaveGame (_Universe->getActiveStarSystem(0)->getFileName().c_str(),_Universe->AccessCockpit()->GetParent()->Position());
      } 
    }
  }
  //    write_config_file();
  //  write_saved_games();
  AUDDestroy();
  destroyObjects();
  delete _Universe;
}

VegaConfig *vs_config;
Mission *mission;
double benchmark=-1.0;

char mission_name[1024];

void bootstrap_main_loop();

int main( int argc, char *argv[] ) 
{

  
    /* Print copyright notice */
  fprintf( stderr, "Vega Strike "  " \n"
	     "See http://www.gnu.org/copyleft/gpl.html for license details.\n\n" );
    /* Seed the random number generator */
    

    if(benchmark<0.0){
      srand( time(NULL) );
    }
    else{
      // in benchmark mode, always use the same seed
      srand(171070);
    }
    //this sets up the vegastrike config variable
    setup_game_data(); 
    // loads the configuration file .vegastrikerc from home dir if such exists
    initpaths();
    //can use the vegastrike config variable to read in the default mission
    strcpy(mission_name,vs_config->getVariable ("general","default_mission","test1.mission").c_str());
    //might overwrite the default mission with the command line
    ParseCommandLine(argc,argv);

#ifdef HAVE_BOOST

    //	Python::init();

#endif

#if defined(HAVE_SDL)
    // && defined(HAVE_SDL_MIXER)
  if (  SDL_Init( SDL_INIT_JOYSTICK )) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

#endif
#if 0
    InitTime();
    UpdateTime();
#endif

    AUDInit();

    /* Set up a function to clean up when program exits */
    if ( atexit( cleanup ) != 0 ) {
	perror( "atexit" );
    }
    /*
#if defined(HAVE_SDL) && defined(HAVE_SDL_MIXER)

    init_audio_data();
    init_audio();
    init_joystick();

#endif
    */
    _Universe= new Universe(argc,argv,vs_config->getVariable ("general","galaxy","milky_way.xml").c_str());   

    _Universe->Loop(bootstrap_main_loop);
    return 0;
}
void bootstrap_draw (Animation * SplashScreen) {
  UpdateTime();
  if (SplashScreen) {
    Matrix tmp;
    Identity (tmp);
    GFXDisable(LIGHTING);
    GFXDisable(DEPTHTEST);
    GFXBlendMode (ONE,ZERO);
    GFXEnable (TEXTURE0);
    GFXColor4f (1,1,1,1);
    ScaleMatrix (tmp,Vector (7,7,0));
    SplashScreen->UpdateAllFrame();
    GFXClear (GFXTRUE);
    GFXLoadIdentity (PROJECTION);
    GFXLoadIdentityView();
    GFXLoadMatrix (MODEL,tmp);
    GFXBeginScene();
    SplashScreen->DrawNow(tmp);
    GFXEndScene();
  }
}
extern Unit **fighters;
void bootstrap_main_loop () {
  static bool LoadMission=true;
  InitTime();
  static Animation * SplashScreen = NULL;
  bootstrap_draw (SplashScreen);
  bootstrap_draw (SplashScreen);
  if (LoadMission) {
    LoadMission=false;
    mission=new Mission(mission_name);

    mission->initMission();

    SplashScreen = new Animation (mission->getVariable ("splashscreen",vs_config->getVariable ("graphics","splash_screen","vega_splash.ani")).c_str(),0);
    bootstrap_draw (SplashScreen);
    bootstrap_draw (SplashScreen);

    Vector pos;
    string planetname;

    mission->GetOrigin(pos,planetname);
    bool setplayerloc;
    string mysystem = mission->getVariable("system","sol.system");

    string savegamefile = mission->getVariable ("savegame","");
    vector <SavedUnits> savedun=ParseSaveGame (savegamefile,mysystem,mysystem,pos);
   
    bootstrap_draw (SplashScreen);
    _Universe->Init (mysystem,pos,planetname);
    bootstrap_draw (SplashScreen);
    createObjects();
    if(!savegamefile.empty()){
    if (setplayerloc&&fighters) {
      if (fighters[0]) {
	fighters[0]->SetPosition (Vector (0,0,0));
      }
    }
    while (!savedun.empty()) {
      AddUnitToSystem (&savedun.back());
      savedun.pop_back();
    }
    }
    InitializeInput();

    vs_config->bindKeys();


    UpdateTime();
    delete SplashScreen;
    SplashScreen= NULL;
    _Universe->Loop(main_loop);
    ///return to idle func which now should call main_loop mohahahah
  }
  ///Draw Texture
  
} 






void ParseCommandLine(int argc, char ** lpCmdLine) {
  Vector PlayerLocation;
  for (int i=1;i<argc;i++) {
    if(lpCmdLine[i][0]=='-') {
      switch(lpCmdLine[i][1]){
      case 'r':
      case 'R':
	break;
      case 'M':
      case 'm':
	g_game.music_enabled=1;
	break;
      case 'S':
      case 's':
	g_game.sound_enabled=1;
	break;
      case 'f':
      case 'F':
	break;
      case 'P':
      case 'p':
	sscanf (lpCmdLine[i]+2,"%f,%f,%f",&PlayerLocation.i,&PlayerLocation.j,&PlayerLocation.k);
	SetPlayerLocation (PlayerLocation);
	break;
      case 'L':
      case 'l'://low rez
	SetStarSystem ( string ((lpCmdLine[i])+2));
	break;
      case 'A'://average rez
      case 'a': 
	g_game.y_resolution = 600;
	g_game.x_resolution = 800;
	break;
      case 'H':
      case 'h'://high rez
	g_game.y_resolution = 768;
	g_game.x_resolution = 1024;
	break;
      case 'V':
      case 'v':
	g_game.y_resolution = 1024;
	g_game.x_resolution = 1280;
	break;
      case 'D':
      case 'd':
	break;
      case 'G':
      case 'g':
	//viddrv = "GLDRV.DLL";
	break;
      case '-':
	// long options
	if(strcmp(lpCmdLine[i],"--benchmark")==0){
	  //benchmark=30.0;
	  benchmark=atof(lpCmdLine[i+1]);
	  i++;
	}
      }
    }
    else{
      // no "-" before it - it's the mission name
      strncpy (mission_name,lpCmdLine[i],1023);
	  mission_name[1023]='\0';
    }
  }
  //FILE *fp = fopen("vid.cfg", "rb");
  //  GUID temp;
  //fread(&temp, sizeof(GUID), 1, fp);
  //fread(&temp, sizeof(GUID), 1, fp);
  //fread(&_ViewPortWidth, sizeof(DWORD), 1, fp);
  //fread(&_ViewPortHeight, sizeof(DWORD), 1, fp);
  //fread(&_ColDepth,sizeof(DWORD),1,fp);
  //fclose(fp);
}
