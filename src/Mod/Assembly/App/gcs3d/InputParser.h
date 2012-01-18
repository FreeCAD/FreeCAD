
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include "GCS.h"

class InputParser
{
public:
   InputParser();
   ~InputParser();
  
   bool readInputFileAS(std::string &fileName,
			std::vector<double *> &variables,
			std::vector<double *> &parameters,
			std::vector<GCS::Point> &points,
			std::vector<GCS::Normal> &normals,
			std::vector<GCS::Displacement> &displacements,
			std::vector<GCS::Quaternion> &quaternions,
			std::vector<GCS::Solid> &solids,
			std::vector<GCS::Constraint *> &constraints);

   int getIntAttr(xercesc::DOMElement* element, XMLCh* attr);
   double getDoubleAttr(xercesc::DOMElement* element, XMLCh* attr);
private:
   xercesc::XercesDOMParser *m_InputFileParser;

   XMLCh* TAG_points;
   XMLCh* TAG_constraints;
   XMLCh* TAG_point;
   XMLCh* TAG_constraint;   
   XMLCh* ATTR_id;
   XMLCh* ATTR_x;
   XMLCh* ATTR_y;
   XMLCh* ATTR_z;
   XMLCh* ATTR_point;
   XMLCh* ATTR_kind;
   XMLCh* ATTR_distance;
   XMLCh* TAG_GCS_AS;
   XMLCh* TAG_quaternions;
   XMLCh* TAG_quaternion;
   XMLCh* TAG_normals;
   XMLCh* TAG_normal;
   XMLCh* TAG_displacements;
   XMLCh* TAG_displacement;
   XMLCh* TAG_solids;
   XMLCh* TAG_solid;
   XMLCh* KIND_Parallel_AS;
   XMLCh* KIND_Distance_AS;
   XMLCh* ATTR_a;
   XMLCh* ATTR_b;
   XMLCh* ATTR_c;
   XMLCh* ATTR_d;
   XMLCh* ATTR_quaternion;
   XMLCh* ATTR_type;
   XMLCh* ATTR_normal;
   XMLCh* ATTR_displacement;
   XMLCh* ATTR_solid1;
   XMLCh* ATTR_solid2;
};
