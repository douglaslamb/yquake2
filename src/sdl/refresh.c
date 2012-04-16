/*
 * Copyright (C) 2010 Yamagi Burmeister
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file implements an OpenGL context via SDL
 *
 * =======================================================================
 */

#if !defined(USE_EGL_RAW)
#include <SDL.h>
#endif

#if defined(GLES)
#include <GLES/gl.h>
#else
#include <GL/gl.h>
#endif

#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
#include "../egl/eglport.h"
#endif

#include "../refresh/header/local.h"
#include "../unix/header/glwindow.h"

/* The window icon */
#include "icon/q2icon.xbm"

#if !defined(USE_EGL_RAW)
/* X.org stuff */
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>

SDL_Surface		*surface;
glwstate_t		glw_state;
qboolean		have_stencil = false;

char *displayname = NULL;
int screen = -1;

#ifdef X11GAMMA
Display *dpy;
XF86VidModeGamma x11_oldgamma;
#endif
#endif

/*
 * Initialzes the SDL OpenGL context
 */
int GLimp_Init(void)
{
#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
	EGL_Open();
#endif 

#if !defined(USE_EGL_RAW)
	if (!SDL_WasInit(SDL_INIT_VIDEO))
	{
		char driverName[ 64 ];

		if (SDL_Init(SDL_INIT_VIDEO) == -1)
		{
			ri.Con_Printf( PRINT_ALL, "Couldn't init SDL video: %s.\n", SDL_GetError());
			return false;
		}
		SDL_VideoDriverName( driverName, sizeof( driverName ) - 1 );
		ri.Con_Printf( PRINT_ALL, "SDL video driver is \"%s\".\n", driverName );
	}
#endif

	return true;
}

/*
 * Sets the window icon
 */
static void SetSDLIcon()
{
#if !defined(USE_EGL_RAW)
	SDL_Surface *icon;
	SDL_Color	color;
	Uint8		*ptr;
	int			i;
	int			mask;

	icon = SDL_CreateRGBSurface(SDL_SWSURFACE, q2icon_width, q2icon_height, 8, 0, 0, 0, 0);

	if (icon == NULL)
	{
		return;
	}

	SDL_SetColorKey(icon, SDL_SRCCOLORKEY, 0);

	color.r = 255;
	color.g = 255;
	color.b = 255;

	SDL_SetColors(icon, &color, 0, 1);

	color.r = 0;
	color.g = 16;
	color.b = 0;

	SDL_SetColors(icon, &color, 1, 1);

	ptr = (Uint8 *)icon->pixels;

	for (i = 0; i < sizeof(q2icon_bits); i++)
	{
		for (mask = 1; mask != 0x100; mask <<= 1) {
			*ptr = (q2icon_bits[i] & mask) ? 1 : 0;
			ptr++;
		}
	}

	SDL_WM_SetIcon(icon, NULL);
	SDL_FreeSurface(icon);
#endif
}

/*
 * Sets the hardware gamma
 */
#ifdef X11GAMMA
	void
UpdateHardwareGamma(void)
{
#if !defined(PANDORA) && !defined(USE_EGL_RAW)
	float gamma;
	XF86VidModeGamma x11_gamma;

	gamma =vid_gamma->value;

	x11_gamma.red = gamma;
	x11_gamma.green = gamma;
	x11_gamma.blue = gamma;

	XF86VidModeSetGamma(dpy, screen, &x11_gamma);

	/* This forces X11 to update the gamma tables */
	XF86VidModeGetGamma(dpy, screen, &x11_gamma);
#endif
}
#else
void
UpdateHardwareGamma(void)
{
#if !defined(PANDORA) && !defined(USE_EGL_RAW)
	float gamma;

	gamma = (vid_gamma->value);
	SDL_SetGamma(gamma, gamma, gamma);
#endif
}
#endif

/*
 * Initializes the OpenGL window
 */
static qboolean GLimp_InitGraphics( qboolean fullscreen )
{
	int counter = 0;
	int flags;
	int stencil_bits;
	char title[24];

#if !defined(USE_EGL_RAW)
	if (surface && (surface->w == vid.width) && (surface->h == vid.height))
	{
		/* Are we running fullscreen? */
		int isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;

		/* We should, but we don't */
		if (fullscreen != isfullscreen)
		{
			SDL_WM_ToggleFullScreen(surface);
		}

		/* Do we now? */
		isfullscreen = (surface->flags & SDL_FULLSCREEN) ? 1 : 0;

		if (fullscreen == isfullscreen)
		{
			return true;
		}
	}

	/* Is the surface used? */
	if (surface)
	{
		SDL_FreeSurface(surface);
	}
#endif

	/* Create the window */
	ri.Vid_NewWindow (vid.width, vid.height);

#if !defined(USE_EGL_RAW)
#if defined(USE_EGL_SDL)
	flags = SDL_SWSURFACE;
#else 

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	/* Initiate the flags */
	flags = SDL_OPENGL;
#endif

	if (fullscreen)
	{
		flags |= SDL_FULLSCREEN;
	}

	/* Set the icon */
	SetSDLIcon();
#endif

#if defined(USE_EGL_RAW)
	ri.Cvar_SetValue("gl_mode", 0);
	ri.Cvar_SetValue("vid_fullscreen", 0);
	vid.width = 320;
	vid.height = 240;
#else 
	while (1)
	{
		if ((surface = SDL_SetVideoMode(vid.width, vid.height, 0, flags)) == NULL) 
		{   
			if (counter == 1)
			{
				Sys_Error(PRINT_ALL, "Failed to revert to gl_mode 4. Exiting...\n");
				return false;
			}

			ri.Con_Printf(PRINT_ALL, "SDL SetVideoMode failed: %s\n", SDL_GetError());
			ri.Con_Printf(PRINT_ALL, "Reverting to gl_mode 4 (640x480) and windowed mode.\n");

			/* Try to recover */
			ri.Cvar_SetValue("gl_mode", 4);
			ri.Cvar_SetValue("vid_fullscreen", 0);
			vid.width = 640;
			vid.height = 480;

			counter++;
			continue;
		}
		else
		{
			break;
		}
	}
#endif

#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
	EGL_Init();
#endif


#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
	have_stencil = true;
#else
	/* Initialize the stencil buffer */
	if (!SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil_bits))
	{
		ri.Con_Printf(PRINT_ALL, "Got %d bits of stencil.\n", stencil_bits);

		if (stencil_bits >= 1)
		{
			have_stencil = true;
		}
	}
#endif

#if !defined(PANDORA) && !defined(USE_EGL_RAW)
	/* Initialize hardware gamma */
#ifdef X11GAMMA
	if ( ( dpy = XOpenDisplay( displayname ) ) == NULL )
	{
		ri.Con_Printf(PRINT_ALL, "Unable to open display.\n");
	}
	else
	{
		if (screen == -1)
		{
			screen = DefaultScreen(dpy);
		}

		gl_state.hwgamma = true;
		vid_gamma->modified = true;

		XF86VidModeGetGamma(dpy, screen, &x11_oldgamma);

		ri.Con_Printf(PRINT_ALL, "Using hardware gamma via X11.\n");
	}
#else
	gl_state.hwgamma = true;
	vid_gamma->modified = true;
	ri.Con_Printf(PRINT_ALL, "Using hardware gamma via SDL.\n");
#endif
#endif

#if !defined(USE_EGL_RAW)
	/* Window title */
	snprintf(title, sizeof(title), "Yamagi Quake II %4.2f", VERSION);
	SDL_WM_SetCaption(title, title);

	/* No cursor */
	SDL_ShowCursor(0);
#endif

	return true;
}

/*
 * Swaps the buffers to show the new frame
 */
void GLimp_EndFrame (void)
{
#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
	EGL_SwapBuffers();
#else
	SDL_GL_SwapBuffers();
#endif
}

/*
 * Changes the video mode
 */
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	ri.Con_Printf (PRINT_ALL, "setting mode %d:", mode );

	/* mode -1 is not in the vid mode table - so we keep the values in pwidth
	   and pheight and don't even try to look up the mode info */
	if ( mode != -1 && !ri.Vid_GetModeInfo( pwidth, pheight, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", *pwidth, *pheight);

	if ( !GLimp_InitGraphics( fullscreen ) )
	{
		return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
 * Shuts the SDL render backend down
 */
void GLimp_Shutdown( void )
{
#if !defined(USE_EGL_RAW)
	if (surface)
	{
		SDL_FreeSurface(surface);
	}

	surface = NULL;

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
	{
		SDL_Quit();
	}
	else
	{
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
#endif

#if !defined(PANDORA) && !defined(USE_EGL_RAW) 
#ifdef X11GAMMA
	if (gl_state.hwgamma == true)
	{
		XF86VidModeSetGamma(dpy, screen, &x11_oldgamma);

		/* This forces X11 to update the gamma tables */
		XF86VidModeGetGamma(dpy, screen, &x11_oldgamma);
	}
#endif
#endif

	gl_state.hwgamma = false;

#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW)
	EGL_Close();
#endif 
}
