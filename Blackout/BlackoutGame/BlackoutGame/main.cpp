#include <irrlicht.h>
#include <irrKlang.h>
#include "ICursorControl.h"
#include "EventReceiverKB.h"
#include "EventReceiver.cpp"
#include "MyFPSCameraAnimator.h"
#include "MyCollisionAnimator.h"
#include "Door.h"

#include <iostream>

using namespace std;

using namespace irr;

using namespace irrklang;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

//Globals
const f32 pJumpSpeed = 0; //Speed at which the player jumps
const f32 gSpeed = -30; //Speed of gravity
const int useDist = 55;
bool canInteract = false;

//Engine components
ICameraSceneNode* camera;
static IrrlichtDevice* gameWindow;
static ISoundEngine* audio;
static IFileSystem* fileSystem;
static IVideoDriver* vdriver;
static ISceneManager* scnmgr;
static IGUIEnvironment* guienv;
IGUIFont* font;

//Sounds
ISoundSource* sFootstep;
ISoundSource* sDoorMove;
ISoundSource* sNoPower;
ISoundSource* sPowerUp;
ISoundSource* sAccessDenied;
ISoundSource* sAccessGranted;
ISoundSource* sButtonPress;

ISound* footstep = 0;
ISound* doorNoise = 0;

//Level stored data
const int numdoors = 10;
Door* door[numdoors];
const int numParticles = 2;
IParticleSystemSceneNode* particles[numParticles];
vector3df intersection; // Tracks the current intersection point of the raycast

//Level One
const int numKeycardSwitches = 2;
ISceneNode* keycardSwitches[numKeycardSwitches];
bool labKeycardCollected = false;
bool doorPowerOn = false;

//Game stored data
bool levelLoaded = false; //Has the level finished loading?
bool gamePaused = false; //Is the game paused?
int currentLevel = 0; //Current level (0 is main menu)
int scrnHeight; //Height of window
int scrnWidth; //Width of window
bool lastFramePressed = false; //Was E pressed in the last frame?
bool lastFramePaused = false; //Was the game paused in the last frame?
ISceneNodeAnimatorCollisionResponse* collision; //Collision box of the player
IMetaTriangleSelector* meta; //Meta triangle selector for level geometry
int scrnCentreX; //X co-ordinate of the centre of the screen
int scrnCentreY; //Y co-ordinate of the centre of the screen

#ifdef _IRR_WINDOWS_
#pragma comment(lib, "Irrlicht.lib")
#pragma comment(lib, "irrKlang.lib")
//#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

void createWater(path name, int sizeX, int sizeZ, int xPos, int yPos, int zPos)
{
	int tileSize = 20;

	int tileAmountX = sizeX/tileSize;
	int tileAmountZ = sizeZ/tileSize;

	IAnimatedMesh* mesh = scnmgr->addHillPlaneMesh(name,
		dimension2d<f32>(tileSize,tileSize),
		dimension2d<u32>(tileAmountX,tileAmountZ), 0, 0,
		dimension2d<f32>(0,0),
		dimension2d<f32>(10,10));
	ISceneNode* node = scnmgr->addWaterSurfaceSceneNode(mesh->getMesh(0), 3.0f, 300.0f, 30.0f);
	node->setPosition(vector3df(xPos,yPos,zPos));
	node->setMaterialTexture(0, vdriver->getTexture("C:/apps/irrlicht-1.8.1/media/stones.jpg"));
	node->setMaterialTexture(1, vdriver->getTexture("C:/apps/irrlicht-1.8.1/media/water.jpg"));
	node->setMaterialType(EMT_REFLECTION_2_LAYER);

	// create light
	node = scnmgr->addLightSceneNode(0, vector3df(0,0,0),
		SColorf(1.0f, 0.6f, 0.7f, 1.0f), 800.0f);
	ISceneNodeAnimator* anim = 0;
	anim = scnmgr->createFlyCircleAnimator (vector3df(0,150,0),250.0f);
	node->addAnimator(anim);
	anim->drop();

	// attach billboard to light
	node = scnmgr->addBillboardSceneNode(node, dimension2d<f32>(50, 50));
	node->setMaterialFlag(EMF_LIGHTING, false);
	node->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
	node->setMaterialTexture(0, vdriver->getTexture("C:/apps/irrlicht-1.8.1/media/particlewhite.bmp"));
}

void createFire(int particleid, float xpos, float ypos, float zpos)
{
		IParticleSystemSceneNode* ps = scnmgr->addParticleSystemSceneNode(false);

		IParticleEmitter* em = ps->createBoxEmitter(
			aabbox3d<f32>(-10,0,-10,10,2,10), //emitter size
			vector3df(0.0f, 0.06f, 0.0f), //initial direction
			140, 160,						//emit rate
			SColor(0,255,255,255), //darkest color
			SColor(0,255,255,255), //brightest color
			600, 800, 0,					//min and max age, angle
			dimension2df(5.f, 5.f),  //minimum particle sizes
			dimension2df(10.f, 10.f));  //maximum particle sizes

		ps->setEmitter(em); //this grabs the emitter
		em->drop(); //so we can drop it here without deleting it

		IParticleAffector* paf = ps->createFadeOutParticleAffector();

		ps->addAffector(paf); //same goes for the affector
		 
		//ps->setScale(vector3df(0.5, 0.1, 0.5));
		ps->setMaterialFlag(EMF_LIGHTING, false);
		ps->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
		ps->setMaterialTexture(0, vdriver->getTexture("assets/textures/fire.bmp"));
		ps->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);

		particles[particleid] = ps;

	particles[particleid]->setPosition(vector3df(xpos, ypos, zpos));
}

ISceneNode* raycast(f32 dist)
{
	//Cast a ray from the camera of the length "dist"
	line3d<f32> ray;
	ray.start = camera->getPosition();
	ray.end = ray.start + (camera->getTarget() - ray.start).normalize() * dist;

	ISceneCollisionManager* collMan = scnmgr->getSceneCollisionManager();

	// Used to show with triangle has been hit
	triangle3df hitTriangle;

	ISceneNode* selectedNode = collMan->getSceneNodeAndCollisionPointFromRay(ray, intersection,
		hitTriangle, 0, 0);

	return selectedNode;
}

void animateDoor(stringc whichdoor)
{
	ISceneNode* doorsn = scnmgr->getSceneNodeFromName(whichdoor.c_str());	

	if(!doorsn)
	{
		printf("cannot find door");
		return;
	}

	int doornumber;

	if (whichdoor == "containmentDoor")
	{
		doornumber = 0;
	}
	if (whichdoor == "labkeycardDoor")
	{
		doornumber = 1;
	}

	door[doornumber]->buttonPress(3);
}

void touchItem(ISceneNode* node, const vector3df& pos)
{
	//Work out distance to item
	vector3df cpos = camera->getPosition();

	float dist = (cpos-pos).getLength();

	stringw name = node->getName();

	if (dist>useDist)
	{
		return;
	}
	
	if (name == "containmentSwitch_01" || name == "containmentSwitch_02")
	{
		if(doorPowerOn)
		{
			//it's the containment door switch
			printf("CONTAINMENT DOOR OPENED");
			animateDoor("containmentDoor");

			if(doorNoise)
			{
				doorNoise->setIsPaused(true);
				doorNoise->drop();
			}

			doorNoise = audio->play2D(sDoorMove, false, false, true);
			audio->play2D(sAccessGranted);
			return;
		}
		else
		{
			printf("POWER OFF");
			audio->play2D(sNoPower);
			return;
		}
	}
	else if (name == "labKeycardSwitch_01" || name == "labKeycardSwitch_02")
	{
		//it's the lab keycard door switch
		if(doorPowerOn)
		{
			if (labKeycardCollected)
			{
				for(int i = 0; i < numKeycardSwitches; i++)
				{
					if(!(name.equals_ignore_case(keycardSwitches[i]->getName())))
					{
						continue;
					}
					keycardSwitches[i]->setMaterialTexture(0, vdriver->getTexture("keycard_granted.png"));
				}
				printf("LAB KEYCARD DOOR OPENED");

				if(doorNoise)
				{
					doorNoise->setIsPaused(true);
					doorNoise->drop();
				}

				doorNoise = audio->play2D(sDoorMove, false, false, true);
				audio->play2D(sAccessGranted);
				animateDoor("labkeycardDoor");
				return;
			}
			else
			{
				for(int i = 0; i < numKeycardSwitches; i++)
				{
					if(!(name.equals_ignore_case(keycardSwitches[i]->getName())))
					{
						continue;
					}
					keycardSwitches[i]->setMaterialTexture(0, vdriver->getTexture("keycard_denied.png"));
				}
				audio->play2D(sAccessDenied);
				printf("ACCESS DENIED");
				return;
			}
		}
		else
		{
			printf("POWER OFF");
			audio->play2D(sNoPower);
			return;
		}
	}

	if (name == "labKeycard")
	{
		labKeycardCollected = true;
		printf("KEYCARD COLLECTED");
		meta->removeTriangleSelector(node->getTriangleSelector());
		node->setVisible(false);
		return;
	}

	if (name == "powerControls")
	{
		if(!doorPowerOn)
		{
			doorPowerOn = true;
			audio->play2D(sButtonPress);
			audio->play2D(sPowerUp);
			return;
		}
		else
		{
			audio->play2D(sButtonPress);
		}
	}

}

ICameraSceneNode* addMyFPSCamera(ISceneNode* parent=0,
	f32 rotateSpeed=150.0f, f32 moveSpeed=0.15f, s32 id=-1, SKeyMap* keyMapArray=0,
	s32 keyMapSize=0, bool noVerticalMovement=true, f32 jumpSpeed=pJumpSpeed,
	bool invertMouseY=false, bool makeActive=true)
{
	ICameraSceneNode* node = scnmgr->addCameraSceneNode(parent, vector3df(),
			vector3df(200,0,100), id, makeActive);

	if (node)
	{
		ISceneNodeAnimator* anm = new MyFPSCameraAnimator(gameWindow->getCursorControl(),
				rotateSpeed, moveSpeed, jumpSpeed,
				keyMapArray, keyMapSize, noVerticalMovement, invertMouseY);


		node->bindTargetAndRotation(true);
		node->addAnimator(anm);
		anm->drop();
	}
		return node;
}

bool initDevice()
{
	audio = createIrrKlangDevice();

	if(!audio)
	{
		return false;
	}

		IrrlichtDevice *nulldevice = createDevice(EDT_NULL); //Create null device
	dimension2d<u32> deskRes = nulldevice->getVideoModeList()->getDesktopResolution(); //Get Desktop Resolution
	nulldevice->drop(); //Drop null devices

	SIrrlichtCreationParameters params;
		params.DriverType=EDT_OPENGL;
		params.WindowSize=dimension2d<u32>(deskRes);
		params.Bits=32;
		params.Fullscreen=true;
		params.Stencilbuffer=true;
		params.Vsync=true;
		params.AntiAlias=8;
		params.EventReceiver=0;
		
	gameWindow = createDeviceEx(params);

	if (!gameWindow)
	{
		//TODO Display error message?
		return false;
	}

	gameWindow->setWindowCaption(L"Blackout");
	
	fileSystem = gameWindow->getFileSystem();
	vdriver = gameWindow->getVideoDriver();
	scnmgr = gameWindow->getSceneManager();
	guienv = gameWindow->getGUIEnvironment();

	//Initalise variables for screen size 
	scrnHeight = vdriver->getScreenSize().Height; 
	scrnWidth = vdriver->getScreenSize().Width;
	scrnCentreY = scrnHeight/2;
	scrnCentreX = scrnWidth/2;

	//Disable mipmaps and optimise texture quality
	vdriver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);
	vdriver->setTextureCreationFlag(ETCF_OPTIMIZED_FOR_QUALITY, true);

	//Set Fog
	vdriver->setFog(SColor(0,0,0,0), EFT_FOG_EXP, 250, 1000, .002f, true, true);

	return true;
}

bool initDebugDevice()
{	
	audio = createIrrKlangDevice();

	if(!audio)
	{
		return false;
	}

	gameWindow =
		createDevice(EDT_OPENGL, dimension2d<u32>(1280, 720), 32,
			false, false, false, 0);

	if (!gameWindow)
	{
		//TODO Display error message?
		return false;
	}

	gameWindow->setWindowCaption(L"Blackout");
	
	fileSystem = gameWindow->getFileSystem();
	vdriver = gameWindow->getVideoDriver();
	scnmgr = gameWindow->getSceneManager();
	guienv = gameWindow->getGUIEnvironment();

	//Initalise variables for screen size 
	scrnHeight = vdriver->getScreenSize().Height; 
	scrnWidth = vdriver->getScreenSize().Width;
	scrnCentreY = scrnHeight/2;
	scrnCentreX = scrnWidth/2;

	//Disable mipmaps and optimise texture quality
	vdriver->setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, false);
	vdriver->setTextureCreationFlag(ETCF_OPTIMIZED_FOR_QUALITY, true);

	//Set Fog
	vdriver->setFog(SColor(0,0,0,0), EFT_FOG_EXP, 250, 1000, .002f, true, true);

	return true;
}

void createMainMenu()
{
	currentLevel = 0;
	gamePaused = false;
	levelLoaded = false;
	//Load and scale menu image
	ITexture* menuTex = vdriver->getTexture("assets/textures/blackoutMenu.bmp");
	IGUIImage* menuScreen = guienv->addImage(rect<s32>(0,0, scrnWidth, scrnHeight));
	menuScreen->setImage(menuTex);
	menuScreen->setScaleImage(true);
	vdriver->removeTexture(menuTex);
	
	//Set up gui

	IGUISkin* skin = guienv->getSkin();
    if (font)
        skin->setFont(font);
	
	guienv->addButton(rect<s32>(scrnWidth/7, (scrnHeight/5) * 4.2, (scrnWidth/7) + (scrnWidth/6), (scrnHeight/5) * 4.2 + (scrnHeight/14)), 0 , GUI_ID_NEW_GAME_BUTTON,
		L"New Game");

	guienv->addButton(rect<s32>((scrnWidth/5) * 3.5,(scrnHeight/5) * 4.2,(scrnWidth/5)* 3.5 + (scrnWidth/6),(scrnHeight/5)* 4.2 + (scrnHeight/14)), 0, GUI_ID_QUIT_BUTTON,
            L"Quit to Desktop");
}

void createLevelCollisions(float cameraX, float cameraY, float cameraZ)
{
	//Camera set-up
	camera = addMyFPSCamera(); //Create camera
	camera->setPosition(vector3df(cameraX,cameraY,cameraZ)); //Set starting position of camera
	gameWindow->getCursorControl()->setVisible(false); //Set visibility of cursor
	
	// Create a meta triangle selector to hold several triangle selectors.
	meta = scnmgr->createMetaTriangleSelector();

	//Create triangle selectors for suitable nodes
	array<ISceneNode *> nodes;
    scnmgr->getSceneNodesFromType(ESNT_ANY, nodes); // Find all nodes

    for (u32 i=0; i < nodes.size(); ++i)
    {
        ISceneNode * node = nodes[i];
        ITriangleSelector * selector = 0;
		stringc name;
		name = node->getName();	

        switch(node->getType())
        {
        case ESNT_CUBE:
        case ESNT_ANIMATED_MESH:
            // Because the selector won't animate with the mesh,
            // and is only being used for camera collision, we'll just use an approximate
            // bounding box instead of ((IAnimatedMeshSceneNode*)node)->getMesh(0)
            selector = scnmgr->createTriangleSelectorFromBoundingBox(node);
        break;
		
        case ESNT_MESH:
			if(name.find("Room") != -1 || name.find("pod") != -1 || name.find("control") != -1)
			{
				selector = scnmgr->createTriangleSelector(((IMeshSceneNode*)node)->getMesh(), node);
			}
			else
			{
				selector = scnmgr->createTriangleSelectorFromBoundingBox(node);
			}
			break;
        case ESNT_SPHERE: // Derived from IMeshSceneNode
			selector = scnmgr->createTriangleSelector(((IMeshSceneNode*)node)->getMesh(), node);		
            break;

        case ESNT_TERRAIN:
            selector = scnmgr->createTerrainTriangleSelector((ITerrainSceneNode*)node);
            break;

        case ESNT_OCTREE:
            selector = scnmgr->createOctreeTriangleSelector(((IMeshSceneNode*)node)->getMesh(), node);
            break;

        default:
            // Don't create a selector for this node type
            break;
        }

        if(selector)
        {
			node->setTriangleSelector(selector);
            // Add it to the meta selector, which will take a reference to it
            meta->addTriangleSelector(selector);
            // And drop my reference to it, so that the meta selector owns it.
            selector->drop();
        }
    }
	
	collision = scnmgr->createCollisionResponseAnimator(
		meta, camera, core::vector3df(20,34,20), // this is how big you are
		core::vector3df(0,gSpeed,0), vector3df(0,32.5,0));

	//collision = new MyCollisionAnimator(scnmgr, meta, camera, vector3df(20,34,20),
 //       vector3df(0,gSpeed,0), vector3df(0,32.5,0));
    
	//Attach collision response animator to camera
    camera->addAnimator(collision);
 
}

void loadPauseMenu()
{
	//Load and scale menu image
	ITexture* pauseTex = vdriver->getTexture("assets/textures/blackoutPauseMenu.png");
	IGUIImage* pauseScreen = guienv->addImage(rect<s32>(0,0, scrnWidth, scrnHeight));
	pauseScreen->setImage(pauseTex);
	pauseScreen->setScaleImage(true);
	vdriver->removeTexture(pauseTex);
	
	//Set up gui
	IGUISkin* skin = guienv->getSkin();
    if (font)
        skin->setFont(font);
	
	guienv->addButton(rect<s32>(scrnWidth/7, (scrnHeight/5) * 4.2, (scrnWidth/7) + (scrnWidth/6), (scrnHeight/5) * 4.2 + (scrnHeight/14)), 0 , GUI_ID_SAVE_AND_QUIT_BUTTON,
		L"Quit to Menu");

	guienv->addButton(rect<s32>((scrnWidth/5) * 3.5,(scrnHeight/5) * 4.2,(scrnWidth/5)* 3.5 + (scrnWidth/6),(scrnHeight/5)* 4.2 + (scrnHeight/14)), 0, GUI_ID_QUIT_BUTTON,
        L"Quit to Desktop");
}

void destroy_levelOne()
{
	currentLevel = -1;
	for (int i=0;i<numdoors;i++)
	{
		if (door[i]) delete door[i];
	}

	fileSystem->removeFileArchive("labTex.zip");
	fileSystem->removeFileArchive("Lab1.zip");

	array<ISceneNode *> nodes;
    scnmgr->getSceneNodesFromType(ESNT_ANY, nodes); // Find all nodes

	for (u32 i=0; i < nodes.size(); ++i)
    {
        ISceneNode * node = nodes[i];
		node->remove();
	}

	camera->removeAll();
}

bool load_levelOne()
{
	levelLoaded = false;
	bool success = true;

	labKeycardCollected = false;
	doorPowerOn = false;

	guienv->clear();
	loadPauseMenu();

	//Load scene
	fileSystem->addFileArchive("assets/textures/labTex.zip");
	fileSystem->addFileArchive("assets/maps/Lab1.zip");
	
	if(!scnmgr->loadScene("lab1.irr"))
	{
		success = false;
	}

	vec3df fireLightPos = scnmgr->getSceneNodeFromName("fireLight")->getAbsolutePosition();

	createFire(0, fireLightPos.X, fireLightPos.Y-20, fireLightPos.Z-5);

	ISound* fireSound = audio->play3D("assets/audio/fire.wav", fireLightPos,
								true, false, true);

	if(fireSound)
	{
		fireSound->setMinDistance(15.0f);
	}
								
	vec3df powerHumPos = scnmgr->getSceneNodeFromName("powerControls")->getAbsolutePosition();

	ISound* powerHum = audio->play3D("assets/audio/transformer.mp3",
                               powerHumPos, true, false, true);
	if(powerHum)
	{
		powerHum->setMinDistance(30.0f);
	}

	//Set material flags on all scene nodes
	array<ISceneNode *> nodes;
    scnmgr->getSceneNodesFromType(ESNT_ANY, nodes); // Find all nodes

	for (u32 i=0; i < nodes.size(); ++i)
    {
        ISceneNode * node = nodes[i];
		node->setMaterialFlag(EMF_FOG_ENABLE, true);
    }

	createLevelCollisions(200.0f, 10.0f, -930.0f);

	ISceneNode* doornode = scnmgr->getSceneNodeFromName("containmentDoor");
	if(doornode)
	{
		door[0] = new Door(scnmgr, doornode, doornode->getPosition(), vector3df(0.0, 100.0, 0.0));
	}
	
	doornode = scnmgr->getSceneNodeFromName("labkeycardDoor");
	if(doornode)
	{
		door[1] = new Door(scnmgr, doornode, doornode->getPosition(), vector3df(0.0, 100.0, 0.0));
	}

	ISceneNode* lightBulb = scnmgr->getSceneNodeFromName("bulb_01");

	lightBulb->setMaterialFlag(EMF_LIGHTING, true);
	lightBulb->getMaterial(0).EmissiveColor = SColor(255,255,255,255);

	lightBulb = scnmgr->getSceneNodeFromName("emergencyBulb_01");

	lightBulb->setMaterialFlag(EMF_LIGHTING, true);
	lightBulb->getMaterial(0).EmissiveColor = SColor(0,255,255,255);

	lightBulb = scnmgr->getSceneNodeFromName("bulb_05");

	lightBulb->setMaterialFlag(EMF_LIGHTING, true);
	lightBulb->getMaterial(0).EmissiveColor = SColor(255,255,255,255);

	lightBulb = scnmgr->getSceneNodeFromName("hologram_01");

	lightBulb->setMaterialFlag(EMF_LIGHTING, true);
	lightBulb->getMaterial(0).EmissiveColor = SColor(255,255,255,255);

	lightBulb = scnmgr->getSceneNodeFromName("hologram_01_back");

	doornode = 0;
	lightBulb = 0;

	keycardSwitches[0] = scnmgr->getSceneNodeFromName("labKeycardSwitch_01");
	keycardSwitches[1] = scnmgr->getSceneNodeFromName("labKeycardSwitch_02");

	gamePaused = false;
	return success;
}

int main()
{
	//TODO Use this code when game is more complete to match window size to screen resolution
	//This code finds the resolution that the user is currently running at

	//Standard device initialisation
	//initDevice();

	initDebugDevice();

	//Load font
	font = guienv->getFont("assets/fonts/fontgoodtimes10px.xml");

	//Load sounds
	sFootstep = audio->getSoundSource("assets/audio/step.wav");
	sFootstep->setDefaultVolume(0.6f);
	footstep = audio->play2D(sFootstep, true, true);
	sDoorMove = audio->getSoundSource("assets/audio/door.wav");
	sNoPower = audio->getSoundSource("assets/audio/nopower.wav");
	sAccessDenied = audio->getSoundSource("assets/audio/access_denied.wav");
	sAccessDenied->setDefaultVolume(0.8f);
	sAccessGranted = audio->getSoundSource("assets/audio/access_granted.wav");
	sAccessGranted->setDefaultVolume(0.6f);
	sButtonPress = audio->getSoundSource("assets/audio/button.wav");
	sPowerUp = audio->getSoundSource("assets/audio/powerup.wav");
	sPowerUp->setDefaultVolume(0.6f);

	//Create Main Menu
	createMainMenu();
	
	// Store the appropriate data in a context structure.
    SAppContext context;
    context.device = gameWindow;
	context.counter = 0;
	// Then create the event receiver, giving it that context structure.
    EventReceiver receiver(context);
    // And tell the device to use our custom event receiver.
    gameWindow->setEventReceiver(&receiver);

	//Create crosshair texture
	ITexture* crosshair = vdriver->getTexture("assets/textures/crosshair.png");
	//vdriver->makeColorKeyTexture(crosshair, position2d<s32>(1,1));

	//Create variables for crosshair position 
	int crosshairX = (scrnWidth/2) - (crosshair->getSize().Width/2); 
	int crosshairY = (scrnHeight/2) - (crosshair->getSize().Height/2);

	//Create and initalise lastfps value
	int lastFPS = -1;

	//Create and initialise then value
	u32 then = gameWindow->getTimer()->getTime();

	//Running the device in a while loop until device no longer wants to run.
	//(i.e when the window is closed)	
	while(gameWindow->run())
	{
		/*
		Drawing takes place between beginScene() and endScene(). Begin
		clears screen with colour and depth buffer. Then scene manager and
		gui environment draw their content. End displays everything on screen.
		*/
		if (gameWindow->isWindowActive())
        {
			const u32 now = gameWindow->getTimer()->getTime();
			const f32 frameDeltaTime = (f32)(now - then) / 1000.f; // Time in seconds
			then = now;	

			vdriver->beginScene(true, true, SColor(255,255,255,255));
			scnmgr->drawAll();
	
			if(currentLevel == 0 && receiver.getGameStarted()) //Determines whether new game button has been pressed
			{
					guienv->clear();
					//Load and scale menu image
					ITexture* loadTex = vdriver->getTexture("assets/textures/loadScreen.png");
					IGUIImage* loadScreen = guienv->addImage(rect<s32>(0,0, scrnWidth, scrnHeight));
					loadScreen->setImage(loadTex);
					loadScreen->setScaleImage(true);
					vdriver->removeTexture(loadTex);
					currentLevel = 1;					
			}
			if(currentLevel == 0 || !levelLoaded) //Determines whether menu or loading screen should be displayed
				guienv->drawAll();	
			if(currentLevel > 0) //Determines if game is not on menu
			{
				if (gamePaused) //Is game paused?
				{
					guienv->drawAll(); //Draw pause menu
				}
				
				if(!gamePaused && levelLoaded) //Are we in game, not paused?
				{
					vdriver->draw2DImage(crosshair, position2d<s32>(crosshairX, crosshairY), rect<s32>(0,0,32,32), 0, SColor(255,255,255,255), true); //Draw Crosshair
				}
				if(canInteract) //Can we interact with what we're looking at?
					font->draw(L"Press E to interact",rect<s32>(scrnCentreX-scrnCentreX/2,scrnCentreY+scrnCentreY/2, scrnCentreX+scrnCentreX/2,(scrnCentreY+scrnCentreY/2)+scrnCentreY/4), SColor(255,255,255,255), true, true); //Draw interact text
			}
			vdriver->endScene();

			/////////////////////////////////////////////
			int fps = vdriver->getFPS();

			if(lastFPS != fps)
			{
				stringw str = L"Blackout  [";
				str += vdriver->getName();
				str	+= "] FPS:";
				str += fps;

				gameWindow->setWindowCaption(str.c_str());
				lastFPS = fps;
			}
			/////////////////////////////////////////////

			if(currentLevel == 1 && !levelLoaded) //Has the new game button been pressed, but level not loaded?
			{
				if(load_levelOne()) //Then load level
				{
					levelLoaded = true;		
				}	
			}
			
			if(currentLevel == 1 && levelLoaded) //Are we on level one?
			{
				//Level 1 code
				ISceneNode* selectedNode = raycast(1000.0f); //Raycast from player
				if(selectedNode)
				{
					stringc name = selectedNode->getName();
					if(name.find("Switch") != -1 || name.find("Keycard") != -1 || name.find("power") != -1)
					{
						vector3df cpos = camera->getPosition();
						vector3df pos = intersection;
						float dist = (cpos-pos).getLength();

						if(dist < useDist)
						{
							canInteract = true;
						}
						else
						{
							canInteract = false;
						}
					}
					else
					{
							canInteract = false;
					}

					if(receiver.IsKeyDown(KEY_KEY_E))
					{
						if(!lastFramePressed)
						{
							printf(selectedNode->getName());
							//Item activated
							touchItem(selectedNode, intersection);
						}

						lastFramePressed = true;
					}
					else
					{
						lastFramePressed = false;
					}
				}

				if(!gamePaused)
				{
					u32 fst = footstep->getPlayPosition();

					if((receiver.IsKeyDown(KEY_KEY_W) || receiver.IsKeyDown(KEY_KEY_S)) || (receiver.IsKeyDown(KEY_KEY_A) || receiver.IsKeyDown(KEY_KEY_D)))
					{
						footstep->setIsPaused(false);
					}
					else if ((fst >= 0 && fst <= 35) || (fst >= 200 && fst <= 400) || (fst >= 600 && fst <= 800) || (fst >= 1000 && fst <= 1200) || (fst >= 1450 && fst <= 1650))
					{
						//Only executed in between steps.
						//This prevents the sound cutting out in the middle of a footstep
						footstep->setIsPaused(true);
						footstep->setPlayPosition(0);
					}
				}

				// Stuck in geometry?
				aabbox3d<f32> box1 = camera->getBoundingBox();
				for (int i=0;i<numdoors;i++) 
				{
					if (!door[i]) continue;

					aabbox3d<f32> box2 = door[i]->getTransformedBoundingBox();

					box1.MinEdge = camera->getPosition()-collision->getEllipsoidRadius()-collision->getEllipsoidTranslation();
					box1.MaxEdge = camera->getPosition()+collision->getEllipsoidRadius()+collision->getEllipsoidTranslation();	

					if (box1.intersectsWithBox(box2)) 
					{
						door[i]->collisionResponse(3);			
					}
				}			
			}

			if (currentLevel > 0 && levelLoaded)
			{
				audio->setListenerPosition(camera->getAbsolutePosition(), camera->getRotation());

				if(receiver.IsKeyDown(KEY_ESCAPE))
				{
					if(!lastFramePaused)
					{
						if(!gamePaused)
						{
							gamePaused = true;
							gameWindow->getCursorControl()->setVisible(true);
							camera->setInputReceiverEnabled(false);
							audio->setAllSoundsPaused();
						}
						else if (gamePaused)
						{
							gamePaused = false;
							gameWindow->getCursorControl()->setVisible(false);
							camera->setInputReceiverEnabled(true);
							audio->setAllSoundsPaused(false);
						}
					}

					lastFramePaused = true;
				}
				else
				{
					lastFramePaused = false;
				}

				if(gamePaused)
				{
					if(receiver.getQuitToMenu())
					{
						guienv->clear();
						destroy_levelOne();
						createMainMenu();
					}
				}
				//What is the value of quit to menu?
				//If quitting to menu, levelLoaded must be set to false & gui must be cleared before loading menu again
			}
		}
		else
		{
			gameWindow->yield();
		}
	}

	gameWindow->drop();
	audio->drop();
	return 0;
}