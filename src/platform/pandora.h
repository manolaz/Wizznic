#ifndef PANDORA_H_INCLUDED
#define PANDORA_H_INCLUDED

//Stats uploading for pandora, using wget ?
#define PLATFORM_SUPPORTS_STATSUPLOAD
#define STR_PLATFORM "Pandora"
#define UPLOAD_PROGRAM "wget "STATS_SERVER_URL"/commit.php -O - -q --user-agent=wizznicPandora --timeout=10 --tries=1 --post-data="

//Video
#define SCREENW 320
#define SCREENH 240

#ifdef HAVE_GLES
    #include <GLES/gl.h>
    #include "eglport.h"
#endif

//Audio
#define SOUND_RATE  22050
#define SOUND_FORMAT  AUDIO_S16
#define SOUND_BUFFERS 256
#define SOUND_MIX_CHANNELS 16

//We use software scaling on the pandora
#define WANT_SWSCALE

//Help file for this platform (appended to DATADIR)
#define PLATFORM_HELP_FILE        "data/menu/helppc.png"

//Button definitions
#define PLATFORM_BUTTON_UP        SDLK_UP
#define PLATFORM_BUTTON_DOWN      SDLK_DOWN
#define PLATFORM_BUTTON_LEFT      SDLK_LEFT
#define PLATFORM_BUTTON_RIGHT     SDLK_RIGHT
#define PLATFORM_BUTTON_Y         SDLK_PAGEUP
#define PLATFORM_BUTTON_X         SDLK_PAGEDOWN
#define PLATFORM_BUTTON_A         SDLK_HOME
#define PLATFORM_BUTTON_B         SDLK_END
#define PLATFORM_SHOULDER_LEFT    SDLK_RSHIFT
#define PLATFORM_SHOULLER_RIGHT   SDLK_RCTRL
#define PLATFORM_BUTTON_MENU      SDLK_LALT
#define PLATFORM_BUTTON_SELECT    SDLK_LCTRL
#define PLATFORM_BUTTON_VOLUP     0 //Not used
#define PLATFORM_BUTTON_VOLDOWN   0 //on Pandora

#endif // PANDORA_H_INCLUDED
