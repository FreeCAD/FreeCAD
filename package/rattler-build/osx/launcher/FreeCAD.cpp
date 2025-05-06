#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

#include <libgen.h>
#include <unistd.h>

int main(int argc, char *argv[], char *const *envp) {
    char *cwd = dirname(realpath(argv[0], NULL));

    std::string FreeCAD = realpath((std::string(cwd) + "/../Resources/bin/freecad").c_str(), NULL);

    std::map<std::string, std::string> env;
    for(int i = 0; envp[i] != NULL; ++i) {
        std::string e(envp[i]);
        auto sep = e.find('=');
        auto var = e.substr(0, sep);
        auto value = e.substr(sep+1, std::string::npos);
        env[var] = value;
    }

    std::string prefix = realpath((std::string(cwd) + "/../Resources").c_str(), NULL);
    env["PREFIX"]               = prefix;
    env["LD_LIBRARY_PATH"]      = prefix + "/lib";
    env["PYTHONPATH"]           = prefix;
    env["PYTHONHOME"]           = prefix;
    env["FONTCONFIG_FILE"]      = "/etc/fonts/fonts.conf";
    env["FONTCONFIG_PATH"]      = "/etc/fonts";
    env["SSL_CERT_FILE"]        = prefix + "/ssl/cacert.pem";    // https://forum.freecad.org/viewtopic.php?f=3&t=42825
    env["GIT_SSL_CAINFO"]       = prefix + "/ssl/cacert.pem";

    char **new_env = new char*[env.size() + 1];
    int i = 0;
    for (const auto& [var, value] : env) {
        auto line = var + '=' + value;
        size_t len = line.length() + 1;
        new_env[i] = new char[len];
        memset(new_env[i], 0, len);
        strncpy(new_env[i], line.c_str(), len);
        i++;
    }
    new_env[i] = NULL;

    i = 0;
    while(new_env[i] != NULL) {
        std::cout << new_env[i] << std::endl;
        i++;
    }

    std::cout << "Running: " << FreeCAD << std::endl;
    i = 0;
    while(argv[i] != NULL) {
        i++;
    }

    char **new_argv = new char*[i + 1];
    new_argv[0] = new char[FreeCAD.length() + 1];
    memset(new_argv[0], 0, FreeCAD.length() + 1);
    strncpy(new_argv[0], FreeCAD.c_str(), FreeCAD.length());

    i = 1;
    while(argv[i] != NULL) {
        new_argv[i] = new char[strlen(argv[i])+1];
        memset(new_argv[i], 0, strlen(argv[i])+1);
        strncpy(new_argv[i], argv[i], strlen(argv[i]));
        i++;
    }
    new_argv[i] = NULL;

    return execve(FreeCAD.c_str(), new_argv, new_env);
}
