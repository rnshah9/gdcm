/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2009 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmDummyValueGenerator.h"
#include "gdcmTrace.h"
#include "gdcmSystem.h"
#include "gdcmMD5.h"

namespace gdcm
{

const char* DummyValueGenerator::Generate(const char *input)
{
  static char digest[32+1] = {};
  bool b = false;
  if( input )
    b = MD5::Compute(input, strlen(input), digest);

  if( b )
    return digest;
  return 0;
}


} // end namespace gdcm
