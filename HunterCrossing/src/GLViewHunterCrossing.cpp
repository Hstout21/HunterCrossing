#include "GLViewHunterCrossing.h"
#include "WorldList.h"
#include "ManagerOpenGLState.h"
#include "Axes.h" 
#include "PhysicsEngineODE.h"
#include "WO.h"
#include "WOStatic.h"
#include "WOStaticPlane.h"
#include "WOStaticTrimesh.h"
#include "WOTrimesh.h"
#include "WOHumanCyborg.h"
#include "WOHumanCal3DPaladin.h"
#include "WOWayPointSpherical.h"
#include "WOLight.h"
#include "WOSkyBox.h"
#include "WOCar1970sBeater.h"
#include "Camera.h"
#include "CameraStandard.h"
#include "CameraChaseActorSmooth.h"
#include "CameraChaseActorAbsNormal.h"
#include "CameraChaseActorRelNormal.h"
#include "Model.h"
#include "ModelDataShared.h"
#include "ModelMesh.h"
#include "ModelMeshDataShared.h"
#include "ModelMeshSkin.h"
#include "WONVStaticPlane.h"
#include "WONVPhysX.h"
#include "WONVDynSphere.h"
#include "WOImGui.h"
#include "AftrImGuiIncludes.h"
#include "AftrGLRendererBase.h"
using namespace Aftr;
using namespace irrklang;

GLViewHunterCrossing* GLViewHunterCrossing::New( const std::vector< std::string >& args )
{
   GLViewHunterCrossing* glv = new GLViewHunterCrossing( args );
   glv->init( Aftr::GRAVITY, Vector( 0, 0, -1.0f ), "aftr.conf", PHYSICS_ENGINE_TYPE::petODE );
   glv->onCreate();
   return glv;
}

GLViewHunterCrossing::GLViewHunterCrossing( const std::vector< std::string >& args ) : GLView( args )
{
    #pragma region Init
        debugMode = true;
        isGameReady = false;

        isBackgroundMusicOn = true;
        engine = createIrrKlangDevice();
        backgroundSound = engine->play2D((ManagerEnvironmentConfiguration::getLMM() + "sounds/music.wav").c_str(), true, true, true);

        isCameraOnPlayer = true;
        cameraZoom = 10;

        //Developer Tools.
        developerListIndex = 0;
        oldDeveloperListIndex = 0;
        currentDeveloperIndexSize = 1;
        currentDeveloperIndexLocation = Vector(0, 0, 10);

        harvestSpeed = 2;
        harvestRange = 3;
        isHarvestableObjectInRange = false;
        axeMarkerHeight = 0;
        islandScore = 0;

        stonePerRock = 15;
        woodPerTree = 10;

        hideDefaultGuis = false;
        isNearTomNook = false;
        isNearNooksCranny = false;
        isNearNpcOne = false;
        isNearNpcTwo = false;
        isNearHome = false;
        isDecorGuiActive = false;

        npcOneLocation = 0;
        npcTwoLocation = 0;
        homeLocation = 0;
        axeTier = 0;

        activeDecorIndex = 0;
        oldActiveDecorIndex = 0;
        inventoryDecorIndex = 0;
        activeDecorIndexLocation = Vector(0, 0, 10);
    #pragma endregion
}

void GLViewHunterCrossing::onCreate() {
   if( this->pe != NULL )
   {
      this->pe->setGravityNormalizedVector( Vector( 0,0,-1.0f ) );
      this->pe->setGravityScalar( Aftr::GRAVITY );
   }  this->setActorChaseType( STANDARDEZNAV );
}

void GLViewHunterCrossing::updateWorld()
{
   GLView::updateWorld();

   //Debug.
   if (debugMode && developerList.size() > 0) {
       updateDeveloperIndexGuiValues();
       updateDeveloperWoLocation();
   }

   //Update World Functions.
   updatePlayerMarker();
   markObjectHarvestable();
   tickHarvestCooldown();
   checkObjectHealth();
   checkObjectCooldown();
   checkTomNook();
   checkNooksCranny();
   checkNpcOne();
   checkNpcTwo();
   checkHome();
   checkToHideDefaultGuis();
   checkIslandScore();

   if (activeDecor.size() > 0) {
       updateDecorIndex();
       updateDecorLocation();
   }

   if (backgroundSound->getIsPaused() && isBackgroundMusicOn) {
       backgroundSound->setIsPaused(false);
   }
   else if (!backgroundSound->getIsPaused() && !isBackgroundMusicOn) {
       backgroundSound->setIsPaused(true);
   }
}

void GLViewHunterCrossing::onKeyDown( const SDL_KeyboardEvent& key )
{
   GLView::onKeyDown( key );
   if( key.keysym.sym == SDLK_0 )
      this->setNumPhysicsStepsPerRender( 1 );

   if (isGameReady) {
       if (key.keysym.sym == SDLK_w) { if (isPlayerInBounds("N")) { movePlayer("N"); } }
       if (key.keysym.sym == SDLK_s) { if (isPlayerInBounds("S")) { movePlayer("S"); } }
       if (key.keysym.sym == SDLK_a) { if (isPlayerInBounds("W")) { movePlayer("W"); } }
       if (key.keysym.sym == SDLK_d) { if (isPlayerInBounds("E")) { movePlayer("E"); } }
       if (key.keysym.sym == SDLK_e) { if (isHarvestableObjectInRange) { tickHarvestableObjectHealth(); axeMarkerHeight = player.currentMarkerHeight; } }
   }
}

void GLViewHunterCrossing::onKeyUp(const SDL_KeyboardEvent& key)
{ 
    GLView::onKeyUp(key); 
    if (key.keysym.sym == SDLK_e) { axeMarkerHeight = 0; }
}

void Aftr::GLViewHunterCrossing::loadMap()
{
   this->worldLst = new WorldList();
   this->actorLst = new WorldList();
   this->netLst = new WorldList();

   ManagerOpenGLState::GL_CLIPPING_PLANE = 1000.0;
   ManagerOpenGLState::GL_NEAR_PLANE = 0.1f;
   ManagerOpenGLState::enableFrustumCulling = false;

   Axes::isVisible = false;
   this->glRenderer->isUsingShadowMapping( false );
   this->cam->setPosition( 15,15,10 );
   
   std::vector< std::string > skyBoxImageNames;
   skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_mountains+6.jpg" );
   {
       WO* wo = WOSkyBox::New(skyBoxImageNames.at(0), this->getCameraPtrPtr());
       wo->setPosition(Vector(0, 0, 0));
       wo->setLabel("Sky Box");
       wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
       worldLst->push_back(wo);
   }
   axe = createWoObject("", ManagerEnvironmentConfiguration::getLMM() + "/models/axe/scene.gltf", 0.005, Vector(5, 5, -40), 0.0f).obj;

   //Setup world.
   createPlayer(Vector(-5, 0, 4.2), "W");
   createWoBorder();
   createEmptyPlot(1);
   createEmptyPlot(2);
   createEmptyPlot(3);
   createMap();
   createTrees();
   createRocks();

   //SQL.
   checkTables();
   currentGameSaves = getSaveFileIds();
   if (currentGameSaves[0] + currentGameSaves[1] + currentGameSaves[2] == 0) { isGameReady = true; }

   WOImGui* gui = WOImGui::New(nullptr);
   gui->subscribe_drawImGuiWidget(
        [this, gui]()
        {
           //New Game Gui.
            #pragma region NewGameGui

                if (!isGameReady) {

                    ImGui::Begin("Welcome to Hunter Crossing!");
                    ImGui::Text("Load Game Save");
                    if (currentGameSaves[0] == 1) { if (ImGui::Button("Save 1")) { loadSaveFile(1); isGameReady = true; } }
                    if (currentGameSaves[1] == 1) { if (ImGui::Button("Save 2")) { loadSaveFile(2); isGameReady = true; } }
                    if (currentGameSaves[2] == 1) { if (ImGui::Button("Save 3")) { loadSaveFile(3); isGameReady = true; } }
                    ImGui::Spacing();
                    if (ImGui::Button("Start New Game")) { isGameReady = true; }
                    ImGui::End();

                    ImGui::Begin("Instructions");
                    ImGui::Text("Player Movement: W, A, S, D");
                    ImGui::Text("Harvest: E");
                    ImGui::End();
                }
            #pragma endregion

            //Default Gui.
            #pragma region DefaultGui

            if (!hideDefaultGuis) {
                ImGui::Begin("Player Menu");
                ImGui::Text(("X: " + std::to_string(player.currentPosition[0]) + " Y: " + std::to_string(player.currentPosition[1])).c_str());
                ImGui::Spacing();
                ImGui::Text(("Island Score: " + std::to_string(islandScore)).c_str());
                ImGui::Text(("Money: $" + std::to_string(inventory.money)).c_str());
                ImGui::Text(("Wood: " + std::to_string(inventory.wood)).c_str());
                ImGui::Text(("Stone: " + std::to_string(inventory.stone)).c_str());
                ImGui::Spacing();
                if (activeDecor.size() > 0 || inventory.decor.size() > 0) {
                    if (ImGui::Button("Open Decor Editor")) { isDecorGuiActive = true; }
                    ImGui::Spacing();
                }
                ImGui::Spacing();
                ImGui::Checkbox("Fixed Camera", &isCameraOnPlayer);
                ImGui::Checkbox("Background Music", &isBackgroundMusicOn);
                ImGui::End();
            }
            #pragma endregion

            //Decor Editor Gui.
            #pragma region DecorGui

            if (isDecorGuiActive) {
                ImGui::Begin("Decor Editor");

                if (inventory.decor.size() > 0) {
                    ImGui::Text("Decor Inventory:");
                    ImGui::Text(("Current Selection: " + inventory.decor[inventoryDecorIndex].name).c_str());
                    ImGui::SliderInt("Scroll", &inventoryDecorIndex, 0, inventory.decor.size() - 1);
                    if (ImGui::Button("Place Object")) { placeDecorIndexIntoWorld(); }
                    ImGui::Spacing(); ImGui::Spacing();
                }

                if (activeDecor.size() > 0) {
                    ImGui::Text("Active Decor:");
                    ImGui::Text(("Current Selection: " + activeDecor[activeDecorIndex].title).c_str());
                    ImGui::SliderInt("Scroll", &activeDecorIndex, 0, activeDecor.size() - 1);
                    ImGui::SliderFloat("X", &activeDecorIndexLocation[0], -7, 20);
                    ImGui::SliderFloat("Y", &activeDecorIndexLocation[1], -42, 12);
                    if (ImGui::Button("Rotate")) { updateDecorRotation(); }
                    if (ImGui::Button("Pick Up")) { stashDecorIndexIntoInventory(); }
                    ImGui::Spacing(); ImGui::Spacing();
                }
                if (ImGui::Button("Close Editor")) { isDecorGuiActive = false; }
                ImGui::End();
            }
            #pragma endregion

            //Debug Gui.
            #pragma region DebugGui

            if (debugMode) {
                ImGui::Begin("Admin / Debug");
                if (developerList.size() > 0) {

                    ImGui::SliderInt("Index", &developerListIndex, 0, developerList.size() - 1);

                    ImGui::Spacing(); ImGui::Spacing();

                    ImGui::SliderFloat("Size", &currentDeveloperIndexSize, 0.0005, 10);
                    ImGui::SliderFloat("Small Size", &currentDeveloperIndexSize, 0.0005, 0.5);

                    if (ImGui::Button("Update Size")) { updateDeveloperWoSize(); }

                    ImGui::Spacing(); ImGui::Spacing();

                    ImGui::SliderFloat("X", &currentDeveloperIndexLocation[0], -50, 50);
                    ImGui::SliderFloat("Y", &currentDeveloperIndexLocation[1], -50, 50);
                    ImGui::SliderFloat("Z", &currentDeveloperIndexLocation[2], -10, 10);

                    ImGui::Spacing(); ImGui::Spacing();

                    if (ImGui::Button("Rotate")) {
                        updateDeveloperWoRotation();
                    }
                    ImGui::Spacing(); ImGui::Spacing();
                }

                if (ImGui::Button("Add Money")) {
                    inventory.money += 50000;
                }

                if (ImGui::Button("Add Mats")) {
                    inventory.wood += 100;
                    inventory.stone += 50;
                }

                ImGui::End();
            }
            #pragma endregion

            //NPC Guis.
            #pragma region NPCGuis

            //TOM NOOK GUI.
            if (isNearTomNook) {
                ImGui::Begin("Tom Nook");

                if (homeLocation == 0) {
                    ImGui::Text("Hello! Would you like help building a home?");
                    ImGui::Spacing();
                    ImGui::Text("Cost: $80,000, 200 wood, 100 stone.");

                    if (inventory.money >= 80000 && inventory.wood >= 200 && inventory.stone >= 100) {
                        if (ImGui::Button("Purchase, Plot 1")) { clearPlot(1); createPlot(1, 1); adjustInventoryAfterPlotPurchase(1); }
                        if (ImGui::Button("Purchase, Plot 2")) { clearPlot(2); createPlot(2, 1); adjustInventoryAfterPlotPurchase(1); }
                        if (ImGui::Button("Purchase, Plot 3")) { clearPlot(3); createPlot(3, 1); adjustInventoryAfterPlotPurchase(1); }
                    }
                }
                else if (npcOneLocation == 0) {
                    ImGui::Text("Hello! It appears that flappy would like to join your");
                    ImGui::Text("island, would you like to help build him a house?");
                    ImGui::Spacing();
                    ImGui::Text("Cost: $100,000, 400 wood, 200 stone.");

                    if (inventory.money >= 100000 && inventory.wood >= 400 && inventory.stone >= 200) {
                        if (homeLocation != 1) { if (ImGui::Button("Purchase, Plot 1")) { clearPlot(1); createPlot(1, 2); adjustInventoryAfterPlotPurchase(2); } }
                        if (homeLocation != 2) { if (ImGui::Button("Purchase, Plot 2")) { clearPlot(2); createPlot(2, 2); adjustInventoryAfterPlotPurchase(2); } }
                        if (homeLocation != 3) { if (ImGui::Button("Purchase, Plot 3")) { clearPlot(3); createPlot(3, 2); adjustInventoryAfterPlotPurchase(2); } }
                    }
                }
                else if (npcTwoLocation == 0) {
                    ImGui::Text("Hello! It appears that pickles would like to join your");
                    ImGui::Text("island, would you like to help build her a house?");
                    ImGui::Spacing();
                    ImGui::Text("Cost: $200,000, 1000 wood, 500 stone.");

                    if (inventory.money >= 200000 && inventory.wood >= 1000 && inventory.stone >= 500) {
                        if (homeLocation != 1 && npcOneLocation != 1) { if (ImGui::Button("Purchase, Plot 1")) { clearPlot(1); createPlot(1, 3); adjustInventoryAfterPlotPurchase(3); } }
                        if (homeLocation != 2 && npcOneLocation != 2) { if (ImGui::Button("Purchase, Plot 2")) { clearPlot(2); createPlot(2, 3); adjustInventoryAfterPlotPurchase(3); } }
                        if (homeLocation != 3 && npcOneLocation != 3) { if (ImGui::Button("Purchase, Plot 3")) { clearPlot(3); createPlot(3, 3); adjustInventoryAfterPlotPurchase(3); } }
                    }
                }
                else {
                    ImGui::Text("It looks like you filled up the open plots already!");
                    ImGui::Text("Maybe you should try purchasing items from Nook's Cranny instead.");
                }
                ImGui::End();
            }

            //NOOKS CRANNY GUI.
            if (isNearNooksCranny) {
                ImGui::Begin("Nooks Cranny");
                ImGui::Text("Welcome to Nook's Cranny!");
                ImGui::Spacing(); ImGui::Spacing();
                ImGui::Text(("Money: " + std::to_string(inventory.money)).c_str());
                ImGui::Text(("Wood: " + std::to_string(inventory.wood)).c_str());
                ImGui::Text(("Stone: " + std::to_string(inventory.stone)).c_str());
                ImGui::Spacing(); ImGui::Spacing();
                if (inventory.money >= 2000) {
                    ImGui::Text("Buy:");
                    ImGui::Spacing();
                    if (ImGui::Button("x1 Rose ($2000)")) { buyNooksCrannyItem(shopNameFlower); }
                    if (inventory.money >= 10000) { if (ImGui::Button("x1 Street Lamp ($10,000)")) { buyNooksCrannyItem(shopNameLamp); } }
                    if (inventory.money >= 15000) { if (ImGui::Button("x1 Basketball ($15,000)")) { buyNooksCrannyItem(shopNameBasketball); } }
                    if (inventory.money >= 50000) { if (ImGui::Button("x1 Playground ($50,000)")) { buyNooksCrannyItem(shopNamePlayground); } }
                    ImGui::Spacing(); ImGui::Spacing();
                }
                if (inventory.wood > 0 || inventory.stone > 0) {
                    ImGui::Text("Sell:");
                    ImGui::Spacing();
                }
                if (inventory.wood >= 100) {
                    if (ImGui::Button("Sell x100 Wood")) {
                        inventory.wood -= 100;
                        inventory.money += 1000;
                    }
                }
                if (inventory.stone >= 100) {
                    if (ImGui::Button("Sell x100 Stone")) {
                        inventory.stone -= 100;
                        inventory.money += 2000;
                    }
                }
                if (inventory.wood > 0) {
                    if (ImGui::Button("Sell All Wood")) {
                        inventory.money += (inventory.wood * 10);
                        inventory.wood = 0;
                    }
                }
                if (inventory.stone > 0) {
                    if (ImGui::Button("Sell All Stone")) {
                        inventory.money += (inventory.stone * 20);
                        inventory.stone = 0;
                    }
                }
                ImGui::End();
            }

            //NPC ONE GUI.
            if (isNearNpcOne) {
                ImGui::Begin("Flappy...");

                if (axeTier == 0) {
                    ImGui::Text("Hello! Thank you for letting me join your island! I'm a skilled");
                    ImGui::Text("blacksmith, maybe I could help you upgrade your tools!");
                    ImGui::Spacing();
                    ImGui::Text("Upgrade: x2 Mining Speed, & x1.5 Materials Per Harvest.");
                    ImGui::Text("Cost: Free!");
                    if (ImGui::Button("Upgrade")) {
                        axeTier += 1;
                        harvestSpeed *= 2;
                        woodPerTree *= 1.5;
                        stonePerRock *= 1.5;
                    }
                }
                else if (axeTier == 1) {
                    ImGui::Text("Hello! Need another upgrade?");
                    ImGui::Spacing();
                    ImGui::Text("Upgrade: x2 Mining Range, & x2 Materials Per Harvest.");
                    ImGui::Text("Cost: $50,000!");
                    if (inventory.money >= 50000) {
                        if (ImGui::Button("Upgrade")) {
                            axeTier += 1;
                            harvestRange *= 2;
                            woodPerTree *= 2;
                            stonePerRock *= 2;
                        }
                    }
                }
                else if (axeTier == 2) {
                    ImGui::Text("Hello! I could probably upgrade those tools one last time?");
                    ImGui::Spacing();
                    ImGui::Text("Upgrade: x3 Mining Speed, & x3 Materials Per Harvest.");
                    ImGui::Text("Cost: $100,000!");
                    if (inventory.money >= 100000) {
                        if (ImGui::Button("Upgrade")) {
                            axeTier += 1;
                            harvestSpeed *= 3;
                            woodPerTree *= 3;
                            stonePerRock *= 3;
                        }
                    }
                }
                else { ImGui::Text("Wow, the weather is amazing today!"); }
                ImGui::End();
            }

            //NPC TWO GUI.
            if (isNearNpcTwo) {
                ImGui::Begin("Pickles...");
                ImGui::Text("Hello! I have some artwork that will really contribute to your island's appearance!");
                ImGui::Text("Although, it is a little bit expensive.");
                if (inventory.money >= 200000) { if (ImGui::Button("x1 Snake Statue ($200,000)")) { buyNooksCrannyItem(shopNameStatue); } }
                ImGui::End();
            }

            //HOME GUI.
            if (isNearHome) {
                ImGui::Begin("Home :D");
                ImGui::Text("Save Game?");

                if (currentGameSaves[0] == 1) { if (ImGui::Button("Slot 1 (In-Use)")) { writeSaveFile(1); currentGameSaves = getSaveFileIds(); } }
                else { if (ImGui::Button("Slot 1 (Open)")) { writeSaveFile(1); currentGameSaves = getSaveFileIds(); } }

                if (currentGameSaves[1] == 1) { if (ImGui::Button("Slot 2 (In-Use)")) { writeSaveFile(2); currentGameSaves = getSaveFileIds(); } }
                else { if (ImGui::Button("Slot 2 (Open)")) { writeSaveFile(2); currentGameSaves = getSaveFileIds(); } }

                if (currentGameSaves[2] == 1) { if (ImGui::Button("Slot 3 (In-Use)")) { writeSaveFile(3); currentGameSaves = getSaveFileIds(); } }
                else { if (ImGui::Button("Slot 3 (Open)")) { writeSaveFile(3); currentGameSaves = getSaveFileIds(); } }

                ImGui::Spacing(); ImGui::Spacing();
                ImGui::Text("Delete Save File?");

                if (currentGameSaves[0] == 1) { if (ImGui::Button("Delete Slot 1")) { deleteSaveFile(1); currentGameSaves = getSaveFileIds(); } }
                if (currentGameSaves[1] == 1) { if (ImGui::Button("Delete Slot 2")) { deleteSaveFile(2); currentGameSaves = getSaveFileIds(); } }
                if (currentGameSaves[2] == 1) { if (ImGui::Button("Delete Slot 3")) { deleteSaveFile(3); currentGameSaves = getSaveFileIds(); } }

                ImGui::End();
            }
            #pragma endregion
        });
   this->worldLst->push_back(gui);

} //EO LoadMap()...


// ################## FUNCTIONS ##################


//############## ISLAND SCORE ##############
#pragma region IslandScore
void GLViewHunterCrossing::checkIslandScore() {
    int score = 0;
    if (homeLocation != 0) { score += 500; }
    if (npcOneLocation != 0) { score += 2000; }
    if (npcTwoLocation != 0) { score += 5000; }

    for (int i = 0; i < activeDecor.size(); i++) {
        int id = getShopItemId(activeDecor[i].title);
        if (id == 1) { score += 3000; }
        else if (id == 2) { score += 1000; }
        else if (id == 3) { score += 500; }
        else if (id == 4) { score += 2000; }
        else if (id == 5) { score += 6000; }
    }
    if (score != islandScore) { islandScore = score; }
}
#pragma endregion

//############## SQL ##############
#pragma region SQL

void GLViewHunterCrossing::executeQuery(std::string query) {

    char* err = 0; int rc;
    rc = sqlite3_open((ManagerEnvironmentConfiguration::getLMM() + "/db/saves.db").c_str(), &db);
    if (rc) { fprintf(stderr, "Error opening db %s\n", sqlite3_errmsg(db)); }

    rc = sqlite3_exec(db, query.c_str(), NULL, NULL, &err);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
    }
    else { fprintf(stdout, "Query Successful\n"); }
    sqlite3_close(db);
}

std::vector<int> GLViewHunterCrossing::getSaveFileIds() {
    std::vector<int> retVal = {0,0,0};
    char* err = 0; int rc;
    rc = sqlite3_open((ManagerEnvironmentConfiguration::getLMM() + "/db/saves.db").c_str(), &db);
    if (rc) { fprintf(stderr, "Error opening db %s\n", sqlite3_errmsg(db)); }

    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db, "SELECT [id] FROM SAVES", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) { printf("SQL error: %s\n", sqlite3_errmsg(db)); }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        int index = sqlite3_column_int(stmt, 0) - 1;
        if (index >= 0 && index <= 3) { retVal[index] = 1; }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return retVal;
}

void GLViewHunterCrossing::loadSaveFile(int save) {

    char* err = 0; int rc;
    rc = sqlite3_open((ManagerEnvironmentConfiguration::getLMM() + "/db/saves.db").c_str(), &db);
    if (rc) { fprintf(stderr, "Error opening db %s\n", sqlite3_errmsg(db)); }

    //Select from saves.
    {
        sqlite3_stmt* stmt = nullptr;
        std::string query = "SELECT * FROM SAVES WHERE id = " + std::to_string(save);
        rc = sqlite3_prepare_v2(db, query.data(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) { printf("SQL error: %s\n", sqlite3_errmsg(db)); return; }

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            inventory.money = sqlite3_column_double(stmt, 1);
            inventory.wood = sqlite3_column_int(stmt, 2);
            inventory.stone = sqlite3_column_int(stmt, 3);
            upgradeAxeTier(sqlite3_column_int(stmt, 4));
            int home = sqlite3_column_int(stmt, 5);
            if (home != 0) { clearPlot(home); createPlot(home, 1); }
            int npcOne = sqlite3_column_int(stmt, 6);
            if (npcOne != 0) { clearPlot(npcOne); createPlot(npcOne, 2); }
            int npcTwo = sqlite3_column_int(stmt, 7);
            if (npcTwo != 0) { clearPlot(npcTwo); createPlot(npcTwo, 3); }
        }
        if (rc != SQLITE_DONE) { printf("SQL error: %s\n", sqlite3_errmsg(db)); }
        sqlite3_finalize(stmt);
    }

    //Select from decor.
    {
        sqlite3_stmt* stmt = nullptr;
        std::string query = "SELECT * FROM DECOR WHERE id = " + std::to_string(save);
        rc = sqlite3_prepare_v2(db, query.data(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) { printf("SQL error: %s\n", sqlite3_errmsg(db)); return; }

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

            int link = sqlite3_column_int(stmt, 5);
            DecorItem item = getShopItem(link);

            if (sqlite3_column_int(stmt, 1) == 1) {
                float x = sqlite3_column_double(stmt, 2);
                float y = sqlite3_column_double(stmt, 3);
                int rot = sqlite3_column_int(stmt, 4);

                WoObject newObj = createWoObject(item.name, item.mmLink, item.size, Vector(x,y,item.height), 0.0f);
                newObj.rotation = rot;
                for (int i = 0; i < rot; i++) { newObj.obj->rotateAboutGlobalZ(1.57f); }
                activeDecor.push_back(newObj);
            }
            else { inventory.decor.push_back(item); }
        }
        if (rc != SQLITE_DONE) { printf("SQL error: %s\n", sqlite3_errmsg(db)); }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
}

void GLViewHunterCrossing::checkTables() {
    executeQuery("CREATE TABLE IF NOT EXISTS \"SAVES\" (" \
                    "\"id\" INTEGER," \
                    "\"money\" REAL," \
                    "\"wood\" INTEGER," \
                    "\"stone\" INTEGER," \
                    "\"axeTier\" INTEGER," \
                    "\"home\" INTEGER," \
                    "\"npcOne\"	INTEGER," \
                    "\"npcTwo\"	INTEGER);"
                "CREATE TABLE IF NOT EXISTS \"DECOR\" (" \
                    "\"id\" INTEGER," \
                    "\"isActive\" INTEGER," \
                    "\"x\" REAL," \
                    "\"y\" REAL," \
                    "\"rotation\" INTEGER," \
                    "\"link\" INTEGER);");
}

void GLViewHunterCrossing::writeSaveFile(int save) {
    executeQuery(
        "DELETE FROM SAVES WHERE id = " + std::to_string(save) + "; " \
        "DELETE FROM DECOR WHERE id = " + std::to_string(save) + "; " \
        "INSERT INTO \"SAVES\" (\"id\", \"money\", \"wood\", \"stone\", \"axeTier\", \"home\", \"npcOne\", \"npcTwo\") " \
            "VALUES (" + std::to_string(save) + ", " +
            std::to_string(inventory.money) + ", " +
            std::to_string(inventory.wood) + ", " +
            std::to_string(inventory.stone) + ", " +
            std::to_string(axeTier) + ", " +
            std::to_string(homeLocation) + ", " +
            std::to_string(npcOneLocation) + ", " +
            std::to_string(npcTwoLocation) + ");");

    for(int i = 0; i < inventory.decor.size(); i++) {
        executeQuery("INSERT INTO \"DECOR\" (\"id\", \"isActive\", \"x\", \"y\", \"rotation\", \"link\") " \
            "VALUES (" + std::to_string(save) + ", 0, 0.0, 0.0, 0, " + std::to_string(getShopItemId(inventory.decor[i].name)) + ")");
    }

    for (int i = 0; i < activeDecor.size(); i++) {
        executeQuery("INSERT INTO \"DECOR\" (\"id\", \"isActive\", \"x\", \"y\", \"rotation\", \"link\") " \
            "VALUES (" + std::to_string(save) + ", 1, " +
            std::to_string(activeDecor[i].location[0]) + ", " +
            std::to_string(activeDecor[i].location[1]) + ", " +
            std::to_string(activeDecor[i].rotation) + ", " +
            std::to_string(getShopItemId(activeDecor[i].title)) + ")");
    }
}

void GLViewHunterCrossing::deleteSaveFile(int save) {
    executeQuery(
        "DELETE FROM SAVES WHERE id = " + std::to_string(save) + "; " \
        "DELETE FROM DECOR WHERE id = " + std::to_string(save) + "; ");
}
#pragma endregion

//############## SHOP / DECOR ##############
#pragma region ShopAndDecor
void GLViewHunterCrossing::updateDecorLocation() {
    if (activeDecorIndexLocation[0] != activeDecor[activeDecorIndex].location[0]) {
        activeDecor[activeDecorIndex].obj->setPosition(Vector(activeDecorIndexLocation[0], activeDecor[activeDecorIndex].location[1], activeDecor[activeDecorIndex].location[2]));
        activeDecor[activeDecorIndex].location[0] = activeDecorIndexLocation[0];
    }
    if (activeDecorIndexLocation[1] != activeDecor[activeDecorIndex].location[1]) {
        activeDecor[activeDecorIndex].obj->setPosition(Vector(activeDecor[activeDecorIndex].location[0], activeDecorIndexLocation[1], activeDecor[activeDecorIndex].location[2]));
        activeDecor[activeDecorIndex].location[1] = activeDecorIndexLocation[1];
    }
}

void GLViewHunterCrossing::updateDecorRotation() {
    activeDecor[activeDecorIndex].obj->rotateAboutGlobalZ(1.57f);
    activeDecor[activeDecorIndex].rotation++;
}

void GLViewHunterCrossing::updateDecorIndex() {

    if (activeDecorIndex != oldActiveDecorIndex) {
        activeDecorIndexLocation = activeDecor[activeDecorIndex].location;
        currentDeveloperIndexSize = activeDecor[activeDecorIndex].size;
        oldActiveDecorIndex = activeDecorIndex;
    }
}

void GLViewHunterCrossing::stashDecorIndexIntoInventory() {
    DecorItem newItem;
    newItem.name = activeDecor[activeDecorIndex].title;
    newItem.mmLink = activeDecor[activeDecorIndex].link;
    newItem.size = activeDecor[activeDecorIndex].size;
    newItem.height = activeDecor[activeDecorIndex].location[2];
    worldLst->eraseViaWOptr(activeDecor[activeDecorIndex].obj);
    activeDecor.erase(activeDecor.begin() + activeDecorIndex);
    activeDecorIndex = 0;
    inventory.decor.push_back(newItem);
}

void GLViewHunterCrossing::buyNooksCrannyItem(std::string item) {
    DecorItem newItem;
    if (item == shopNamePlayground) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/playground/scene.gltf";
        newItem.size = 1.7;
        newItem.height = 4.66;
        newItem.name = "Playground";
        inventory.money -= 50000;
    }
    else if (item == shopNameBasketball) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/basketball/scene.gltf";
        newItem.size = 0.14;
        newItem.height = 3.3;
        newItem.name = "Basketball";
        inventory.money -= 15000;
    }
    else if (item == shopNameFlower) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/flower/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 3.3;
        newItem.name = "Flower";
        inventory.money -= 2000;
    }
    else if (item == shopNameLamp) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/lamp/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 5.15;
        newItem.name = "Street Lamp";
        inventory.money -= 10000;
    }
    else if (item == shopNameStatue) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/statue/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 5.25;
        newItem.name = "Statue";
        inventory.money -= 200000;
    }
    else { return; }
    inventory.decor.push_back(newItem);
}

DecorItem GLViewHunterCrossing::getShopItem(int id) {
    DecorItem newItem;
    if (id == 1) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/playground/scene.gltf";
        newItem.size = 1.7;
        newItem.height = 4.66;
        newItem.name = "Playground";
    }
    else if (id == 2) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/basketball/scene.gltf";
        newItem.size = 0.14;
        newItem.height = 3.3;
        newItem.name = "Basketball";
    }
    else if (id == 3) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/flower/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 3.3;
        newItem.name = "Flower";
    }
    else if (id == 4) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/lamp/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 5.15;
        newItem.name = "Street Lamp";
    }
    else if (id == 5) {
        newItem.mmLink = ManagerEnvironmentConfiguration::getLMM() + "/models/statue/scene.gltf";
        newItem.size = 0.5;
        newItem.height = 5.25;
        newItem.name = "Statue";
    }
    return newItem;
}

int GLViewHunterCrossing::getShopItemId(std::string name) {
    if (name == "Playground")
        return 1;
    else if (name == "Basketball")
        return 2;
    else if (name == "Flower")
        return 3;
    else if (name == "Street Lamp")
        return 4;
    else if (name == "Statue")
        return 5;
    return 0;
}

void GLViewHunterCrossing::placeDecorIndexIntoWorld() {
    activeDecor.push_back(createWoObject(inventory.decor[inventoryDecorIndex].name, inventory.decor[inventoryDecorIndex].mmLink, inventory.decor[inventoryDecorIndex].size, Vector(5, 5, inventory.decor[inventoryDecorIndex].height), 0.0f));
    inventory.decor.erase(inventory.decor.begin() + inventoryDecorIndex);
    inventoryDecorIndex = 0;
}

void GLViewHunterCrossing::adjustInventoryAfterPlotPurchase(int type) {
    if (type == 1) {
        inventory.money -= 80000;
        inventory.wood -= 200;
        inventory.stone -= 100;
    }
    else if (type == 2) {
        inventory.money -= 100000;
        inventory.wood -= 400;
        inventory.stone -= 200;
    }
    else if (type == 3) {
        inventory.money -= 200000;
        inventory.wood -= 1000;
        inventory.stone -= 500;
    }
}
#pragma endregion

//############## GUIs / NPCs ##############
#pragma region NPCs
void GLViewHunterCrossing::checkTomNook() {
    if (std::abs(20 - player.currentPosition[0]) + std::abs(15 - player.currentPosition[1]) <= 3)
        isNearTomNook = true;
    else { isNearTomNook = false; }
}

void GLViewHunterCrossing::checkNooksCranny() {
    if (std::abs(-4 - player.currentPosition[0]) + std::abs(15 - player.currentPosition[1]) <= 3)
        isNearNooksCranny = true;
    else { isNearNooksCranny = false; }
}

void GLViewHunterCrossing::checkNpcOne() {

    if (npcOneLocation == 0) { return; }
    int x = -9.5, y = -33;
    if (npcOneLocation == 2) { x = 26.5; y = -14; }
    else if (npcOneLocation == 3) { x = 26.5; y = 0; }

    if (std::abs(x - player.currentPosition[0]) + std::abs(y - player.currentPosition[1]) <= 3)
        isNearNpcOne = true;
    else { isNearNpcOne = false; }
}

void GLViewHunterCrossing::checkNpcTwo() {

    if (npcTwoLocation == 0) { return; }
    int x = -9.5, y = -33;
    if (npcTwoLocation == 2) { x = 26.5; y = -14; }
    else if (npcTwoLocation == 3) { x = 26.5; y = 0; }

    if (std::abs(x - player.currentPosition[0]) + std::abs(y - player.currentPosition[1]) <= 3)
        isNearNpcTwo = true;
    else { isNearNpcTwo = false; }
}

void GLViewHunterCrossing::checkHome() {

    if (homeLocation == 0) { return; }
    int x = -9.5, y = -33;
    if (homeLocation == 2) { x = 26.5; y = -14; }
    else if (homeLocation == 3) { x = 26.5; y = 0; }

    if (std::abs(x - player.currentPosition[0]) + std::abs(y - player.currentPosition[1]) <= 3)
        isNearHome = true;
    else { isNearHome = false; }
}

void GLViewHunterCrossing::checkToHideDefaultGuis() {
    if (isNearNooksCranny || isNearTomNook || isNearNpcOne || isNearNpcTwo || isNearHome || isDecorGuiActive || !isGameReady) { hideDefaultGuis = true; }
    else { hideDefaultGuis = false; }
}
#pragma endregion

//############## HARVESTABLE OBJECTS ##############
#pragma region HarvestableObjects
void GLViewHunterCrossing::checkObjectHealth() {
    for (int i = 0; i < trees.size(); i++) {
        if (trees[i].health <= 0)
        { 
            trees[i].isActive = false; 
            trees[i].health = DefaultHealth;
            HarvestLocation newTree;
            newTree.type = "TREE";
            newTree.index = i;
            inventory.wood += woodPerTree;
            shrinkObject(newTree);
        }
    }
    for (int i = 0; i < rocks.size(); i++) {
        if (rocks[i].health <= 0) 
        {
            rocks[i].isActive = false;
            rocks[i].health = DefaultHealth;
            HarvestLocation newRock;
            newRock.type = "ROCK";
            newRock.index = i;
            inventory.stone += stonePerRock;
            shrinkObject(newRock);
        }
    }
}

void GLViewHunterCrossing::checkObjectCooldown() {
    for (int i = 0; i < trees.size(); i++) {
        if (trees[i].cooldown <= 0)
        {
            trees[i].isActive = true;
            trees[i].cooldown = DefaultCooldown;
            HarvestLocation newTree;
            newTree.type = "TREE";
            newTree.index = i;
            growObject(newTree);
        }
    }
    for (int i = 0; i < rocks.size(); i++) {
        if (rocks[i].cooldown <= 0)
        {
            rocks[i].isActive = true;
            rocks[i].cooldown = DefaultCooldown;
            HarvestLocation newRock;
            newRock.type = "ROCK";
            newRock.index = i;
            growObject(newRock);
        }
    }
}

void GLViewHunterCrossing::shrinkObject(HarvestLocation loc) {
    if (loc.type == "TREE")
        trees[loc.index].woObj.obj->setPosition(Vector(trees[loc.index].woObj.location[0], trees[loc.index].woObj.location[1], trees[loc.index].woObj.location[2] - 20));
    else if (loc.type == "ROCK")
        rocks[loc.index].woObj.obj->setPosition(Vector(rocks[loc.index].woObj.location[0], rocks[loc.index].woObj.location[1], rocks[loc.index].woObj.location[2] - 20));
}

void GLViewHunterCrossing::growObject(HarvestLocation loc) {
    if (loc.type == "TREE")
        trees[loc.index].woObj.obj->setPosition(trees[loc.index].woObj.location);
    else if (loc.type == "ROCK")
        rocks[loc.index].woObj.obj->setPosition(rocks[loc.index].woObj.location);
}

void GLViewHunterCrossing::tickHarvestCooldown() {
    for (int i = 0; i < trees.size(); i++) {
        if (!trees[i].isActive) { trees[i].cooldown -= 1; }
    }
    for (int i = 0; i < rocks.size(); i++) {
        if (!rocks[i].isActive) { rocks[i].cooldown -= 1; }
    }
}

void GLViewHunterCrossing::tickHarvestableObjectHealth() { 
    if (currentHarvestObject.type == "TREE") {
        trees[currentHarvestObject.index].health -= harvestSpeed;
    }
    else if (currentHarvestObject.type == "ROCK") {
        rocks[currentHarvestObject.index].health -= (harvestSpeed/2);
    }
}

void GLViewHunterCrossing::markObjectHarvestable() {

    currentHarvestObject = findNearestObject();
    if (isObjectHarvestable(currentHarvestObject)) { 
        isHarvestableObjectInRange = true;
        if (currentHarvestObject.type == "TREE") {
            axe->setPosition(Vector(trees[currentHarvestObject.index].woObj.location[0]-0.5, trees[currentHarvestObject.index].woObj.location[1], trees[currentHarvestObject.index].woObj.location[2]+4+axeMarkerHeight));
        }
        else if (currentHarvestObject.type == "ROCK") {
            axe->setPosition(Vector(rocks[currentHarvestObject.index].woObj.location[0]-0.5, rocks[currentHarvestObject.index].woObj.location[1], rocks[currentHarvestObject.index].woObj.location[2]+4+axeMarkerHeight));
        }
    }
    else { 
        isHarvestableObjectInRange = false; 
        axe->setPosition(-1, -1, -40);
    }
}

HarvestLocation GLViewHunterCrossing::findNearestObject() {

    HarvestLocation retVal;
    retVal.index = 0;
    retVal.type = "TREE";

    for (int i = 1; i < trees.size(); i++) {
        int currentDif = std::abs(trees[retVal.index].woObj.location[0] - player.currentPosition[0]) + std::abs(trees[retVal.index].woObj.location[1] - player.currentPosition[1]);
        int thisDif = std::abs(trees[i].woObj.location[0] - player.currentPosition[0]) + std::abs(trees[i].woObj.location[1] - player.currentPosition[1]);

        if (thisDif < currentDif) { 
            retVal.index = i;
            retVal.type = "TREE";
        }
    }
    for (int i = 0; i < rocks.size(); i++) {

        int currentDif = std::abs(trees[retVal.index].woObj.location[0] - player.currentPosition[0]) + std::abs(trees[retVal.index].woObj.location[1] - player.currentPosition[1]);
        if (retVal.type == "ROCK") { currentDif = std::abs(rocks[retVal.index].woObj.location[0] - player.currentPosition[0]) + std::abs(rocks[retVal.index].woObj.location[1] - player.currentPosition[1]); }
        int thisDif = std::abs(rocks[i].woObj.location[0] - player.currentPosition[0]) + std::abs(rocks[i].woObj.location[1] - player.currentPosition[1]);

        if (thisDif < currentDif) {
            retVal.index = i;
            retVal.type = "ROCK";
        }
    }

    return retVal;
}

bool GLViewHunterCrossing::isObjectHarvestable(HarvestLocation loc) {
    if (loc.type == "TREE") {
        int dif = std::abs(trees[loc.index].woObj.location[0] - player.currentPosition[0]) + std::abs(trees[loc.index].woObj.location[1] - player.currentPosition[1]);
        return dif <= harvestRange && trees[loc.index].isActive;
    }
    else if (loc.type == "ROCK") {
        int dif = std::abs(rocks[loc.index].woObj.location[0] - player.currentPosition[0]) + std::abs(rocks[loc.index].woObj.location[1] - player.currentPosition[1]);
        return dif <= harvestRange && rocks[loc.index].isActive;
    }
    return false;
}

void GLViewHunterCrossing::upgradeAxeTier(int tier) {

    if (tier == 1) {
        axeTier += 1;
        harvestSpeed *= 2;
        woodPerTree *= 1.5;
        stonePerRock *= 1.5;
    }
    else if (tier == 2) {
        axeTier += 2;
        harvestSpeed *= 2;
        woodPerTree *= 1.5;
        stonePerRock *= 1.5;
        harvestRange *= 2;
        woodPerTree *= 2;
        stonePerRock *= 2;
    }
    else if (tier == 3) {
        axeTier += 3;
        harvestSpeed *= 2;
        woodPerTree *= 1.5;
        stonePerRock *= 1.5;
        harvestRange *= 2;
        woodPerTree *= 2;
        stonePerRock *= 2;
        harvestSpeed *= 3;
        woodPerTree *= 3;
        stonePerRock *= 3;
    }
}
#pragma endregion

//############## PLOTS ##############
#pragma region PlotManager
bool GLViewHunterCrossing::createEmptyPlot(int location) {

    if (isPlotInUse(location)) { return false; }

    LandPlot retVal;
    retVal.location = location;

    Vector plotCoords = getEmptyPlotCoords(location);
    float plotDirection = getEmptyPlotDirection(location);

    retVal.objs.push_back(WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/buildSite/scene.gltf", Vector(2, 2, 2), MESH_SHADING_TYPE::mstFLAT));
    retVal.objs[0]->setPosition(Vector(plotCoords[0], plotCoords[1], plotCoords[2] + 1.05));
    retVal.objs[0]->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    retVal.objs[0]->rotateAboutGlobalZ(plotDirection);
    worldLst->push_back(retVal.objs[0]);

    retVal.objs.push_back(WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/emptySite/scene.gltf", Vector(6, 6, 6), MESH_SHADING_TYPE::mstFLAT));
    retVal.objs[1]->setPosition(plotCoords);
    retVal.objs[1]->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    retVal.objs[1]->rotateAboutGlobalZ(1.19f + plotDirection);
    worldLst->push_back(retVal.objs[1]);

    inventory.plots.push_back(retVal);
    return true;
}

bool GLViewHunterCrossing::createPlot(int location, int type) {

    if (isPlotInUse(location)) { return false; }

    LandPlot retVal;
    retVal.location = location;
    retVal.type = type;
    retVal.isActive = true;

    Vector plotCoords = getPlotCoords(location);
    float plotDirection = getPlotDirection(location);

    retVal.objs.push_back(WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/house1/scene.gltf", Vector(0.025, 0.025, 0.025), MESH_SHADING_TYPE::mstFLAT));
    retVal.objs[0]->setPosition(plotCoords);
    retVal.objs[0]->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    retVal.objs[0]->rotateAboutGlobalZ(plotDirection);
    worldLst->push_back(retVal.objs[0]);

    if (type == 1) { homeLocation = location; }
    else if (type == 2) {
        npcOneLocation = location;
        retVal.objs.push_back(createWoObject("", ManagerEnvironmentConfiguration::getLMM() + "/models/npc1/scene.gltf", 0.55, getVillagerCoords(location, type), getVillagerDirection(location)).obj);
    }
    else if (type == 3) {
        npcTwoLocation = location;
        retVal.objs.push_back(createWoObject("", ManagerEnvironmentConfiguration::getLMM() + "/models/npc2/scene.gltf", 0.093, getVillagerCoords(location, type), getVillagerDirection(location)).obj);
    }

    inventory.plots.push_back(retVal);
    return true;
}

void GLViewHunterCrossing::clearPlot(int location) {

    if (!isPlotInUse(location)) { return; }

    if (homeLocation == location) { homeLocation = 0; }
    else if (npcOneLocation == location) { npcOneLocation = 0; }
    else if (npcTwoLocation == location) { npcTwoLocation = 2; }

    for (int i = 0; i < inventory.plots.size(); i++) {
        if (inventory.plots[i].location == location) {
            for (int j = 0; j < inventory.plots[i].objs.size(); j++) {
                worldLst->eraseViaWOptr(inventory.plots[i].objs[j]);
            } inventory.plots.erase(inventory.plots.begin() + i);
        }
    }
}

bool GLViewHunterCrossing::isPlotInUse(int location) {
    for (int i = 0; i < inventory.plots.size(); i++) {
        if (inventory.plots[i].location == location) return true;
    } return false;
}

Vector GLViewHunterCrossing::getEmptyPlotCoords(int location)
{
    if (location == 1)
        return plotLocation1;
    else if (location == 2)
        return plotLocation2;
    else if (location == 3)
        return plotLocation3;
    return Vector(0, 0, 0);
}

Vector GLViewHunterCrossing::getPlotCoords(int location)
{
    if (location == 1)
        return houseLocation1;
    else if (location == 2)
        return houseLocation2;
    else if (location == 3)
        return houseLocation3;
    return Vector(0, 0, 0);
}

Vector GLViewHunterCrossing::getVillagerCoords(int location, int type)
{
    float y = 4.04;
    if (type == 3) y+=0.1;

    if (location == 1)
        return Vector(houseLocation1[0]+4, houseLocation1[1], y);
    else if (location == 2)
        return Vector(houseLocation2[0]-4, houseLocation2[1], y);
    else if (location == 3)
        return Vector(houseLocation3[0]-4, houseLocation3[1], y);
    return Vector(0, 0, 0);
}

float GLViewHunterCrossing::getEmptyPlotDirection(int location) 
{
    if (location == 1)
        return 1.57f;
    else if (location == 2 || location == 3)
        return -1.57f;
    return 0.0f;
}

float GLViewHunterCrossing::getPlotDirection(int location)
{
    if (location == 2 || location == 3)
        return 3.14f;
    return 0.0f;
}

float GLViewHunterCrossing::getVillagerDirection(int location)
{
    if (location == 1)
        return 1.57f;
    else if (location == 2 || location == 3)
        return -1.57f;
    return 0.0f;
}
#pragma endregion

//############## PLAYER ##############
#pragma region Player
void GLViewHunterCrossing::createPlayer(Vector position, std::string currentDirection) {

    player.currentPosition = position;
    player.currentDirection = currentDirection;
    player.currentMarkerHeight = 0;
    player.movementSpeed = 0.3;

    player.player = Aftr::WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/player/scene.gltf", Vector(0.15, 0.15, 0.15), MESH_SHADING_TYPE::mstFLAT);
    player.player->setPosition(position);
    worldLst->push_back(player.player);

    if (player.currentDirection == "N") { player.player->rotateAboutGlobalZ(3.14f); } 
    else if (player.currentDirection == "E") { player.player->rotateAboutGlobalZ(1.57f); }
    else if (player.currentDirection == "W") { player.player->rotateAboutGlobalZ(-1.57f); }

    player.marker = Aftr::WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/playerArrow/scene.gltf", Vector(0.005, 0.005, 0.005), MESH_SHADING_TYPE::mstFLAT);
    player.marker->setPosition(Vector(position[0], position[1], position[2]+2.8));
    player.marker->rotateAboutGlobalY(1.57f);
    worldLst->push_back(player.marker);

    if (isCameraOnPlayer) { this->cam->setPosition(Vector(player.currentPosition[0] - cameraZoom, player.currentPosition[1] - cameraZoom, player.currentPosition[2] + cameraZoom)); }
}

bool GLViewHunterCrossing::isPlayerInBounds(std::string direction) {

    if (direction == "N")
        return player.currentPosition[1] + player.movementSpeed <= 14.5;

    else if (direction == "S")
        return ((player.currentPosition[1] - player.movementSpeed >= -44.5) && (player.currentPosition[0] <= 16)) ||
        ((player.currentPosition[1] - player.movementSpeed >= -25.5) && (player.currentPosition[0] >= 16));

    else if (direction == "W")
        return player.currentPosition[0] - player.movementSpeed >= -9.5;

    else if (direction == "E")
        return ((player.currentPosition[1] <= -25.5) && (player.currentPosition[0] + player.movementSpeed <= 15.5)) ||
        ((player.currentPosition[1] >= -25.5) && (player.currentPosition[0] + player.movementSpeed <= 26.5));

    return false;
}

void GLViewHunterCrossing::movePlayer(std::string direction) {

    if (direction == "N") {
        player.currentPosition[1] += player.movementSpeed;
        rotatePlayer(direction);
    }
    else if (direction == "S") {
        player.currentPosition[1] -= player.movementSpeed;
        rotatePlayer(direction);
    }
    else if (direction == "W") {
        player.currentPosition[0] -= player.movementSpeed;
        rotatePlayer(direction);
    }
    else if (direction == "E") {
        player.currentPosition[0] += player.movementSpeed;
        rotatePlayer(direction);
    }

    player.marker->setPosition(Vector(player.currentPosition[0], player.currentPosition[1], player.currentPosition[2] + 2.8));
    player.player->setPosition(player.currentPosition);

    if (isCameraOnPlayer) { this->cam->setPosition(Vector(player.currentPosition[0] - cameraZoom, player.currentPosition[1] - cameraZoom, player.currentPosition[2] + cameraZoom)); }
}

void GLViewHunterCrossing::rotatePlayer(std::string direction) {

    if (player.currentDirection == direction) return;

    if (player.currentDirection == "N") { player.player->rotateAboutGlobalZ(-3.14f); }
    else if (player.currentDirection == "E") { player.player->rotateAboutGlobalZ(-1.57f); }
    else if (player.currentDirection == "W") { player.player->rotateAboutGlobalZ(1.57f); }

    if (direction == "N") { player.player->rotateAboutGlobalZ(3.14f); }
    else if (direction == "E") { player.player->rotateAboutGlobalZ(1.57f); }
    else if (direction == "W") { player.player->rotateAboutGlobalZ(-1.57f); }

    player.currentDirection = direction;
}

void GLViewHunterCrossing::updatePlayerMarker() {

    if (player.isMarkerHeightIncreasing) {
        player.currentMarkerHeight += 0.02;
        if (player.currentMarkerHeight >= 1) { player.isMarkerHeightIncreasing = false; }
    }
    else {
        player.currentMarkerHeight -= 0.02;
        if (player.currentMarkerHeight <= 0) { player.isMarkerHeightIncreasing = true; }
    }
    player.marker->setPosition(Vector(player.currentPosition[0], player.currentPosition[1], player.currentPosition[2] + 2.8 + player.currentMarkerHeight));
}
#pragma endregion

//############## CREATE BORDER ##############
#pragma region CreateBorder
void GLViewHunterCrossing::createWoBorder() {

    std::string fenceLink = ManagerEnvironmentConfiguration::getLMM() + "/models/fence/scene.gltf";
    for (int i = 0; i < 5; i++) {
        int y = 11 - (i * 9);
        createOneWoBorder(-11, y, 1.57f);
    }
    for (int i = 0; i < 3; i++) {
        int x = -6 + (i * 9);
        createOneWoBorder(x, -46, 3.14f);
    }
    for (int i = 0; i < 2; i++) {
        int y = -41 + (i * 9);
        createOneWoBorder(17, y, 1.57f);
    }
    createOneWoBorder(-11, -41, 1.57f);
    createOneWoBorder(23, -27, 3.14f);
    createOneWoBorder(28, -21, 1.57f);
    createOneWoBorder(28, -7, 1.57f);
    createOneWoBorder(28, 7, 1.57f);
    createOneWoBorder(28, 16, 1.57f);
    createOneWoBorder(8.5, 16, 3.14f);
}

void GLViewHunterCrossing::createOneWoBorder(float x, float y, float rotation) {
    WO* wo = WO::New(ManagerEnvironmentConfiguration::getLMM() + "/models/fence/scene.gltf", Vector(2, 2, 2), MESH_SHADING_TYPE::mstFLAT);
    wo->setPosition(Vector(x, y, 3.9));
    wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    wo->rotateAboutGlobalZ(1.19f + rotation);
    worldLst->push_back(wo);
}
#pragma endregion

//############## CREATE MAP ##############
#pragma region CreateMap
void GLViewHunterCrossing::createMap() {
    mapBedrock.push_back(createWoObject("MAP", ManagerEnvironmentConfiguration::getLMM() + "/models/map/scene.gltf", 80, Vector(0, 0, 10), 0.0f));
    mapBedrock.push_back(createWoObject("OCEAN", ManagerEnvironmentConfiguration::getLMM() + "/models/ocean/scene.gltf", 100, Vector(0, 0, 7), 0.0f));
    mapBedrock.push_back(createWoObject("NOOKS", ManagerEnvironmentConfiguration::getLMM() + "/models/nooksCranny/scene.gltf", 0.04, Vector(-3.5, 20, 5.8), 0.0f));
    mapBedrock.push_back(createWoObject("TOWNHALL", ManagerEnvironmentConfiguration::getLMM() + "/models/townhall/scene.gltf", 0.08, Vector(20, 20, 5.5), 0.0f));
    mapBedrock.push_back(createWoObject("TOM", ManagerEnvironmentConfiguration::getLMM() + "/models/tom/scene.gltf", 0.054, Vector(20, 15, 4.12), 0.0f));
}

void GLViewHunterCrossing::createTrees() {
    for (int i = 0; i < treeLocations.size(); ++i) {
        HarvestableWoObject newTree;
        newTree.woObj = createWoObject("TREE" + i, ManagerEnvironmentConfiguration::getLMM() + "/models/tree1/scene.gltf", 2.4, treeLocations[i], 0.0f);
        trees.push_back(newTree);
    }
}

void GLViewHunterCrossing::createRocks() {
    for (int i = 0; i < rockLocations.size(); ++i) {
        HarvestableWoObject newRock;
        newRock.woObj = createWoObject("ROCK" + i, ManagerEnvironmentConfiguration::getLMM() + "/models/rock1/scene.gltf", 0.038, rockLocations[i], 0.0f);
        rocks.push_back(newRock);
    }
}

WoObject GLViewHunterCrossing::createWoObject(std::string title, std::string mmLink, float size, Vector location, float zRotation) {

    WoObject newObj;
    newObj.title = title;
    newObj.link = mmLink;
    newObj.size = size;
    newObj.location = location;
    newObj.rotation = zRotation;

    newObj.obj = WO::New(newObj.link, Vector(newObj.size, newObj.size, newObj.size), MESH_SHADING_TYPE::mstFLAT);
    newObj.obj->setPosition(newObj.location);
    newObj.obj->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    newObj.obj->rotateAboutGlobalZ(newObj.rotation);
    worldLst->push_back(newObj.obj);
    return newObj;
}
#pragma endregion

//############## DEV FUNCTIONS ##############
#pragma region DeveloperFunctions
void GLViewHunterCrossing::createDeveloperWo(std::string mmLink)
{
    WoObject newDevItem;

    newDevItem.location = Vector(0, 0, 10);
    newDevItem.size = 1;
    newDevItem.link = mmLink;

    newDevItem.obj = WO::New(mmLink, Vector(newDevItem.size, newDevItem.size, newDevItem.size), MESH_SHADING_TYPE::mstFLAT);
    newDevItem.obj->setPosition(newDevItem.location);
    newDevItem.obj->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
    worldLst->push_back(newDevItem.obj);

    developerList.push_back(newDevItem);
}

void GLViewHunterCrossing::updateDeveloperWoSize() {

    if (currentDeveloperIndexSize != developerList[developerListIndex].size) {

        worldLst->eraseViaWOptr(developerList[developerListIndex].obj);
        developerList[developerListIndex].obj = WO::New(developerList[developerListIndex].link,
            Vector(currentDeveloperIndexSize, currentDeveloperIndexSize, currentDeveloperIndexSize), MESH_SHADING_TYPE::mstFLAT);
        developerList[developerListIndex].obj->setPosition(developerList[developerListIndex].location);
        developerList[developerListIndex].obj->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
        worldLst->push_back(developerList[developerListIndex].obj);
        developerList[developerListIndex].size = currentDeveloperIndexSize;
    }
}

void GLViewHunterCrossing::updateDeveloperWoLocation() {

    if (currentDeveloperIndexLocation[0] != developerList[developerListIndex].location[0]) {
        developerList[developerListIndex].obj->setPosition(Vector(currentDeveloperIndexLocation[0], developerList[developerListIndex].location[1], developerList[developerListIndex].location[2]));
        developerList[developerListIndex].location[0] = currentDeveloperIndexLocation[0];
    }
    if (currentDeveloperIndexLocation[1] != developerList[developerListIndex].location[1]) {
        developerList[developerListIndex].obj->setPosition(Vector(developerList[developerListIndex].location[0], currentDeveloperIndexLocation[1], developerList[developerListIndex].location[2]));
        developerList[developerListIndex].location[1] = currentDeveloperIndexLocation[1];
    }
    if (currentDeveloperIndexLocation[2] != developerList[developerListIndex].location[2]) {
        developerList[developerListIndex].obj->setPosition(Vector(developerList[developerListIndex].location[0], developerList[developerListIndex].location[1], currentDeveloperIndexLocation[2]));
        developerList[developerListIndex].location[2] = currentDeveloperIndexLocation[2];
    }
}

void GLViewHunterCrossing::updateDeveloperWoRotation() {
    developerList[developerListIndex].obj->rotateAboutGlobalZ(1.57f);
}

void GLViewHunterCrossing::updateDeveloperIndexGuiValues() {

    if (developerListIndex != oldDeveloperListIndex) {
        currentDeveloperIndexLocation = developerList[developerListIndex].location;
        currentDeveloperIndexSize = developerList[developerListIndex].size;
        oldDeveloperListIndex = developerListIndex;
    }
}
#pragma endregion