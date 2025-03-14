/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * PS 3.15 / E.1 / Basic Application Level Confidentiality Profile
 * Implementation of E.1.1 De-identify & E.1.2 Re-identify
 */

#include <memory>

#include "gdcmReader.h"
#include "gdcmWriter.h"
#include "gdcmVersion.h"
#include "gdcmSystem.h"
#include "gdcmCryptoFactory.h"
#include "gdcmUIDGenerator.h"
#include "gdcmAnonymizer.h"
#include "gdcmGlobal.h"
#include "gdcmDefs.h"
#include "gdcmDirectory.h"

#include <getopt.h>


static void PrintVersion()
{
  std::cout << "gdcmanon: gdcm " << gdcm::Version::GetVersion() << " ";
  const char date[] = "$Date$";
  std::cout << date << std::endl;
}

// FIXME
  int deidentify = 0;
  int reidentify = 0;


static bool AnonymizeOneFileDumb(gdcm::Anonymizer &anon, const char *filename, const char *outfilename,
 std::vector<gdcm::Tag> const &empty_tags, std::vector<gdcm::Tag> const &clear_tags, std::vector<gdcm::Tag> const &remove_tags, std::vector< std::pair<gdcm::Tag, std::string> > const & replace_tags,
 std::vector<gdcm::PrivateTag> const &empty_privatetags, std::vector<gdcm::PrivateTag> const &clear_privatetags, std::vector<gdcm::PrivateTag> const &remove_privatetags, std::vector< std::pair<gdcm::PrivateTag, std::string> > const & replace_privatetags,
 bool continuemode = false)
{
  gdcm::Reader reader;
  reader.SetFileName( filename );
  if( !reader.Read() )
    {
    std::cerr << "Could not read : " << filename;
    if( continuemode )
      {
      std::cerr << " -> Skipping from anonymization process (continue mode)." << std::endl;
      return true;
      }
    else
      {
      std::cerr << " -> Check [--continue] option for skipping files." << std::endl;
      return false;
      }
    }
  gdcm::File &file = reader.GetFile();

  anon.SetFile( file );

  if( empty_tags.empty() && clear_tags.empty() && replace_tags.empty() && remove_tags.empty() 
   && empty_privatetags.empty() && clear_privatetags.empty() && replace_privatetags.empty() && remove_privatetags.empty() )
    {
    std::cerr << "No operation(s) to be done." << std::endl;
    return false;
    }

  bool success = true;
  // Private Creator(s) must be done first:
{
  std::vector< std::pair<gdcm::Tag, std::string> >::const_iterator it2 = replace_tags.begin();
  for(; it2 != replace_tags.end(); ++it2)
    {
    success = success && anon.Replace( it2->first, it2->second.c_str() );
    }
}

  // Regular Private:
{
  std::vector<gdcm::PrivateTag>::const_iterator it = empty_privatetags.begin();
  for(; it != empty_privatetags.end(); ++it)
    {
    success = success && anon.Empty( *it );
    }
  it = clear_privatetags.begin();
  for(; it != clear_privatetags.end(); ++it)
    {
    success = success && anon.Clear( *it );
    }
  it = remove_privatetags.begin();
  for(; it != remove_privatetags.end(); ++it)
    {
    success = success && anon.Remove( *it );
    }

  std::vector< std::pair<gdcm::PrivateTag, std::string> >::const_iterator it2 = replace_privatetags.begin();
  for(; it2 != replace_privatetags.end(); ++it2)
    {
    success = success && anon.Replace( it2->first, it2->second.c_str() );
    }
}

  // Public Remove or Empty may impact Private Creator, so do them last.
{
  std::vector<gdcm::Tag>::const_iterator it = empty_tags.begin();
  for(; it != empty_tags.end(); ++it)
    {
    success = success && anon.Empty( *it );
    }
  it = clear_tags.begin();
  for(; it != clear_tags.end(); ++it)
    {
    success = success && anon.Clear( *it );
    }
  it = remove_tags.begin();
  for(; it != remove_tags.end(); ++it)
    {
    success = success && anon.Remove( *it );
    }
}

  gdcm::Writer writer;
  writer.SetFileName( outfilename );
  writer.SetFile( file );
  if( !writer.Write() )
    {
    std::cerr << "Could not Write : " << outfilename << std::endl;
    if( strcmp(filename,outfilename) != 0 )
      {
      gdcm::System::RemoveFile( outfilename );
      }
    else
      {
      std::cerr << "gdcmanon just corrupted: " << filename << " for you (data lost)." << std::endl;
      }

    return false;
    }
  return success;
}

static bool AnonymizeOneFile(gdcm::Anonymizer &anon, const char *filename, const char *outfilename, bool continuemode = false)
{
  gdcm::Reader reader;
  reader.SetFileName( filename );
  if( !reader.Read() )
    {
    std::cerr << "Could not read : " << filename << std::endl;
    if( continuemode )
      {
      std::cerr << "Skipping from anonymization process (continue mode)." << std::endl;
      return true;
      }
    else
      {
      std::cerr << "Check [--continue] option for skipping files." << std::endl;
      return false;
      }
    }
  gdcm::File &file = reader.GetFile();
  gdcm::MediaStorage ms;
  ms.SetFromFile(file);
  if( !gdcm::Defs::GetIODNameFromMediaStorage(ms) )
    {
    std::cerr << "The Media Storage Type of your file is not supported: " << ms << std::endl;
    std::cerr << "Please report" << std::endl;
    return false;
    }

  anon.SetFile( file );

  if( deidentify )
    {
    //anon.RemovePrivateTags();
    //anon.RemoveRetired();
    if( !anon.BasicApplicationLevelConfidentialityProfile( true ) )
      {
      std::cerr << "Could not De-indentify : " << filename << std::endl;
      return false;
      }
    }
  else if ( reidentify )
    {
    if( !anon.BasicApplicationLevelConfidentialityProfile( false ) )
      {
      std::cerr << "Could not Re-indentify : " << filename << std::endl;
      return false;
      }
    }

  gdcm::FileMetaInformation &fmi = file.GetHeader();
  fmi.Clear();

  gdcm::Writer writer;
  writer.SetFileName( outfilename );
  writer.SetFile( file );
  if( !writer.Write() )
    {
    std::cerr << "Could not Write : " << outfilename << std::endl;
    if( strcmp(filename,outfilename) != 0 )
      {
      gdcm::System::RemoveFile( outfilename );
      }
    else
      {
      std::cerr << "gdcmanon just corrupted: " << filename << " for you (data lost)." << std::endl;
      }

    return false;
    }
  return true;
}

static bool GetRSAKeys(gdcm::CryptographicMessageSyntax &cms, const char *privpath = nullptr, const char *certpath = nullptr)
{
  if( privpath && *privpath )
    {
    if( !cms.ParseKeyFile( privpath ) )
      {
      std::cerr << "Could not parse Private Key: " << privpath << std::endl;
      return false;
      }
    }

  if( certpath && *certpath )
    {
    if( !cms.ParseCertificateFile( certpath ) )
      {
      std::cerr << "Could not parse Certificate Key: " << certpath << std::endl;
      return false;
      }
    }
  return true;
}

static void PrintHelp()
{
  PrintVersion();
  std::cout << "Usage: gdcmanon [OPTION]... FILE..." << std::endl;
  std::cout << "PS 3.15 / E.1 / Basic Application Level Confidentiality Profile" << std::endl;
  std::cout << "Implementation of E.1.1 De-identify & E.1.2 Re-identify" << std::endl;
  std::cout << "Parameter (required):" << std::endl;
  std::cout << "  -e --de-identify (encrypt)  De-identify DICOM (default)" << std::endl;
  std::cout << "  -d --re-identify (decrypt)  Re-identify DICOM" << std::endl;
  std::cout << "     --dumb                   Dumb mode anonymizer" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -i --input                  DICOM filename / directory" << std::endl;
  std::cout << "  -o --output                 DICOM filename / directory" << std::endl;
  std::cout << "  -r --recursive              recursively process (sub-)directories." << std::endl;
  std::cout << "     --continue               Do not stop when file found is not DICOM." << std::endl;
  std::cout << "     --root-uid               Root UID." << std::endl;
  std::cout << "     --resources-path         Resources path." << std::endl;
  std::cout << "  -k --key                    Path to RSA Private Key." << std::endl;
  std::cout << "  -c --certificate            Path to Certificate." << std::endl;
  std::cout << "  -p --password               Encryption passphrase." << std::endl;
  std::cout << "Crypto Library Options:" << std::endl;
  std::cout << "  --crypto=" << std::endl;
  std::cout << "           openssl            OpenSSL (default on non-Windows systems)." << std::endl;
  std::cout << "           capi               Microsoft CryptoAPI (default on Windows systems)." << std::endl;
  std::cout << "           openssl-p7         Old OpenSSL implementation." << std::endl;
  std::cout << "Encryption Algorithm Options:" << std::endl;
  std::cout << "     --des3                   Triple DES." << std::endl;
  std::cout << "     --aes128                 AES 128." << std::endl;
  std::cout << "     --aes192                 AES 192." << std::endl;
  std::cout << "     --aes256                 AES 256 (default)." << std::endl;
  std::cout << "Dumb mode options:" << std::endl;
  std::cout << "     --empty   %d,%d          DICOM tag(s) to empty" << std::endl;
  std::cout << "               %d,%d,%s       DICOM private tag(s) to empty" << std::endl;
  std::cout << "     --clear   %d,%d          DICOM tag(s) to clear" << std::endl;
  std::cout << "               %d,%d,%s       DICOM private tag(s) to clear" << std::endl;
  std::cout << "     --remove  %d,%d          DICOM tag(s) to remove" << std::endl;
  std::cout << "               %d,%d,%s       DICOM private tag(s) to remove" << std::endl;
  std::cout << "     --replace %d,%d=%s       DICOM tag(s) to replace" << std::endl;
  std::cout << "               %d,%d,%s=%s    DICOM private tag(s) to replace" << std::endl;
  std::cout << "General Options:" << std::endl;
  std::cout << "  -V --verbose                more verbose (warning+error)." << std::endl;
  std::cout << "  -W --warning                print warning info." << std::endl;
  std::cout << "  -D --debug                  print debug info." << std::endl;
  std::cout << "  -E --error                  print error info." << std::endl;
  std::cout << "  -h --help                   print help." << std::endl;
  std::cout << "  -v --version                print version." << std::endl;
  std::cout << "Env var:" << std::endl;
  std::cout << "  GDCM_ROOT_UID Root UID" << std::endl;
  std::cout << "  GDCM_RESOURCES_PATH path pointing to resources files (Part3.xml, ...)" << std::endl;
}

static gdcm::CryptographicMessageSyntax::CipherTypes GetFromString( const char * str )
{
  gdcm::CryptographicMessageSyntax::CipherTypes ciphertype;
  if( strcmp( str, "des3" ) == 0 )
    {
    ciphertype = gdcm::CryptographicMessageSyntax::DES3_CIPHER;
    }
  else if( strcmp( str, "aes128" ) == 0 )
    {
    ciphertype = gdcm::CryptographicMessageSyntax::AES128_CIPHER;
    }
  else if( strcmp( str, "aes192" ) == 0 )
    {
    ciphertype = gdcm::CryptographicMessageSyntax::AES192_CIPHER;
    }
  else if( strcmp( str, "aes256" ) == 0 )
    {
    ciphertype = gdcm::CryptographicMessageSyntax::AES256_CIPHER;
    }
  else
    {
    // if unrecognized return aes 256...
    ciphertype = gdcm::CryptographicMessageSyntax::AES256_CIPHER;
    }
  return ciphertype;
}

int main(int argc, char *argv[])
{
  int c;
  //int digit_optind = 0;

  std::string filename;
  gdcm::Directory::FilenamesType filenames;
  std::string outfilename;
  gdcm::Directory::FilenamesType outfilenames;
  std::string root;
  std::string xmlpath;
  std::string rsa_path;
  std::string cert_path;
  std::string password;
  int resourcespath = 0;
  int dumb_mode = 0;
  int des3 = 0;
  int aes128 = 0;
  int aes192 = 0;
  int aes256 = 0;
  int rootuid = 0;
  int verbose = 0;
  int warning = 0;
  int debug = 0;
  int error = 0;
  int help = 0;
  int version = 0;
  int recursive = 0;
  int continuemode = 0;
  int empty_tag = 0;
  int clear_tag = 0;
  int remove_tag = 0;
  int replace_tag = 0;
  int crypto_api = 0;
  std::vector<gdcm::Tag> empty_tags;
  std::vector<gdcm::Tag> clear_tags;
  std::vector<gdcm::Tag> remove_tags;
  std::vector< std::pair<gdcm::Tag, std::string> > replace_tags_value;
  std::vector<gdcm::PrivateTag> empty_privatetags;
  std::vector<gdcm::PrivateTag> clear_privatetags;
  std::vector<gdcm::PrivateTag> remove_privatetags;
  std::vector< std::pair<gdcm::PrivateTag, std::string> > replace_privatetags_value;
  gdcm::Tag tag;
  gdcm::PrivateTag privatetag;
  gdcm::CryptoFactory::CryptoLib crypto_lib;
  crypto_lib = gdcm::CryptoFactory::DEFAULT;

  while (true) {
    //int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
        {"input", required_argument, nullptr, 'i'},                 // i
        {"output", required_argument, nullptr, 'o'},                // o
        {"root-uid", required_argument, &rootuid, 1}, // specific Root (not GDCM)
        {"resources-path", required_argument, &resourcespath, 1},
        {"de-identify", no_argument, nullptr, 'e'},
        {"re-identify", no_argument, nullptr, 'd'},
        {"key", required_argument, nullptr, 'k'},
        {"certificate", required_argument, nullptr, 'c'}, // 7
        {"password", required_argument, nullptr, 'p'},

        {"des3", no_argument, &des3, 1},
        {"aes128", no_argument, &aes128, 1},
        {"aes192", no_argument, &aes192, 1},
        {"aes256", no_argument, &aes256, 1},

        {"recursive", no_argument, nullptr, 'r'},
        {"dumb", no_argument, &dumb_mode, 1},
        {"empty", required_argument, &empty_tag, 1}, // 15
        {"clear", required_argument, &clear_tag, 1}, // 16
        {"remove", required_argument, &remove_tag, 1},
        {"replace", required_argument, &replace_tag, 1},
        {"continue", no_argument, &continuemode, 1},
        {"crypto", required_argument, &crypto_api, 1}, //20

        {"verbose", no_argument, nullptr, 'V'},
        {"warning", no_argument, nullptr, 'W'},
        {"debug", no_argument, nullptr, 'D'},
        {"error", no_argument, nullptr, 'E'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},

        {nullptr, 0, nullptr, 0}
    };

    c = getopt_long (argc, argv, "i:o:rdek:c:p:VWDEhv",
      long_options, &option_index);
    if (c == -1)
      {
      break;
      }

    switch (c)
      {
    case 0:
        {
        const char *s = long_options[option_index].name; (void)s;
        //printf ("option %s", s);
        if (optarg)
          {
          //if( option_index == 0 ) /* input */
          //  {
          //  assert( strcmp(s, "input") == 0 );
          //  assert( filename.empty() );
          //  filename = optarg;
          //  }
          //else if( option_index == 1 ) /* output */
          //  {
          //  assert( strcmp(s, "output") == 0 );
          //  assert( outfilename.empty() );
          //  outfilename = optarg;
          //  }
          /*else*/ if( option_index == 2 ) /* root-uid */
            {
            assert( strcmp(s, "root-uid") == 0 );
            assert( root.empty() );
            root = optarg;
            }
          else if( option_index == 3 ) /* resources-path */
            {
            assert( strcmp(s, "resources-path") == 0 );
            assert( xmlpath.empty() );
            xmlpath = optarg;
            }
          //else if( option_index == 6 ) /* key */
          //  {
          //  assert( strcmp(s, "key") == 0 );
          //  assert( rsa_path.empty() );
          //  rsa_path = optarg;
          //  }
          //else if( option_index == 7 ) /* certificate */
          //  {
          //  assert( strcmp(s, "certificate") == 0 );
          //  assert( cert_path.empty() );
          //  cert_path = optarg;
          //  }
          else if( option_index == 15 ) /* empty */
            {
            assert( strcmp(s, "empty") == 0 );
            if( privatetag.ReadFromCommaSeparatedString(optarg) )
              empty_privatetags.push_back( privatetag );
            else if( tag.ReadFromCommaSeparatedString(optarg) )
              empty_tags.push_back( tag );
            else
              {
              std::cerr << "Could not read Tag/PrivateTag: " << optarg << std::endl;
              return 1;
              }
            }
          else if( option_index == 16 ) /* clear */
            {
            assert( strcmp(s, "clear") == 0 );
            if( privatetag.ReadFromCommaSeparatedString(optarg) )
              clear_privatetags.push_back( privatetag );
            else if( tag.ReadFromCommaSeparatedString(optarg) )
              clear_tags.push_back( tag );
            else
              {
              std::cerr << "Could not read Tag/PrivateTag: " << optarg << std::endl;
              return 1;
              }
            }
          else if( option_index == 17 ) /* remove */
            {
            assert( strcmp(s, "remove") == 0 );
            if( privatetag.ReadFromCommaSeparatedString(optarg) )
              remove_privatetags.push_back( privatetag );
            else if( tag.ReadFromCommaSeparatedString(optarg) )
              remove_tags.push_back( tag );
            else
              {
              std::cerr << "Could not read Tag/PrivateTag: " << optarg << std::endl;
              return 1;
              }
            }
          else if( option_index == 18 ) /* replace */
            {
            assert( strcmp(s, "replace") == 0 );
            if( privatetag.ReadFromCommaSeparatedString(optarg) )
              {
              return 1;
              }
            else if( tag.ReadFromCommaSeparatedString(optarg) )
              {
              std::stringstream ss;
              ss.str( optarg );
              uint16_t dummy;
              char cdummy; // comma
              ss >> std::hex >> dummy;
              assert( tag.GetGroup() == dummy );
              ss >> cdummy;
              assert( cdummy == ',' );
              ss >> std::hex >> dummy;
              assert( tag.GetElement() == dummy );
              ss >> cdummy;
              assert( cdummy == ',' || cdummy == '=' );
              std::string str;
              //ss >> str;
              std::getline(ss, str); // do not skip whitespace
              replace_tags_value.emplace_back(tag, str );
              }
            else
              {
              std::cerr << "Could not read Tag/PrivateTag: " << optarg << std::endl;
              return 1;
              }
            }
          else if( option_index == 20 ) /* crypto */
            {
            assert( strcmp(s, "crypto") == 0 );
            if (strcmp(optarg, "openssl") == 0)
              crypto_lib = gdcm::CryptoFactory::OPENSSL;
            else if (strcmp(optarg, "capi") == 0)
              crypto_lib = gdcm::CryptoFactory::CAPI;
            else if (strcmp(optarg, "openssl-p7") == 0)
              crypto_lib = gdcm::CryptoFactory::OPENSSLP7;
            else
              {
              std::cerr << "Cryptography library id not recognized: " << optarg << std::endl;
              return 1;
              }
            }
          //printf (" with arg %s", optarg);
          }
        //printf ("\n");
        }
      break;

    case 'i':
      assert( filename.empty() );
      filename = optarg;
      break;

    case 'o':
      assert( outfilename.empty() );
      outfilename = optarg;
      break;

    case 'r':
      recursive = 1;
      break;

    case 'k': // key
      assert( rsa_path.empty() );
      rsa_path = optarg;
      break;

    case 'c': // certificate
      assert( cert_path.empty() );
      cert_path = optarg;
      break;

    case 'p': // password
      assert( password.empty() );
      password = optarg;
      break;

    case 'e': // encrypt
      deidentify = 1;
      break;

    case 'd': // decrypt
      reidentify = 1;
      break;

    case 'V':
      verbose = 1;
      break;

    case 'W':
      warning = 1;
      break;

    case 'D':
      debug = 1;
      break;

    case 'E':
      error = 1;
      break;

    case 'h':
      help = 1;
      break;

    case 'v':
      version = 1;
      break;

    case '?':
      break;

    default:
      printf ("?? getopt returned character code 0%o ??\n", c);
      }
  }

  if (optind < argc)
    {
    std::vector<std::string> files;
    while (optind < argc)
      {
      //printf ("%s\n", argv[optind++]);
      files.emplace_back(argv[optind++] );
      }
    //printf ("\n");
    if( files.size() == 2
      && filename.empty()
      && outfilename.empty()
    )
      {
      filename = files[0];
      outfilename = files[1];
      }
    else
      {
      PrintHelp();
      return 1;
      }
    }

  if( version )
    {
    //std::cout << "version" << std::endl;
    PrintVersion();
    return 0;
    }

  if( help )
    {
    //std::cout << "help" << std::endl;
    PrintHelp();
    return 0;
    }

  if( filename.empty() )
    {
    //std::cerr << "Need input file (-i)\n";
    PrintHelp();
    return 1;
    }

  // by default de-identify
  if( !deidentify && !reidentify && !dumb_mode)
    {
    deidentify = 1;
    }

  // one option only please
  if( deidentify && reidentify )
    {
    std::cerr << "One option please" << std::endl;
    return 1;
    }
  // dumb mode vs smart mode:
  if( ( deidentify || reidentify ) && dumb_mode )
    {
    std::cerr << "One option please" << std::endl;
    return 1;
    }

  gdcm::CryptoFactory* crypto_factory = nullptr;
  if( deidentify || reidentify )
    {
    crypto_factory = gdcm::CryptoFactory::GetFactoryInstance(crypto_lib);
    if (!crypto_factory)
      {
      std::cerr << "Requested cryptographic library not configured." << std::endl;
      return 1;
      }
    }

  // by default AES 256
  gdcm::CryptographicMessageSyntax::CipherTypes ciphertype =
    gdcm::CryptographicMessageSyntax::AES256_CIPHER;
  if( !dumb_mode )
    {
    if( !des3 && !aes128 && !aes192 && !aes256 )
      {
      aes256 = 1;
      }

    if( des3 )
      {
      ciphertype = GetFromString( "des3" );
      }
    else if( aes128 )
      {
      ciphertype = GetFromString( "aes128" );
      }
    else if( aes192 )
      {
      ciphertype = GetFromString( "aes192" );
      }
    else if( aes256 )
      {
      ciphertype = GetFromString( "aes256" );
      }
    else
      {
      return 1;
      }
    }

  if( !gdcm::System::FileExists(filename.c_str()) )
    {
    std::cerr << "Could not find file: " << filename << std::endl;
    return 1;
    }

  // Are we in single file or directory mode:
  unsigned int nfiles = 1;
  gdcm::Directory dir;
  if( gdcm::System::FileIsDirectory(filename.c_str()) )
    {
    if( !gdcm::System::FileIsDirectory(outfilename.c_str()) )
      {
      if( gdcm::System::FileExists( outfilename.c_str() ) )
        {
        std::cerr << "Could not create directory since " << outfilename << " is already a file" << std::endl;
        return 1;
        }

      }
    // For now avoid user mistake
    if( filename == outfilename )
      {
      std::cerr << "Input directory should be different from output directory" << std::endl;
      return 1;
      }
    if( outfilename.back() != '/' ) outfilename += '/';
    nfiles = dir.Load(filename, (recursive > 0 ? true : false));
    filenames = dir.GetFilenames();
    gdcm::Directory::FilenamesType::const_iterator it = filenames.begin();
    // Prepare outfilenames
    for( ; it != filenames.end(); ++it )
      {
      std::string dup = *it; // make a copy
      std::string &out = dup.replace(0, filename.size(), outfilename );
      outfilenames.push_back( out );
      }
    // Prepare outdirectory
    gdcm::Directory::FilenamesType const &dirs = dir.GetDirectories();
    gdcm::Directory::FilenamesType::const_iterator itdir = dirs.begin();
    for( ; itdir != dirs.end(); ++itdir )
      {
      std::string dirdup = *itdir; // make a copy
      std::string &dirout = dirdup.replace(0, filename.size(), outfilename );
      //std::cout << "Making directory: " << dirout << std::endl;
      if( !gdcm::System::MakeDirectory( dirout.c_str() ) )
        {
        std::cerr << "Could not create directory: " << dirout << std::endl;
        return 1;
        }
      }
    }
  else
    {
    filenames.push_back( filename );
    outfilenames.push_back( outfilename );
    }

  if( filenames.size() != outfilenames.size() )
    {
    std::cerr << "Something went really wrong" << std::endl;
    return 1;
    }

  // Debug is a little too verbose
  gdcm::Trace::SetDebug( (debug  > 0 ? true : false));
  gdcm::Trace::SetWarning(  (warning  > 0 ? true : false));
  gdcm::Trace::SetError(  (error  > 0 ? true : false));
  // when verbose is true, make sure warning+error are turned on:
  if( verbose )
    {
    gdcm::Trace::SetWarning( (verbose > 0 ? true : false) );
    gdcm::Trace::SetError( (verbose  > 0 ? true : false) );
    }

  gdcm::FileMetaInformation::SetSourceApplicationEntityTitle( "gdcmanon" );
  if( !dumb_mode )
  {
    gdcm::Global& g = gdcm::Global::GetInstance();
    if( !resourcespath )
      {
      const char *xmlpathenv = getenv("GDCM_RESOURCES_PATH");
      if( xmlpathenv )
        {
        // Make sure to look for XML dict in user explicitly specified dir first:
        xmlpath = xmlpathenv;
        resourcespath = 1;
        }
      }
    if( resourcespath )
      {
      // xmlpath is set either by the cmd line option or the env var
      if( !g.Prepend( xmlpath.c_str() ) )
        {
        std::cerr << "Specified Resources Path is not valid: " << xmlpath << std::endl;
        return 1;
        }
      }
    // All set, then load the XML files:
    if( !g.LoadResourcesFiles() )
      {
      std::cerr << "Could not load XML file from specified path" << std::endl;
      return 1;
      }
    const gdcm::Defs &defs = g.GetDefs(); (void)defs;
  }
  if( !rootuid )
    {
    // only read the env var if no explicit cmd line option
    // maybe there is an env var defined... let's check
    const char *rootuid_env = getenv("GDCM_ROOT_UID");
    if( rootuid_env )
      {
      rootuid = 1;
      root = rootuid_env;
      }
    }
  if( rootuid )
    {
    // root is set either by the cmd line option or the env var
    if( !gdcm::UIDGenerator::IsValid( root.c_str() ) )
      {
      std::cerr << "specified Root UID is not valid: " << root << std::endl;
      return 1;
      }
    gdcm::UIDGenerator::SetRoot( root.c_str() );
    }

  // Get private key/certificate
  gdcm::CryptographicMessageSyntax *cms_ptr = nullptr;
  if( crypto_factory )
    {
    cms_ptr = crypto_factory->CreateCMSProvider();
    }
  if( !dumb_mode )
    {
    if( !GetRSAKeys(*cms_ptr, rsa_path.c_str(), cert_path.c_str() ) )
      {
      delete cms_ptr;
      return 1;
      }
    if (!password.empty() && !cms_ptr->SetPassword(password.c_str(), password.length()) )
      {
      std::cerr << "Could not set the password " << std::endl;
      delete cms_ptr;
      return 1;
      }
    cms_ptr->SetCipherType( ciphertype );
    }

  // Setup gdcm::Anonymizer
  gdcm::Anonymizer anon;
  if( !dumb_mode )
    {
    anon.SetCryptographicMessageSyntax( cms_ptr );
    }

  if( dumb_mode )
    {
    for(unsigned int i = 0; i < nfiles; ++i)
      {
      const char *in  = filenames[i].c_str();
      const char *out = outfilenames[i].c_str();
      if( !AnonymizeOneFileDumb(anon, in, out, empty_tags, clear_tags, remove_tags, replace_tags_value,
        empty_privatetags, clear_privatetags, remove_privatetags, replace_privatetags_value, (continuemode > 0 ? true: false)) )
        {
        //std::cerr << "Could not anonymize: " << in << std::endl;
        delete cms_ptr;
        return 1;
        }
      }
    }
  else
    {
    for(unsigned int i = 0; i < nfiles; ++i)
      {
      const char *in  = filenames[i].c_str();
      const char *out = outfilenames[i].c_str();
      if( !AnonymizeOneFile(anon, in, out, (continuemode > 0 ? true: false)) )
        {
        //std::cerr << "Could not anonymize: " << in << std::endl;
        delete cms_ptr;
        return 1;
        }
      }
    }
  delete cms_ptr;
  return 0;
}
