// Copyright 2018 CEZEO software Ltd. ( http://www.cezeo.com ). All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
//
#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <stdint.h>

class Console
{
public:
  Console()
  {
    stdOut = GetStdHandle( STD_OUTPUT_HANDLE );
  }

  bool IsInitialized()
  {
    return stdOut != NULL && stdOut != INVALID_HANDLE_VALUE;
  }

  void WriteAnsi( const char* text )
  {
    DWORD written, mode;
    if ( GetConsoleMode( stdOut, &mode ) )
    {
      WriteConsoleA( stdOut, text, strlen( text ), &written, NULL );
    }
    else
    {
      WriteFile( stdOut, text, strlen( text ), &written, NULL );
    }
  }

  void WriteUnicode( const wchar_t* text )
  {
    DWORD written, mode;
    if ( GetConsoleMode( stdOut, &mode ) )
    {
      WriteConsoleA( stdOut, text, wcslen( text ), &written, NULL );
    }
    else
    {
      WriteFile( stdOut, text, wcslen( text ) * sizeof( wchar_t ), &written, NULL );
    }
  }

private:

  HANDLE stdOut;

};

// get file size
bool GetFileSize( const wchar_t* filePath, uint64_t& fileSize )
{
  bool result = false;
  HANDLE fileHandle = CreateFile( filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if ( fileHandle != INVALID_HANDLE_VALUE )
  {
    LARGE_INTEGER size;
    result = GetFileSizeEx( fileHandle, &size ) != FALSE;
    fileSize = size.QuadPart;
    CloseHandle( fileHandle );
  }
  return result;
}

int wmain( int argc, wchar_t* argv[] )
{
  // check for console
  Console output;
  if ( output.IsInitialized() == false )
  {
    return -1;
  }

#pragma region input check
  // input:
  // argv[ 1 ] = to encoding
  // argv[ 2 ] = file
  if ( argc < 3 )
  {
    output.WriteAnsi( " not enough arguments, use: \n wiconv.exe <to encoding> <file to convert> \n utf-8 or ansi can be converted to utf-16, utf-16 and ansi can be converted to utf-8 \n source encoding is detected by file signature \n ansi to utf-8 is direct copy - no conversion" );
    return -1;
  }

  // save time by using lowercase always
  // to
  CharLower( argv[ 1 ] );

  if ( wcscmp( argv[ 1 ], L"utf-8" ) != 0 && wcscmp( argv[ 1 ], L"utf-16") != 0 )
  {
    output.WriteAnsi( " unsupported to encoding: " );
    output.WriteUnicode( argv[ 1 ] );
    output.WriteAnsi( "\n utf-8 and utf-16 is the only supported as target encodings \n" );
    return -1;
  }

#pragma endregion

  // check file
  uint64_t fileSize64 = 0;
  if ( GetFileSize( argv[ 2 ], fileSize64 ) == false )
  {
    output.WriteAnsi( " inaccesible file:\n" );
    output.WriteUnicode( argv[ 2 ] );
    output.WriteAnsi( "\n" );
    return -1;
  }

  if ( fileSize64 == 0 )
  {
    // just exit with success if file is empty
    return 0;
  }

  if ( fileSize64 > 0x40000000 )
  {
    output.WriteAnsi( " input file can not be bigger that 1gb\n" );
    return -1;
  }

  size_t fileSize = size_t( fileSize64 );
  size_t fileSizeWithZero = fileSize + sizeof( wchar_t );

  LPBYTE resultMemory = NULL;
  LPBYTE fileMemory = ( LPBYTE ) malloc( fileSizeWithZero );
  if ( fileMemory == NULL )
  {
    output.WriteAnsi( " can't allocate memory\n" );
    return -1;
  }
  ZeroMemory( fileMemory, fileSizeWithZero );

  bool fileLoaded = false;
  HANDLE fileHandle = CreateFile( argv[ 2 ], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
  if ( fileHandle != INVALID_HANDLE_VALUE )
  {
    DWORD read = 0;
    fileLoaded = ReadFile( fileHandle, fileMemory, fileSize, &read, NULL ) != FALSE && read == fileSize;
    CloseHandle( fileHandle );
  }

  if ( fileLoaded == false )
  {
    output.WriteAnsi( " can't open/read file:\n" );
    output.WriteUnicode( argv[ 2 ] );
    output.WriteAnsi( "\n" );

    free( fileMemory );

    return -1;
  }

  // if to utf-16
  if ( wcscmp( argv[ 1 ], L"utf-16" ) == 0 )
  {
    // convert from utf-8/ansi to utf-16
    // check if the source is already utf-16
    if ( fileSize > 1 && fileMemory[ 0 ] == 0xff && fileMemory[ 1 ] == 0xfe )
    {
      // file already in utf-16
      output.WriteUnicode( LPWSTR( fileMemory ) );
    }
    else
    {
      UINT sourceCodePage = CP_ACP;
      // check if the source is in utf-8
      if ( fileSize > 2 && fileMemory[ 0 ] == 0xef && fileMemory[ 1 ] == 0xbb && fileMemory[ 2 ] == 0xbf )
      {
        sourceCodePage = CP_UTF8;
      }

      // from multibyte to widestring
      // load file
      int size_needed = MultiByteToWideChar( sourceCodePage, 0, LPCSTR( fileMemory ), fileSize, NULL, 0 );
      resultMemory = ( LPBYTE ) malloc( ( size_needed + 1 ) * sizeof( wchar_t ) );
      if ( resultMemory == NULL )
      {
        output.WriteAnsi( " can't allocate memory\n" );
        free( fileMemory );
        return -1;
      }
      ZeroMemory( resultMemory, ( size_needed + 1 ) * sizeof( wchar_t ) );

      size_needed = MultiByteToWideChar( sourceCodePage, 0, LPCSTR( fileMemory ), fileSize, LPWSTR( resultMemory ), size_needed );

      if ( size_needed > 0 )
      {
        output.WriteUnicode( LPWSTR( resultMemory ) );
      }
      else
      {
        output.WriteAnsi( " conversion unsucessful\n" );
        free( fileMemory );
        free( resultMemory );
        return -1;
      }
    }
  }
  else
  {
    // convert from ansi/utf-16 to utf-8
    // check if the source already in utf-8
    if ( fileSize > 2 && fileMemory[ 0 ] == 0xef && fileMemory[ 1 ] == 0xbb && fileMemory[ 2 ] == 0xbf )
    {
      // output as is because it's already utf-8
      output.WriteAnsi( LPSTR( fileMemory ) );
    }
    else
    {
      // check if the source is in utf-16
      if ( fileSize > 1 && fileMemory[ 0 ] == 0xff && fileMemory[ 1 ] == 0xfe )
      {
        // from widestring to multibyte
        int size_needed = WideCharToMultiByte( CP_UTF8, 0, LPCWSTR( fileMemory ), fileSize / sizeof( wchar_t ), NULL, 0, NULL, NULL );
        resultMemory = ( LPBYTE ) malloc( size_needed + 1 );
        if ( resultMemory == NULL )
        {
          output.WriteAnsi( " can't allocate memory\n" );
          free( fileMemory );
          return -1;
        }
        ZeroMemory( resultMemory, size_needed + 1 );
        size_needed = WideCharToMultiByte( CP_UTF8, 0, LPCWSTR( fileMemory ), fileSize / sizeof( wchar_t ), LPSTR( resultMemory ), size_needed, NULL, NULL );
        if ( size_needed > 0 )
        {
          output.WriteAnsi( LPSTR( resultMemory ) );
        }
        else
        {
          output.WriteAnsi( " conversion unsucessful\n" );
          free( fileMemory );
          free( resultMemory );
          return -1;
        }
      }
      else
      {
        // source usually in ansi, so output as is
        output.WriteAnsi( LPSTR( fileMemory ));
      }
    }
  }

  free( fileMemory );
  if ( resultMemory != NULL )
  {
    free( resultMemory );
  }
  return 0;
}

