/* 
 * LibPSn00b Example Programs
 *
 * Hello World Example
 * 2019-2020 Meido-Tek Productions / PSn00bSDK Project
 *
 * The obligatory hello world example normally included in nearly every
 * SDK package. This example should also get you started in how to manage
 * the display using psxgpu.
 *
 * Example by Lameguy64
 *
 *
 * Changelog:
 *
 *	  January 1, 2020 - Initial version
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

/* Screen resolution */
#define SCREEN_XRES		320
#define SCREEN_YRES		240

/* Screen center position */
#define CENTERX			SCREEN_XRES>>1
#define CENTERY			SCREEN_YRES>>1

#define OT_LEN 128
#define NB_FACES 2

// Define display/draw environments for double buffering
DISPENV disp[2];
DRAWENV draw[2];
uint32_t ot[2][OT_LEN];
int db;

SVECTOR verts[] = {
	{ -100, -100, -100, 0 },
	{  100, -100, -100, 0 },
	{ -100,  100, -100, 0 },
  {  100,  100, -100, 0 },
  {  100, -100,  100, 0 },
	{ -100, -100,  100, 0 },
	{  100,  100,  100, 0 },
	{ -100,  100,  100, 0 }
};

SVECTOR norms[] = {
	{ 0, 0, -ONE, 0 },
  { 0, 0, ONE, 0 }
};

typedef struct {
	short v0, v1, v2, v3;
} INDEX;

INDEX indices[] = {
	{ 0, 1, 2, 3 },
	{ 4, 5, 6, 7 }
};

MATRIX color_mtx = {
	ONE * 3/4, 0, 0,	/* Red   */
	ONE * 3/4, 0, 0,	/* Green */
	ONE * 3/4, 0, 0	/* Blue  */
};

MATRIX light_mtx = {
	/* X,  Y,  Z */
	-2048 , -2048 , -2048,
	0	  , 0	  , 0,
	0	  , 0	  , 0
};

/* Reference texture data */
extern const uint32_t tim_texture[];

/* To keep a copy of the TIM coordinates for later */
int tim_mode;
RECT tim_prect, tim_crect;

SVECTOR	rot = { 0 };			/* Rotation vector for Rotmatrix */
VECTOR	pos = { 0, 0, 400 };	/* Translation vector for TransMatrix */
MATRIX	mtx, lmtx;				/* Rotation matrices for geometry and lighting */

char pribuff[2][32768];
char* nextpri;

// Texture upload function
void LoadTexture(u_long *tim, TIM_IMAGE *tparam) {
  // Read TIM parameters (PSn00bSDK)
  GetTimInfo(tim, tparam);

  // Upload pixel data to framebuffer
  LoadImage(tparam->prect, (u_long*)tparam->paddr);
  DrawSync(0);

  // Upload CLUT to framebuffer if present
  if( tparam->mode & 0x8 ) {
    LoadImage(tparam->crect, (u_long*)tparam->caddr);
    DrawSync(0);
  }
}

// Init function
void init(void)
{
	// This not only resets the GPU but it also installs the library's
	// ISR subsystem to the kernel
	ResetGraph(0);
	
	// Define display environments, first on top and second on bottom
	SetDefDispEnv(&disp[0], 0, 0, 320, 240);
	SetDefDispEnv(&disp[1], 0, 240, 320, 240);
	
	// Define drawing environments, first on bottom and second on top
	SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
	SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
	
	// Set and enable clear color
	setRGB0(&draw[0], 0, 96, 0);
	setRGB0(&draw[1], 0, 96, 0);
	draw[0].isbg = 1;
	draw[1].isbg = 1;
	
	// Clear double buffer counter
	db = 0;
	
	// Apply the GPU environments
	PutDispEnv(&disp[db]);
	PutDrawEnv(&draw[db]);
	
	// Load test font
	FntLoad(960, 0);
	
	// Open up a test font text stream of 100 characters
	FntOpen(0, 8, 320, 224, 0, 100);

  ClearOTagR(ot[db], OT_LEN);

  // Ready the GTE
  InitGeom();

  // Set GTE offset
  gte_SetGeomOffset(CENTERX, CENTERY);

  // Set GTE screen depth (for FOV)
  gte_SetGeomScreen(CENTERX);

	gte_SetBackColor(63, 63, 163);
	gte_SetColorMatrix(&color_mtx);

  nextpri = pribuff[0];

  // Load Tim texture
  TIM_IMAGE img;
  LoadTexture((u_long*)tim_texture, &img);

  tim_prect = *img.prect;
  tim_crect = *img.crect;
  tim_mode = img.mode;

  // Ready tpage
  draw[0].tpage = getTPage(tim_mode&0x3, 0, tim_prect.x, tim_prect.y);
  draw[1].tpage = getTPage(tim_mode&0x3, 0, tim_prect.x, tim_prect.y);
}

void drawGeometry() {
  int32_t p;

  /* Set rotation and translation to the matrix */
  RotMatrix(&rot, &mtx);
  TransMatrix(&mtx, &pos);

  /* Multiply light matrix by rotation matrix so light source */
  /* won't appear relative to the model's rotation */
	MulMatrix0(&light_mtx, &mtx, &lmtx);

  /* Set rotation and translation matrix */
  gte_SetRotMatrix(&mtx);
  gte_SetTransMatrix(&mtx);

  rot.vy += 16;
  
  /* Set light matrix */
  gte_SetLightMatrix(&lmtx);

  for (int i = 0; i < NB_FACES; i++) {
    /* Load first 3 vertices to make a face */
    gte_ldv3(
      &verts[indices[i].v0],
      &verts[indices[i].v1],
      &verts[indices[i].v2]
      );

    /* Rotation, Translation and Perspective Triple */
    gte_rtpt();

    /* /\* Cull if looking at backface *\/ */
    /* gte_nclip(); */
    /* gte_stopz(&p); */
    /* if (p < 0) */
    /*   continue; */

    gte_avsz4();
    gte_stotz(&p);

    /* Skip if clipping off */
    if((p>>2) > OT_LEN)
      continue;

    /* Initialize a triangle primitive */
    POLY_FT4* pol4 = (POLY_FT4*) nextpri;
    setPolyFT4(pol4);

    /* Set the projected vertices to the primitive */
    gte_stsxy0(&pol4->x0);
    gte_stsxy1(&pol4->x1);
    gte_stsxy2(&pol4->x2);

    gte_ldv0(&verts[indices[i].v3]);
    gte_rtps();
    gte_stsxy(&(pol4->x3));

    gte_ldrgb(&pol4->r0);
    gte_ldv0(&norms[i]);
    gte_ncs();
    gte_strgb(&pol4->r0);

    /* Set face texture */
    setUVWH(pol4, 0, 1, 128, 128);
    pol4->tpage = draw[db].tpage;

    addPrim(ot[db]+(p>>2), pol4);
    nextpri += sizeof(POLY_FT4);
  }
}

void drawTestTile(int x, int y, int r, int g, int b) {
  TILE* tile = (TILE*) nextpri;
  setTile(tile);              // Initialize the primitive (very important)
  setXY0(tile, x, y);     // Set primitive (x,y) position
  setWH(tile, 32, 32);        // Set primitive size
  setRGB0(tile, r, g, b);   // Set color
  addPrim(ot[db], tile);      // Add primitive to the ordering table
  nextpri += sizeof(TILE);
}

// Display function
void display(void)
{
	// Wait for all drawing to complete
	DrawSync(0);

	// Wait for vertical sync to cap the logic to 60fps (or 50 in PAL mode)
	// and prevent screen tearing
	VSync(0);

  // Flip buffer index
	db = !db;
  nextpri = pribuff[db];

  ClearOTagR(ot[db], OT_LEN);

	// Switch pages	
	PutDispEnv(&disp[db]);
	PutDrawEnv(&draw[db]);

	// Enable display output, ResetGraph() disables it by default
	SetDispMask(1);

  DrawOTag(ot[!db]+OT_LEN-1);
}

// Main function, program entrypoint
int main(int argc, const char *argv[])
{
	int counter;

	// Init stuff	
	init();
	
	// Main loop
	counter = 0;
	while (1)
	{
		// Print the obligatory hello world and counter to show that the
		// program isn't locking up to the last created text stream
		FntPrint(-1, "[D]ivertissement\n[V]ideoludique\n[S]apiocentrique\n");
		FntPrint(-1, "COUNTER=%d\n", counter);

    // Update geometry
    drawTestTile(260, 10, 255, 0, 255);
    drawTestTile(260, 45, 255, 0, 0);
    drawTestTile(260, 80, 255, 255, 0);
    drawTestTile(260, 115, 0, 255, 0);
    drawTestTile(260, 150, 0, 255, 255);
    drawTestTile(260, 185, 0, 0, 255);
    drawGeometry();

    // Draw the last created text stream
		FntFlush(-1);
		
		// Update display
		display();
		
		// Increment the counter
		counter++;
	}
	
	return 0;
}
