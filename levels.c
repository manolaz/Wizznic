/************************************************************************
 * This file is part of Wizznic.                                        *
 * Copyright 2009-2012 Jimmy Christensen <dusted@dusted.dk>             *
 * Wizznic is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * Wizznic is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.      *
 ************************************************************************/

#include "levels.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include "strings.h"
#include "teleport.h"
#include "pack.h"

#include "userfiles.h"

static listItem* userLevelFiles;
static int numUserLevels=0;


//Returns pointr to levelInfo_t if level successfully opened, returns nullptr if not.
levelInfo_t* mkLevelInfo(const char* fileName)
{
  int gotData=0; //If this is still 0 after the loop, level has no [data]
  FILE *f;
  levelInfo_t* tl; //Temp levelInfo.
  char* buf = malloc(sizeof(char)*256); //Read line buffer
  char* set = malloc(sizeof(char)*128); //Buffer for storing setting
  char* val = malloc(sizeof(char)*128); //Buffer for storing value

  tl=0; //Return null ptr if no file is found (malloc won't get called then)
  f = fopen(fileName, "r");
  if(f)
  {
    //Allocate memory for level info.
    tl=malloc(sizeof(levelInfo_t));

    //Set everything 0.
    memset(tl, 0, sizeof(levelInfo_t));

    //Level file name
    tl->file=malloc( sizeof(char)*( strlen(fileName)+1 ) );
    strcpy(tl->file, fileName);

    //preview file name
    sprintf( buf, "%s.png", fileName);
    tl->imgFile=malloc( sizeof(char)*( strlen(buf)+1 ) );
    strcpy(tl->imgFile, buf);

    //default char map
    strcpy( buf, "charmap" );
    tl->fontName=malloc( sizeof(char)*( strlen(buf)+1) );
    strcpy( tl->fontName, buf );

    //Default cursor
    strcpy( buf, "cursor.png" );
    tl->cursorFile=malloc( sizeof(char)*(strlen(buf)+1) );
    strcpy( tl->cursorFile, buf );

    //start/stop images
    tl->startImg=0;
    tl->stopImg=0;

    //Default brick die time
    tl->brick_die_ticks=500;

    tl->brickDieParticles=1;

    //Initiate teleList
    tl->teleList = initList();

    //Loop through file
    while(fgets(buf, 255, f))
    {
      //We don't want \r or \n in the end of the string.
      stripNewLine(buf);
      //Stop reading when we reach [data]
      if(strcmp(buf,"[data]")==0)
      {
        gotData=1;
        break;
      } else {
        //Try and split string at =
        if(splitVals('=',buf, set,val) )
        {
          //Check what we got.
          //Time left?
          if(strcmp("seconds",set)==0)
          {
            tl->time=atoi(val);
          } else
          if(strcmp("bgfile",set)==0)
          {
            tl->bgFile=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->bgFile, val);
          } else
          if(strcmp("tilebase",set)==0)
          {
            tl->tileBase=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->tileBase, val);
          } else
          if(strcmp("explbase",set)==0)
          {
            tl->explBase=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->explBase, val);
          } else
          if(strcmp("wallbase",set)==0)
          {
            tl->wallBase=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->wallBase, val);
          } else
          if(strcmp("author",set)==0)
          {
            tl->author=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->author, val);
          } else
          if(strcmp("levelname",set)==0)
          {
            tl->levelName=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->levelName, val);
          } else
          if(strcmp("sounddir",set)==0)
          {
            tl->soundDir=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->soundDir, val);
          } else
          if(strcmp("charbase",set)==0)
          {
            free(tl->fontName);
            tl->fontName=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->fontName, val);
          } else
          if(strcmp("cursorfile",set)==0)
          {
            free( tl->cursorFile );
            tl->cursorFile=malloc( sizeof(char)*( strlen(val)+1 ) );
            strcpy(tl->cursorFile, val);
          } else
          if(strcmp("startimage",set)==0)
          {
            //Ignore none keyword for start image
            if( strcmp( "none", val) != 0)
            {
              tl->startImg=malloc( sizeof(char)*( strlen(val)+1 ) );
              strcpy(tl->startImg, val);
            }
          } else
          if(strcmp("stopimage", set)==0)
          {
            //Ignore none keyword for stop image
            if( strcmp( "none", val) != 0)
            {
              tl->stopImg=malloc( sizeof(char)*( strlen(val)+1 ) );
              strcpy(tl->stopImg, val);
            }
          } else
          if(strcmp("brickdietime", set)==0)
          {
            tl->brick_die_ticks=atoi(val);
          } else
          if(strcmp("brickdieparticles", set)==0)
          {
            tl->brickDieParticles=atoi(val);
          } else
          if(strcmp("teleport", set)==0)
          {
            teleAddFromString(tl->teleList, val);
          }
        } //Got a = in the line
      } //Not [data]
    } //Reading file
    //Close file
    fclose(f);
  }

  if(!gotData)
  {
    //The reason we don't tell that there's no [data] section is because this
    //function is also used to check for the existance of levels, so it'd always
    //Return "no [data] found for the levelfile name just after the last level in a pack.
    free(tl);
    tl=0;
  }


  free(set);
  free(val);
  free(buf);
  set=0;
  val=0;
  buf=0;

  //Return ptr, is null if file couldnt be opened.
  return(tl);

}

void makeLevelList(listItem** list, const char* dir)
{
  int i=0;
  char* buf = malloc(sizeof(char)*1024);

  //Init the list to hold the filenames
  *list = initList();
  levelInfo_t* tl;

  //List all levels in dir.
  while(1)
  {
    sprintf(buf, "%s/levels/level%03i.wzp",dir, i);
    tl=mkLevelInfo( buf );
    if(tl)
    {
      listAddData(*list, (void*)tl);
    } else {
      break;
    }
    i++; //Increment file-number.
  }

  // Add a "Completed" level at the very end of the list
  tl=malloc(sizeof(levelInfo_t));
  memset(tl, 0, sizeof(levelInfo_t));

  sprintf( buf, packGetFile(".","complete.png"),dir );
  tl->imgFile = malloc( sizeof(char)*(strlen(buf)+1) );
  strcpy(tl->imgFile, buf);

  listAddData(*list, (void*)tl);

  free(buf);
  buf=0;

}

void makeUserLevelList()
{
   //List userlevels
  userLevelFiles = initList();
  int i=0;

  levelInfo_t* tl;
  char* buf = malloc(sizeof(char)*256);
  while(1)
  {
    sprintf(buf, "%s/level%03i.wzp", getUserLevelDir(), i);

    tl=mkLevelInfo( buf );
    if( tl )
    {
      listAddData(userLevelFiles, (void*)tl);
    } else {
      break;
    }
    i++;
  }

  numUserLevels=listSize(userLevelFiles);
  free(buf);
}


// Userlevels
int getNumUserLevels()
{
  return(numUserLevels);
}

//Adds the level to list if it's not allready there.
void addUserLevel(const char* fn)
{
  levelInfo_t* tl;
  listItem* l=userLevelFiles;
  //Check if it's there
  while( (l=l->next) )
  {
    if(strcmp( ((levelInfo_t*)l->data)->file, fn )==0)
    {
      return;
    }
  }


  tl=mkLevelInfo(fn);

  if(tl)
  {
    listAddData(userLevelFiles, (void*)tl);
    numUserLevels=listSize(userLevelFiles);
  } else {
    printf("Strange error, couldn't open saved level.\n");
  }
}

char* userLevelFile(int num)
{
  if(num < numUserLevels)
  {
    return( ((levelInfo_t*)listGetItemData(userLevelFiles,num))->file );
  } else {
    return(NULL);
  }
}


//given a pointer to the pointer, so it can dereference it properly.
void freeLevelInfo(levelInfo_t** p)
{
  //Free all strings that are allocated
  if( (*p)->file ) free( (*p)->file );
  if( (*p)->imgFile ) free( (*p)->imgFile );
  if( (*p)->author ) free( (*p)->author );
  if( (*p)->levelName ) free( (*p)->levelName );
  if( (*p)->tileBase ) free( (*p)->tileBase );
  if( (*p)->explBase ) free( (*p)->explBase );
  if( (*p)->wallBase ) free( (*p)->wallBase );
  if( (*p)->bgFile ) free( (*p)->bgFile );
  if( (*p)->musicFile ) free( (*p)->musicFile );
  if( (*p)->soundDir ) free( (*p)->soundDir );
  if( (*p)->fontName ) free( (*p)->fontName );
  if( (*p)->cursorFile ) free( (*p)->cursorFile );
  if( (*p)->startImg ) free( (*p)->startImg );
  if( (*p)->stopImg ) free( (*p)->stopImg );

  if( (*p)->teleList ) teleFreeList( (*p)->teleList );

  //Set everything 0 for good measure.
  memset( *p, 0, sizeof(levelInfo_t));

  //Free the struct itself
  free( *p );

  //Set ptr null
  *p=0;
}

