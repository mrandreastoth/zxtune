/*
Abstract:
  Parameters::ZXTune::IO::Providers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <zxtune.h>
#include <io/io_parameters.h>
#include <io/providers_parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace IO
    {
      extern const NameType PREFIX = ZXTune::PREFIX + "io";

      namespace Providers
      {
        extern const NameType PREFIX = IO::PREFIX + "providers";

        namespace File
        {
          extern const NameType PREFIX = Providers::PREFIX + "file";
          extern const NameType MMAP_THRESHOLD = PREFIX + "mmap_threshold";
        }

        namespace Network
        {
          extern const NameType PREFIX = Providers::PREFIX + "network";

          namespace Http
          {
            extern const NameType PREFIX = Network::PREFIX + "http";
            extern const NameType USERAGENT = PREFIX + "useragent";
          }
        }
      }
    }
  }
}
