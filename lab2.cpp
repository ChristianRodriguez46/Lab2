//
//modified by: Christian Rodriguez
//date: 1/28/25		 Spring 2025
//
//original author: Gordon Griesel
//date:            Fall 2024
//purpose:         OpenGL sample program
//
// This version has been refactored to:
//  1) Change box color with bounce rate (left/right).
//  2) Hide box if window is too narrow.
//  3) Make box move up/down and bounce vertically.
//  4) Use key-presses to speed up or slow down.
//
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>

//some structures

//=================================================================================
// 			----- New variables
//   velX, velY: Current velocities in X and Y direction.
//
//   color[3]   : The current color of the box, as (r, g, b).
//   bounceFreq : A measure of how frequently the box bounces off LEFT or RIGHT walls.
//   frames     : A simple counter to track how many times `physics()` has been called.
//   lastBounceFrame : The value of `frames` when we last bounced off a left or right wall.
//


class Global {
public:
	int xres, yres;
    float w;

	// New
	float velX,velY;	// Separate x and y velocities
	float pos[2];		// Current position of the box center
	
	// For bounce-frequency/color calculation:
    float color[3];         // Current color (r,g,b)
    float bounceFreq;       // Estimated bounces per frame (for left/right only)
    unsigned int frames;    // Count frames for frequency measurement
    unsigned int lastBounceFrame; // Frame index of the last left/right bounce
	
	
	Global(){
        xres =400;
        yres = 200;
        w = 20.0f;

		// New
        velX = 30.0f;	//Horizontal speed
		velY = 5.0f;	//vertical speed
		pos[0] = w;
        pos[1] = yres / 2.0f;

        // Start color as something neutral
        color[0] = 1.0f; // Red
        color[1] = 0.0f; // Green
        color[2] = 1.0f; // Blue

		// Initialize bounce frequency tracking
        bounceFreq = 0.0f;
        frames = 0;
        lastBounceFrame = 0;
    };
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);


int main()
{
	init_opengl();
	int done = 0;
	//main game loop
	while (!done) {
		//look for external events such as keyboard, mouse.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
		usleep(200);
	}
	return 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab-1");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//Window has been resized.
	g.xres = width;
	g.yres = height;
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			//int y = g.yres - e->xbutton.y;
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.


		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_a:
				//the 'a' key was pressed
				break;
			 //--- NEW ---
            case XK_w:
                // 'W' key to speed up horizontally & vertically
                // Increase velocity in both X and Y directions.
                g.velX += 2.0f;
                g.velY += 1.0f;
                break;
            case XK_s:
                // 'S' key to slow down horizontally & vertically
                // Decrease velocity, but clamp to 0 so it doesn't reverse.
                g.velX -= 2.0f;
                g.velY -= 1.0f;
                if (g.velX < 0.0f) g.velX = 0.0f;
                if (g.velY < 0.0f) g.velY = 0.0f;
                break;

			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
    //
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
    //Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
    //Set the screen background color
	glClearColor(0.3, 0.1, 0.1, 1.0);
	// Original values: (r)0.1, (g)0.1, (b)0.1, (transparency?)1.0
}

void physics()
{
    //--- NEW ---
    // Increment frame counter (for bounce frequency calculations).
    g.frames++;

    // Move the box in X and Y directions.
    g.pos[0] += g.velX;
    g.pos[1] += g.velY;

    //--- Collision detection (left/right) ---
    if (g.pos[0] >= (g.xres - g.w)) {
        // Box hits right wall => place it at right boundary.
        g.pos[0] = (g.xres - g.w);
        // Reverse horizontal velocity
        g.velX = -g.velX;

        // We have a left/right bounce. Update bounce frequency.
        unsigned int framesSinceLast = g.frames - g.lastBounceFrame;
        if (framesSinceLast > 0) {
            // bounceFreq is 1 bounce per "framesSinceLast" frames
            g.bounceFreq = 1.0f / (float)framesSinceLast;
        }
        g.lastBounceFrame = g.frames;
    }
    if (g.pos[0] <= g.w) {
        // Box hits left wall => place it at left boundary.
        g.pos[0] = g.w;
        // Reverse horizontal velocity
        g.velX = -g.velX;

        // We have a left/right bounce. Update bounce frequency.
        unsigned int framesSinceLast = g.frames - g.lastBounceFrame;
        if (framesSinceLast > 0) {
            g.bounceFreq = 1.0f / (float)framesSinceLast;
        }
        g.lastBounceFrame = g.frames;
    }

    //--- NEW ---
    // Collision detection (top/bottom)
    if (g.pos[1] >= (g.yres - g.w)) {
        g.pos[1] = (g.yres - g.w);
        g.velY = -g.velY;
    }
    if (g.pos[1] <= g.w) {
        g.pos[1] = g.w;
        g.velY = -g.velY;
    }

    //--- NEW ---
    // We now compute a color based on bounce frequency from left/right.
    //   freq = 0 means no recent bounces => more blue
    //   freq = big number => bounces happen more frequently => more red

    float freq = g.bounceFreq;
    // Clamp the frequency to a certain maximum to avoid extreme values
    // e.g. if the box bounces every frame or so.
    if (freq > 0.05f) {
        freq = 0.05f; 
    }

    // We'll map freq from [0, 0.05] to an interpolation factor f in [0,1].
    //   f = 0 => color is fully blue
    //   f = 1 => color is fully red
    float f = freq / 0.05f;  // 0 .. 1

    // Color formula: 
    //   R = f
    //   G = 0.0
    //   B = 1.0 - f
    g.color[0] = f;
    g.color[1] = 0.0f;
    g.color[2] = 1.0f - f;
}


void render()
{
	//
	glClear(GL_COLOR_BUFFER_BIT);

	//--- NEW ---
    // If window width is too small to fit the box, do not draw it.
	/* The problem states "if the window width becomes smaller than the box width,
    	there will be no bouncing, so make the box disappear." */
    if (g.xres < 2*g.w) {
        return;
    }
	
	//draw the box
	glPushMatrix();

	// Use the color we've computed
    glColor3f(g.color[0], g.color[1], g.color[2]);
    glTranslatef(g.pos[0], g.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-g.w, -g.w);
		glVertex2f(-g.w,  g.w);
		glVertex2f( g.w,  g.w);
		glVertex2f( g.w, -g.w);
	glEnd();
	glPopMatrix();
}