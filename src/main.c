/* 
 * LibPSn00b Example Programs
 *
 * Hello World Example originally from:
 * 2019-2020 Meido-Tek Productions / PSn00bSDK Project
 * By: Lameguy64
 *
 * Modified by Blaxar for the sake of testing things
 */
 
#include <stdio.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <inline_c.h>

/* Screen resolution */
#define SCREEN_XRES    320
#define SCREEN_YRES    240

/* Screen center position */
#define CENTERX      SCREEN_XRES>>1
#define CENTERY      SCREEN_YRES>>1

#define OT_LEN 8192
#define NB_FACES 2

// Define display/draw environments for double buffering
DISPENV disp[2];
DRAWENV draw[2];
uint32_t ot[2][OT_LEN];
int db;

SVECTOR verts[] = {
  { -400, -400, -400, 0 },
  {  400, -400, -400, 0 },
  { -400,  400, -400, 0 },
  {  400,  400, -400, 0 },
  {  400, -400,  400, 0 },
  { -400, -400,  400, 0 },
  {  400,  400,  400, 0 },
  { -400,  400,  400, 0 }
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
  ONE * 3/4, 0, 0,  /* Red   */
  ONE * 3/4, 0, 0,  /* Green */
  ONE * 3/4, 0, 0   /* Blue  */
};

MATRIX light_mtx = {
  /* X,  Y,  Z */
  -2048 , -2048 , -2048,
  0    , 0    , 0,
  0    , 0    , 0
};

/* Reference texture data */
extern const uint32_t tim_texture[];

/* Reference RWX model data */
extern const uint8_t rwx_model[];

const uint16_t* rwx_nb_vertices = (uint16_t*) &rwx_model[0];
const uint16_t* rwx_nb_faces = (uint16_t*) &rwx_model[2];
const int16_t* rwx_vertices = (int16_t*) &rwx_model[4];
uint16_t* rwx_faces = NULL;
int16_t* rwx_normals = NULL;

SVECTOR tmp_vertices[3];
SVECTOR tmp_normal;

/* To keep a copy of the TIM coordinates for later */
int tim_mode;
RECT tim_prect, tim_crect;

SVECTOR  rot = { 0 };      /* Rotation vector for Rotmatrix */
VECTOR  pos = { 0, 2800, 8000};  /* Translation vector for TransMatrix */
MATRIX  mtx, lmtx;        /* Rotation matrices for geometry and lighting */

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

void drawDummyGeometry() {
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

    /* Cull if looking at backface */
    gte_nclip();
    gte_stopz(&p);
    if (p < 0)
      continue;

    gte_avsz4();
    gte_stotz(&p);

    /* Skip if clipping off */
    if((p>>2) > OT_LEN)
      continue;

    /* Initialize a triangle primitive */
    POLY_F3* pol3 = (POLY_F3*) nextpri;
    setPolyF3(pol3);

    /* Set the projected vertices to the primitive */
    gte_stsxy0(&pol3->x0);
    gte_stsxy1(&pol3->x1);
    gte_stsxy2(&pol3->x2);

    gte_ldrgb(&pol3->r0);
    gte_ldv0(&norms[i]);
    gte_ncs();
    gte_strgb(&pol3->r0);

    addPrim(ot[db]+(p>>2), pol3);
    nextpri += sizeof(POLY_F3);
  }
}

void printFace(const uint16_t* face) {
  FntPrint(-1, "%hu, %hu, %hu\n", face[0], face[1], face[2]);
}

void printFaceVertices(const uint16_t* face, const int16_t* vertices) {
  FntPrint(-1, "%hd, %hd, %hd\n%hd, %hd, %hd\n%hd, %hd, %hd\n",
           vertices[face[0] * 3],
           vertices[face[0] * 3 + 1],
           vertices[face[0] * 3 + 2],
           vertices[face[1] * 3],
           vertices[face[1] * 3 + 1],
           vertices[face[1] * 3 + 2],
           vertices[face[2] * 3],
           vertices[face[2] * 3 + 1],
           vertices[face[2] * 3 + 2]
    );
}

void setFaceVertices(SVECTOR vert[3], const size_t id, const int16_t* vertices,
               const uint16_t* faces) {
  vert[0].vx = vertices[faces[id * 3] * 3];
  vert[0].vy = -vertices[faces[id * 3] * 3 + 1];
  vert[0].vz = vertices[faces[id * 3] * 3 + 2];
  vert[0].pad = 0;
  vert[1].vx = vertices[faces[id * 3 + 1] * 3];
  vert[1].vy = -vertices[faces[id * 3 + 1] * 3 + 1];
  vert[1].vz = vertices[faces[id * 3 + 1] * 3 + 2];
  vert[1].pad = 0;
  vert[2].vx = vertices[faces[id * 3 + 2] * 3];
  vert[2].vy = -vertices[faces[id * 3 + 2] * 3 + 1];
  vert[2].vz = vertices[faces[id * 3 + 2] * 3 + 2];
  vert[2].pad = 0;
}

void setFaceNormal(SVECTOR* norm, const size_t id,
                   const int16_t* normals) {
  norm->vx = normals[id * 3];
  norm->vy = normals[id * 3 + 1];
  norm->vz = normals[id * 3 + 2];
  norm->pad = 0;
}

void drawObject(const uint16_t nb_vertices, uint16_t nb_faces,
                const int16_t* vertices, const uint16_t* faces,
                const int16_t* normals) {
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

  rot.vx += 4;
  rot.vy += 16;
  rot.vz += 8;

  /* Set light matrix */
  gte_SetLightMatrix(&lmtx);

  for (int i = 0; i < nb_faces; i++) {
    setFaceVertices(tmp_vertices, i, vertices, faces);
    setFaceNormal(&tmp_normal, i, normals);

    /* Load first 3 vertices to make a face */
    gte_ldv3(
      &tmp_vertices[0],
      &tmp_vertices[1],
      &tmp_vertices[2]
      );

    /* Rotation, Translation and Perspective Triple */
    gte_rtpt();

    /* Cull if looking at backface */
    gte_nclip();
    gte_stopz(&p);
    if (p < 0)
       continue;

    gte_avsz4();
    gte_stotz(&p);

    /* Skip if clipping off */
    if((p>>2) > OT_LEN)
      continue;

    /* Initialize a triangle primitive */
    POLY_F3* pol3 = (POLY_F3*) nextpri;
    setPolyF3(pol3);

    /* Set the projected vertices to the primitive */
    gte_stsxy0(&pol3->x0);
    gte_stsxy1(&pol3->x1);
    gte_stsxy2(&pol3->x2);

    gte_ldrgb(&pol3->r0);
    gte_ldv0(&tmp_normal);
    gte_ncs();
    gte_strgb(&pol3->r0);

    addPrim(ot[db]+(p>>2), pol3);
    nextpri += sizeof(POLY_F3);
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

  rwx_faces = (uint16_t*) &rwx_model[4 + (*rwx_nb_vertices) * 6];
  rwx_normals = (int16_t*) &rwx_model[4 + (*rwx_nb_vertices) * 6 + (*rwx_nb_faces) * 6];

  while (1)
  {
    // Print the obligatory hello world and counter to show that the
    // program isn't locking up to the last created text stream
    FntPrint(-1, "[D]ivertissement\n[V]ideoludique\n[S]apiocentrique\n");
    FntPrint(-1, "#verts: %u, #faces: %u\n", *rwx_nb_vertices, *rwx_nb_faces);
    //printFace(&rwx_faces[0]);
    //printVertex(&rwx_vertices[0]);
    //printFaceVertices(&rwx_faces[0], rwx_vertices);
    FntPrint(-1, "COUNTER=%d\n", counter);

    // Update geometry
    drawTestTile(260, 10, 255, 0, 255);
    drawTestTile(260, 45, 255, 0, 0);
    drawTestTile(260, 80, 255, 255, 0);
    drawTestTile(260, 115, 0, 255, 0);
    drawTestTile(260, 150, 0, 255, 255);
    drawTestTile(260, 185, 0, 0, 255);
    //drawGeometry();
    //drawDummyGeometry();
    drawObject(*rwx_nb_vertices, *rwx_nb_faces,
               rwx_vertices, rwx_faces, rwx_normals);

    // Draw the last created text stream
    FntFlush(-1);

    // Update display
    display();

    // Increment the counter
    counter++;
  }

  return 0;
}
