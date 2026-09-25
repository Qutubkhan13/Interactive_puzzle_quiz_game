# Cyber Security Puzzle Quiz (OpenGL / FreeGLUT)

Author: Qutubkhan Nalwala

## Files
- `main.cpp` - the whole game (drawing helpers, scenes, puzzles, questions, logic)
- `build.bat` - compiles `main.cpp` to `game.exe`
- `run.bat` - runs `game.exe` (adds the FreeGLUT DLL folder to PATH)

## Setup (Windows 11, MSYS2 UCRT64 + g++)
1. Install MSYS2 from https://www.msys2.org/ (default folder `C:\msys64`).
2. Open the **MSYS2 UCRT64** terminal and install the compiler and FreeGLUT:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
   ```
3. Compile and run from the project folder (cmd or PowerShell):
   ```cmd
   build.bat
   run.bat
   ```
   The exact compile command inside `build.bat` is:
   ```cmd
   set PATH=C:\msys64\ucrt64\bin;%PATH%
   g++ main.cpp -o game.exe -IC:\msys64\ucrt64\include -LC:\msys64\ucrt64\lib -lfreeglut -lopengl32 -lglu32
   ```
   `game.exe` needs `freeglut.dll`, found in `C:\msys64\ucrt64\bin` (that is why `run.bat` sets PATH).

## Controls
`1-4` or `A-D` answer | `ENTER` continue | `R` restart | `ESC` quit

## Design Logic and Game Concept Explanation
The game uses a cybersecurity theme. The player is rebuilding a broken digital defence: a golden padlock in Level 1 and a protective shield in Level 2. Both pictures are drawn only from OpenGL primitives (quads, triangles, polygons, lines and circles built from triangle fans), with no images or textures.

The questions are multiple choice with four options each. They cover everyday cyber safety: strong passwords, HTTPS, phishing, two-factor authentication, malware, firewalls, software updates, public Wi-Fi, ransomware and social engineering. Each level has five questions.

Puzzle progression is simple. Each level starts with only dashed outlines of its five pieces. Every correct answer sets one flag in the `solved[]` array, and the drawing code then fills in that piece, so the picture builds up gradually. A wrong answer or a timeout reveals nothing, and that question returns later until it is answered correctly. When all five pieces are placed, a level-complete screen appears, then Level 2 starts with a new night-time background, and finally a mission-complete screen shows the score.

The scene creates a game-like feel through animation and feedback: moving clouds, a pulsing sun, twinkling drifting stars, a glowing neon line with moving data packets, a shrinking countdown bar, a score that rewards fast answers, and green or red flashes on correct and wrong answers. The padlock looks three-dimensional because it is drawn as a box with lit top, front and shaded side faces, plus a ground shadow. The shield uses a drop shadow, layered rim and a light facet. The author's name is engraved on a wooden sign hanging in the scene.

## Requirement checklist (main.cpp)
| # | Requirement | Where |
|---|---|---|
| 1 | Background scene with decorations | `drawDayScene()`, `drawNightScene()`, `drawBackground()`, helpers `drawCloud()`, `drawBuilding()` |
| 2 | Puzzle starts incomplete, one piece per correct answer | `solved[]`, `drawPadlock()`, `drawShield()`, `drawPuzzle()`, ghost outlines `ghostPolygon()`, `submitAnswer()` |
| 3 | 5+ multiple-choice questions, correct/wrong handling, feedback | `struct Question`, `questions[]`, `drawQuestion()`, `submitAnswer()`, `findNextUnsolved()` |
| 4 | Keyboard-only input | `keyboard()` (1-4, A-D, ENTER, R, ESC) |
| 5 | Level 2 with new background + completion screen | `GameState` enum, `startLevel()`, `drawNightScene()`, `drawLevelCompletePanel()`, `drawFinalPanel()` |
| 6 | 3D-looking object | Padlock box faces + shadow in `drawPadlock()`; drop shadow and facet in `drawShield()` |
| 7 | Name inside the scene | `drawNamePlaque()` (engraved text on a hanging sign, shown on every screen) |
| Bonus | Animation | `tick()` (timer-driven), `animTime`, `cloudX[]`, `starDrift` |
| Bonus | Score and countdown per question | `score`, `timeLeft`, timer bar in `drawQuestion()` |
| Bonus | Green/red flash | `flashTimer`, overlay at end of `display()`, button colours in `drawQuestion()` |

## Notes
The project uses legacy fixed-function OpenGL (`glBegin`/`glEnd`) with FreeGLUT, as required. If the screen is shorter than 800 px the window shrinks automatically and the drawing scales with it.
