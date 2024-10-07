#pragma once

#include "GLView.h"
#include "irrKlang.h"
#include "sqlite3.h"

namespace Aftr
{
    class Camera;

	//Custom DTOs.
	class Player {

		public:
			WO* player;
			WO* marker;
			Vector currentPosition;
			std::string currentDirection;
			float currentMarkerHeight;
			bool isMarkerHeightIncreasing;
			float movementSpeed;
	};

	class WoObject {
		public:
			WO* obj;
			float size;
			Vector location;
			std::string link;
			std::string title;
			float rotation;
	};

	class HarvestLocation {
		public:
			int index;
			std::string type;
	};

	class HarvestableWoObject {
		public:
			WoObject woObj;
			bool isActive;
			int health;
			int cooldown;

			HarvestableWoObject() {
				isActive = true;
				health = 200;
				cooldown = 2500;
			}
	};

	class LandPlot {
		public:
			bool isActive;
			int type; // (0 == NONE, 1 == Player, 2 == NPC1, 3 == NPC2).
			int location;

			std::vector<WO*> objs;

			LandPlot() {
				isActive = false;
				type = 0;
				location = 0;
			}
	};

	class DecorItem {
	public:
		std::string mmLink;
		std::string name;
		float size;
		float height;
	};

	class PlayerInventory {
	public:
		float money;
		int wood;
		int stone;
		std::vector<LandPlot> plots;
		std::vector<DecorItem> decor;

		PlayerInventory() {
			money = 0;
			wood = 0;
			stone = 0;
		}
	};

	class GLViewHunterCrossing : public GLView
	{
		public:
		   //Built in.
		   static GLViewHunterCrossing* New( const std::vector< std::string >& outArgs );
		   virtual ~GLViewHunterCrossing() { }
		   virtual void updateWorld();
		   virtual void loadMap();
		   virtual void onResizeWindow( GLsizei width, GLsizei height ) { GLView::onResizeWindow(width, height); }
		   virtual void onMouseDown( const SDL_MouseButtonEvent& e ) { GLView::onMouseDown(e); }
		   virtual void onMouseUp( const SDL_MouseButtonEvent& e ) { GLView::onMouseUp(e); }
		   virtual void onMouseMove( const SDL_MouseMotionEvent& e ) { GLView::onMouseMove(e); }
		   virtual void onKeyDown( const SDL_KeyboardEvent& key );
		   virtual void onKeyUp(const SDL_KeyboardEvent& key);

		   //Island score.
		   virtual void checkIslandScore();

		   //Sql.
		   virtual void executeQuery(std::string query);
		   virtual void checkTables();
		   virtual std::vector<int> getSaveFileIds();
		   virtual void loadSaveFile(int save);
		   virtual void writeSaveFile(int save);
		   virtual void deleteSaveFile(int save);

		   //Create Map.
		   virtual void createMap();
		   virtual void createTrees();
		   virtual void createRocks();
		   virtual WoObject createWoObject(std::string title, std::string mmLink, float size, Vector location, float zRotation);

		   //Debug / Developer helpers.
		   virtual void createDeveloperWo(std::string mmLink);
		   virtual void updateDeveloperWoSize();
		   virtual void updateDeveloperWoLocation();
		   virtual void updateDeveloperWoRotation();
		   virtual void updateDeveloperIndexGuiValues();

		   //Player functions.
		   virtual void createPlayer(Vector position, std::string currentDirection);
		   virtual void movePlayer(std::string direction);
		   virtual void rotatePlayer(std::string direction);
		   virtual void updatePlayerMarker();

		   //World border functions.
		   virtual void createWoBorder();
		   virtual void createOneWoBorder(float x, float y, float rotation);
		   virtual bool isPlayerInBounds(std::string direction);
		   
		   //Create plots.
		   virtual bool createEmptyPlot(int location);
		   virtual bool createPlot(int location, int type);
		   virtual bool isPlotInUse(int location);
		   virtual void clearPlot(int location);
		   virtual Vector getEmptyPlotCoords(int location);
		   virtual Vector getPlotCoords(int location);
		   virtual Vector getVillagerCoords(int location, int type);
		   virtual float getEmptyPlotDirection(int location);
		   virtual float getPlotDirection(int location);
		   virtual float getVillagerDirection(int location);
		   virtual void adjustInventoryAfterPlotPurchase(int type);

		   //Harvestable Objects.
		   virtual void markObjectHarvestable();
		   virtual void tickHarvestCooldown();
		   virtual void tickHarvestableObjectHealth();
		   virtual void checkObjectHealth();
		   virtual void checkObjectCooldown();
		   virtual void shrinkObject(HarvestLocation loc);
		   virtual void growObject(HarvestLocation loc);
		   virtual HarvestLocation findNearestObject();
		   virtual bool isObjectHarvestable(HarvestLocation loc);
		   virtual void upgradeAxeTier(int tier);

		   //NPCs
		   virtual void checkToHideDefaultGuis();
		   virtual void checkTomNook();
		   virtual void checkNooksCranny();
		   virtual void checkNpcOne();
		   virtual void checkNpcTwo();
		   virtual void checkHome();

		   //Decor
		   virtual void buyNooksCrannyItem(std::string item);
		   virtual void placeDecorIndexIntoWorld();
		   virtual void updateDecorLocation();
		   virtual void updateDecorRotation();
		   virtual void updateDecorIndex();
		   virtual void stashDecorIndexIntoInventory();
		   virtual DecorItem getShopItem(int id);
		   virtual int getShopItemId(std::string name);

		   //Db.
		   sqlite3* db;
		   std::vector<int> currentGameSaves;
		   bool isGameReady;

		   //Sound.
		   irrklang::ISoundEngine* engine;
		   irrklang::ISound* backgroundSound;
		   bool isBackgroundMusicOn;

		   //Variables.
		   bool isCameraOnPlayer;
		   int cameraZoom;
		   Player player;
		   PlayerInventory inventory;
		   int islandScore;

		   //Map.
		   std::vector<WoObject> mapBedrock;
		   std::vector<HarvestableWoObject> trees;
		   std::vector<HarvestableWoObject> rocks;

		   //Decor.
		   std::vector<WoObject> activeDecor;
		   int activeDecorIndex, oldActiveDecorIndex;
		   int inventoryDecorIndex;
		   Vector activeDecorIndexLocation;

		   //Harvest Variables.
		   HarvestLocation currentHarvestObject;
		   bool isHarvestableObjectInRange;
		   int harvestSpeed;
		   int harvestRange;
		   int stonePerRock;
		   int woodPerTree;
		   WO* axe;
		   float axeMarkerHeight;

		   //NPCs
		   bool hideDefaultGuis;
		   bool isNearTomNook;
		   bool isNearNooksCranny;
		   bool isNearNpcOne;
		   bool isNearNpcTwo;
		   bool isNearHome;
		   bool isDecorGuiActive;

		   //Tool upgrade status.
		   int axeTier;

		   //Quick plot indicators.
		   int npcOneLocation;
		   int npcTwoLocation;
		   int homeLocation;

		   //Debug.
		   bool debugMode;

		   std::vector<WoObject> developerList;
		   int developerListIndex, oldDeveloperListIndex;
		   float currentDeveloperIndexSize;
		   Vector currentDeveloperIndexLocation;

		   const int DefaultCooldown = 2500;
		   const int DefaultHealth = 200;

		   //Const.
		   const Vector plotLocation1 = Vector(-13, -33, 3.25);
		   const Vector plotLocation2 = Vector(31, -14, 3.1);
		   const Vector plotLocation3 = Vector(31, 0, 3.1);
		   const Vector houseLocation1 = Vector(-15, -33, 5.6);
		   const Vector houseLocation2 = Vector(32, -14, 5.6);
		   const Vector houseLocation3 = Vector(32, 0, 5.6);
		   const std::vector<Vector> treeLocations = {
				Vector(-6,7,5.1),
				Vector(6,10,5.1),
				Vector(10.5,0,5.1),
				Vector(24,-19.5,5.1),
				Vector(19,-23.5,5.1),
				Vector(11.5,-16,5.1),
				Vector(5,-6.5,5.1),
				Vector(-7,-6,5.1),
				Vector(-4,-19,5.1),
				Vector(-2.6,-42,5.1),
				Vector(0,-30.5,5.1),
				Vector(10.6,-38.5,5.1),
		   };
		   const std::vector<Vector> rockLocations = {
				Vector(10, 5, 3.64),
				Vector(0, -12.5, 3.64),
				Vector(8.5, -27, 3.64),
				Vector(5, -42, 3.64),
				Vector(17.5, -10, 3.64),
		   };

		   const std::string shopNamePlayground = "PLAYGROUND";
		   const std::string shopNameBasketball = "BASKETBALL";
		   const std::string shopNameLamp = "LAMP";
		   const std::string shopNameFlower = "FLOWER";
		   const std::string shopNameStatue = "STATUE";

		protected:
		   GLViewHunterCrossing( const std::vector< std::string >& args );
		   virtual void onCreate();   
	};
}
