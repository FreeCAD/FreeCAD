// Public header for FT2FC.cpp

#ifndef FT2FC_H
#define FT2FC_H
// public function
std::vector <std::vector <TopoDS_Wire> > FT2FC(const std::string shapestring,
                          const std::string FontPath, 
                          const std::string FontName,
                          const float stringheight,
                          const int tracking);
#endif // FT2FC_H

