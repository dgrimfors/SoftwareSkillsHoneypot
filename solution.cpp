#ifndef Solution_h
#define Solution_h
#include "API.h"

#include <iostream>
using namespace std;

class API;

#define MAP_SIZE 100
#define FIRE_DISTANCE 3
#define ACTION_HISTORY_LENGTH 10


enum MapValue
{
  UNKNOWN = 0,
  EMPTY = 1,
  BLOCK = 2, 
  WALL = 3, 
  DRONE = 4
};

enum Direction
{
  NORTH = 0,
  EAST = 1,
  SOUTH = 2,
  WEST = 3,
  DIR_MAX = 4
};

char MapValueStrings[][10] = {"Unknown", "Empty", "Block", "Wall", "Drone"};
char RelDirStrings[][10] = {"Front", "Right", "Back", "Left"};
char DirStrings[][10] = {"North", "East", "South", "West"};

enum RelativeDirection
{
  FRONT = 0,
  RIGHT = 1,
  BACK = 2,
  LEFT = 3,
  RELDIR_MAX = 4
};

struct Target{
  int distance;
  RelativeDirection direction;
  MapValue type;
};

enum Action
{
  WAIT,
  COVER,
  FIRE,
  TURN_LEFT,
  TURN_RIGHT,
  MOVE_FORWARD,
  MOVE_BACKWARD
};

class Solution {
  int posX = MAP_SIZE/2;
    int posY = MAP_SIZE/2;
    int targetDistance = 0;
    Direction currentDir = NORTH;
    int estimatedFuel;
    
    bool wasAttacked = false;
    int counter = 0;
    int coveringTimer = 0;
    int map[MAP_SIZE][MAP_SIZE];
    int timeSinceAttacked = 999;
    int timeSinceFiring = 0;
    int timeFruitlessExploring = 0;
    
    int COVERTIME = 60;
    int EXPLORETIME = 40;
    Action actionHistory[ACTION_HISTORY_LENGTH];
    unsigned int lastActionIndex = 0;
    
 public:
  Solution() {
    // If you need initialization code, you can write it here!
    // Do not remove.
    
    for(int i = 0; i < MAP_SIZE; i++)
    {
      for(int j = 0; j < MAP_SIZE; j++)
      {
        map[i][j] = UNKNOWN;  
      }
    }
    
    estimatedFuel = API::currentFuel();
    timeSinceAttacked = 999;
    COVERTIME = estimatedFuel / 10;
    COVERTIME = COVERTIME > 50 ? 50 : COVERTIME;
  }
  
  //Returns true if target available, false otherwise, fills target struct if true
  bool getTarget(int map[MAP_SIZE][MAP_SIZE], int posX, int posY, Direction currentDir, Target* pTarget, Target allTargets[4])
  {
    Target target = {0, FRONT, UNKNOWN};
    bool foundTarget = false;
    int xFactor;
    int yFactor;
    int numDronesFound= 0;
    int  j;
    int evalX;
    int evalY;
    
     for(int i =0; i < RELDIR_MAX; i++)
    {
      switch((currentDir + i) % DIR_MAX)
      {
        case NORTH:
          xFactor = 0; 
          yFactor = 1;
          break;
        case EAST:
          xFactor = 1;
          yFactor = 0;
          break;
        case SOUTH:
          xFactor = 0; 
          yFactor = -1;
          break;
        case WEST:
          xFactor = -1;
          yFactor = 0;
          break;
      }
     
      for(j = 1; ;j++ )
      {
        evalX = posX + xFactor * j;
        evalY = posY + yFactor * j;
        
        if(evalX < 0 || evalX >= MAP_SIZE || evalY < 0 || evalY >= MAP_SIZE)
        {
          break;
        }
        if(map[evalX][evalY] == BLOCK || map[evalX][evalY] == WALL)
        {
          /*if(!foundTarget)
          {
              target.distance = j;
              target.direction = (RelativeDirection) i;
              foundTarget = true;
              target.type = BLOCK;
          }*/
          
          break;
        }
        if(map[evalX][evalY] == DRONE)
        {
          if(foundTarget)
          {
            if(j < target.distance || (target.direction == BACK && target.distance > 2) || target.type != DRONE)
            {
              target.distance = j;
              target.direction = (RelativeDirection) i;
              foundTarget = true;
              target.type = DRONE;
              numDronesFound++;
            }
          }
          else
          {
            target.distance = j;
            target.direction = (RelativeDirection) i;
            foundTarget = true;
            target.type = DRONE;
            numDronesFound++;
          }
          break;
        }
      }
      allTargets[i].distance = j;
      allTargets[i].direction = (RelativeDirection) i;
      allTargets[i].type = (MapValue) map[evalX][evalY];
    }
    
    pTarget->direction = target.direction;
    pTarget->distance = target.distance;
    pTarget->type = target.type;
    return foundTarget;
  }
  
  bool hasGoodCover()
  {
    return hasGoodCover(posX, posY, currentDir);
  }
  
  bool hasGoodCover(int x, int y, Direction dir)
  {
    int coveredSides = 0;
    int freeFiringDistance = 0;
    int xFactor;
    int yFactor;
      
    for(int i = 0; i < RELDIR_MAX; i++)
    {
      switch((dir + i) % DIR_MAX)
      {
        case NORTH:
          xFactor = 0; 
          yFactor = 1;
          break;
        case EAST:
          xFactor = 1;
          yFactor = 0;
          break;
        case SOUTH:
          xFactor = 0; 
          yFactor = -1;
          break;
        case WEST:
          xFactor = -1;
          yFactor = 0;
          break;
      }
     
      int evalDistance = ((i == 0) ? 3 : 1);
      for(int j = 1; j <= evalDistance; j++)
      {
        int evalX = x + xFactor * j;
        int evalY = y + yFactor * j;
        
        if(evalX < 0 || evalX >= MAP_SIZE || evalY < 0 || evalY >= MAP_SIZE)
        {
          break;
        }
        
        if(i == 0)
        {
          if(map[evalX][evalY] != BLOCK && map[evalX][evalY] != WALL)
          {
            freeFiringDistance++;
          }
        }
        else if(map[evalX][evalY] == BLOCK || map[evalX][evalY] == WALL)
        {
          coveredSides++;
        }
      }
    }
    
    return (coveredSides >= 2 && freeFiringDistance >= 3);
  }
  
  void updateMap(int map[MAP_SIZE][MAP_SIZE], int posX, int posY, Direction currentDir)
  {
    map[posX][posY] = EMPTY;
    
    for(int i =0; i < DIR_MAX; i++)
    {
      int distance;
      int xFactor;
      int yFactor;
      MapValue mapVal = UNKNOWN;
      
      switch(i)
      {
        case 0:
          distance = API::lidarFront();
          mapVal = API::identifyTarget() ? DRONE : WALL;
          break;
        case 1:
          distance = API::lidarRight();
          mapVal = BLOCK;
          break;
        case 2:
          distance = API::lidarBack();
          mapVal = BLOCK;
          break;
        case 3:
          distance = API::lidarLeft();
          mapVal = BLOCK;
          break;
      }
      
      switch((currentDir + i) % DIR_MAX)
      {
        case NORTH:
          xFactor = 0; 
          yFactor = 1;
          break;
        case EAST:
          xFactor = 1;
          yFactor = 0;
          break;
        case SOUTH:
          xFactor = 0; 
          yFactor = -1;
          break;
        case WEST:
          xFactor = -1;
          yFactor = 0;
          break;
        default:
          printf("ERROR %d, %d, %d\n", (currentDir + i) % DIR_MAX, currentDir, i);
      }
      
      for(int j = 1; j < distance; j++)
      {
        if(map[posX+xFactor*j][posY+yFactor*j] != EMPTY)
        {
          printf("Updating map[%d][%d] from %s to %s\n", posX+xFactor*j, posY+yFactor*j, MapValueStrings[map[posX+xFactor*j][posY+yFactor*j]], MapValueStrings[EMPTY]);
          map[posX+xFactor*j][posY+yFactor*j] = EMPTY;
        }
      }
      
      if(mapVal == BLOCK && (map[posX+xFactor*distance][posY+yFactor*distance] == EMPTY))
      {
        mapVal = DRONE;
      }
      
      if(map[posX+xFactor*distance][posY+yFactor*distance] != mapVal && map[posX+xFactor*distance][posY+yFactor*distance] != WALL)
      {
        if(!(mapVal == BLOCK && map[posX+xFactor*distance][posY+yFactor*distance] == DRONE))
        {
          printf("Updating map[%d][%d] from %s to %s\n", posX+xFactor*distance, posY+yFactor*distance, MapValueStrings[map[posX+xFactor*distance][posY+yFactor*distance]], MapValueStrings[mapVal]);
          map[posX+xFactor*distance][posY+yFactor*distance] = mapVal;
        }
      }
    }
  }
  
  bool isUnderAttack()
  {
     bool attacked = false;
     
     attacked = (API::currentFuel() < estimatedFuel) ? true : false;
/*     if(!attacked && wasAttacked)
     {
       wasAttacked = false;
       attacked = true;
     }
     else
     {
       wasAttacked = attacked;
     }*/
     
     return attacked;
  }
  
  void turnLeft()
  {
    printf("Turn left\n");
    estimatedFuel -= 1;
    API::turnLeft();
    currentDir = (Direction) (((int) currentDir - 1 + DIR_MAX) % DIR_MAX);
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = TURN_LEFT;
  }
  
  void turnRight()
  {
    printf("Turn right\n");
    estimatedFuel -= 1;
    API::turnRight();
    currentDir = (Direction) (((int) currentDir + 1) % DIR_MAX);
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = TURN_RIGHT;
  }
  
  void moveForward()
  {
    printf("Move forward\n");
    posX += ((currentDir == EAST) ? 1 : ((currentDir == WEST) ? -1 : 0));
    posY += ((currentDir == NORTH) ? 1 : ((currentDir == SOUTH) ? -1 : 0));
    estimatedFuel -= 1;
    API::moveForward();
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = MOVE_FORWARD;
  }
  
  void moveBack()
  {
    printf("Move back\n");
    posX += ((currentDir == EAST) ? -1 : ((currentDir == WEST) ? 1 : 0));
    posY += ((currentDir == NORTH) ? -1 : ((currentDir == SOUTH) ? 1 : 0));
    estimatedFuel -= 1;
    API::moveBackward();
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = MOVE_BACKWARD;
  }
  
  void fireCannon()
  {
    printf("Fire\n");
    estimatedFuel -= 5;
    API::fireCannon();
    coveringTimer -= 1;
    coveringTimer = (coveringTimer < 0 ? 0 : coveringTimer);
    timeSinceFiring = 0;
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = FIRE;
  }
  
  void cover()
  {
    printf("Covering %d\n", coveringTimer);
    coveringTimer++;
    lastActionIndex = (lastActionIndex + 1) % ACTION_HISTORY_LENGTH;
    actionHistory[lastActionIndex] = COVER;
  }
  
  /**
   * Executes a single step of the tank's programming. The tank can only move, 
   * turn, or fire its cannon once per turn. Between each update, the tank's 
   * engine remains running and consumes 1 fuel. This function will be called 
   * repeatedly until there are no more targets left on the grid, or the tank 
   * runs out of fuel.
   */
  void update() {
    Target target;
    Target allTargets[4];
    bool foundTarget;
    
    counter++;
    timeSinceFiring++;
    printf("\n");
    std::printf("%d: Pos: %d, %d, Dir: %s, Front: %d Back: %d Left: %d Right %d, Fuel: %d, CoverTime: %d, timeSinceAttacked: %d, timeSinceFiring: %d\n", counter, posX, posY, DirStrings[currentDir], API::lidarFront(), API::lidarBack(), API::lidarLeft(), API::lidarRight(), API::currentFuel(), coveringTimer, timeSinceAttacked, timeSinceFiring);

    counter++;
    COVERTIME = API::currentFuel() / 10;
    COVERTIME = COVERTIME > 50 ? 50 : COVERTIME;
    
    updateMap(map, posX, posY, currentDir);
    foundTarget = getTarget(map, posX, posY, currentDir, &target, allTargets);
    
    for(int i = 0; i < 4; i++)
    {
      printf("Distance: %d, Dir: %s, Type: %s\n", allTargets[i].distance, RelDirStrings[allTargets[i].direction], MapValueStrings[allTargets[i].type]);
    }
    
    if(foundTarget == true)
    {
      timeFruitlessExploring = 0;
      
    }
    
    printf("Estimated fuel: %d, Current fuel: %d\n", estimatedFuel, API::currentFuel());
    if(isUnderAttack())
    {
      printf("Attack detected. ");
      estimatedFuel = API::currentFuel();
      timeSinceAttacked = 0;
      coveringTimer -= 5;
      coveringTimer = (coveringTimer < 0 ? 0 : coveringTimer);
      if(target.type == DRONE && target.distance == 1)
      {
        printf("Found attacker: Distance: %d, Dir: %s\n", target.distance, RelDirStrings[target.direction]);
      }
    }
    
    //Attacked. 
    //Fire if attacker in front. 
    //Else retreat if possible. 
    //Else fire if target close or closing from front.
    //Else turn to whatever side contains attacker. 
    //Else turn to whatever side might contain attacker.
    //Else attacker might be behind us, turn to side with longest firing range. 
    if(timeSinceAttacked <= 1) 
    {
      if(((allTargets[LEFT].type == DRONE && allTargets[LEFT].distance < 3) || (allTargets[RIGHT].type == DRONE && allTargets[RIGHT].distance < 3)) && (allTargets[BACK].type != DRONE && allTargets[BACK].distance > 1))
      {
        moveBack();
      }
      else if(API::identifyTarget() && API::lidarFront() <= 2)
      {
        fireCannon();
      }
      else if(API::identifyTarget() && API::lidarFront() < targetDistance)
      {
        fireCannon();
        targetDistance = 0;
      }
      else if(allTargets[LEFT].type == DRONE && allTargets[LEFT].distance == 1)
      {
        turnLeft();
      }
      else if(allTargets[RIGHT].type == DRONE && allTargets[RIGHT].distance == 1)
      {
        turnRight();
      }
      else if(API::lidarLeft() == 1 && allTargets[LEFT].type != WALL)
      {
        turnLeft();
        targetDistance = API::lidarFront();
      }
      else if(API::lidarRight() == 1 && allTargets[RIGHT].type != WALL)
      {
        turnRight();
        targetDistance = API::lidarFront();
      }
      else
      {
        if(API::lidarLeft() < API::lidarRight())
        {
          turnLeft();
        }
        else
        {
          turnRight();
        }
      }
    }
    //Continue retreating?
    else if(actionHistory[lastActionIndex] == MOVE_BACKWARD && timeSinceAttacked <= 4 && (allTargets[BACK].type != DRONE && allTargets[BACK].distance > 1))
    {
       moveBack();
    }
    //Not attacked. 
    //If target close
      //If on side and no target behind, retreat. 
      //Else, if target in front, fire.
      //Else, turn towards target.
    //If no target close by, check if target approaching from front, allowing longshot
    //Else, if good cover, wait a while
    //Else if clear in front, move forward
    //Else turn
    else 
    {
      if(((allTargets[LEFT].type == DRONE && allTargets[LEFT].distance < 3) || (allTargets[RIGHT].type == DRONE && allTargets[RIGHT].distance < 3)) && (allTargets[BACK].type != DRONE && allTargets[BACK].distance > 1))
      {
        moveBack();
      }
      else if(foundTarget)
      {
        printf("Aqcuired target\n");
        switch(target.direction)
        {
          case FRONT:
            if(target.distance <= FIRE_DISTANCE || target.distance < targetDistance)
            {
                fireCannon();
            }
            else if(coveringTimer < COVERTIME && hasGoodCover())
            {
              cover();
            }
            else
            {
              printf("Approaching target\n");
              targetDistance = target.distance - 1;
              moveForward();
            }
            break;
          case RIGHT:
            turnRight();
            break;
          case LEFT:
            turnLeft();
            break;
          case BACK:
            if(API::lidarLeft() > API::lidarRight())
            {
              turnLeft();
            }
            else
            {
              turnRight();
            }
            break;
        }
      }
      else if(API::identifyTarget())
      {
        if((API::lidarFront() < targetDistance) || API::lidarFront() <= FIRE_DISTANCE)
        {
          fireCannon();
          targetDistance = 0;
        }
        else
        {
          printf("Approach target\n");
          API::moveForward();
          targetDistance = API::lidarFront();
        }
      }
      else //Explore
      {
        targetDistance = 0;
        
        if(coveringTimer < COVERTIME && hasGoodCover()) //Cover
        {
          cover();
        }
        else if(timeFruitlessExploring > EXPLORETIME) //Meticulous explore
        {
          printf("Meticulous explore\n");
          RelativeDirection recommendedDirection = FRONT;
          int distToUnknown = 0;
          if(allTargets[LEFT].type == UNKNOWN || allTargets[LEFT].type == BLOCK)
          {
            distToUnknown = allTargets[LEFT].distance;
            recommendedDirection = LEFT;
          }
          if(allTargets[RIGHT].type == UNKNOWN || allTargets[RIGHT].type == BLOCK)
          {
            recommendedDirection = (allTargets[RIGHT].distance > distToUnknown) ? RIGHT : recommendedDirection;
            distToUnknown = (allTargets[RIGHT].distance > distToUnknown) ? allTargets[RIGHT].distance : distToUnknown;
          }
          
          if(distToUnknown == 0) //Nothing unknown found
          {
            if((allTargets[FRONT].distance - 1) + (allTargets[LEFT].distance - 1) + (allTargets[RIGHT].distance - 1) == 0) //Dead end
            {
              turnLeft();
            }
            else
            {
              int random = rand() % (2 * (allTargets[FRONT].distance - 1) + allTargets[LEFT].distance - 1 + allTargets[RIGHT].distance - 1);
              if(random <= allTargets[LEFT].distance)
              {
                recommendedDirection = LEFT;
              }
              else if(random <= (allTargets[LEFT].distance + allTargets[RIGHT].distance))
              {
                recommendedDirection = RIGHT;
              }
              else 
              {
                recommendedDirection = FRONT;
              }
            }
          }
          
          
          switch(recommendedDirection)
          {
            case FRONT:
              moveForward();
              break;
            case LEFT:
              turnLeft();
              break;
            case RIGHT:
              turnRight();
              break;
            default:
            printf("ERROR");
          }
        }
        else //Fast explore
        {
          printf("Fast explore\n");
          timeFruitlessExploring++;
          if(API::lidarFront() >= 2)

          {
            moveForward();
          }
          else 
          {
            if(API::lidarLeft() > API::lidarRight())
            {
              turnLeft();
            }
            else
            {
              turnRight();
            }
          }
        }
      }
    }
    estimatedFuel-=1;
    timeSinceAttacked ++;
  }
};

#endif