#include "pch.h"

#include "chipmunk/chipmunk.h"

#include "Phys.h"
#include "Game.h"



cpShapeFilter GRAB_FILTER = { CP_NO_GROUP, GRABBABLE_MASK_BIT, GRABBABLE_MASK_BIT };
cpShapeFilter NOT_GRABBABLE_FILTER = { CP_NO_GROUP, ~GRABBABLE_MASK_BIT, ~GRABBABLE_MASK_BIT };

// Physics


void PostStepAddJoint(cpSpace *space, void *key, void *data)
{
//	printf("Adding joint for %p\n", data);
	
	cpConstraint *joint = (cpConstraint *)key;
	cpSpaceAddConstraint(space, joint);
}

cpBool StickyPreSolve(cpArbiter *arb, cpSpace *space, void *data)
{
	// We want to fudge the collisions a bit to allow shapes to overlap more.
	// This simulates their squishy sticky surface, and more importantly
	// keeps them from separating and destroying the joint.
	
	// Track the deepest collision point and use that to determine if a rigid collision should occur.
	cpFloat deepest = INFINITY;
	
	// Grab the contact set and iterate over them.
	cpContactPointSet contacts = cpArbiterGetContactPointSet(arb);
	for(int i=0; i<contacts.count; i++){
		// Sink the contact points into the surface of each shape.
		contacts.points[i].pointA = cpvsub(contacts.points[i].pointA, cpvmult(contacts.normal, STICK_SENSOR_THICKNESS));
		contacts.points[i].pointB = cpvadd(contacts.points[i].pointB, cpvmult(contacts.normal, STICK_SENSOR_THICKNESS));
		deepest = cpfmin(deepest, contacts.points[i].distance);// + 2.0f*STICK_SENSOR_THICKNESS);
	}
	
	// Set the new contact point data.
	cpArbiterSetContactPointSet(arb, &contacts);
	
	// If the shapes are overlapping enough, then create a
	// joint that sticks them together at the first contact point.
	if(!cpArbiterGetUserData(arb) && deepest <= 0.0f){
		CP_ARBITER_GET_BODIES(arb, bodyA, bodyB);
	
		// Create a joint at the contact point to hold the body in place.
		cpVect anchorA = cpBodyWorldToLocal(bodyA, contacts.points[0].pointA);
		cpVect anchorB = cpBodyWorldToLocal(bodyB, contacts.points[0].pointB);
		cpConstraint *joint = NULL;

        BodyState *bsA = (BodyState*) cpBodyGetUserData(bodyA);
        BodyState *bsB = (BodyState*) cpBodyGetUserData(bodyB);
		if ( bsA && bsB && bsA->group_id == bsB->group_id ) {
			joint = cpPivotJointNew2(bodyA, bodyB, anchorA, anchorB);

			// Give it a finite force for the stickyness.
			cpConstraintSetMaxForce(joint, 4e3);

			// Schedule a post-step() callback to add the joint.
			cpSpaceAddPostStepCallback(space, PostStepAddJoint, joint, NULL);

			// Store the joint on the arbiter so we can remove it later.
			cpArbiterSetUserData(arb, joint);
            Game *game = bsA->game;
            game->onBodyJointed(bodyA,bodyB);
		} else {
            Game *game = bsA->game;
            game->onBodyCollide(bodyA,bodyB);
        }
	}
	
	// Position correction and velocity are handled separately so changing
	// the overlap distance alone won't prevent the collision from occuring.
	// Explicitly the collision for this frame if the shapes don't overlap using the new distance.
	return (deepest <= 0.0f);
	
	// Lots more that you could improve upon here as well:
	// * Modify the joint over time to make it plastic.
	// * Modify the joint in the post-step to make it conditionally plastic (like clay).
	// * Track a joint for the deepest contact point instead of the first.
	// * Track a joint for each contact point. (more complicated since you only get one data pointer).
}

void PostStepRemoveJoint(cpSpace *space, void *key, void *data)
{
//	printf("Removing joint for %p\n", data);
	
	cpConstraint *joint = (cpConstraint *)key;
	cpSpaceRemoveConstraint(space, joint);
	cpConstraintFree(joint);
}

void StickySeparate(cpArbiter *arb, cpSpace *space, void *data)
{
	cpConstraint *joint = (cpConstraint *)cpArbiterGetUserData(arb);
	
	if(joint){
		// The joint won't be removed until the step is done.
		// Need to disable it so that it won't apply itself.
		// Setting the force to 0 will do just that
		cpConstraintSetMaxForce(joint, 0.0f);
		
		// Perform the removal in a post-step() callback.
		cpSpaceAddPostStepCallback(space, PostStepRemoveJoint, joint, NULL);
		
		// NULL out the reference to the joint.
		// Not required, but it's a good practice.
		cpArbiterSetUserData(arb, NULL);

        cpBody *bodyA = cpConstraintGetBodyA(joint);
        cpBody *bodyB = cpConstraintGetBodyB(joint);        
        BodyState *bs = (BodyState*) cpBodyGetUserData(bodyA);
        Game *game = bs->game;
        game->onBodySeparated( bodyA, bodyB );
	}
}

void PhysUpdateSpace(cpSpace *space, double dt)
{
	cpSpaceStep(space, dt);
}

static cpFloat springForce(cpConstraint *spring, cpFloat dist)
{
	cpFloat clamp = 200000.0f;
	return cpfclamp(cpDampedSpringGetRestLength(spring) - dist, -clamp, clamp)*cpDampedSpringGetStiffness(spring);
}


cpConstraint *new_spring(cpBody *a, cpBody *b, cpVect anchorA, cpVect anchorB, cpFloat restLength, cpFloat stiff, cpFloat damp)
{
	cpConstraint *spring = cpDampedSpringNew(a, b, anchorA, anchorB, restLength, stiff, damp);
	cpDampedSpringSetSpringForceFunc(spring, springForce);
	
	return spring;
}
