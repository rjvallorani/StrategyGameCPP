# StrategyGameCPP
Contains the functionality for a RTS game created using C++

SPlayerPawn
 
-This is a camera pawn that allows the player to move across the map, using both keys and edge scrolling. The player is also able to rotate the camera by holding down the middle mouse button, zoom in and out by scrolling, or rotate incrementally using keys. Included is a terrain mapping function which will change the yaw of the camera depending on the terrain, preventing the camera from clipping into the map.

Placement Preview
-A class that is referenced when the player is spawning in new units to the game to give a preview of the unit being placed.

Selection Box
-This is a click and drag box that allows the player to select multiple units at a time. Can be used to select or deselect. Has terrain functionality so that the box does not clip into hills or valleys when dragging across differing terrain.

SPlayerController
-Custom player controller with enhanced input capabilities.

