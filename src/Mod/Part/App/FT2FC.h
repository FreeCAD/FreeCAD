// Public header for FT2FC.cpp

#ifndef FT2FC_H
#define FT2FC_H
// public functions
/*std::vector <std::vector <TopoDS_Wire> > FT2FCc(const char *cstring,
                          const std::string FontPath, 
                          const std::string FontName,
                          const float stringheight,
                          const int tracking);*/
std::vector <std::vector <TopoDS_Wire> > FT2FCpu(const Py_UNICODE *unichars,
                          const size_t length,
                          const char *FontPath,
                          const char *FontName,
                          const float stringheight,
                          const int tracking);
#endif // FT2FC_H

