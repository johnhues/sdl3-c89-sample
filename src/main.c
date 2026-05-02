/* SDL_MAIN_USE_CALLBACKS is necessary for the new callbacks API.
   To use the legacy API, don't define this. */
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>
#include <stdlib.h>

const uint32_t windowStartWidth = 400;
const uint32_t windowStartHeight = 400;

typedef struct AppContext AppContext;
struct AppContext
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture *messageTex, *imageTex;
	SDL_FRect messageDest;
	SDL_AudioDeviceID audioDevice;
	MIX_Track* track;
	SDL_AppResult app_quit;
};

SDL_AppResult SDL_Fail( void )
{
	SDL_LogError( SDL_LOG_CATEGORY_CUSTOM, "Error %s", SDL_GetError() );
	return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit( void** appstate, int argc, char* argv[] )
{
	char fontPath[ 1024 ];
	char svgPath[ 1024 ];
	char musicPath[ 1024 ];
	const char* basePath = NULL;
	const char* text = "Hello SDL!";
	SDL_Window* window;
	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Color white;
	SDL_Surface* surfaceMessage;
	SDL_Texture* messageTex;
	SDL_Surface* svgSurface;
	SDL_Texture* tex;
	SDL_PropertiesID messageTexProps;
	SDL_FRect textRect;
	MIX_Mixer* mixer;
	MIX_Track* mixerTrack;
	MIX_Audio* music;
	AppContext* app;

	(void)argc; /* unused function parameter */
	(void)argv; /* unused function parameter */

	/* init the library */
	if( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) )
	{
		return SDL_Fail();
	}

	/* init TTF */
	if( !TTF_Init() )
	{
		return SDL_Fail();
	}

	/* init Mixer */
	if( !MIX_Init() )
	{
		return SDL_Fail();
	}

	/* create a window */
	window = SDL_CreateWindow( "SDL Minimal Sample", windowStartWidth, windowStartHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY );
	if( !window )
	{
		return SDL_Fail();
	}

	/* create a renderer */
	renderer = SDL_CreateRenderer( window, NULL );
	if( !renderer )
	{
		return SDL_Fail();
	}

#if __ANDROID__
	basePath = ""; /* on Android assets are available at the root directory */
#else
	basePath = SDL_GetBasePath();
	if( !basePath )
	{
		return SDL_Fail();
	}
#endif

	/* load the font */
	SDL_snprintf( fontPath, sizeof( fontPath ), "%s%s", basePath, "Inter-VariableFont.ttf" );
	font = TTF_OpenFont( fontPath, 36 );
	if( !font )
	{
		return SDL_Fail();
	}

	/* render the font to a surface */
	white.r = 255;
	white.g = 255;
	white.b = 255;
	white.a = 255;
	surfaceMessage = TTF_RenderText_Solid( font, text, SDL_strlen( text ), white );

	/* make a texture from the surface */
	messageTex = SDL_CreateTextureFromSurface( renderer, surfaceMessage );

	/* we no longer need the font or the surface */
	TTF_CloseFont( font );
	SDL_DestroySurface( surfaceMessage );

	/* load the SVG */
	SDL_snprintf( svgPath, sizeof( svgPath ), "%s%s", basePath, "gs_tiger.svg" );
	svgSurface = IMG_Load( svgPath );
	tex = SDL_CreateTextureFromSurface( renderer, svgSurface );
	SDL_DestroySurface( svgSurface );

	/* get the on-screen dimensions of the text */
	messageTexProps = SDL_GetTextureProperties( messageTex );
	textRect.x = 0;
	textRect.y = 0;
	textRect.w = (float)SDL_GetNumberProperty( messageTexProps, SDL_PROP_TEXTURE_WIDTH_NUMBER, 0 );
	textRect.h = (float)SDL_GetNumberProperty( messageTexProps, SDL_PROP_TEXTURE_HEIGHT_NUMBER, 0 );

	/* init SDL Mixer */
	mixer = MIX_CreateMixerDevice( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL );
	if( mixer == NULL )
	{
		return SDL_Fail();
	}

	mixerTrack = MIX_CreateTrack( mixer );

	/* load the music */
	SDL_snprintf( musicPath, sizeof( musicPath ), "%s%s", basePath, "the_entertainer.ogg" );
	music = MIX_LoadAudio( mixer, musicPath, 0 );
	if( !music )
	{
		return SDL_Fail();
	}

	/* play the music (does not loop) */
	MIX_SetTrackAudio( mixerTrack, music );
	MIX_PlayTrack( mixerTrack, 0 );

	/* print some information about the window */
	SDL_ShowWindow( window );
	{
		int width, height, bbwidth, bbheight;
		SDL_GetWindowSize( window, &width, &height );
		SDL_GetWindowSizeInPixels( window, &bbwidth, &bbheight );
		SDL_Log( "Window size: %ix%i", width, height );
		SDL_Log( "Backbuffer size: %ix%i", bbwidth, bbheight );
		if( width != bbwidth )
		{
			SDL_Log( "This is a highdpi environment." );
		}
	}

	/* set up the application data */
	app = (AppContext*)calloc( 1, sizeof( AppContext ) );
	*appstate = app;
	app->window = window;
	app->renderer = renderer;
	app->messageTex = messageTex;
	app->imageTex = tex;
	app->messageDest = textRect;
	app->track = mixerTrack;
	app->app_quit = SDL_APP_CONTINUE;

	SDL_SetRenderVSync( renderer, -1 ); /* enable vsync */

	SDL_Log( "Application started successfully!" );

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent( void* appstate, SDL_Event* event )
{
	AppContext* app = (AppContext*)appstate;

	if( event->type == SDL_EVENT_QUIT )
	{
		app->app_quit = SDL_APP_SUCCESS;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate( void* appstate )
{
	AppContext* app = (AppContext*)appstate;
	const float time = SDL_GetTicks() / 1000.f;
	const double red = ( sin( time ) + 1 ) / 2.0 * 255;
	const double green = ( sin( time / 2 ) + 1 ) / 2.0 * 255;
	const double blue = ( sin( time * 2 ) + 1 ) / 2.0 * 255;

	SDL_SetRenderDrawColor( app->renderer, (Uint8)red, (Uint8)green, (Uint8)blue, SDL_ALPHA_OPAQUE );
	SDL_RenderClear( app->renderer );

	/* painter's algorithm: render image first so text appears on top */
	SDL_RenderTexture( app->renderer, app->imageTex, NULL, NULL );
	SDL_RenderTexture( app->renderer, app->messageTex, NULL, &app->messageDest );

	SDL_RenderPresent( app->renderer );

	return app->app_quit;
}

void SDL_AppQuit( void* appstate, SDL_AppResult result )
{
	AppContext* app = (AppContext*)appstate;
	(void)result;
	if( app )
	{
		SDL_DestroyRenderer( app->renderer );
		SDL_DestroyWindow( app->window );

		/* prevent the music from abruptly ending */
		MIX_StopTrack( app->track, MIX_TrackMSToFrames( app->track, 1000 ) );
		SDL_Delay( 1000 );
		SDL_CloseAudioDevice( app->audioDevice );

		free( app );
	}
	TTF_Quit();
	MIX_Quit();

	SDL_Log( "Application quit successfully!" );
	SDL_Quit();
}
