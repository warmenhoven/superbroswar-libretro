#include "global.h"

extern void ResetTourStops();
extern TourStop * ParseTourStopLine(char * buffer, short iVersion[4], bool fIsWorld);
extern void WriteTourStopLine(TourStop * ts, char * buffer, bool fIsWorld);
extern WorldMap g_worldmap;
extern bool LoadMenuSkin(short playerID, short skinID, short colorID, bool fLoadBothDirections);

extern short g_iVersion[];

/**********************************
* WorldMovingObject
**********************************/

WorldMovingObject::WorldMovingObject()
{
	iDrawSprite = 0;
	iDrawDirection = 0;

	SetPosition(0, 0);
}

WorldMovingObject::~WorldMovingObject()
{}

void WorldMovingObject::Init(short iCol, short iRow, short iSprite, short iInitialDirection)
{
	SetPosition(iCol, iRow);
		
	iDrawSprite = iSprite;
	iDrawDirection = iInitialDirection;
}

void WorldMovingObject::Move(short iDirection)
{
	if(iDirection == 0)
	{
		iDestTileY--;
		iState = 1;
	}
	else if(iDirection == 1)
	{
		iDestTileY++;
		iState = 2;
	}
	else if(iDirection == 2)
	{
		iDestTileX--;
		iState = 3;
		iDrawDirection = 1;
	}
	else if(iDirection == 3)
	{
		iDestTileX++;
		iState = 4;
		iDrawDirection = 0;
	}
}

bool WorldMovingObject::Update()
{
	if(++iAnimationTimer > 15)
	{
		iAnimationTimer = 0;
		iAnimationFrame += 2;
		if(iAnimationFrame > 2)
			iAnimationFrame = 0;
	}

	if(iState == 1)
	{
		iy -= 2;
		if(iy <= iDestTileY * TILESIZE)
		{
			iy = iDestTileY * TILESIZE;
			iState = 0;
			iCurrentTileY = iDestTileY;

			return true;
		}
	}
	else if(iState == 2) //down
	{
		iy += 2;
		if(iy >= iDestTileY * TILESIZE)
		{
			iy = iDestTileY * TILESIZE;
			iState = 0;
			iCurrentTileY = iDestTileY;

			return true;
		}
	}
	else if(iState == 3) //left
	{
		ix -= 2;
		if(ix <= iDestTileX * TILESIZE)
		{
			ix = iDestTileX * TILESIZE;
			iState = 0;
			iCurrentTileX = iDestTileX;

			return true;
		}
	}
	else if(iState == 4) //right
	{
		ix += 2;
		if(ix >= iDestTileX * TILESIZE)
		{
			ix = iDestTileX * TILESIZE;
			iState = 0;
			iCurrentTileX = iDestTileX;

			return true;
		}
	}

	return false;
}

void WorldMovingObject::FaceDirection(short iDirection)
{
	iDrawDirection = iDirection;
}

void WorldMovingObject::SetPosition(short iCol, short iRow)
{
	ix = iCol * TILESIZE;
	iy = iRow * TILESIZE;
	iCurrentTileX = iCol;
	iCurrentTileY = iRow;
	iDestTileX = iCol;
	iDestTileY = iRow;
		
	iState = 0;
	iAnimationFrame = 0;
	iAnimationTimer = 0;
}

/**********************************
* WorldPlayer
**********************************/

WorldPlayer::WorldPlayer() :
	WorldMovingObject()
{}

WorldPlayer::~WorldPlayer()
{}

void WorldPlayer::Init(short iCol, short iRow)
{
	WorldMovingObject::Init(iCol, iRow, 0, 0);
}

void WorldPlayer::Draw(short iMapOffsetX, short iMapOffsetY)
{
	spr_player[iDrawSprite][iAnimationFrame + iDrawDirection]->draw(ix + iMapOffsetX, iy + iMapOffsetY, 0, 0, 32, 32);
}

void WorldPlayer::SetSprite(short iPlayer)
{
	while(!LoadMenuSkin(iPlayer, game_values.skinids[iPlayer], game_values.colorids[iPlayer], true))
	{
		if(++game_values.skinids[iPlayer] >= skinlist.GetCount())
			game_values.skinids[iPlayer] = 0;
	}

	iDrawSprite = iPlayer;
}


/**********************************
* WorldVehicle
**********************************/

WorldVehicle::WorldVehicle() :
	WorldMovingObject()
{}

WorldVehicle::~WorldVehicle()
{}

void WorldVehicle::Init(short iCol, short iRow, short iAction, short iSprite, short minMoves, short maxMoves, bool spritePaces, short iInitialDirection)
{
	WorldMovingObject::Init(iCol, iRow, iSprite, iInitialDirection);

	fEnabled = true;

	short iRectOffsetX = 0;
	short iRectOffsetY = 0;

	if(iDrawSprite >= 0 && iDrawSprite <= 8)
	{
		iRectOffsetX = 0;
		iRectOffsetY = iDrawSprite * TILESIZE;
	}

	for(short iRect = 0; iRect < 4; iRect++)
		gfx_setrect(&srcRects[iRect], iRect * TILESIZE + iRectOffsetX, iRectOffsetY, 32, 32);

	iNumMoves = 0;
	iActionId = iAction;

	iMinMoves = minMoves;
	iMaxMoves = maxMoves;

	fSpritePaces = spritePaces;
	iPaceOffset = 0;
	iPaceTimer = 0;
}

void WorldVehicle::Move()
{
	iNumMoves = rand() % (iMaxMoves - iMinMoves + 1) + iMinMoves;

	if(iNumMoves > 0)
	{
		iPaceOffset = 0;
		iPaceTimer = 0;
	}

	SetNextDest();
}

void WorldVehicle::SetNextDest()
{
	if(iState != 0 || iMaxMoves == 0)
		return;

	WorldMapTile * tile = &g_worldmap.tiles[iCurrentTileX][iCurrentTileY];

	short iPlayerCurrentTileX, iPlayerCurrentTileY;
	g_worldmap.GetPlayerCurrentTile(&iPlayerCurrentTileX, &iPlayerCurrentTileY);

	if(iNumMoves-- <= 0)
	{
		if(tile->iType == 0 && (iPlayerCurrentTileX != iCurrentTileX || iPlayerCurrentTileY != iCurrentTileY) && 
			g_worldmap.NumVehiclesInTile(iCurrentTileX, iCurrentTileY) <= 1)
			return;
	}

	//Don't allow vehicle to move forever, cap it at 10 moves over the number attempted
	if(iNumMoves <= -10)
		return;

	short iConnections[4];
	short iNumConnections = 0;
	for(short iDirection = 0; iDirection < 4; iDirection ++)
	{
		if(tile->fConnection[iDirection])
			iConnections[iNumConnections++] = iDirection;
	}

	if(iNumConnections > 0)
	{
		short iConnection = iConnections[rand() % iNumConnections];
		WorldMovingObject::Move(iConnection);
	}
}

bool WorldVehicle::Update()
{
	bool fMoveDone = WorldMovingObject::Update();

	if(fMoveDone)
		SetNextDest();

	//If we're done moving, start pacing in place
	if(fSpritePaces && iState == 0 && ++iPaceTimer > 1)
	{
		iPaceTimer = 0;

		if(iDrawDirection)
		{
			if(--iPaceOffset <= -16)
				iDrawDirection = 0;
		}
		else
		{
			if(++iPaceOffset >= 16)
				iDrawDirection = 1;
		}
	}

	return false;
}

void WorldVehicle::Draw(short iWorldOffsetX, short iWorldOffsetY)
{
	SDL_Rect rDst = {ix + iWorldOffsetX + iPaceOffset, iy + iWorldOffsetY, 32, 32};
	SDL_BlitSurface(spr_worldvehicle.getSurface(), &srcRects[iDrawDirection + iAnimationFrame], blitdest, &rDst);
}


/**********************************
* WorldWarp
**********************************/

WorldWarp::WorldWarp()
{
	iCol1 = 0;
	iRow1 = 0;
	iCol2 = 0;
	iRow2 = 0;
}

void WorldWarp::Init(short col1, short row1, short col2, short row2)
{
	iCol1 = col1;
	iRow1 = row1;
	iCol2 = col2;
	iRow2 = row2;
}

void WorldWarp::GetOtherSide(short iCol, short iRow, short * iOtherCol, short * iOtherRow)
{
	if(iCol1 == iCol && iRow1 == iRow)
	{
		*iOtherCol = iCol2;
		*iOtherRow = iRow2;
	}
	else if(iCol2 == iCol && iRow2 == iRow)
	{
		*iOtherCol = iCol1;
		*iOtherRow = iRow1;
	}
	else
	{
		*iOtherCol = iCol;
		*iOtherRow = iRow;
	}
}


/**********************************
* WorldMap
**********************************/

WorldMap::WorldMap()
{
	iWidth = 0;
	iHeight = 0;
	tiles = NULL;
	vehicles = NULL;
	warps = NULL;
	iNumVehicles = 0;
	iNumStages = 0;
	iNumWarps = 0;
}

WorldMap::~WorldMap()
{
	Cleanup();
}

bool WorldMap::Load()
{
	Cleanup();

	FILE * file = fopen(worldlist.GetIndex(game_values.worldindex), "r");

	if(!file)
		return false;

	char buffer[256];
	short iReadType = 0;
	short iVersion[4] = {0, 0, 0, 0};
	short iMapTileReadRow = 0;
	short iCurrentStage = 0;
	short iCurrentWarp = 0;
	short iCurrentVehicle = 0;
	
	while(fgets(buffer, 256, file))
	{
		if(buffer[0] == '#' || buffer[0] == '\n' || buffer[0] == '\r' || buffer[0] == ' ' || buffer[0] == '\t')
			continue;

		if(iReadType == 0)  //Read version number
		{
			char * psz = strtok(buffer, ".\n");
			if(psz)
				iVersion[0] = atoi(psz);

			psz = strtok(NULL, ".\n");
			if(psz)
				iVersion[1] = atoi(psz);

			psz = strtok(NULL, ".\n");
			if(psz)
				iVersion[2] = atoi(psz);

			psz = strtok(NULL, ".\n");
			if(psz)
				iVersion[3] = atoi(psz);

			iReadType = 1;
		}
		else if(iReadType == 1) //world width
		{
			iWidth = atoi(buffer);
			iReadType = 2;
		}
		else if(iReadType == 2) //world height
		{
			iHeight = atoi(buffer);
			iReadType = 3;

			tiles = new WorldMapTile*[iWidth];

			for(short iCol = 0; iCol < iWidth; iCol++)
				tiles[iCol] = new WorldMapTile[iHeight];
		}
		else if(iReadType == 3) //background sprites
		{
			char * psz = strtok(buffer, ",\n");
			
			for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
			{
				if(!psz)
					goto RETURN;

				WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
				tile->iBackgroundSprite = atoi(psz);
				tile->iBackgroundStyle = 0;
				tile->fAnimated = tile->iBackgroundSprite == 0 || (tile->iBackgroundSprite >= 2 && tile->iBackgroundSprite <= 27);
				
				psz = strtok(NULL, ",\n");
			}

			if(++iMapTileReadRow == iHeight)
			{
				iReadType = 4;
				iMapTileReadRow = 0;
			}
		}
		else if(iReadType == 4) //foreground sprites
		{
			char * psz = strtok(buffer, ",\n");
			
			for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
			{
				if(!psz)
					goto RETURN;

				WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
				tile->iForegroundSprite = atoi(psz);

				if(!tile->fAnimated)
					tile->fAnimated = tile->iForegroundSprite >= 3 && tile->iForegroundSprite <= 10;

				if(!tile->fAnimated)
					tile->fAnimated = tile->iForegroundSprite >= WORLD_FOREGROUND_SPRITE_OFFSET && tile->iForegroundSprite <= WORLD_FOREGROUND_SPRITE_OFFSET + 399;
				
				psz = strtok(NULL, ",\n");
			}

			if(++iMapTileReadRow == iHeight)
			{
				iReadType = 5;
				iMapTileReadRow = 0;
			}
		}
		else if(iReadType == 5) //path connections
		{
			char * psz = strtok(buffer, ",\n");
			
			for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
			{
				if(!psz)
					goto RETURN;

				WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
				tile->iConnectionType = atoi(psz);
		
				psz = strtok(NULL, ",\n");
			}

			if(++iMapTileReadRow == iHeight)
			{
				iReadType = 6;
				iMapTileReadRow = 0;

				//1 == |  2 == -  3 == -!  4 == L  5 == ,-  6 == -,
				//7 == -|  8 == -`-  9 == |-  10 == -,-  11 == +
				for(short iRow = 0; iRow < iHeight; iRow++)
				{
					for(short iCol = 0; iCol < iWidth; iCol++)
					{
						WorldMapTile * tile = &tiles[iCol][iRow];

						for(short iDirection = 0; iDirection < 4; iDirection++)
							tile->fConnection[iDirection] = false;

						if(iRow > 0)
						{
							WorldMapTile * topTile = &tiles[iCol][iRow - 1];

							tile->fConnection[0] = (topTile->iConnectionType == 1 || topTile->iConnectionType == 5 || topTile->iConnectionType == 6 || 
								topTile->iConnectionType == 7 || topTile->iConnectionType == 9 || topTile->iConnectionType == 10 ||
								topTile->iConnectionType == 11) && (tile->iConnectionType == 1 || tile->iConnectionType == 3 ||
								tile->iConnectionType == 4 || tile->iConnectionType == 7 || tile->iConnectionType == 8 ||
								tile->iConnectionType == 9 || tile->iConnectionType == 11);
						}

						if(iRow < iHeight - 1)
						{
							WorldMapTile * bottomTile = &tiles[iCol][iRow + 1];

							tile->fConnection[1] = (bottomTile->iConnectionType == 1 || bottomTile->iConnectionType == 3 || bottomTile->iConnectionType == 4 || 
								bottomTile->iConnectionType == 7 || bottomTile->iConnectionType == 8 || bottomTile->iConnectionType == 9 ||
								bottomTile->iConnectionType == 11) && (tile->iConnectionType == 1 || tile->iConnectionType == 5 ||
								tile->iConnectionType == 6 || tile->iConnectionType == 7 || tile->iConnectionType == 9 ||
								tile->iConnectionType == 10 || tile->iConnectionType == 11);
						}

						if(iCol > 0)
						{
							WorldMapTile * leftTile = &tiles[iCol - 1][iRow];

							tile->fConnection[2] = (leftTile->iConnectionType == 2 || leftTile->iConnectionType == 4 || leftTile->iConnectionType == 5 || 
								leftTile->iConnectionType == 8 || leftTile->iConnectionType == 9 || leftTile->iConnectionType == 10 ||
								leftTile->iConnectionType == 11) && (tile->iConnectionType == 2 || tile->iConnectionType == 3 ||
								tile->iConnectionType == 6 || tile->iConnectionType == 7 || tile->iConnectionType == 8 ||
								tile->iConnectionType == 10 || tile->iConnectionType == 11);
						}

						if(iCol < iWidth - 1)
						{
							WorldMapTile * rightTile = &tiles[iCol + 1][iRow];

							tile->fConnection[3] = (rightTile->iConnectionType == 2 || rightTile->iConnectionType == 3 || rightTile->iConnectionType == 6 || 
								rightTile->iConnectionType == 7 || rightTile->iConnectionType == 8 || rightTile->iConnectionType == 10 ||
								rightTile->iConnectionType == 11) && (tile->iConnectionType == 2 || tile->iConnectionType == 4 ||
								tile->iConnectionType == 5 || tile->iConnectionType == 8 || tile->iConnectionType == 9 ||
								tile->iConnectionType == 10 || tile->iConnectionType == 11);
						}
					}	
				}
			}
		}
		else if(iReadType == 6) //stages
		{
			char * psz = strtok(buffer, ",\n");
			
			for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
			{
				if(!psz)
					goto RETURN;

				WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
				tile->iType = atoi(psz);
				tile->iWarp = -1;

				if(tile->iType == 1)
				{
					iStartX = iMapTileReadCol;
					iStartY = iMapTileReadRow;
				}
				
				tile->fCompleted = tile->iType <= 1;

				psz = strtok(NULL, ",\n");
			}

			if(++iMapTileReadRow == iHeight)
				iReadType = 7;
		}
		else if(iReadType == 7) //number of stages
		{
			iNumStages = atoi(buffer);
			
			iReadType = iNumStages == 0 ? 9 : 8;
		}
		else if(iReadType == 8) //stage details
		{
			TourStop * ts = ParseTourStopLine(buffer, iVersion, true);
		
			game_values.tourstops.push_back(ts);
			game_values.tourstoptotal++;

			if(++iCurrentStage >= iNumStages)
				iReadType = 9;
		}
		else if(iReadType == 9) //number of warps
		{
			iNumWarps = atoi(buffer);

			if(iNumWarps < 0)
				iNumWarps = 0;

			if(iNumWarps > 0)
				warps = new WorldWarp[iNumWarps];
			
			iReadType = iNumWarps == 0 ? 11 : 10;
		}
		else if(iReadType == 10) //warp details
		{
			char * psz = strtok(buffer, ",\n");
			
			if(!psz)
				goto RETURN;

			short iCol1 = atoi(psz);
			if(iCol1 < 0)
				iCol1 = 0;

			psz = strtok(NULL, ",\n");

			short iRow1 = atoi(psz);
			if(iRow1 < 0)
				iRow1 = 0;

			psz = strtok(NULL, ",\n");

			short iCol2 = atoi(psz);
			if(iCol2 < 0)
				iCol2 = 0;

			psz = strtok(NULL, ",\n");

			short iRow2 = atoi(psz);
			if(iRow2 < 0)
				iRow2 = 0;
			
			warps[iCurrentWarp].Init(iCol1, iRow1, iCol2, iRow2);

			tiles[iCol1][iRow1].iWarp = iCurrentWarp;
			tiles[iCol2][iRow2].iWarp = iCurrentWarp;

			if(++iCurrentWarp >= iNumWarps)
				iReadType = 11;
		}
		else if(iReadType == 11) //number of vehicles
		{
			iNumVehicles = atoi(buffer);

			if(iNumVehicles < 0)
				iNumVehicles = 0;

			if(iNumVehicles > 0)
				vehicles = new WorldVehicle[iNumVehicles];

			iReadType = iNumVehicles == 0 ? 13 : 12;
		}
		else if(iReadType == 12) //moving objects
		{
			char * psz = strtok(buffer, ",\n");
			
			if(!psz)
				goto RETURN;

			short iSprite = atoi(psz);

			psz = strtok(NULL, ",\n");

			short iStage = atoi(psz);

			if(iStage > iNumStages)
				iStage = 0;

			psz = strtok(NULL, ",\n");
			short iCol = atoi(psz);

			psz = strtok(NULL, ",\n");
			short iRow = atoi(psz);

			psz = strtok(NULL, ",\n");
			short iMinMoves = atoi(psz);

			if(iMinMoves < 0)
				iMinMoves = 0;

			psz = strtok(NULL, ",\n");
			short iMaxMoves = atoi(psz);

			if(iMaxMoves < iMinMoves)
				iMaxMoves = iMinMoves;

			psz = strtok(NULL, ",\n");
			bool fSpritePaces = atoi(psz) == 1;

			psz = strtok(NULL, ",\n");
			short iInitialDirection = atoi(psz);

			if(iInitialDirection != 0)
				iInitialDirection = 1;

			vehicles[iCurrentVehicle].Init(iCol, iRow, iStage, iSprite, iMinMoves, iMaxMoves, fSpritePaces, iInitialDirection);

			if(++iCurrentVehicle >= iNumVehicles)
				iReadType = 13;
		}
	}

RETURN:

	fclose(file);

	return iReadType == 13;
}

//Saves world to file
bool WorldMap::Save()
{
	return Save(worldlist.GetIndex(game_values.worldindex));
}

bool WorldMap::Save(const char * szPath)
{
	FILE * file = fopen(szPath, "w");

	if(!file)
		return false;

	fprintf(file, "#Version\n");
	fprintf(file, "%d.%d.%d.%d\n\n", g_iVersion[0], g_iVersion[1], g_iVersion[2], g_iVersion[3]);

	fprintf(file, "#Width\n");
	fprintf(file, "%d\n\n", iWidth);

	fprintf(file, "#Height\n");
	fprintf(file, "%d\n\n", iHeight);

	fprintf(file, "#Sprite Backgroud Layer\n");

	for(short iMapTileReadRow = 0; iMapTileReadRow < iHeight; iMapTileReadRow++)
	{
		for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
		{
			WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
			fprintf(file, "%d", tile->iBackgroundSprite);
			
			if(iMapTileReadCol == iWidth - 1)
				fprintf(file, "\n");
			else
				fprintf(file, ",");
		}
	}
	fprintf(file, "\n");

	fprintf(file, "#Sprite Foregroud Layer\n");

	for(short iMapTileReadRow = 0; iMapTileReadRow < iHeight; iMapTileReadRow++)
	{
		for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
		{
			WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
			fprintf(file, "%d", tile->iForegroundSprite);
			
			if(iMapTileReadCol == iWidth - 1)
				fprintf(file, "\n");
			else
				fprintf(file, ",");
		}
	}
	fprintf(file, "\n");

	fprintf(file, "#Connections\n");

	for(short iMapTileReadRow = 0; iMapTileReadRow < iHeight; iMapTileReadRow++)
	{
		for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
		{
			WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
			fprintf(file, "%d", tile->iConnectionType);
			
			if(iMapTileReadCol == iWidth - 1)
				fprintf(file, "\n");
			else
				fprintf(file, ",");
		}
	}
	fprintf(file, "\n");

	fprintf(file, "#Tile Types\n");

	for(short iMapTileReadRow = 0; iMapTileReadRow < iHeight; iMapTileReadRow++)
	{
		for(short iMapTileReadCol = 0; iMapTileReadCol < iWidth; iMapTileReadCol++)
		{
			WorldMapTile * tile = &tiles[iMapTileReadCol][iMapTileReadRow];
			fprintf(file, "%d", tile->iType);
			
			if(iMapTileReadCol == iWidth - 1)
				fprintf(file, "\n");
			else
				fprintf(file, ",");
		}
	}
	fprintf(file, "\n");

	fprintf(file, "#Stages\n");
	fprintf(file, "#Map,Mode,Goal,Points,Bonus,Name,End World, then mode settings (see sample tour file for details)\n");

	fprintf(file, "%d\n", game_values.tourstoptotal);

	for(short iStage = 0; iStage < game_values.tourstoptotal; iStage++)
	{
		char szLine[1024];
		WriteTourStopLine(game_values.tourstops[iStage], szLine, true);
		fprintf(file, szLine);
	}
	fprintf(file, "\n");

	fprintf(file, "#Warps\n");
	fprintf(file, "#location 1 x, y, location 2 x, y\n");

	fprintf(file, "%d\n", iNumWarps);

	for(short iWarp = 0; iWarp < iNumWarps; iWarp++)
	{
		fprintf(file, "%d,", warps[iWarp].iCol1);
		fprintf(file, "%d,", warps[iWarp].iRow1);
		fprintf(file, "%d,", warps[iWarp].iCol2);
		fprintf(file, "%d\n", warps[iWarp].iRow2);
	}
	fprintf(file, "\n");

	fprintf(file, "#Vehicles\n");
	fprintf(file, "#Sprite,Stage Type, Start Column, Start Row, Min Moves, Max Moves, Sprite Paces, Sprite Direction\n");

	fprintf(file, "%d\n", iNumVehicles);

	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		fprintf(file, "%d,", vehicles[iVehicle].iDrawSprite);
		fprintf(file, "%d,", vehicles[iVehicle].iActionId);
		fprintf(file, "%d,", vehicles[iVehicle].iCurrentTileX);
		fprintf(file, "%d,", vehicles[iVehicle].iCurrentTileY);
		fprintf(file, "%d,", vehicles[iVehicle].iMinMoves);
		fprintf(file, "%d,", vehicles[iVehicle].iMaxMoves);
		fprintf(file, "%d,", vehicles[iVehicle].fSpritePaces);
		fprintf(file, "%d\n", vehicles[iVehicle].iDrawDirection);
	}
	fprintf(file, "\n");

	fclose(file);

	return true;
}

void WorldMap::Clear()
{
	if(tiles)
	{
		for(short iCol = 0; iCol < iWidth; iCol++)
		{
			for(short iRow = 0; iRow < iHeight; iRow++)
			{
				tiles[iCol][iRow].iBackgroundSprite = 0;
				tiles[iCol][iRow].iBackgroundStyle = 0;
				tiles[iCol][iRow].iForegroundSprite = 0;
				tiles[iCol][iRow].iConnectionType = 0;
				tiles[iCol][iRow].iType = 0;
			}
		}
	}

	if(vehicles)
	{
		delete [] vehicles;
		vehicles = NULL;
	}

	iNumVehicles = 0;

	if(warps)
	{
		delete [] warps;
		warps = NULL;
	}

	iNumWarps = 0;
}

//Creates clears world and resizes (essentially creating a new world to work on for editor)
void WorldMap::New(short w, short h)
{
	Cleanup();
	
	iWidth = w;
	iHeight = h;

	tiles = new WorldMapTile*[iWidth];

	for(short iCol = 0; iCol < iWidth; iCol++)
		tiles[iCol] = new WorldMapTile[iHeight];

	Clear();
}

//Resizes world keeping intact current tiles (if possible)
void WorldMap::Resize(short w, short h)
{
	//Copy tiles from old map
	WorldMapTile ** tempTiles = NULL;

	if(tiles)
	{
		WorldMapTile ** tempTiles = new WorldMapTile*[iWidth];

		for(short iCol = 0; iCol < iWidth; iCol++)
		{
			tempTiles[iCol] = new WorldMapTile[iHeight];
			
			for(short iRow = 0; iRow < iHeight; iRow++)
			{
				tempTiles[iCol][iRow].iBackgroundSprite = tiles[iCol][iRow].iBackgroundSprite;
				tempTiles[iCol][iRow].iBackgroundStyle = tiles[iCol][iRow].iBackgroundStyle;
				tempTiles[iCol][iRow].iForegroundSprite = tiles[iCol][iRow].iForegroundSprite;
				tempTiles[iCol][iRow].iConnectionType = tiles[iCol][iRow].iConnectionType;
				tempTiles[iCol][iRow].iType = tiles[iCol][iRow].iType;
			}
		}
	}

	//Create new map
	New(iWidth, iHeight);

	//Copy into new map
	if(tempTiles)
	{
		for(short iCol = 0; iCol < w && iCol < iWidth; iCol++)
		{
			for(short iRow = 0; iRow < h && iRow < iHeight; iRow++)
			{
				tiles[iCol][iRow].iBackgroundSprite = tempTiles[iCol][iRow].iBackgroundSprite;
				tiles[iCol][iRow].iBackgroundStyle = tempTiles[iCol][iRow].iBackgroundStyle;
				tiles[iCol][iRow].iForegroundSprite = tempTiles[iCol][iRow].iForegroundSprite;
				tiles[iCol][iRow].iConnectionType = tempTiles[iCol][iRow].iConnectionType;
				tiles[iCol][iRow].iType = tempTiles[iCol][iRow].iType;
			}

			delete [] tempTiles[iCol];
		}

		delete [] tempTiles;
	}
}

void WorldMap::InitPlayer()
{
	player.Init(iStartX, iStartY);
}

bool WorldMap::Update()
{
	bool fPlayMovingVehicleSound = false;

	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		if(!vehicles[iVehicle].fEnabled)
			continue;

		vehicles[iVehicle].Update();

		if(vehicles[iVehicle].iState > 0)
			fPlayMovingVehicleSound = true;
	}

	if(fPlayMovingVehicleSound && !sfx_boomerang.isplaying())
		ifsoundonplay(sfx_boomerang);

	return player.Update();
}

void WorldMap::Draw(short iMapOffsetX, short iMapOffsetY)
{
	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		if(!vehicles[iVehicle].fEnabled)
			continue;

		vehicles[iVehicle].Draw(iMapOffsetX, iMapOffsetY);
	}

	player.Draw(iMapOffsetX, iMapOffsetY);
}

void WorldMap::DrawMapToSurface(bool fInit, SDL_Surface * surface, short iMapDrawOffsetCol, short iMapDrawOffsetRow, short iAnimationFrame)
{
	for(short iRow = 0; iRow < 19 && iRow + iMapDrawOffsetRow < iHeight; iRow++)
	{
		for(short iCol = 0; iCol < 24 && iCol + iMapDrawOffsetCol < iWidth; iCol++)
		{
			SDL_Rect r = {iCol * TILESIZE, iRow * TILESIZE, TILESIZE, TILESIZE};
		
			WorldMapTile * tile = &tiles[iCol + iMapDrawOffsetCol][iRow + iMapDrawOffsetRow];
			short iBackgroundSprite = tile->iBackgroundSprite;
			short iBackgroundStyle = tile->iBackgroundStyle;
			short iForegroundSprite = tile->iForegroundSprite;

			if(tile->fAnimated || fInit)
			{
				if(iBackgroundSprite == 0 || (iBackgroundSprite >= 2 && iBackgroundSprite <= 27) || (iBackgroundSprite >= 45 && iBackgroundSprite <= 48))
				{
					SDL_Rect rSrc = {iAnimationFrame, 0, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);

					if((iBackgroundSprite >= 2 && iBackgroundSprite <= 27) || (iBackgroundSprite >= 45 && iBackgroundSprite <= 48))
					{
						if(iBackgroundSprite >= 45)
						{
							SDL_Rect rSrc = {96, (iBackgroundSprite - 44) << 5, TILESIZE, TILESIZE};
							SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
						}
						else if(iBackgroundSprite >= 16)
						{
							SDL_Rect rSrc = {TILESIZE, (iBackgroundSprite - 14) << 5, TILESIZE, TILESIZE};
							SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
						}
						else
						{
							SDL_Rect rSrc = {0, iBackgroundSprite << 5, TILESIZE, TILESIZE};
							SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
						}
					}
				}
				else if(iBackgroundSprite == 1)
				{
					SDL_Rect rSrc = {TILESIZE, TILESIZE, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
				}
				else if(iBackgroundSprite == 28 || iBackgroundSprite == 29)
				{
					SDL_Rect rSrc = {TILESIZE, (iBackgroundSprite - 14) << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
				}
				else if(iBackgroundSprite >= 30 && iBackgroundSprite <= 44)
				{
					SDL_Rect rSrc = {64, (iBackgroundSprite - 29) << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
				}

				if(iForegroundSprite == 1 || iForegroundSprite == 2)
				{
					SDL_Rect rSrc = {0, (iForegroundSprite - 1) << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldpaths.getSurface(), &rSrc, surface, &r);
				}
				else if(iForegroundSprite >= 3 && iForegroundSprite <= 10)
				{
					SDL_Rect rSrc = {iAnimationFrame, (iForegroundSprite - 1) << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldpaths.getSurface(), &rSrc, surface, &r);
				}
				else if(iForegroundSprite >= 11 && iForegroundSprite <= 18)
				{
					short iSpriteX = (((iForegroundSprite - 11) / 2) + 1) << 5;
					short iSpriteY = ((iForegroundSprite - 11) % 2) << 5;

					SDL_Rect rSrc = {iSpriteX, iSpriteY, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldpaths.getSurface(), &rSrc, surface, &r);
				}
				else if(iForegroundSprite >= WORLD_FOREGROUND_SPRITE_OFFSET && iForegroundSprite <= WORLD_FOREGROUND_SPRITE_OFFSET + 399)
				{
					short iTileColor = (iForegroundSprite - WORLD_FOREGROUND_SPRITE_OFFSET) / 100;
					SDL_Rect rSrc = {320 + iAnimationFrame, iTileColor << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldforeground.getSurface(), &rSrc, surface, &r);

					short iTileNumber = (iForegroundSprite - WORLD_FOREGROUND_SPRITE_OFFSET) % 100;
					rSrc.x = (iTileNumber % 10) << 5;
					rSrc.y = (iTileNumber / 10) << 5;
					SDL_BlitSurface(spr_worldforeground.getSurface(), &rSrc, surface, &r);
				}
				else if(iForegroundSprite >= WORLD_WINNING_TEAM_SPRITE_OFFSET && iForegroundSprite <= WORLD_WINNING_TEAM_SPRITE_OFFSET + 3)
				{
					SDL_Rect rSrc = {(iForegroundSprite - WORLD_WINNING_TEAM_SPRITE_OFFSET + 15) << 5, TILESIZE, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldforeground.getSurface(), &rSrc, surface, &r);
				}

				/*
				if(iForegroundSprite == 1 || iForegroundSprite == 2)
				{
					SDL_Rect rSrc = {96, iForegroundSprite << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
				}
				else if(iForegroundSprite >= 3 && iForegroundSprite <= 6)
				{
					SDL_Rect rSrc = {iAnimationFrame, (iForegroundSprite + 14) << 5, TILESIZE, TILESIZE};
					SDL_BlitSurface(spr_worldbackground.getSurface(), &rSrc, surface, &r);
				}
				*/
			}
		}
	}		
}

void WorldMap::Cleanup()
{
	ResetTourStops();

	if(tiles)
	{
		for(short iCol = 0; iCol < iWidth; iCol++)
			delete [] tiles[iCol];

		delete [] tiles;

		tiles = NULL;
	}

	if(vehicles)
	{
		delete [] vehicles;
		vehicles = NULL;
	}

	iNumVehicles = 0;

	if(warps)
	{
		delete [] warps;
		warps = NULL;
	}

	iNumWarps = 0;
}

void WorldMap::SetPlayerSprite(short iPlayerSprite)
{
	player.SetSprite(iPlayerSprite);
}

bool WorldMap::IsVehicleMoving()
{
	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		if(!vehicles[iVehicle].fEnabled)
			continue;

		if(vehicles[iVehicle].iState > 0)
			return true;
	}

	return false;
}

void WorldMap::GetPlayerPosition(short * iPlayerX, short * iPlayerY)
{
	*iPlayerX = player.ix;
	*iPlayerY = player.iy;
}

void WorldMap::SetPlayerPosition(short iPlayerCol, short iPlayerRow)
{
	player.SetPosition(iPlayerCol, iPlayerRow);
}

void WorldMap::GetPlayerCurrentTile(short * iPlayerCurrentTileX, short * iPlayerCurrentTileY)
{
	*iPlayerCurrentTileX = player.iCurrentTileX;
	*iPlayerCurrentTileY = player.iCurrentTileY;
}

short WorldMap::GetPlayerState()
{
	return player.iState;
}

short WorldMap::GetVehicleInPlayerTile(short * vehicleIndex)
{
	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		WorldVehicle * vehicle = &vehicles[iVehicle];

		if(!vehicle->fEnabled)
			continue;

		if(vehicle->iCurrentTileX == player.iCurrentTileX && vehicle->iCurrentTileY == player.iCurrentTileY)
		{
			*vehicleIndex = iVehicle;
			return vehicle->iActionId;
		}
	}

	*vehicleIndex = -1;
	return -1;
}

bool WorldMap::GetWarpInPlayerTile(short * iWarpCol, short * iWarpRow)
{
	short iWarp = tiles[player.iCurrentTileX][player.iCurrentTileY].iWarp;

	if(iWarp < 0)
		return false;

	warps[iWarp].GetOtherSide(player.iCurrentTileX, player.iCurrentTileY, iWarpCol, iWarpRow);
	return true;
}

void WorldMap::MovePlayer(short iDirection)
{
	player.Move(iDirection);
}

void WorldMap::FacePlayer(short iDirection)
{
	player.FaceDirection(iDirection);
}

void WorldMap::MoveVehicles()
{
	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		if(!vehicles[iVehicle].fEnabled)
			continue;

		vehicles[iVehicle].Move();
	}
}

void WorldMap::RemoveVehicle(short iVehicleIndex)
{
	vehicles[iVehicleIndex].fEnabled = false;
}

short WorldMap::NumVehiclesInTile(short iTileX, short iTileY)
{
	short iVehicleCount = 0;
	for(short iVehicle = 0; iVehicle < iNumVehicles; iVehicle++)
	{
		WorldVehicle * vehicle = &vehicles[iVehicle];

		if(!vehicle->fEnabled)
			continue;

		if(vehicle->iCurrentTileX == iTileX && vehicle->iCurrentTileY == iTileY)
			iVehicleCount++;
	}

	return iVehicleCount;
}

short WorldMap::GetVehicleStageScore(short iVehicleIndex)
{
	return game_values.tourstops[vehicles[iVehicleIndex].iActionId]->iPoints;
}