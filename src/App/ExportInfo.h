
#ifndef SRC_APP_EXPORTINFO_H_
#define SRC_APP_EXPORTINFO_H_

#include "DocumentObject.h"

#include <string>

namespace App
{

/// Struct that holds information about the last
/// export so that the user does not have to reenter
/// them on each export. The export command either uses
/// a document's export info or the global application's
/// export info depending on the "SingularExportPathPerDocument" parameter
struct ExportInfo {
    std::string location {};
    std::string filename {};
    std::string filter {};
    bool generatedName {false};
    App::DocumentObject* object {nullptr};
};

}

#endif