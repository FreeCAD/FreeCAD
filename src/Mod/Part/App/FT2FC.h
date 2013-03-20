// Public header for FT2FC.cpp

#ifndef FT2FC_H
#define FT2FC_H
// public function
std::vector <std::vector <TopoDS_Wire> > FT2FC(const Py_UNICODE *unichars,
                          const size_t length,
                          const char *FontPath,
                          const char *FontName,
                          const float stringheight,
                          const int tracking);
#endif // FT2FC_H

