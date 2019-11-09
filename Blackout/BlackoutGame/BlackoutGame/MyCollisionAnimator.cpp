#include "MyCollisionAnimator.h"
#include "ISceneCollisionManager.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "ITimer.h"

namespace irr
{
namespace scene
{

//! constructor
MyCollisionAnimator::MyCollisionAnimator(
		ISceneManager* scenemanager,
		ITriangleSelector* world, ISceneNode* object,
		const core::vector3df& ellipsoidRadius,
		const core::vector3df& gravityPerSecond,
		const core::vector3df& ellipsoidTranslation,
		f32 slidingSpeed)
: Radius(ellipsoidRadius), Gravity(gravityPerSecond), Translation(ellipsoidTranslation),
	World(world), Object(object), SceneManager(scenemanager), LastTime(0),
	SlidingSpeed(slidingSpeed), CollisionNode(0), CollisionCallback(0),
	Falling(false), IsCamera(false), AnimateCameraTarget(true), CollisionOccurred(false),
	FirstUpdate(true)
{
	#ifdef _DEBUG
	setDebugName("MyCollisionAnimator");
	#endif

	if (World)
		World->grab();

	setNode(Object);
}


//! destructor
MyCollisionAnimator::~MyCollisionAnimator()
{
	if (World)
		World->drop();

	if (CollisionCallback)
		CollisionCallback->drop();
}


//! Returns if the attached scene node is falling, which means that
//! there is no blocking wall from the scene node in the direction of
//! the gravity.
bool MyCollisionAnimator::isFalling() const
{
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return Falling;
}


//! Sets the radius of the ellipsoid with which collision detection and
//! response is done.
void MyCollisionAnimator::setEllipsoidRadius(
	const core::vector3df& radius)
{
	Radius = radius;
	FirstUpdate = true;
}


//! Returns the radius of the ellipsoid with wich the collision detection and
//! response is done.
core::vector3df MyCollisionAnimator::getEllipsoidRadius() const
{
	return Radius;
}


//! Sets the gravity of the environment.
void MyCollisionAnimator::setGravity(const core::vector3df& gravity)
{
	Gravity = gravity;
	FirstUpdate = true;
}


//! Returns current vector of gravity.
core::vector3df MyCollisionAnimator::getGravity() const
{
	return Gravity;
}


//! 'Jump' the animator, by adding a jump speed opposite to its gravity
void MyCollisionAnimator::jump(f32 jumpSpeed)
{
	FallingVelocity -= (core::vector3df(Gravity).normalize()) * jumpSpeed;
	Falling = true;
}


//! Sets the translation of the ellipsoid for collision detection.
void MyCollisionAnimator::setEllipsoidTranslation(const core::vector3df &translation)
{
	Translation = translation;
}


//! Returns the translation of the ellipsoid for collision detection.
core::vector3df MyCollisionAnimator::getEllipsoidTranslation() const
{
	return Translation;
}


//! Sets a triangle selector holding all triangles of the world with which
//! the scene node may collide.
void MyCollisionAnimator::setWorld(ITriangleSelector* newWorld)
{
	if (newWorld)
		newWorld->grab();

	if (World)
		World->drop();

	World = newWorld;

	FirstUpdate = true;
}


//! Returns the current triangle selector containing all triangles for
//! collision detection.
ITriangleSelector* MyCollisionAnimator::getWorld() const
{
	return World;
}


void MyCollisionAnimator::animateNode(ISceneNode* node, u32 timeMs)
{

	CollisionOccurred = false;

	if (node != Object)
		setNode(node);

	if(!Object || !World)
		return;

	// trigger reset
	if ( timeMs == 0 )
	{
		FirstUpdate = true;
		timeMs = LastTime;
	}

	if ( FirstUpdate )
	{
		LastPosition = Object->getPosition();
		Falling = false;
		LastTime = timeMs;
		FallingVelocity.set ( 0, 0, 0 );

		FirstUpdate = false;
	}

	const u32 diff = timeMs - LastTime;
	LastTime = timeMs;

	CollisionResultPosition = Object->getPosition();
	core::vector3df vel = CollisionResultPosition - LastPosition;

	FallingVelocity += Gravity * (f32)diff * 0.001f;

	CollisionTriangle = RefTriangle;
	CollisionPoint = core::vector3df();
	CollisionResultPosition = core::vector3df();
	CollisionNode = 0;

	//core::vector3df force = vel + FallingVelocity;

	if ( AnimateCameraTarget )
	{
		// TODO: divide SlidingSpeed by frame time
		f32 mSlidingSpeed = SlidingSpeed/(f32)diff;

		bool f = false;
		CollisionResultPosition
			= SceneManager->getSceneCollisionManager()->getCollisionResultPosition(
				World, LastPosition-Translation,
				Radius, vel, CollisionTriangle, CollisionPoint, f,
				CollisionNode, mSlidingSpeed, FallingVelocity * (f32)diff * 0.02f);

		CollisionOccurred = (CollisionTriangle != RefTriangle);

		CollisionResultPosition += Translation;

		if (f)//CollisionTriangle == RefTriangle)
		{
			Falling = true;
		}
		else
		{
			Falling = false;
			FallingVelocity.set(0, 0, 0);
		}

		bool collisionConsumed = false;

		if (CollisionOccurred && CollisionCallback)
			collisionConsumed = CollisionCallback->onCollision(*this);

		if(!collisionConsumed)
			Object->setPosition(CollisionResultPosition);
	}

	// move camera target
	if (AnimateCameraTarget && IsCamera)
	{
		const core::vector3df pdiff = Object->getPosition() - LastPosition - vel;
		ICameraSceneNode* cam = (ICameraSceneNode*)Object;
		cam->setTarget(cam->getTarget() + pdiff);
	}

	LastPosition = Object->getPosition();
}


void MyCollisionAnimator::setNode(ISceneNode* node)
{
	Object = node;

	if (Object)
	{
		LastPosition = Object->getPosition();
		IsCamera = (Object->getType() == ESNT_CAMERA);
	}

	/*LastTime = os::Timer::getTime();*/
}


//! Writes attributes of the scene node animator.
void MyCollisionAnimator::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addVector3d("Radius", Radius);
	out->addVector3d("Gravity", Gravity);
	out->addVector3d("Translation", Translation);
	out->addBool("AnimateCameraTarget", AnimateCameraTarget);
}


//! Reads attributes of the scene node animator.
void MyCollisionAnimator::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Radius = in->getAttributeAsVector3d("Radius");
	Gravity = in->getAttributeAsVector3d("Gravity");
	Translation = in->getAttributeAsVector3d("Translation");
	AnimateCameraTarget = in->getAttributeAsBool("AnimateCameraTarget");
}


ISceneNodeAnimator* MyCollisionAnimator::createClone(ISceneNode* node, ISceneManager* newManager)
{
	if (!newManager) newManager = SceneManager;

	MyCollisionAnimator * newAnimator =
		new MyCollisionAnimator(newManager, World, Object, Radius,
				(Gravity * 1000.0f), Translation, SlidingSpeed);

	return newAnimator;
}

void MyCollisionAnimator::setCollisionCallback(ICollisionCallback* callback)
{
	if ( CollisionCallback == callback )
		return;

	if (CollisionCallback)
		CollisionCallback->drop();

	CollisionCallback = callback;

	if (CollisionCallback)
		CollisionCallback->grab();
}

//! Should the Target react on collision ( default = true )
void MyCollisionAnimator::setAnimateTarget ( bool enable )
{
	AnimateCameraTarget = enable;
	FirstUpdate = true;
}

//! Should the Target react on collision ( default = true )
bool MyCollisionAnimator::getAnimateTarget () const
{
	return AnimateCameraTarget;
}

} // end namespace scene
} // end namespace irr