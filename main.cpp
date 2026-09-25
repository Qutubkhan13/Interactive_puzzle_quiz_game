// =============================================================================
//  CYBER SECURITY PUZZLE QUIZ  -  OpenGL (legacy fixed-function) + FreeGLUT
//  Author: Qutubkhan Nalwala
//
//  Idea: answer cyber-safety questions. Every CORRECT answer reveals one piece
//  of a picture (Level 1 = padlock, Level 2 = shield). Wrong answers reveal
//  nothing and the question comes back later. Finish both pictures to win.
//
//  Everything is drawn with OpenGL primitives only (quads, triangles, polygons,
//  circles made from triangle fans, lines). No images or textures.
// =============================================================================
#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <string>

// ----------------------------------------------------------------------------
//  1. CONSTANTS AND GLOBAL GAME DATA
// ----------------------------------------------------------------------------
const int   WIN_W = 1200;                 // window size in pixels; the 2D
const int   WIN_H = 800;                  // coordinate system matches it 1:1
const float PI    = 3.14159265f;

const char* PLAYER_NAME = "Qutubkhan Nalwala";   // engraved on the plaque

// GLUT bitmap fonts used for all text
void* FONT_SMALL = GLUT_BITMAP_HELVETICA_12;
void* FONT_MED   = GLUT_BITMAP_HELVETICA_18;
void* FONT_BIG   = GLUT_BITMAP_TIMES_ROMAN_24;

// Game states (a simple state machine - the display and keyboard functions
// look at this to decide what to draw / what a key means)
enum GameState { MENU, PLAYING, LEVEL_COMPLETE, GAME_COMPLETE };

// One quiz question: text, four options, index (0-3) of the correct option
struct Question {
    const char* text;
    const char* options[4];
    int         correct;
};

const int LEVEL_COUNT         = 2;
const int QUESTIONS_PER_LEVEL = 5;        // = 5 puzzle pieces per level
const int TOTAL_QUESTIONS     = LEVEL_COUNT * QUESTIONS_PER_LEVEL;
const float TIME_PER_QUESTION = 15.0f;    // seconds

// Questions 0-4 belong to level 1 (padlock), 5-9 to level 2 (shield).
// Question number i inside a level reveals puzzle piece i of that level.
Question questions[TOTAL_QUESTIONS] = {
    // ---- Level 1 ----
    {"Which of these is the strongest password?",
     {"password123", "Qutub2005", "T!9x#Lm2$vQ8", "letmein"}, 2},
    {"What does the 'S' in HTTPS stand for?",
     {"Secure", "Speed", "Server", "Simple"}, 0},
    {"An email urgently asks you to 'verify your bank login' via a link. It is:",
     {"A normal bank notice", "Phishing", "A software update", "A prize"}, 1},
    {"What does two-factor authentication (2FA) add?",
     {"A second proof of identity", "A faster login", "A longer password", "Free antivirus"}, 0},
    {"Which software is designed to harm or spy on your computer?",
     {"Browser", "Malware", "Compiler", "Driver"}, 1},
    // ---- Level 2 ----
    {"What does a firewall do?",
     {"Cools the CPU", "Filters network traffic", "Compresses files", "Speeds up Wi-Fi"}, 1},
    {"Why should you install software updates?",
     {"They change your wallpaper", "They delete old files", "They patch security holes", "They dim the screen"}, 2},
    {"What is the safest way to use public Wi-Fi?",
     {"Share your passwords", "Turn off HTTPS", "Disable antivirus", "Use a VPN"}, 3},
    {"Ransomware is malware that...",
     {"Encrypts files and demands payment", "Speeds up your PC", "Blocks pop-up ads", "Backs up your data"}, 0},
    {"What is 'social engineering'?",
     {"Building social networks", "Tricking people into giving away secrets", "Designing web pages", "Repairing hardware"}, 1}
};

// Game variables
GameState state         = MENU;
int   currentLevel      = 0;              // 0 or 1
int   currentQuestion   = 0;              // index into questions[]
int   chosenOption      = -1;             // option the player picked (-1 = none / timed out)
bool  answered          = false;          // true after answering, until ENTER is pressed
bool  lastCorrect       = false;
bool  timedOut          = false;
int   score             = 0;
float timeLeft          = TIME_PER_QUESTION;
bool  solved[TOTAL_QUESTIONS];            // solved[i] == true -> that puzzle piece is revealed

// Animation variables
float animTime   = 0.0f;                  // seconds since start, drives sin() pulses
float flashTimer = 0.0f;                  // screen flash (green/red) fades from 0.5 to 0
float cloudX[4]  = {100, 420, 700, 950};
float cloudY[4]  = {610, 700, 640, 720};
float starDrift  = 0.0f;
int   lastTicks  = 0;

// ----------------------------------------------------------------------------
//  2. BASIC DRAWING HELPERS  (rectangles, circles, lines, polygons, text)
// ----------------------------------------------------------------------------

// Rectangle with lower-left corner (x,y). 'a' is opacity (1 = solid).
void drawRect(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

// Rectangle that fades smoothly from a bottom colour to a top colour
// (OpenGL interpolates colour between vertices = smooth shading).
void drawGradientRect(float x, float y, float w, float h,
                      float r1, float g1, float b1,      // bottom colour
                      float r2, float g2, float b2) {    // top colour
    glBegin(GL_QUADS);
    glColor3f(r1, g1, b1); glVertex2f(x, y);     glVertex2f(x + w, y);
    glColor3f(r2, g2, b2); glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

// Ellipse (or circle when rx == ry) drawn as a triangle fan.
void drawEllipse(float cx, float cy, float rx, float ry, float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 48; i++) {
        float ang = 2.0f * PI * i / 48.0f;
        glVertex2f(cx + rx * cosf(ang), cy + ry * sinf(ang));
    }
    glEnd();
}

void drawCircle(float cx, float cy, float radius, float r, float g, float b, float a = 1.0f) {
    drawEllipse(cx, cy, radius, radius, r, g, b, a);
}

void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                  float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLES);
    glVertex2f(x1, y1); glVertex2f(x2, y2); glVertex2f(x3, y3);
    glEnd();
}

void drawLine(float x1, float y1, float x2, float y2, float width,
              float r, float g, float b, float a = 1.0f) {
    glLineWidth(width);
    glColor4f(r, g, b, a);
    glBegin(GL_LINES);
    glVertex2f(x1, y1); glVertex2f(x2, y2);
    glEnd();
    glLineWidth(1.0f);
}

// Convex polygon from an array of x,y pairs: p = {x0,y0, x1,y1, ...}
void drawPolygon(const float* p, int n, float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) glVertex2f(p[2 * i], p[2 * i + 1]);
    glEnd();
}

// --- "Ghost" outlines: a dashed white outline shows where an UNREVEALED
//     puzzle piece will appear, so the picture visibly starts incomplete.
void ghostBegin() {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(3, 0x5555);
    glLineWidth(2.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
}
void ghostEnd() {
    glDisable(GL_LINE_STIPPLE);
    glLineWidth(1.0f);
}
void ghostPolygon(const float* p, int n) {
    ghostBegin();
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n; i++) glVertex2f(p[2 * i], p[2 * i + 1]);
    glEnd();
    ghostEnd();
}
void ghostEllipse(float cx, float cy, float rx, float ry) {
    ghostBegin();
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 40; i++) {
        float ang = 2.0f * PI * i / 40.0f;
        glVertex2f(cx + rx * cosf(ang), cy + ry * sinf(ang));
    }
    glEnd();
    ghostEnd();
}

// Piece helpers: draw filled when revealed (on == true), ghost outline otherwise.
void polyPiece(const float* p, int n, bool on, float r, float g, float b) {
    if (on) drawPolygon(p, n, r, g, b); else ghostPolygon(p, n);
}

// --- Text ---
void drawText(float x, float y, const std::string& s, void* font,
              float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
    glRasterPos2f(x, y);
    for (size_t i = 0; i < s.size(); i++)
        glutBitmapCharacter(font, (unsigned char)s[i]);
}

// Text centred horizontally around cx
void drawTextCentered(float cx, float y, const std::string& s, void* font,
                      float r, float g, float b, float a = 1.0f) {
    int w = glutBitmapLength(font, (const unsigned char*)s.c_str());
    drawText(cx - w / 2.0f, y, s, font, r, g, b, a);
}

// ----------------------------------------------------------------------------
//  3. BACKGROUND SCENES  (one per level)
// ----------------------------------------------------------------------------

void drawCloud(float x, float y) {
    drawCircle(x,      y,      26, 1, 1, 1);
    drawCircle(x + 28, y + 10, 30, 1, 1, 1);
    drawCircle(x + 58, y,      26, 1, 1, 1);
    drawRect  (x,      y - 26, 58, 26, 1, 1, 1);
}

void drawBuilding(float x, float w, float h, float groundY, float r, float g, float b, bool night) {
    drawRect(x, groundY, w, h, r, g, b);
    // windows: a grid of small squares; at night some are lit and flicker
    for (float wy = groundY + 15; wy < groundY + h - 20; wy += 32) {
        for (float wx = x + 10; wx < x + w - 18; wx += 26) {
            bool lit = night ? (((int)(wx * 7 + wy * 3)) % 3 != 0) : true;
            if (night && lit) drawRect(wx, wy, 14, 18, 1.0f, 0.9f, 0.4f, 0.75f + 0.25f * sinf(animTime * 2 + wx));
            else if (!night)  drawRect(wx, wy, 14, 18, 0.7f, 0.85f, 0.95f);
            else              drawRect(wx, wy, 14, 18, 0.1f, 0.1f, 0.2f);
        }
    }
}

// Level 1: sunny day - sky gradient, pulsing sun with rotating rays,
// moving clouds, buildings, trees and a grass ground.
void drawDayScene() {
    drawGradientRect(0, 0, WIN_W, WIN_H, 0.75f, 0.90f, 1.0f, 0.25f, 0.55f, 0.92f);   // sky

    // sun (pulses) with rotating rays
    float pulse = 1.0f + 0.06f * sinf(animTime * 2.0f);
    for (int i = 0; i < 12; i++) {
        float ang = animTime * 0.3f + i * PI / 6.0f;
        drawLine(1080 + 62 * cosf(ang), 700 + 62 * sinf(ang),
                 1080 + 88 * cosf(ang), 700 + 88 * sinf(ang), 3, 1.0f, 0.85f, 0.2f);
    }
    drawCircle(1080, 700, 52 * pulse, 1.0f, 0.88f, 0.25f);

    for (int i = 0; i < 4; i++) drawCloud(cloudX[i], cloudY[i]);   // animated clouds

    // buildings (kept to the sides so the padlock stays clear)
    drawBuilding( 40, 90, 230, 340, 0.55f, 0.55f, 0.65f, false);
    drawBuilding(140, 110, 300, 340, 0.45f, 0.5f, 0.62f, false);
    drawBuilding(260, 100, 180, 340, 0.6f, 0.52f, 0.55f, false);
    drawBuilding(830, 100, 200, 340, 0.6f, 0.52f, 0.55f, false);
    drawBuilding(940, 110, 290, 340, 0.45f, 0.5f, 0.62f, false);
    drawBuilding(1060, 110, 210, 340, 0.55f, 0.55f, 0.65f, false);

    // grass ground + darker edge
    drawRect(0, 0, WIN_W, 345, 0.25f, 0.62f, 0.28f);
    drawRect(0, 335, WIN_W, 12, 0.18f, 0.48f, 0.2f);

    // trees (trunk rectangle + leaf circles)
    for (int i = 0; i < 2; i++) {
        float tx = (i == 0) ? 440.0f : 790.0f;
        drawRect(tx - 6, 345, 12, 40, 0.4f, 0.25f, 0.1f);
        drawCircle(tx, 400, 30, 0.15f, 0.5f, 0.18f);
        drawCircle(tx - 18, 385, 22, 0.18f, 0.55f, 0.2f);
        drawCircle(tx + 18, 385, 22, 0.18f, 0.55f, 0.2f);
    }
}

// Level 2: futuristic night - twinkling drifting stars, moon, hills,
// lit skyline and a glowing neon "data line" with moving packets.
void drawNightScene() {
    drawGradientRect(0, 0, WIN_W, WIN_H, 0.20f, 0.10f, 0.40f, 0.02f, 0.02f, 0.12f);  // sky

    // stars: drift sideways (wrap around) and twinkle using sin()
    for (int i = 0; i < 70; i++) {
        float x = fmodf(i * 137.5f + starDrift, (float)WIN_W);
        float y = 360.0f + fmodf(i * 61.3f, 430.0f);
        float size = 1.6f + 1.2f * sinf(animTime * 3.0f + i);
        drawCircle(x, y, size, 1.0f, 1.0f, 0.85f);
    }

    // moon (crescent = light circle with a sky-coloured circle on top)
    drawCircle(1080, 700, 50, 0.95f, 0.95f, 0.8f);
    drawCircle(1100, 712, 44, 0.04f, 0.03f, 0.18f);

    // far hills (triangles)
    drawTriangle(-50, 345, 200, 470, 450, 345, 0.14f, 0.12f, 0.30f);
    drawTriangle(750, 345, 1000, 480, 1250, 345, 0.14f, 0.12f, 0.30f);

    // skyline
    drawBuilding( 40, 90, 200, 340, 0.10f, 0.10f, 0.25f, true);
    drawBuilding(140, 110, 280, 340, 0.08f, 0.09f, 0.22f, true);
    drawBuilding(260, 100, 170, 340, 0.12f, 0.10f, 0.26f, true);
    drawBuilding(830, 100, 190, 340, 0.12f, 0.10f, 0.26f, true);
    drawBuilding(940, 110, 270, 340, 0.08f, 0.09f, 0.22f, true);
    drawBuilding(1060, 110, 200, 340, 0.10f, 0.10f, 0.25f, true);

    // ground with glowing neon line and moving data packets
    drawRect(0, 0, WIN_W, 345, 0.08f, 0.06f, 0.18f);
    float glow = 0.6f + 0.4f * sinf(animTime * 3.0f);
    drawRect(0, 340, WIN_W, 5, 0.1f, 0.9f, 1.0f, glow);
    for (int i = 0; i < 6; i++) {
        float px = fmodf(animTime * 150.0f + i * 200.0f, (float)WIN_W);
        drawRect(px, 337, 30, 11, 0.6f, 1.0f, 1.0f);
    }
}

void drawBackground(int level) {
    if (level == 0) drawDayScene(); else drawNightScene();
}

// ----------------------------------------------------------------------------
//  4. THE PUZZLES  (5 pieces each; pieces[i] == true -> piece i is revealed)
// ----------------------------------------------------------------------------

// ---------- Level 1: PADLOCK ----------
// Pieces: 0 shackle, 1 body (3D box), 2 front plate, 3 keyhole, 4 rivets+shine
void drawPadlock(const bool* pieces) {
    // ground shadow under the lock - always visible (gives depth)
    drawEllipse(615, 348, 170, 22, 0.0f, 0.0f, 0.0f, 0.35f);

    // ---- piece 0: shackle (silver arch) ----
    {
        const float cx = 600, cy = 540, ro = 80, ri = 60, rho = 90, rhi = 70;
        const int N = 20;
        if (pieces[0]) {
            glColor3f(0.78f, 0.80f, 0.86f);
            glBegin(GL_QUAD_STRIP);                       // thick arch between two arcs
            for (int i = 0; i <= N; i++) {
                float a = PI * i / N;
                glVertex2f(cx + ro * cosf(a), cy + rho * sinf(a));
                glVertex2f(cx + ri * cosf(a), cy + rhi * sinf(a));
            }
            glEnd();
            drawRect(cx + ri, 510, ro - ri, 30, 0.78f, 0.80f, 0.86f);     // right leg
            drawRect(cx - ro, 510, ro - ri, 30, 0.78f, 0.80f, 0.86f);     // left leg
            glLineWidth(3.0f);                                              // highlight
            glColor4f(1, 1, 1, 0.7f);
            glBegin(GL_LINE_STRIP);
            for (int i = 3; i <= N - 6; i++) {
                float a = PI * i / N;
                glVertex2f(cx + 73 * cosf(a), cy + 83 * sinf(a));
            }
            glEnd();
            glLineWidth(1.0f);
        } else {
            ghostBegin();
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= N; i++) { float a = PI * i / N; glVertex2f(cx + ro * cosf(a), cy + rho * sinf(a)); }
            glEnd();
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= N; i++) { float a = PI * i / N; glVertex2f(cx + ri * cosf(a), cy + rhi * sinf(a)); }
            glEnd();
            glBegin(GL_LINES);
            glVertex2f(520, 510); glVertex2f(520, 540);
            glVertex2f(540, 510); glVertex2f(540, 540);
            glVertex2f(660, 510); glVertex2f(660, 540);
            glVertex2f(680, 510); glVertex2f(680, 540);
            glEnd();
            ghostEnd();
        }
    }

    // ---- piece 1: body drawn as a 3D box (oblique perspective) ----
    // front face + RIGHT side + TOP side, each with a different shade of gold.
    {
        float front[] = {480,350, 720,350, 720,530, 480,530};
        if (pieces[1]) {
            float top[]  = {480,530, 720,530, 740,548, 500,548};       // lightest: faces the light
            float side[] = {720,350, 740,368, 740,548, 720,530};       // darkest: in shadow
            drawPolygon(top,  4, 1.00f, 0.88f, 0.50f);
            drawPolygon(side, 4, 0.55f, 0.38f, 0.06f);
            drawPolygon(front,4, 0.85f, 0.62f, 0.12f);
        } else {
            ghostPolygon(front, 4);
        }
    }

    // ---- piece 2: front plate (layered inset = bevel) ----
    {
        float plate[] = {498,368, 702,368, 702,512, 498,512};
        if (pieces[2]) {
            drawRect(494, 364, 212, 152, 0.60f, 0.42f, 0.08f);          // dark border
            drawRect(498, 368, 204, 144, 0.97f, 0.80f, 0.28f);          // light plate
            drawRect(498, 368, 204, 8,   0.80f, 0.60f, 0.14f);          // bottom bevel
        } else {
            ghostPolygon(plate, 4);
        }
    }

    // ---- piece 3: keyhole (circle + tapered slot) ----
    {
        float slot[] = {588,455, 612,455, 624,392, 576,392};
        if (pieces[3]) {
            drawCircle(600, 462, 27, 0.15f, 0.10f, 0.04f);
            drawPolygon(slot, 4, 0.15f, 0.10f, 0.04f);
        } else {
            ghostEllipse(600, 462, 27, 27);
            ghostPolygon(slot, 4);
        }
    }

    // ---- piece 4: four rivets + diagonal shine ----
    {
        float rx[4] = {516, 684, 516, 684};
        float ry[4] = {384, 384, 496, 496};
        for (int i = 0; i < 4; i++) {
            if (pieces[4]) {
                drawCircle(rx[i], ry[i], 9, 0.70f, 0.72f, 0.78f);
                drawCircle(rx[i] - 2, ry[i] + 2, 3.5f, 1, 1, 1);
            } else {
                ghostEllipse(rx[i], ry[i], 9, 9);
            }
        }
        if (pieces[4]) {
            float shine[] = {520,512, 552,512, 620,368, 588,368};
            drawPolygon(shine, 4, 1, 1, 1, 0.22f);
        }
    }
}

// ---------- Level 2: SHIELD ----------
// Pieces: 0 silver rim, 1 blue body, 2 light facet, 3 gold badge, 4 check mark

// Shield outline scaled about its centre: s = 1 is full size.
void shieldPoints(float s, float* out) {
    const float base[12] = {480,650, 600,682, 720,650, 720,540, 600,380, 480,540};
    const float cx = 600, cy = 535;
    for (int i = 0; i < 6; i++) {
        out[2 * i]     = cx + s * (base[2 * i]     - cx);
        out[2 * i + 1] = cy + s * (base[2 * i + 1] - cy);
    }
}

void drawShield(const bool* pieces) {
    // pulsing glow behind the shield + ground shadow
    float glow = 0.10f + 0.05f * sinf(animTime * 2.5f);
    drawCircle(600, 535, 235, 0.2f, 0.9f, 1.0f, glow);
    drawEllipse(610, 352, 150, 18, 0, 0, 0, 0.40f);

    float outer[12], inner[12], shadow[12];
    shieldPoints(1.00f, outer);
    shieldPoints(0.86f, inner);
    for (int i = 0; i < 6; i++) { shadow[2 * i] = outer[2 * i] + 16; shadow[2 * i + 1] = outer[2 * i + 1] - 16; }

    drawPolygon(shadow, 6, 0, 0, 0, 0.35f);          // DROP SHADOW (offset copy) -> 3D look

    // piece 0: silver rim
    polyPiece(outer, 6, pieces[0], 0.76f, 0.79f, 0.86f);
    // piece 1: blue body
    polyPiece(inner, 6, pieces[1], 0.14f, 0.38f, 0.85f);
    // piece 2: lighter left facet (makes the shield look folded/3D)
    {
        // facet = top-middle, top-left, left, bottom points of the inner shield
        float f[] = {inner[2],inner[3], inner[0],inner[1], inner[10],inner[11], inner[8],inner[9]};
        polyPiece(f, 4, pieces[2], 0.32f, 0.62f, 1.0f);
    }
    // piece 3: gold badge (two layered circles)
    if (pieces[3]) {
        drawCircle(600, 548, 54, 0.75f, 0.55f, 0.10f);
        drawCircle(600, 548, 46, 1.00f, 0.82f, 0.25f);
    } else {
        ghostEllipse(600, 548, 50, 50);
    }
    // piece 4: white check mark (with a small dark shadow line under it)
    if (pieces[4]) {
        drawLine(577, 545, 597, 522, 12, 0.5f, 0.35f, 0.05f);
        drawLine(597, 522, 631, 574, 12, 0.5f, 0.35f, 0.05f);
        drawLine(575, 549, 595, 526, 10, 1, 1, 1);
        drawLine(595, 526, 629, 578, 10, 1, 1, 1);
    } else {
        ghostBegin();
        glBegin(GL_LINE_STRIP);
        glVertex2f(575, 549); glVertex2f(595, 526); glVertex2f(629, 578);
        glEnd();
        ghostEnd();
    }
}

// Draws the puzzle of the given level using that level's slice of solved[]
void drawPuzzle(int level, bool showAll) {
    static const bool ALL_ON[QUESTIONS_PER_LEVEL] = {true, true, true, true, true};
    const bool* p = showAll ? ALL_ON : &solved[level * QUESTIONS_PER_LEVEL];
    if (level == 0) drawPadlock(p); else drawShield(p);
}

// ----------------------------------------------------------------------------
//  5. UI: PLAQUE WITH NAME, TITLE, PANELS, QUESTION
// ----------------------------------------------------------------------------

// Hanging wooden sign with my name ENGRAVED on it (light offset copy of the
// text below-right + dark text on top = carved look). Shown on every screen.
void drawNamePlaque() {
    drawLine(80, 800, 80, 772, 3, 0.7f, 0.7f, 0.7f);              // chains
    drawLine(360, 800, 360, 772, 3, 0.7f, 0.7f, 0.7f);
    drawRect(36, 684, 380, 90, 0, 0, 0, 0.35f);                    // drop shadow
    drawRect(30, 690, 380, 82, 0.30f, 0.17f, 0.06f);               // dark frame
    drawRect(38, 698, 364, 66, 0.62f, 0.40f, 0.18f);               // wood
    drawRect(38, 698, 364, 6,  0.50f, 0.31f, 0.12f);               // wood grain lines
    drawRect(38, 730, 364, 3,  0.55f, 0.35f, 0.15f);
    drawTextCentered(220, 745, "Created by", FONT_SMALL, 0.25f, 0.13f, 0.03f);
    // engraved name: highlight (offset), then dark text
    drawTextCentered(221, 711, PLAYER_NAME, FONT_BIG, 0.90f, 0.70f, 0.45f);
    drawTextCentered(220, 712, PLAYER_NAME, FONT_BIG, 0.18f, 0.09f, 0.01f);
}

// Level title, top centre
void drawTitle(const std::string& s) {
    drawTextCentered(WIN_W / 2 + 40 + 1, 748 - 1, s, FONT_BIG, 0, 0, 0, 0.6f);
    drawTextCentered(WIN_W / 2 + 40,     748,     s, FONT_BIG, 1, 1, 1);
}

// Dark rounded-look panel at the bottom for question / messages
void drawPanel() {
    drawRect(66, 14, 1080, 290, 0, 0, 0, 0.4f);        // shadow
    drawRect(60, 20, 1080, 280, 0.35f, 0.40f, 0.55f);  // border
    drawRect(64, 24, 1072, 272, 0.06f, 0.08f, 0.14f);  // body
}

int piecesSolved(int level) {
    int c = 0;
    for (int i = 0; i < QUESTIONS_PER_LEVEL; i++) if (solved[level * QUESTIONS_PER_LEVEL + i]) c++;
    return c;
}

// Question text, 4 option buttons, timer bar, score and feedback
void drawQuestion() {
    const Question& q = questions[currentQuestion];
    int numInLevel = currentQuestion - currentLevel * QUESTIONS_PER_LEVEL + 1;

    drawPanel();
    drawText(90, 268, "Q" + std::to_string(numInLevel) + ".  " + q.text, FONT_MED, 1, 1, 1);
    drawText(965, 268, "Score: " + std::to_string(score), FONT_MED, 0.4f, 1.0f, 0.6f);

    // countdown bar: green -> yellow -> red (flashes when nearly out)
    float frac = timeLeft / TIME_PER_QUESTION;
    float br = frac > 0.5f ? 0.2f : 1.0f;
    float bg = frac > 0.25f ? 0.85f : 0.2f;
    float ba = (frac < 0.25f && !answered) ? 0.6f + 0.4f * sinf(animTime * 12.0f) : 1.0f;
    drawRect(90, 243, 860, 14, 0.15f, 0.17f, 0.25f);
    drawRect(90, 243, 860 * frac, 14, br, bg, 0.2f, ba);
    drawText(965, 244, "Time: " + std::to_string((int)ceilf(timeLeft)) + "s", FONT_MED, 1.0f, 0.9f, 0.3f);

    // option buttons in a 2x2 grid
    for (int i = 0; i < 4; i++) {
        float bx = 90 + (i % 2) * 520;
        float by = 170 - (i / 2) * 70;
        float r = 0.18f, g = 0.24f, b = 0.40f;                 // normal
        float pulse = 0.5f + 0.5f * sinf(animTime * 8.0f);
        if (answered) {
            if (i == q.correct)      { r = 0.10f; g = 0.55f + 0.25f * pulse; b = 0.20f; }   // correct = green
            else if (i == chosenOption) { r = 0.65f + 0.2f * pulse; g = 0.10f; b = 0.10f; } // picked wrong = red
            else                     { r = 0.11f; g = 0.13f; b = 0.20f; }                   // dimmed
        }
        drawRect(bx + 3, by - 3, 500, 50, 0, 0, 0, 0.4f);     // button shadow
        drawRect(bx, by, 500, 50, r, g, b);
        drawRect(bx, by + 44, 500, 6, r + 0.1f, g + 0.1f, b + 0.1f);   // top highlight
        drawText(bx + 15, by + 18, std::to_string(i + 1) + ")  " + q.options[i], FONT_MED, 1, 1, 1);
    }

    // feedback message + prompt
    if (answered) {
        if (lastCorrect) {
            bool done = piecesSolved(currentLevel) == QUESTIONS_PER_LEVEL;
            drawTextCentered(600, 62, done ? "CORRECT! The puzzle is complete!"
                                           : "CORRECT!  A new puzzle piece appeared!",
                             FONT_BIG, 0.2f, 1.0f, 0.4f);
        } else if (timedOut) {
            drawTextCentered(600, 62, "TIME'S UP!  No piece revealed - this question will return.",
                             FONT_MED, 1.0f, 0.35f, 0.35f);
        } else {
            drawTextCentered(600, 62, "WRONG!  No piece revealed - this question will return.",
                             FONT_MED, 1.0f, 0.35f, 0.35f);
        }
        drawTextCentered(600, 34, "Press ENTER to continue", FONT_MED, 1, 1, 1, 0.6f + 0.4f * sinf(animTime * 5.0f));
    } else {
        drawTextCentered(600, 45, "Press 1-4 or A-D to answer", FONT_SMALL, 0.7f, 0.75f, 0.9f);
    }
}

void drawMenuPanel() {
    drawPanel();
    drawTextCentered(600, 250, "CYBER SECURITY PUZZLE QUIZ", FONT_BIG, 1.0f, 0.9f, 0.3f);
    drawTextCentered(600, 205, "Answer cyber-safety questions to rebuild the vault padlock and the shield!",
                     FONT_MED, 1, 1, 1);
    drawTextCentered(600, 170, "Every correct answer reveals one puzzle piece. Wrong answers reveal nothing.",
                     FONT_MED, 0.8f, 0.85f, 1.0f);
    drawTextCentered(600, 125, "Controls:  1-4 or A-D = answer   |   ENTER = continue   |   R = restart   |   ESC = quit",
                     FONT_SMALL, 0.7f, 0.75f, 0.9f);
    float a = 0.55f + 0.45f * sinf(animTime * 4.0f);
    drawTextCentered(600, 65, "Press ENTER to start", FONT_BIG, 0.3f, 1.0f, 0.7f, a);
}

void drawLevelCompletePanel() {
    drawPanel();
    std::string head = (currentLevel == 0) ? "LEVEL 1 COMPLETE - The padlock is rebuilt!"
                                           : "LEVEL 2 COMPLETE - The shield is rebuilt!";
    drawTextCentered(600, 235, head, FONT_BIG, 0.3f, 1.0f, 0.5f);
    drawTextCentered(600, 180, "Score so far: " + std::to_string(score), FONT_MED, 1, 0.9f, 0.3f);
    float a = 0.55f + 0.45f * sinf(animTime * 4.0f);
    drawTextCentered(600, 110, (currentLevel == 0) ? "Press ENTER for Level 2" : "Press ENTER to finish",
                     FONT_BIG, 1, 1, 1, a);
}

void drawFinalPanel() {
    drawPanel();
    drawTextCentered(600, 235, "MISSION COMPLETE!  You secured the system!", FONT_BIG, 1.0f, 0.9f, 0.3f);
    drawTextCentered(600, 185, "Final score: " + std::to_string(score), FONT_BIG, 0.3f, 1.0f, 0.6f);
    drawTextCentered(600, 140, "Thanks for playing, and stay safe online!", FONT_MED, 1, 1, 1);
    float a = 0.55f + 0.45f * sinf(animTime * 4.0f);
    drawTextCentered(600, 70, "Press R or ENTER to play again  |  ESC to quit", FONT_MED, 1, 1, 1, a);

    // falling confetti (animated)
    for (int i = 0; i < 60; i++) {
        float x = fmodf(i * 97.0f, (float)WIN_W);
        float y = WIN_H - fmodf(animTime * (60.0f + (i % 5) * 20.0f) + i * 53.0f, (float)WIN_H);
        int c = i % 3;
        drawRect(x, y, 10, 10, c == 0 ? 1.0f : 0.2f, c == 1 ? 1.0f : 0.5f, c == 2 ? 1.0f : 0.3f, 0.9f);
    }
}

// ----------------------------------------------------------------------------
//  6. GAME LOGIC
// ----------------------------------------------------------------------------

// Find the next UNSOLVED question of this level after 'from' (wraps around).
// Returns -1 when every piece has been revealed.
int findNextUnsolved(int level, int from) {
    int start = level * QUESTIONS_PER_LEVEL;
    for (int k = 1; k <= QUESTIONS_PER_LEVEL; k++) {
        int idx = start + ((from - start + k) % QUESTIONS_PER_LEVEL);
        if (!solved[idx]) return idx;
    }
    return -1;
}

void startQuestion(int idx) {
    currentQuestion = idx;
    answered = false;
    chosenOption = -1;
    timedOut = false;
    timeLeft = TIME_PER_QUESTION;
}

void startLevel(int level) {
    currentLevel = level;
    state = PLAYING;
    startQuestion(level * QUESTIONS_PER_LEVEL);       // first question of the level
}

void resetGame() {
    for (int i = 0; i < TOTAL_QUESTIONS; i++) solved[i] = false;
    score = 0;
    currentLevel = 0;
    currentQuestion = 0;
    answered = false;
    flashTimer = 0;
    state = MENU;
}

// option = 0..3, or -1 when the timer ran out
void submitAnswer(int option) {
    if (state != PLAYING || answered) return;
    answered = true;
    chosenOption = option;
    timedOut = (option < 0);
    lastCorrect = (option == questions[currentQuestion].correct);
    if (lastCorrect) {
        solved[currentQuestion] = true;               // reveals ONE puzzle piece
        score += 10 + (int)timeLeft;                  // faster answer = more points
    }
    flashTimer = 0.5f;                                // start green/red flash
}

// ENTER pressed on the feedback screen: next question, or level finished
void continueAfterAnswer() {
    int next = findNextUnsolved(currentLevel, currentQuestion);
    if (next < 0) state = LEVEL_COMPLETE;
    else          startQuestion(next);
}

// ----------------------------------------------------------------------------
//  7. GLUT CALLBACKS
// ----------------------------------------------------------------------------

// Which background/puzzle to show depends on the state
int sceneLevel() {
    if (state == MENU) return 0;
    if (state == GAME_COMPLETE) return 1;
    return currentLevel;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    int lvl = sceneLevel();
    drawBackground(lvl);
    drawPuzzle(lvl, state == MENU || state == GAME_COMPLETE);   // menu/end show the finished picture

    drawNamePlaque();

    if (state == PLAYING) {
        drawTitle(lvl == 0 ? "LEVEL 1 - THE PADLOCK VAULT" : "LEVEL 2 - THE CYBER SHIELD");
        drawRect(WIN_W / 2 + 40 - 75, 705, 150, 32, 0.05f, 0.08f, 0.16f, 0.75f);   // dark badge
        drawTextCentered(WIN_W / 2 + 40, 714,
                 "Pieces: " + std::to_string(piecesSolved(lvl)) + " / " + std::to_string(QUESTIONS_PER_LEVEL),
                 FONT_MED, 1, 1, 1);
        drawQuestion();
    } else if (state == MENU) {
        drawMenuPanel();
    } else if (state == LEVEL_COMPLETE) {
        drawTitle(lvl == 0 ? "LEVEL 1 - THE PADLOCK VAULT" : "LEVEL 2 - THE CYBER SHIELD");
        drawLevelCompletePanel();
    } else {
        drawTitle("GAME COMPLETE");
        drawFinalPanel();
    }

    // full-screen colour flash: green for correct, red for wrong; fades out
    if (flashTimer > 0.0f) {
        float a = 0.30f * (flashTimer / 0.5f);
        if (lastCorrect) drawRect(0, 0, WIN_W, WIN_H, 0.1f, 1.0f, 0.2f, a);
        else             drawRect(0, 0, WIN_W, WIN_H, 1.0f, 0.1f, 0.1f, a);
    }

    glutSwapBuffers();
}

// Runs about every 16 ms (~60 FPS): advances timers and animations
void tick(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now - lastTicks) / 1000.0f;
    lastTicks = now;
    if (dt > 0.1f) dt = 0.1f;

    animTime  += dt;
    starDrift += 15.0f * dt;
    if (flashTimer > 0.0f) flashTimer -= dt;

    for (int i = 0; i < 4; i++) {                       // clouds drift right and wrap
        cloudX[i] += (12.0f + i * 5.0f) * dt;
        if (cloudX[i] > WIN_W + 100) cloudX[i] = -150;
    }

    if (state == PLAYING && !answered) {                // countdown
        timeLeft -= dt;
        if (timeLeft <= 0.0f) { timeLeft = 0.0f; submitAnswer(-1); }
    }

    glutPostRedisplay();
    glutTimerFunc(16, tick, 0);
}

void keyboard(unsigned char key, int, int) {
    if (key == 27) exit(0);                                       // ESC
    if (key == 'r' || key == 'R') { resetGame(); return; }        // restart

    if (key == 13) {                                              // ENTER
        if (state == MENU)               startLevel(0);
        else if (state == PLAYING && answered) continueAfterAnswer();
        else if (state == LEVEL_COMPLETE) {
            if (currentLevel == 0) startLevel(1);
            else                   state = GAME_COMPLETE;
        }
        else if (state == GAME_COMPLETE) resetGame();
        return;
    }

    if (state == PLAYING) {                                       // answer keys
        int opt = -1;
        if (key >= '1' && key <= '4') opt = key - '1';
        if (key >= 'a' && key <= 'd') opt = key - 'a';
        if (key >= 'A' && key <= 'D') opt = key - 'A';
        if (opt >= 0) submitAnswer(opt);
    }
}

void reshape(int w, int h) { glViewport(0, 0, w, h); }

void initGL() {
    glClearColor(0, 0, 0, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIN_W, 0, WIN_H, -1, 1);           // 2D coordinates = pixels
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_BLEND);                           // needed for transparency (shadows, flash)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    // Shrink the window (keeping the 3:2 shape) if the screen is too short for
    // 800 px; the reshape callback + fixed ortho projection scale the drawing.
    int winH = WIN_H;
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    if (screenH > 0 && screenH - 120 < winH) winH = screenH - 120;
    glutInitWindowSize(winH * WIN_W / WIN_H, winH);
    glutInitWindowPosition(40, 20);
    glutCreateWindow("Cyber Security Puzzle Quiz - Qutubkhan Nalwala");
    initGL();
    resetGame();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    lastTicks = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, tick, 0);
    glutMainLoop();
    return 0;
}
