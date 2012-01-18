#include <sys/stat.h>
#include <iostream>

#include "InputParser.h"

#include <xercesc/util/XMLDouble.hpp>



InputParser::InputParser()
{
    try {
        xercesc::XMLPlatformUtils::Initialize();  // Initialize Xerces infrastructure
    }
    catch (xercesc::XMLException& e) {
        char* message = xercesc::XMLString::transcode(e.getMessage());
        std::cout << "XML toolkit initialization error: " << message << std::endl;
        xercesc::XMLString::release(&message);
    }

    // Tags and attributes used in XML file.
    // Can't call transcode till after Xerces Initialize()
    TAG_points     = xercesc::XMLString::transcode("points");
    TAG_point      = xercesc::XMLString::transcode("point");
    TAG_constraints= xercesc::XMLString::transcode("constraints");
    TAG_constraint = xercesc::XMLString::transcode("constraint");
    ATTR_id        = xercesc::XMLString::transcode("id");
    ATTR_x         = xercesc::XMLString::transcode("x");
    ATTR_y         = xercesc::XMLString::transcode("y");
    ATTR_z         = xercesc::XMLString::transcode("z");
    ATTR_point     = xercesc::XMLString::transcode("point");
    ATTR_kind      = xercesc::XMLString::transcode("kind");
    ATTR_distance  = xercesc::XMLString::transcode("distance");

    TAG_GCS_AS = xercesc::XMLString::transcode("GCS_AS");
    TAG_quaternion = xercesc::XMLString::transcode("quaternion");
    TAG_quaternions = xercesc::XMLString::transcode("quaternions");
    TAG_normal = xercesc::XMLString::transcode("normal");
    TAG_normals = xercesc::XMLString::transcode("normals");
    TAG_displacement = xercesc::XMLString::transcode("displacement");
    TAG_displacements = xercesc::XMLString::transcode("displacements");
    TAG_solid = xercesc::XMLString::transcode("solid");
    TAG_solids = xercesc::XMLString::transcode("solids");
    KIND_Parallel_AS = xercesc::XMLString::transcode("Parallel_AS");
    KIND_Distance_AS = xercesc::XMLString::transcode("Distance_AS");
    ATTR_a = xercesc::XMLString::transcode("a");
    ATTR_b= xercesc::XMLString::transcode("b");
    ATTR_c = xercesc::XMLString::transcode("c");
    ATTR_d = xercesc::XMLString::transcode("d");
    ATTR_type = xercesc::XMLString::transcode("type");
    ATTR_quaternion = xercesc::XMLString::transcode("quaternion");
    ATTR_normal = xercesc::XMLString::transcode("normal");
    ATTR_displacement = xercesc::XMLString::transcode("displacement");
    ATTR_solid1 = xercesc::XMLString::transcode("solid1");
    ATTR_solid2 = xercesc::XMLString::transcode("solid2");
    
    
    m_InputFileParser = new xercesc::XercesDOMParser;
}

InputParser::~InputParser()
{
    // Free memory
    delete m_InputFileParser;
}

bool InputParser::readInputFileAS(std::string &fileName,
                                  std::vector<double *> &variables,
                                  std::vector<double *> &parameters,
                                  std::vector<GCS::Point> &points,
                                  std::vector<GCS::Normal> &norms,
                                  std::vector<GCS::Displacement> &disps,
                                  std::vector<GCS::Quaternion> &quats,
				  std::vector<GCS::Solid> &solids,
                                  std::vector<GCS::Constraint *> &constraints)
{
    int pointsOffset = points.size();
    int normsOffset  = norms.size();
    int dispsOffset = disps.size();
    int quatsOffset =  quats.size();
    int solidsOffset = solids.size();

    // Test to see if the file is ok.
    struct stat fileStatus;
    if (stat(fileName.c_str(), &fileStatus) != 0) {
        std::cout << "Error reading the file " << fileName << std::endl;
        return false;
    }

    // Configure DOM parser.
    m_InputFileParser->setValidationScheme(xercesc::XercesDOMParser::Val_Never);
    m_InputFileParser->setDoNamespaces(false);
    m_InputFileParser->setDoSchema(false);
    m_InputFileParser->setLoadExternalDTD(false);

    try {
        m_InputFileParser->parse(fileName.c_str());

        // no need to free this pointer - owned by the parent parser object
        xercesc::DOMDocument* xmlDoc = m_InputFileParser->getDocument();

        // Get the top-level element: Name is "GCS_3D". No attributes for "GCS_3D"
        xercesc::DOMElement* elementRoot = xmlDoc->getDocumentElement();
        if (!elementRoot) {
            std::cout << "empty XML document" << std::endl;
            return false;
        }
        else if (!xercesc::XMLString::equals(elementRoot->getTagName(), TAG_GCS_AS)) {
            std::cout << "wrong root element in the XML file" << std::endl;
            return false;
        }

        // Parse XML file for tags of interest: "ApplicationSettings"
        // Look one level nested within "root". (child of root)
        xercesc::DOMNodeList* children = elementRoot->getChildNodes();
        const XMLSize_t nodeCount = children->getLength();

        // Points
        xercesc::DOMNodeList* pointsNodes = elementRoot->getElementsByTagName(TAG_points);
        const XMLSize_t pointsCount = pointsNodes->getLength();

        for (XMLSize_t i = 0; i < pointsCount; ++i) {
            xercesc::DOMNode* pointsNode = pointsNodes->item(i);
            xercesc::DOMElement* pointsElement = dynamic_cast<xercesc::DOMElement*>(pointsNode);

            xercesc::DOMNodeList* pointNodes = pointsElement->getElementsByTagName(TAG_point);
            const XMLSize_t pointCount = pointNodes->getLength();

            for (XMLSize_t j = 0; j < pointCount; ++j) {
                xercesc::DOMNode* pointNode = pointNodes->item(j);
                xercesc::DOMElement* pointElement = dynamic_cast<xercesc::DOMElement*>(pointNode);

                int id = getIntAttr(pointElement, ATTR_id);
                double x = getDoubleAttr(pointElement, ATTR_x);
                double y = getDoubleAttr(pointElement, ATTR_y);
                double z = getDoubleAttr(pointElement, ATTR_z);

                if (id + pointsOffset > points.size())
                    points.resize(id + pointsOffset);

                GCS::Point p;
                p.x = x;
                p.y = y;
                p.z = z;
                points[id + pointsOffset -1] = p;
            }
        }

        // normals
        xercesc::DOMNodeList* normsNodes = elementRoot->getElementsByTagName(TAG_normals);
        const XMLSize_t normsCount = normsNodes->getLength();

        for (XMLSize_t i = 0; i < normsCount; ++i) {
            xercesc::DOMNode* normsNode = normsNodes->item(i);
            xercesc::DOMElement* normsElement = dynamic_cast<xercesc::DOMElement*>(normsNode);

            xercesc::DOMNodeList* lineNodes = normsElement->getElementsByTagName(TAG_normal);
            const XMLSize_t lineCount = lineNodes->getLength();

            for (XMLSize_t j = 0; j < lineCount; ++j) {
                xercesc::DOMNode* normNode = lineNodes->item(j);
                xercesc::DOMElement* normElement = dynamic_cast<xercesc::DOMElement*>(normNode);

                int id 	 = getIntAttr(normElement, ATTR_id);
                double x = getDoubleAttr(normElement, ATTR_x);
                double y = getDoubleAttr(normElement, ATTR_y);
                double z = getDoubleAttr(normElement, ATTR_z);

                if (id + normsOffset > norms.size())
                    norms.resize(id + normsOffset);

                GCS::Normal p;
                p.x = x;
                p.y = y;
                p.z = z;
                norms[id + normsOffset -1] = p;
            }
        }

         // Displacements
        xercesc::DOMNodeList* dispsNodes = elementRoot->getElementsByTagName(TAG_displacements);
        const XMLSize_t dispsCount = dispsNodes->getLength();

        for (XMLSize_t i = 0; i < dispsCount; ++i) {
            xercesc::DOMNode* dispsNode = dispsNodes->item(i);
            xercesc::DOMElement* dispsElement = dynamic_cast<xercesc::DOMElement*>(dispsNode);

            xercesc::DOMNodeList* lineNodes = dispsElement->getElementsByTagName(TAG_displacement);
            const XMLSize_t lineCount = lineNodes->getLength();

            for (XMLSize_t j = 0; j < lineCount; ++j) {
                xercesc::DOMNode* dispNode = lineNodes->item(j);
                xercesc::DOMElement* dispElement = dynamic_cast<xercesc::DOMElement*>(dispNode);

                int id 	 = getIntAttr(dispElement, ATTR_id);
                double x = getDoubleAttr(dispElement, ATTR_x);
                double y = getDoubleAttr(dispElement, ATTR_y);
                double z = getDoubleAttr(dispElement, ATTR_z);

                if (id + dispsOffset > disps.size())
                    disps.resize(id + dispsOffset);

                // Memory allocation!!
                int varStartIndex = variables.size();
                variables.push_back(new double(x));
                variables.push_back(new double(y));
                variables.push_back(new double(z));

                GCS::Displacement p;
                p.x = variables[varStartIndex+0];
                p.y = variables[varStartIndex+1];
                p.z = variables[varStartIndex+2];
                disps[id + dispsOffset -1] = p;
            }
        }

        // Quaternions
        xercesc::DOMNodeList* quatsNodes = elementRoot->getElementsByTagName(TAG_quaternions);
        const XMLSize_t quatCount = quatsNodes->getLength();

        for (XMLSize_t i = 0; i < quatCount; ++i) {
            xercesc::DOMNode* quatsNode = quatsNodes->item(i);
            xercesc::DOMElement* quatsElement = dynamic_cast<xercesc::DOMElement*>(quatsNode);

            xercesc::DOMNodeList* quatNodes = quatsElement->getElementsByTagName(TAG_quaternion);
            const XMLSize_t quatCount = quatNodes->getLength();

            for (XMLSize_t j = 0; j < quatCount; ++j) {
                xercesc::DOMNode* quatNode = quatNodes->item(j);
                xercesc::DOMElement* quatElement = dynamic_cast<xercesc::DOMElement*>(quatNode);

                int id = getIntAttr(quatElement, ATTR_id);
                double a = getDoubleAttr(quatElement, ATTR_a);
                double b = getDoubleAttr(quatElement, ATTR_b);
                double c = getDoubleAttr(quatElement, ATTR_c);
                double d = getDoubleAttr(quatElement, ATTR_d);

                if (id + quatsOffset > quats.size())
                    quats.resize(id + quatsOffset);

                GCS::Quaternion q;
                // Memory allocation!!
                int varStartIndex = variables.size();
                variables.push_back(new double(a));
                variables.push_back(new double(b));
                variables.push_back(new double(c));
                variables.push_back(new double(d));

                q.a = variables[varStartIndex+0];
                q.b = variables[varStartIndex+1];
                q.c = variables[varStartIndex+2];
                q.d = variables[varStartIndex+3];
                quats[id + quatsOffset -1] = q;
            }
        }
        
        // Solids
        xercesc::DOMNodeList* solidsNodes = elementRoot->getElementsByTagName(TAG_solids);
        const XMLSize_t solidCount = solidsNodes->getLength();

        for (XMLSize_t i = 0; i < solidCount; ++i) {
            xercesc::DOMNode* solidsNode = solidsNodes->item(i);
            xercesc::DOMElement* solidsElement = dynamic_cast<xercesc::DOMElement*>(solidsNode);

            xercesc::DOMNodeList* solidNodes = solidsElement->getElementsByTagName(TAG_solid);
            const XMLSize_t solidCount = solidNodes->getLength();

            for (XMLSize_t j = 0; j < solidCount; ++j) {
                xercesc::DOMNode* solidNode = solidNodes->item(j);
                xercesc::DOMElement* solidElement = dynamic_cast<xercesc::DOMElement*>(solidNode);

                int id 	= getIntAttr(solidElement, ATTR_id);
                int p 	= getIntAttr(solidElement, ATTR_point);
		int n 	= getIntAttr(solidElement, ATTR_normal);
		int d 	= getIntAttr(solidElement, ATTR_displacement);
		int q 	= getIntAttr(solidElement, ATTR_quaternion);

                GCS::Solid s;
                s.p = points[p-1];
		s.n = norms[n-1];
		s.d = disps[d-1];
		s.q = quats[q-1];
		
		solids.push_back(s);
            }
        }
        
        

        // Constraints
        xercesc::DOMNodeList* constraintsNodes = elementRoot->getElementsByTagName(TAG_constraints);
        const XMLSize_t constraintsCount = constraintsNodes->getLength();

        for (XMLSize_t i = 0; i < constraintsCount; ++i) {
            xercesc::DOMNode* constraintsNode = constraintsNodes->item(i);
            xercesc::DOMElement* constraintsElement = dynamic_cast<xercesc::DOMElement*>(constraintsNode);

            xercesc::DOMNodeList* constraintNodes = constraintsElement->getElementsByTagName(TAG_constraint);
            const XMLSize_t constraintCount = constraintNodes->getLength();

            for (XMLSize_t j = 0; j < constraintCount; ++j) {
                xercesc::DOMNode* constraintNode = constraintNodes->item(j);
                xercesc::DOMElement* constraintElement = dynamic_cast<xercesc::DOMElement*>(constraintNode);

                int id = getIntAttr(constraintElement, ATTR_id);

                const XMLCh* xmlch_kind = constraintElement->getAttribute(ATTR_kind);

		//PlaneParallel Assembly
                if (xercesc::XMLString::equals(xmlch_kind, KIND_Parallel_AS)) {
                    int s1 = getIntAttr(constraintElement, ATTR_solid1);
                    int s2 = getIntAttr(constraintElement, ATTR_solid2);
		    int t  = getIntAttr(constraintElement, ATTR_type);

                    int paramStartIndex = parameters.size();
                    parameters.push_back(new double(t));

                    GCS::Constraint *constr
                    = new GCS::ConstraintParralelFaceAS( solids[s1-1], solids[s2-1], 
							 (GCS::ParallelType*) parameters[paramStartIndex] );
                    constraints.push_back(constr);
                }
                //Distance Assembly
                if (xercesc::XMLString::equals(xmlch_kind, KIND_Distance_AS)) {
                    int s1 = getIntAttr(constraintElement, ATTR_solid1);
                    int s2 = getIntAttr(constraintElement, ATTR_solid2);
		    double t  = getDoubleAttr(constraintElement, ATTR_distance);

                    int paramStartIndex = parameters.size();
                    parameters.push_back(new double(t));

                    GCS::Constraint *constr
                    = new GCS::ConstraintFaceDistanceAS( solids[s1-1], solids[s2-1], parameters[paramStartIndex] );
                    constraints.push_back(constr);
                }
            }
        }
        return true;
    }
    catch (xercesc::XMLException& e) {
        char* message = xercesc::XMLString::transcode(e.getMessage());
        std::cout << "Error parsing file: " << message << std::endl;
        xercesc::XMLString::release(&message);
    }
}

int InputParser::getIntAttr(xercesc::DOMElement* element, XMLCh* attr)
{
    const XMLCh* xmlch_val = element->getAttribute(attr);
    return xercesc::XMLString::parseInt(xmlch_val);
}

double InputParser::getDoubleAttr(xercesc::DOMElement* element, XMLCh* attr)
{
    const XMLCh* xmlch_val = element->getAttribute(attr);
    xercesc::XMLDouble xmldouble_val(xmlch_val);
    return xmldouble_val.getValue();
}

