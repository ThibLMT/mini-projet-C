/*******************************************************************************************
*
*   raylib - sample game: gorilas
*
*   Sample game Marc Palau and Ramon Santamaria
*
*   This game has been created using raylib v1.3 (www.raylib.com)
*   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
*
*   Copyright (c) 2015 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define MAX_BUILDINGS                    15
#define MAX_EXPLOSIONS                  200
#define MAX_PLAYERS                       2

#define BUILDING_RELATIVE_ERROR          30        // Building size random range %
#define BUILDING_MIN_RELATIVE_HEIGHT     20        // Minimum height in % of the screenHeight
#define BUILDING_MAX_RELATIVE_HEIGHT     60        // Maximum height in % of the screenHeight
#define BUILDING_MIN_GREENSCALE_COLOR    120        // Minimum gray color for the buildings
#define BUILDING_MAX_GREENSCALE_COLOR    200        // Maximum gray color for the buildings

#define MIN_PLAYER_POSITION               5        // Minimum x position %
#define MAX_PLAYER_POSITION              20        // Maximum x position %

#define GRAVITY                       9.81f
#define DELTA_FPS                        60
#define LIVES                             3

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    Vector2 position;
    Vector2 size;

    Vector2 aimingPoint;
    int aimingAngle;
    int aimingPower;

    Vector2 previousPoint;
    int previousAngle;
    int previousPower;

    Vector2 impactPoint;

    bool isLeftTeam;                // This player belongs to the left or to the right team
    bool isPlayer;                  // If is a player or an AI
    bool isAlive;
    int lives;

    Texture2D chassis;


} Player;

int frameWidth;
int frameHeight;

typedef struct Building {
    Rectangle rectangle;
    Color color;
} Building;

typedef struct Explosion {
    Vector2 position;
    int radius;
    bool active;
} Explosion;

typedef struct Shell {
    Vector2 position;
    Vector2 speed;
    int radius;
    bool active;
    double rotation;
    int length;
    Rectangle rectangle;
    Sound fxBoom;
} Shell;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 1280;
static const int screenHeight = 720;

static bool gameOver = false;
static bool pause = false;

static Player player[MAX_PLAYERS] = { 0 };
static Building building[MAX_BUILDINGS] = { 0 };
static Explosion explosion[MAX_EXPLOSIONS] = { 0 };
static Shell shell = { 0 };

static int playerTurn = 0;
static bool shellOnAir = false;
static bool redemarrage = false;
static int winner = 0;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);         // Initialize game
static void InitLives(void);
static void UpdateGame(void);       // Update game (one frame)
static void DrawGame(void);         // Draw game (one frame)
static void UnloadGame(void);       // Unload game
static void UpdateDrawFrame(void);  // Update and Draw (one frame)

// Additional module functions
static void InitBuildings(void);
static void InitPlayers(void);
static bool UpdatePlayer(int playerTurn);
static bool UpdateShell(int playerTurn);

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Tank Destroyer");

    InitLives();

    InitGame();

    InitAudioDevice();                                            //Initialize audio device

    Music music = LoadMusicStream("resources/soundtrack.mp3");    //Load la soundtrack

    SetMasterVolume(0.1);                                        //On baisse le volume sinon ça pique les oreilles

    music.loopCount = 0;                                         //La musique jouera indéfiniment

    PlayMusicStream(music);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();

        UpdateMusicStream(music);
        //----------------------------------------------------------------------------------
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame();         // Unload loaded data (textures, sounds, models...)


    UnloadMusicStream(music);

    UnloadSound(shell.fxBoom);

    CloseAudioDevice();

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // Init shoot
    shell.radius = 10;
    shell.rotation = 0.0;
    shell.length = 15;
    shell.rectangle.height = 5;
    shell.rectangle.width = 10;
    shell.rectangle.x = 0;
    shell.rectangle.y = 0;
    shellOnAir = false;
    shell.active = false;
    shell.fxBoom = LoadSound("resources/boom.wav");  

    redemarrage = false;

    InitBuildings();
    InitPlayers();
    
    // Init explosions
    for (int i = 0; i < MAX_EXPLOSIONS; i++)
    {
        explosion[i].position = (Vector2){ 0.0f, 0.0f };
        explosion[i].radius = 30;
        explosion[i].active = false;
    }
}

//Initialize the player lives
void InitLives(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        player[i].lives = LIVES;
    }
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver)
    {
        if (IsKeyPressed('P')) pause = !pause;

        if (IsKeyPressed('R')) InitGame();  // Refresh the map in case it's too hard to touch the enemy tank

        if (!pause)
        {
            if (!shellOnAir) shellOnAir = UpdatePlayer(playerTurn); // If we are aiming
            else
            {
                if (UpdateShell(playerTurn))                       // If collision
                {
                    // Game over logic
                    bool leftTeamAlive = false;
                    bool rightTeamAlive = false;

                    for (int i = 0; i < MAX_PLAYERS; i++)
                    {
                        if (!player[i].isAlive && player[i].lives >0)
                        {
                            player[i].isAlive = true;
                            InitGame();
                            
                        }
                        if (player[i].isAlive)
                        {
                            if (player[i].isLeftTeam) leftTeamAlive = true;
                            if (!player[i].isLeftTeam) rightTeamAlive = true;
                        }
                        
                    }
                    if (leftTeamAlive && rightTeamAlive)
                    {
                        shellOnAir = false;
                        shell.active = false;

                        playerTurn++;

                        if (playerTurn == MAX_PLAYERS) playerTurn = 0;
                    }
                    else
                    {
                        gameOver = true;
                        
                        if (leftTeamAlive) winner = 1;
                        if (rightTeamAlive) winner = 2;
                    }
                }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitLives();
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

        ClearBackground(RAYWHITE);

        if (!gameOver)
        {
            //Draw background
            DrawRectangle(0,0, screenWidth, screenHeight, SKYBLUE);

            DrawText(TextFormat("%i LIVES",player[0].lives), screenWidth /2 - 100, 20 , 20,DARKBLUE);
            DrawText("---", screenWidth /2, 20 , 20,BLACK);
            DrawText(TextFormat("%i LIVES ",player[1].lives), screenWidth /2 + 50, 20 , 20, RED);

            // Draw buildings
            for (int i = 0; i < MAX_BUILDINGS; i++) DrawRectangleRec(building[i].rectangle, building[i].color);

            // Draw explosions
            for (int i = 0; i < MAX_EXPLOSIONS; i++)
            {
                if (explosion[i].active) DrawCircle(explosion[i].position.x, explosion[i].position.y, explosion[i].radius, SKYBLUE);
            }
            
            // Draw players
            for (int i = 0; i < MAX_PLAYERS; i++)
            {
                if (player[i].isAlive)
                {
                    if (player[i].isLeftTeam){
                        DrawTexture(player[i].chassis, player[i].position.x - player[i].size.x/2, player[i].position.y + player[i].size.y/2 -player[i].chassis.height , RAYWHITE );              
                    }
                    else {
                        DrawTexture(player[i].chassis, player[i].position.x - player[i].size.x/2, player[i].position.y + player[i].size.y/2 -player[i].chassis.height , RAYWHITE );
                    }
                }
            }

            // Draw shell
            if (shell.active){
                    DrawRectanglePro(shell.rectangle, (Vector2){0,0},shell.rotation, MAROON);
            } 


            // Draw the angle and the power of the aim, and the previous ones
            if (!shellOnAir)
            {
                // Draw aim
                if (player[playerTurn].isLeftTeam)
                {
                    // Previous aiming
                    DrawLineEx((Vector2){ player[playerTurn].position.x, player[playerTurn].position.y },
                               player[playerTurn].previousPoint,2 , GRAY);


                    // Actual aiming
                    DrawLineEx((Vector2){ player[playerTurn].position.x, player[playerTurn].position.y },
                               player[playerTurn].aimingPoint,2 , DARKBLUE);
                }
                else
                {
                    // Previous aiming
                    DrawLineEx((Vector2){ player[playerTurn].position.x, player[playerTurn].position.y },
                               player[playerTurn].previousPoint,2 , GRAY);

                    // Actual aiming
                    DrawLineEx((Vector2){ player[playerTurn].position.x, player[playerTurn].position.y },
                               player[playerTurn].aimingPoint,2 , MAROON);
                }
            }

            if (pause) DrawText("GAME PAUSED", screenWidth/2 - MeasureText("GAME PAUSED", 40)/2, screenHeight/2 - 40, 40, GRAY);


        }
        else 
        {
            DrawText("PRESS [ENTER] TO PLAY AGAIN", GetScreenWidth()/2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20)/2, GetScreenHeight()/2 - 50, 20, GRAY);
            if (winner == 1)
            {
                DrawText("BLUE TEAM WINS",GetScreenWidth()/2 - MeasureText("BLUE TEAM WINS", 20)/2, 40, 20, BLUE );
            }
            else {
                DrawText("RED TEAM WINS",GetScreenWidth()/2 - MeasureText("RED TEAM WINS", 20)/2, 40, 20, RED );
            }
        }
            

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
   for (int i = 0; i < MAX_PLAYERS; i++)
   {
       UnloadTexture(player[i].chassis);
   }

}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}

//--------------------------------------------------------------------------------------
// Additional module functions
//--------------------------------------------------------------------------------------
static void InitBuildings(void)
{
    // Horizontal generation
    int currentWidth = 0;

    // We make sure the absolute error randomly generated for each building, has as a minimum value the screenWidth.
    // This way all the screen will be filled with buildings. Each building will have a different, random width.

    float relativeWidth = 100/(100 - BUILDING_RELATIVE_ERROR);
    float buildingWidthMean = (screenWidth*relativeWidth/MAX_BUILDINGS) + 1;        // We add one to make sure we will cover the whole screen.

    // Vertical generation
    int currentHeighth = 0;
    int greenLevel;

    // Creation
    for (int i = 0; i < MAX_BUILDINGS; i++)
    {
        // Horizontal
        building[i].rectangle.x = currentWidth;
        building[i].rectangle.width = GetRandomValue(buildingWidthMean*(100 - BUILDING_RELATIVE_ERROR/2)/100 + 1, buildingWidthMean*(100 + BUILDING_RELATIVE_ERROR)/100);

        currentWidth += building[i].rectangle.width;

        // Vertical
        currentHeighth = GetRandomValue(BUILDING_MIN_RELATIVE_HEIGHT, BUILDING_MAX_RELATIVE_HEIGHT);
        building[i].rectangle.y = screenHeight - (screenHeight*currentHeighth/100);
        building[i].rectangle.height = screenHeight*currentHeighth/100 + 1;

        // Color
        greenLevel = GetRandomValue(BUILDING_MIN_GREENSCALE_COLOR, BUILDING_MAX_GREENSCALE_COLOR);
        building[i].color = (Color){ 0, greenLevel, greenLevel/4, 255 };
    }
}

static void InitPlayers(void)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        player[i].isAlive = true;
        // Decide the team of this player
        if (i % 2 == 0) {
            player[i].isLeftTeam = true;
            player[i].chassis = LoadTexture("C:/MLOD/Projets/mini-projet-language-c/resources/TankBleu.png");
            }
        else {
            player[i].isLeftTeam = false;
            player[i].chassis = LoadTexture("C:/MLOD/Projets/mini-projet-language-c/resources/TankRouge.png");
            }


        // Now there is no AI
        player[i].isPlayer = true;

        // Set size, by default by now
        player[i].size = (Vector2){ 50, 50 };

        // Set position
        if (player[i].isLeftTeam) player[i].position.x = GetRandomValue(screenWidth*MIN_PLAYER_POSITION/100, screenWidth*MAX_PLAYER_POSITION/100);
        else player[i].position.x = screenWidth - GetRandomValue(screenWidth*MIN_PLAYER_POSITION/100, screenWidth*MAX_PLAYER_POSITION/100);

        for (int j = 0; j < MAX_BUILDINGS; j++)
        {
            if (building[j].rectangle.x > player[i].position.x)
            {
                // Set the player in the center of the building
                player[i].position.x = building[j-1].rectangle.x + building[j-1].rectangle.width/2;
                // Set the player at the top of the building
                player[i].position.y = building[j-1].rectangle.y - player[i].size.y/2;
                break;
            }
        }

        // Set statistics to 0
        player[i].aimingPoint = player[i].position;
        player[i].previousAngle = 0;
        player[i].previousPower = 0;
        player[i].previousPoint = player[i].position;
        player[i].aimingAngle = 0;
        player[i].aimingPower = 0;

        player[i].impactPoint = (Vector2){ -100, -100 };
    }
}

static bool UpdatePlayer(int playerTurn)
{
    // If we are aiming at the firing quadrant, we calculate the angle
    if (GetMousePosition().y <= player[playerTurn].position.y)
    {
        // Left team
        if (player[playerTurn].isLeftTeam && GetMousePosition().x >= player[playerTurn].position.x)
        {
            // Distance (calculating the fire power)
            player[playerTurn].aimingPower = sqrt(pow(player[playerTurn].position.x - GetMousePosition().x, 2) + pow(player[playerTurn].position.y - GetMousePosition().y, 2));
            // Calculates the angle via arcsin
            player[playerTurn].aimingAngle = asin((player[playerTurn].position.y - GetMousePosition().y)/player[playerTurn].aimingPower)*RAD2DEG;
            // Point of the screen we are aiming at
            player[playerTurn].aimingPoint = GetMousePosition();

            // Shell fired
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                player[playerTurn].previousPoint = player[playerTurn].aimingPoint;
                player[playerTurn].previousPower = player[playerTurn].aimingPower;
                player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
                shell.position = player[playerTurn].position;
                shell.rotation = player[playerTurn].aimingAngle;

                return true;
            }
        }
        // Right team
        else if (!player[playerTurn].isLeftTeam && GetMousePosition().x <= player[playerTurn].position.x)
        {
            // Distance (calculating the fire power)
            player[playerTurn].aimingPower = sqrt(pow(player[playerTurn].position.x - GetMousePosition().x, 2) + pow(player[playerTurn].position.y - GetMousePosition().y, 2));
            // Calculates the angle via arcsin
            player[playerTurn].aimingAngle = asin((player[playerTurn].position.y - GetMousePosition().y)/player[playerTurn].aimingPower)*RAD2DEG;
            // Point of the screen we are aiming at
            player[playerTurn].aimingPoint = GetMousePosition();

            // Shell fired
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                player[playerTurn].previousPoint = player[playerTurn].aimingPoint;
                player[playerTurn].previousPower = player[playerTurn].aimingPower;
                player[playerTurn].previousAngle = player[playerTurn].aimingAngle;
                shell.position = player[playerTurn].position;
                shell.rectangle.x = shell.position.x - shell.rectangle.width/2;
                shell.rectangle.y = shell.position.y - shell.rectangle.height/2;
                shell.rotation = player[playerTurn].aimingAngle;

                return true;
            }
        }
        else
        {
            player[playerTurn].aimingPoint = player[playerTurn].position;
            player[playerTurn].aimingPower = 0;
            player[playerTurn].aimingAngle = 0;
        }
    }
    else
    {
        player[playerTurn].aimingPoint = player[playerTurn].position;
        player[playerTurn].aimingPower = 0;
        player[playerTurn].aimingAngle = 0;
    }

    return false;
}

static bool UpdateShell(int playerTurn)
{
    static int explosionNumber = 0;

    // Activate shell
    if (!shell.active)
    {
        if (player[playerTurn].isLeftTeam)
        {
            shell.speed.x = cos(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            shell.speed.y = -sin(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            shell.active = true;
        }
        else
        {
            shell.speed.x = -cos(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            shell.speed.y = -sin(player[playerTurn].previousAngle*DEG2RAD)*player[playerTurn].previousPower*3/DELTA_FPS;
            shell.active = true;
        }
    }

    shell.position.x += shell.speed.x;
    shell.position.y += shell.speed.y;
    shell.rectangle.x = shell.position.x - shell.rectangle.width/2;
    shell.rectangle.y = shell.position.y - shell.rectangle.height/2;
    shell.speed.y += GRAVITY/DELTA_FPS;
    shell.rotation = atan2(shell.speed.y, shell.speed.x)*RAD2DEG;

    // Collision
    if (shell.position.x + shell.radius < 0) return true;                        // These two first cases are when the shell goes out of the window, either on the left or the right
    else if (shell.position.x - shell.radius > screenWidth) return true;
    else
    {
        // Player collision
      for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if (CheckCollisionCircleRec(shell.position, shell.radius,  (Rectangle){ player[i].position.x - player[i].size.x/2, player[i].position.y + player[i].size.y/2 - player[i].chassis.height ,
                                                                                  player[i].size.x, player[i].chassis.height }))
            {
                // We can't hit ourselves
                if (i == playerTurn) return false;
                else
                {
                    // We set the impact point
                    player[playerTurn].impactPoint.x = shell.position.x;
                    player[playerTurn].impactPoint.y = shell.position.y + shell.radius;

                    player[i].lives --;
                    player[i].isAlive = false;
                    return true;
                }
            }
        }

        // Building collision
        // NOTE: We only check building collision if we are not inside an explosion
        for (int i = 0; i < MAX_BUILDINGS; i++)
        {
            if (CheckCollisionCircles(shell.position, shell.radius, explosion[i].position, explosion[i].radius - shell.radius))
            {
                return false;
            }
        }

        for (int i = 0; i < MAX_BUILDINGS; i++)
        {
            if (CheckCollisionCircleRec(shell.position, shell.radius, building[i].rectangle))
            {
                // We set the impact point
                player[playerTurn].impactPoint.x = shell.position.x;
                player[playerTurn].impactPoint.y = shell.position.y + shell.radius;

                // We create an explosion
                explosion[explosionNumber].position = player[playerTurn].impactPoint;
                explosion[explosionNumber].active = true;
                explosionNumber++;

                PlaySound(shell.fxBoom);

                return true;
            }
        }
    }

    return false;
}