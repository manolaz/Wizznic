/************************************************************************
 * This file is part of Wizznic.                                        *
 * Copyright 2009-2013 Jimmy Christensen <dusted@dusted.dk>             *
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
 * along with Wizznic.  If not, see <http://www.gnu.org/licenses/>.     *
 ************************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include "bundle.h"
#include "list.h"

#define bundleFileIdentString "Wizznic Bundle "
#define bundleFileEndMarkString "<End of Wizznic Bundle>"
#define bundleFileVersion 0x10

static char* lastExtractedBundle=NULL;
static int lastError=0;

#define TYPE_FILE 1
#define TYPE_DIR 2

#pragma pack(push, 1)
typedef struct {
    char ident[16];
    uint8_t version; //File version
    uint16_t numEntries;
} bundleHeader_t;

typedef struct {
    uint8_t type;
    uint32_t dataOffset;
    uint32_t dataLen;
    uint16_t nameLen;
} bundleFileEntry;
#pragma pack(pop)

typedef struct {
    uint8_t type;
    char name[1024];
    uint32_t dataOffset;
    uint32_t dataSize; //Actual size + filename
    void* data;
} entity;

int dirScan( const char* dir,int type, listItem* list )
{
  entity* fe;
  FILE* f;
  struct dirent *pent;
  struct stat st;
  char* buf=malloc(sizeof(char)*2048);
  DIR *pdir= opendir( dir );

  if(pdir)
  {
    while( (pent=readdir(pdir)) )
    {
      //We're not going to read hidden files or . / ..
      if(pent->d_name[0] != '.')
      {
        sprintf(buf, "%s/%s",dir,pent->d_name);
        if(stat(buf, &st)==0)
        {
          if( (st.st_mode&S_IFDIR)==S_IFDIR )
          {
            if( type==TYPE_DIR)
            {
              fe = malloc( sizeof( entity ) );
              fe->type=TYPE_DIR;
              fe->dataSize = 0;
              strcpy( fe->name, buf );

              listAddData( list, (void*)fe );
            }

            if(!dirScan( buf, type, list ))
            {
              return(0);
            }
          } else if( (st.st_mode&&S_IFREG))
          {
            if( type== TYPE_FILE )
            {
              fe = malloc( sizeof( entity ) );
              fe->type=TYPE_FILE;
              strcpy( fe->name, buf );

              f = fopen( buf, "rb" );
              if(!f)
              {
                printf("Could not read file %s\n", buf);
                return(0);
              }

              fseek(f, 0L, SEEK_END);
              fe->dataSize = ftell(f);
              fseek(f, 0L, SEEK_SET);

              fe->data = malloc( fe->dataSize+strlen(fe->name) );
              strcpy( fe->data, fe->name );
              if( fread( (void*)(((char*)fe->data)+strlen(fe->name)), fe->dataSize, 1, f ) != 1 )
              {
                printf("Could not read input data from file '%s'.\n", fe->name);
                return(0);
              }

              listAddData( list, (void*)fe );
            }
          } else {
            printf("Bundles must only contain directories and regular files.\n");
            return(0);
          }

        }
      }
    }
    return(1);
  }
  printf("Could not open directory: %s\n",dir);
  return(0);
}


int debundleCreateEntry( bundleFileEntry* fe, const char* fileName, FILE* f )
{
  int retVal = BUNDLE_SUCCESS;
  void* data=NULL;
  struct stat st;
  FILE* wf=NULL;

  //Check if destination exists
  if(stat(fileName,&st) < 0)
  {
    //It does not
    if( fe->type==TYPE_DIR)
    {
#ifdef WIN32
      if( mkdir( fileName ) != 0 )
#else
      if( mkdir( fileName,S_IRWXU ) != 0 )
#endif
      {
        printf("ERROR: Could not create directory '%s'\n", fileName);
        retVal=BUNDLE_FAIL_NO_WRITE_PERMISSION;
      }
    } else if( fe->type==TYPE_FILE)
    {
      data = malloc( fe->dataLen );
      if( fread( data, fe->dataLen, 1, f) == 1 )
      {
        wf = fopen( fileName, "wb" );
        if( wf )
        {
          fwrite( data, fe->dataLen, 1, wf );
        } else {
          printf("ERROR: Could not open file %s for writing.\n", fileName);
          retVal = BUNDLE_FAIL_NO_WRITE_PERMISSION;
        }
      } else {
        printf("ERROR: Could not get data for '%s'.\n", fileName);
        retVal = BUNDLE_FAIL_CORRUPT;
      } //Read file data
    } //Is a file
  } else { //Exists
    printf("ERROR: The destination file '%s' already exists.\n", fileName);
    retVal = BUNDLE_FAIL_DIR_EXISTS;
  }

  //Free resources.
  if( wf ) { fclose(wf); }
  if( data ) { free(data); }

  lastError = retVal;
  return(retVal);
}



int debundle( const char* file, const char* outDir )
{
  int i;
  int ret=BUNDLE_FAIL_NOT_BUNDLEFILE;
  FILE* f=NULL;
  char* name=NULL;
  char buf[2048];
  printf("Trying to debundle %s into %s\n", file,outDir);

  bundlePathReset();

  bundleHeader_t header;
  bundleFileEntry* fe=NULL;

  //Open file
  f = fopen( file, "rb");
  if(f)
  {
    //Read header
    if( fread( &header, sizeof(bundleHeader_t), 1, f ) == 1 )
    {
      //Verify it's a bundle
      if( strcmp(header.ident, bundleFileIdentString) == 0 )
      {
        //This code reads version 0x10 bundles.
        if( header.version == 0x10 )
        {
          //Create output directory (We ignore this return value because it may exist and that is okay too).
          //mkdir(outDir, S_IRWXU);

          fe = malloc( sizeof(bundleFileEntry)* header.numEntries );

          // Read all the entries into an array
          if( fread( fe, sizeof(bundleFileEntry), header.numEntries, f ) == header.numEntries )
          {

            //Create directories and files
            for( i=0; i < header.numEntries; i++ )
            {
              //Verify that type is sane
              if( fe[i].type == TYPE_DIR || fe[i].type == TYPE_FILE )
              {
                name = malloc( fe[i].nameLen+1 );
                memset(name,0,fe[i].nameLen+1);
                //Read name
                if( fread(name, fe[i].nameLen, 1, f) == 1 )
                {
                  //Prepend the output directory name to the filename.
                  sprintf(buf, "%s/%s", outDir, name );
                  free(name);

                  //Create entry in the filesystem
                  ret = debundleCreateEntry( &fe[i], buf, f );
                  if( ret != BUNDLE_SUCCESS )
                  {
                    break;
                  } else {
                    //Hack: We assume that the first entry to be debundled is the topdirectory.
                    if( lastExtractedBundle == NULL )
                    {
                      lastExtractedBundle = malloc( strlen(buf)+1 );
                      sprintf(lastExtractedBundle, "%s", buf );
                    }
                  } //Entry was created

                } else { //Read name
                    printf("  ERROR: Could not read a filename from bundlefile.\n");
                    ret = BUNDLE_FAIL_CORRUPT;
                } //Could not read name
              } //Type check
            } //Loop through entries

            //Check for the end marker
            if( ret == BUNDLE_SUCCESS )
            {
              memset( buf, 0, strlen(bundleFileEndMarkString)+1 );
              if( fread(buf, strlen(bundleFileEndMarkString), 1, f) != 1 )
              {
                printf("ERROR: Could not read end of file marker.\n");
                ret = BUNDLE_FAIL_CORRUPT;
              } else if( strcmp(buf, bundleFileEndMarkString) != 0 )
              {
                printf("ERROR: Expected to read '%s' but got '%s'.\n", bundleFileEndMarkString, buf);
                ret = BUNDLE_FAIL_CORRUPT;
              }
            }
          } else {
            printf("ERROR: Could not read %i entries from file.\n", header.numEntries);
            ret = BUNDLE_FAIL_CORRUPT;
          } // Read all the entries into an array

        } else { //Version 0x10
          printf("Error: Bundle File version %i is unsupported by this version of Wizznic.\n", header.version );
          ret = BUNDLE_FAIL_UNSUPPORTED_VERSION;
        } //Unknown version
      } else { // Read ident string
        printf("ERROR: File %s is not a Wizznic Bundle file.\n", file);
        ret = BUNDLE_FAIL_NOT_BUNDLEFILE;
      }
    } else { // < Read header
      printf("ERROR: File %s is not a Wizznic Bundle file.\n", file);
      ret = BUNDLE_FAIL_NOT_BUNDLEFILE;
    }
  } else {
    ret = BUNDLE_FAIL_COULD_NOT_OPEN;
    printf("Could not open file '%s' for input.\n", file);
  }

  //Free resources
  if( fe ) { free(fe); }
  if( f ) { fclose(f); }

  lastError=ret;
  return(ret);
}



void bundle( const char* file, const char* inDir)
{
  FILE* f;
  entity* e;
  listItem* entryList = initList();
  listItem* it=entryList;
  bundleHeader_t header;
  bundleFileEntry* be;
  uint32_t dataOffset=0;
  header.version = bundleFileVersion;
  sprintf( header.ident, "%s", bundleFileIdentString );

  char endMark[] = bundleFileEndMarkString;

  e = malloc(sizeof(entity) );
  e->data=0;
  e->dataOffset=0;
  e->dataSize=0;
  strcpy( e->name, inDir );
  e->type = TYPE_DIR;
  listAddData( entryList, (void*)e );

  printf("Listing directories...\n");
  if( dirScan( inDir, TYPE_DIR, entryList ) )
  {
    header.numEntries = listSize(entryList);
    printf("Added %i directories.\n", header.numEntries );

    printf("Listing files...\n");
    if( dirScan( inDir, TYPE_FILE, entryList ) )
    {
      printf("Added %i files.\n", (listSize(entryList)-header.numEntries) );
      header.numEntries = listSize(entryList);
      printf("There are now %i entries.\n", header.numEntries);
      //First dataoffset is after the list of all entries
      dataOffset = sizeof(bundleHeader_t) + (sizeof(bundleFileEntry)*header.numEntries);


      printf("Bundling...\n");

      f = fopen( file, "wb" );

      if(f)
      {
        //Write header
        fwrite( (void*)(&header), sizeof(bundleHeader_t),1, f );

        //Write entries
        while( (it=it->next) )
        {
          e = (entity*)it->data;
          be = malloc( sizeof(bundleFileEntry) );
          be->type = e->type;
          be->nameLen = strlen( e->name );

          be->dataOffset = dataOffset;
          be->dataLen = e->dataSize;

          fwrite( (void*)be, sizeof(bundleFileEntry), 1, f );

          dataOffset += be->dataLen+be->nameLen;

          free(be);
        }

        it=entryList;
        //Write file data
        while( (it=it->next) )
        {
          e = (entity*)it->data;
          //Write file-name
          if( e->type == TYPE_DIR )
          {
            fwrite( (void*)e->name, sizeof(char)*strlen(e->name), 1, f);
            printf("Added Dir: %s\n", e->name);
          }
          //If it's a file, write content
          if( e->type == TYPE_FILE )
          {
            fwrite( e->data, e->dataSize+strlen(e->name), 1, f);
            printf("Added File: %s\n", e->name);
          }
        }

        fwrite( (void*)&endMark, strlen(endMark), 1, f );

        fclose(f);

      } else {
        printf("Could not open outputfile %s for writing.\n", file);
      }
    } else {
      printf("fileScan of %s failed.\n", inDir);
    }
  } else {
    printf("dirScan of %s failed.\n", inDir);
  }
}

//Get a string telling the path to the last extracted bundle (or NULL if no bundle was extracted).
const char* bundlePath()
{
  return( lastExtractedBundle );
}

//Frees the bundle-path and sets it to NULL
void bundlePathReset()
{
  free( lastExtractedBundle );
  lastExtractedBundle=NULL;
}

int getBundleError()
{
  return(lastError);
}
