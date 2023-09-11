#ifndef META_IOSTREAMS_H
#define META_IOSTREAMS_H

// Includes the different iostream libraries

#include "zipios++/zipios-config.h"

#include <iostream>
#include <fstream>

#if defined (HAVE_STD_IOSTREAM) && defined (USE_STD_IOSTREAM)
#include <sstream>
#else
#include <strstream>
#endif

#endif
