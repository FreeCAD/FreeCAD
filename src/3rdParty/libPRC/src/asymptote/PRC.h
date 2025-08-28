#ifndef __PRC_H
#define __PRC_H

#ifdef _MSC_VER
#if _MSC_VER >= 1600
#include <stdint.h>
#else
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif // _MSC_VER >= 1600
#else
#include <inttypes.h>
#endif // _MSC_VER

//const uint32_t PRCVersion=7094;   // For Adobe Reader 8 or later
const uint32_t PRCVersion=8137; // For Adobe Reader 9 or later

// from Adobe's documentation

#define PRC_TYPE_Unknown			( (uint32_t)-1 )

#define PRC_TYPE_ROOT				0			// This type does not correspond to any entity

#define PRC_TYPE_ROOT_PRCBase			( PRC_TYPE_ROOT + 1 )	// Abstract root type for any PRC entity. 
#define PRC_TYPE_ROOT_PRCBaseWithGraphics	( PRC_TYPE_ROOT + 2 )	// Abstract root type for any PRC entity which can bear graphics. 

#define PRC_TYPE_CRV				( PRC_TYPE_ROOT + 10 )	// Types for PRC geometrical curves
#define PRC_TYPE_SURF				( PRC_TYPE_ROOT + 75 )	// Types for PRC geometrical surfaces
#define PRC_TYPE_TOPO				( PRC_TYPE_ROOT + 140 )	// Types for PRC topology
#define PRC_TYPE_TESS				( PRC_TYPE_ROOT + 170 )	// Types for PRC tessellation
#define PRC_TYPE_MISC				( PRC_TYPE_ROOT + 200 )	// Types for PRC global data
#define PRC_TYPE_RI				( PRC_TYPE_ROOT + 230 )	// Types for PRC representation items
#define PRC_TYPE_ASM				( PRC_TYPE_ROOT + 300 )	// Types for PRC assembly
#define PRC_TYPE_MKP				( PRC_TYPE_ROOT + 500 )	// Types for PRC markup
#define PRC_TYPE_GRAPH				( PRC_TYPE_ROOT + 700 )	// Types for PRC graphics
#define PRC_TYPE_MATH				( PRC_TYPE_ROOT + 900 )	// Types for PRC mathematical operators

#define PRC_TYPE_CRV_Base			( PRC_TYPE_CRV + 1 )	// Abstract type for all geometric curves. 
#define PRC_TYPE_CRV_Blend02Boundary		( PRC_TYPE_CRV + 2 )	// Boundary Curve. 
#define PRC_TYPE_CRV_NURBS			( PRC_TYPE_CRV + 3 )	// Non Uniform BSpline curve. 
#define PRC_TYPE_CRV_Circle			( PRC_TYPE_CRV + 4 )	// Circle. 
#define PRC_TYPE_CRV_Composite			( PRC_TYPE_CRV + 5 )	// Array of oriented curves. 
#define PRC_TYPE_CRV_OnSurf			( PRC_TYPE_CRV + 6 )	// Curve defined by a UV curve on a surface. 
#define PRC_TYPE_CRV_Ellipse			( PRC_TYPE_CRV + 7 )	// Ellipse. 
#define PRC_TYPE_CRV_Equation			( PRC_TYPE_CRV + 8 )	// curve described by specific law elements 
#define PRC_TYPE_CRV_Helix			( PRC_TYPE_CRV + 9 )	// Helix curve. 
#define PRC_TYPE_CRV_Hyperbola			( PRC_TYPE_CRV + 10 )	// Hyperbola. 
#define PRC_TYPE_CRV_Intersection		( PRC_TYPE_CRV + 11 )	// Intersection between 2 surfaces. 
#define PRC_TYPE_CRV_Line			( PRC_TYPE_CRV + 12 )	// Line. 
#define PRC_TYPE_CRV_Offset			( PRC_TYPE_CRV + 13 )	// Offset curve. 
#define PRC_TYPE_CRV_Parabola			( PRC_TYPE_CRV + 14 )	// Parabola. 
#define PRC_TYPE_CRV_PolyLine			( PRC_TYPE_CRV + 15 )	// Polyedric curve. 
#define PRC_TYPE_CRV_Transform			( PRC_TYPE_CRV + 16 )	// Transformed curve. 

#define PRC_TYPE_SURF_Base			( PRC_TYPE_SURF + 1 )	// Abstract type for all geometric surfaces. 
#define PRC_TYPE_SURF_Blend01			( PRC_TYPE_SURF + 2 )	// Blend surface. 
#define PRC_TYPE_SURF_Blend02			( PRC_TYPE_SURF + 3 )	// Blend Surface. 
#define PRC_TYPE_SURF_Blend03			( PRC_TYPE_SURF + 4 )	// Blend Surface. 
#define PRC_TYPE_SURF_NURBS			( PRC_TYPE_SURF + 5 )	// Non Uniform BSpline surface. 
#define PRC_TYPE_SURF_Cone			( PRC_TYPE_SURF + 6 )	// Cone. 
#define PRC_TYPE_SURF_Cylinder			( PRC_TYPE_SURF + 7 )	// Cylinder. 
#define PRC_TYPE_SURF_Cylindrical		( PRC_TYPE_SURF + 8 )	// Surface who is defined in cylindrical space. 
#define PRC_TYPE_SURF_Offset			( PRC_TYPE_SURF + 9 )	// Offset surface. 
#define PRC_TYPE_SURF_Pipe			( PRC_TYPE_SURF + 10 )	// Pipe. 
#define PRC_TYPE_SURF_Plane			( PRC_TYPE_SURF + 11 )	// Plane. 
#define PRC_TYPE_SURF_Ruled			( PRC_TYPE_SURF + 12 )	// Ruled surface. 
#define PRC_TYPE_SURF_Sphere			( PRC_TYPE_SURF + 13 )	// Sphere. 
#define PRC_TYPE_SURF_Revolution		( PRC_TYPE_SURF + 14 )	// Surface of revolution. 
#define PRC_TYPE_SURF_Extrusion			( PRC_TYPE_SURF + 15 )	// Surface of extrusion. 
#define PRC_TYPE_SURF_FromCurves		( PRC_TYPE_SURF + 16 )	// Surface from two curves. 
#define PRC_TYPE_SURF_Torus			( PRC_TYPE_SURF + 17 )	// Torus. 
#define PRC_TYPE_SURF_Transform			( PRC_TYPE_SURF + 18 )	// Transformed surface. 
#define PRC_TYPE_SURF_Blend04			( PRC_TYPE_SURF + 19 )	// defined for future use. 

#define PRC_TYPE_TOPO_Context			( PRC_TYPE_TOPO + 1 )	// Self-containing set of topological entities. 
#define PRC_TYPE_TOPO_Item			( PRC_TYPE_TOPO + 2 )	// Abstract root type for any topological entity (body or single item).
#define PRC_TYPE_TOPO_MultipleVertex		( PRC_TYPE_TOPO + 3 )	// Vertex whose position is the average of all edges' extremity positions to whom it belongs.
#define PRC_TYPE_TOPO_UniqueVertex		( PRC_TYPE_TOPO + 4 )	// Vertex with one set of coordinates (absolute position).
#define PRC_TYPE_TOPO_WireEdge			( PRC_TYPE_TOPO + 5 )	// Edge belonging to a wire body / single wire body.
#define PRC_TYPE_TOPO_Edge			( PRC_TYPE_TOPO + 6 )	// Edge belonging to a brep data.
#define PRC_TYPE_TOPO_CoEdge			( PRC_TYPE_TOPO + 7 )	// Usage of an edge in a loop.
#define PRC_TYPE_TOPO_Loop			( PRC_TYPE_TOPO + 8 )	// Array of co edges which delimits a face.
#define PRC_TYPE_TOPO_Face			( PRC_TYPE_TOPO + 9 )	// Topological face delimiting a shell.
#define PRC_TYPE_TOPO_Shell			( PRC_TYPE_TOPO + 10 )	// Topological shell (open or closed).
#define PRC_TYPE_TOPO_Connex			( PRC_TYPE_TOPO + 11 )	// Topological region delimited by one or several shells. 
#define PRC_TYPE_TOPO_Body			( PRC_TYPE_TOPO + 12 )	// Abstract root type for any topological body.
#define PRC_TYPE_TOPO_SingleWireBody		( PRC_TYPE_TOPO + 13 )	// Single wire body.
#define PRC_TYPE_TOPO_BrepData			( PRC_TYPE_TOPO + 14 )	// Main entry to solid and surface topology (regular form).
#define PRC_TYPE_TOPO_SingleWireBodyCompress	( PRC_TYPE_TOPO + 15 )	// Single wire body. (ultra compressed form).
#define PRC_TYPE_TOPO_BrepDataCompress		( PRC_TYPE_TOPO + 16 )	// Main entry to solid and surface topology (ultra compressed form).
#define PRC_TYPE_TOPO_WireBody			( PRC_TYPE_TOPO + 17 )	// This type is the main entry to wire topology. 

#define PRC_TYPE_TESS_Base			( PRC_TYPE_TESS + 1 )	// Abstract root type for any tessellated entity. 
#define PRC_TYPE_TESS_3D			( PRC_TYPE_TESS + 2 )	// Tessellated faceted data; regular form. 
#define PRC_TYPE_TESS_3D_Compressed		( PRC_TYPE_TESS + 3 )	// Tessellated faceted data; highly compressed form. 
#define PRC_TYPE_TESS_Face			( PRC_TYPE_TESS + 4 )	// Tessellated face. 
#define PRC_TYPE_TESS_3D_Wire			( PRC_TYPE_TESS + 5 )	// Tessellated wireframe. 
#define PRC_TYPE_TESS_Markup			( PRC_TYPE_TESS + 6 )	// Tessellated markup. 

#define PRC_TYPE_MISC_Attribute			( PRC_TYPE_MISC + 1 )	// Entity attribute. 
#define PRC_TYPE_MISC_CartesianTransformation	( PRC_TYPE_MISC + 2 )	// Cartesian transformation. 
#define PRC_TYPE_MISC_EntityReference		( PRC_TYPE_MISC + 3 )	// Entity reference. 
#define PRC_TYPE_MISC_MarkupLinkedItem		( PRC_TYPE_MISC + 4 )	// Link between a markup and an entity. 
#define PRC_TYPE_MISC_ReferenceOnPRCBase	( PRC_TYPE_MISC + 5 )	// Reference pointing on a regular entity (not topological). 
#define PRC_TYPE_MISC_ReferenceOnTopology	( PRC_TYPE_MISC + 6 )	// Reference pointing on a topological entity. 
#define PRC_TYPE_MISC_GeneralTransformation	( PRC_TYPE_MISC + 7 )	// General transformation.

#define PRC_TYPE_RI_RepresentationItem		( PRC_TYPE_RI + 1 )	// Basic abstract type for representation items. 
#define PRC_TYPE_RI_BrepModel			( PRC_TYPE_RI + 2 )	// Basic type for surfaces and solids. 
#define PRC_TYPE_RI_Curve			( PRC_TYPE_RI + 3 )	// Basic type for curves. 
#define PRC_TYPE_RI_Direction			( PRC_TYPE_RI + 4 )	// Optional point + vector. 
#define PRC_TYPE_RI_Plane			( PRC_TYPE_RI + 5 )	// Construction plane, as opposed to planar surface. 
#define PRC_TYPE_RI_PointSet			( PRC_TYPE_RI + 6 )	// Set of points. 
#define PRC_TYPE_RI_PolyBrepModel		( PRC_TYPE_RI + 7 )	// Basic type to polyhedral surfaces and solids. 
#define PRC_TYPE_RI_PolyWire			( PRC_TYPE_RI + 8 )	// Polyedric wireframe entity. 
#define PRC_TYPE_RI_Set				( PRC_TYPE_RI + 9 )	// Logical grouping of arbitrary number of representation items. 
#define PRC_TYPE_RI_CoordinateSystem		( PRC_TYPE_RI + 10 )	// Coordinate system. 

#define PRC_TYPE_ASM_ModelFile			( PRC_TYPE_ASM + 1 )	// Basic entry type for PRC. 
#define PRC_TYPE_ASM_FileStructure		( PRC_TYPE_ASM + 2 )	// Basic structure for PRC files. 
#define PRC_TYPE_ASM_FileStructureGlobals	( PRC_TYPE_ASM + 3 )	// Basic structure for PRC files : globals. 
#define PRC_TYPE_ASM_FileStructureTree		( PRC_TYPE_ASM + 4 )	// Basic structure for PRC files : tree. 
#define PRC_TYPE_ASM_FileStructureTessellation	( PRC_TYPE_ASM + 5 )	// Basic structure for PRC files : tessellation. 
#define PRC_TYPE_ASM_FileStructureGeometry	( PRC_TYPE_ASM + 6 )	// Basic structure for PRC files : geometry. 
#define PRC_TYPE_ASM_FileStructureExtraGeometry	( PRC_TYPE_ASM + 7 )	// Basic structure for PRC files : extra geometry data.
#define PRC_TYPE_ASM_ProductOccurence		( PRC_TYPE_ASM + 10 )	// Basic contruct for assemblies. 
#define PRC_TYPE_ASM_PartDefinition		( PRC_TYPE_ASM + 11 )	// Basic construct for parts. 
#define PRC_TYPE_ASM_Filter			( PRC_TYPE_ASM + 20 )

#define PRC_TYPE_MKP_View			( PRC_TYPE_MKP + 1 )	// Grouping of markup by views. 
#define PRC_TYPE_MKP_Markup			( PRC_TYPE_MKP + 2 )	// Basic type for simple markups. 
#define PRC_TYPE_MKP_Leader			( PRC_TYPE_MKP + 3 )	// basic type for markup leader 
#define PRC_TYPE_MKP_AnnotationItem		( PRC_TYPE_MKP + 4 )	// Usage of a markup.
#define PRC_TYPE_MKP_AnnotationSet		( PRC_TYPE_MKP + 5 )	// Group of annotations.
#define PRC_TYPE_MKP_AnnotationReference	( PRC_TYPE_MKP + 6 )	// Logical grouping of annotations for reference.

#define PRC_TYPE_GRAPH_Style			( PRC_TYPE_GRAPH + 1 )	// Display style. 
#define PRC_TYPE_GRAPH_Material			( PRC_TYPE_GRAPH + 2 )	// Display material properties. 
#define PRC_TYPE_GRAPH_Picture			( PRC_TYPE_GRAPH + 3 )	// Picture. 
#define PRC_TYPE_GRAPH_TextureApplication	( PRC_TYPE_GRAPH + 11 )	// Texture application. 
#define PRC_TYPE_GRAPH_TextureDefinition	( PRC_TYPE_GRAPH + 12 )	// Texture definition. 
#define PRC_TYPE_GRAPH_TextureTransformation	( PRC_TYPE_GRAPH + 13 )	// Texture transformation. 
#define PRC_TYPE_GRAPH_LinePattern		( PRC_TYPE_GRAPH + 21 )	// One dimensional display style. 
#define PRC_TYPE_GRAPH_FillPattern		( PRC_TYPE_GRAPH + 22 )	// Abstract class for two-dimensional display style. 
#define PRC_TYPE_GRAPH_DottingPattern		( PRC_TYPE_GRAPH + 23 )	// Two-dimensional filling with dots. 
#define PRC_TYPE_GRAPH_HatchingPattern		( PRC_TYPE_GRAPH + 24 )	// Two-dimensional filling with hatches. 
#define PRC_TYPE_GRAPH_SolidPattern		( PRC_TYPE_GRAPH + 25 )	// Two-dimensional filling with particular style (color, material, texture). 
#define PRC_TYPE_GRAPH_VPicturePattern		( PRC_TYPE_GRAPH + 26 )	// Two-dimensional filling with vectorised picture. 
#define PRC_TYPE_GRAPH_AmbientLight		( PRC_TYPE_GRAPH + 31 )	// Scene ambient illumination. 
#define PRC_TYPE_GRAPH_PointLight		( PRC_TYPE_GRAPH + 32 )	// Scene point illumination. 
#define PRC_TYPE_GRAPH_DirectionalLight		( PRC_TYPE_GRAPH + 33 )	// Scene directional illumination. 
#define PRC_TYPE_GRAPH_SpotLight		( PRC_TYPE_GRAPH + 34 )	// Scene spot illumination. 
#define PRC_TYPE_GRAPH_SceneDisplayParameters	( PRC_TYPE_GRAPH + 41 )	// Parameters for scene visualisation. 
#define PRC_TYPE_GRAPH_Camera			( PRC_TYPE_GRAPH + 42 )	// 

#define PRC_TYPE_MATH_FCT_1D			( PRC_TYPE_MATH + 1 )	// Basic type for one degree equation object. 
#define PRC_TYPE_MATH_FCT_1D_Polynom		( PRC_TYPE_MATH_FCT_1D + 1 )	// Polynomial equation. 
#define PRC_TYPE_MATH_FCT_1D_Trigonometric	( PRC_TYPE_MATH_FCT_1D + 2 )	// Cosinus based equation. 
#define PRC_TYPE_MATH_FCT_1D_Fraction		( PRC_TYPE_MATH_FCT_1D + 3 )	// Fraction between 2 one degree equation object. 
#define PRC_TYPE_MATH_FCT_1D_ArctanCos		( PRC_TYPE_MATH_FCT_1D + 4 )	// Specific equation. 
#define PRC_TYPE_MATH_FCT_1D_Combination	( PRC_TYPE_MATH_FCT_1D + 5 )	// Combination of one degree equation object. 
#define PRC_TYPE_MATH_FCT_3D			( PRC_TYPE_MATH + 10 )	// Basic type for 3rd degree equation object. 
#define PRC_TYPE_MATH_FCT_3D_Linear		( PRC_TYPE_MATH_FCT_3D + 1 )	// Linear transformation ( with a matrix ). 
#define PRC_TYPE_MATH_FCT_3D_NonLinear		( PRC_TYPE_MATH_FCT_3D + 2 )	// Specific transformation. 

#define PRC_PRODUCT_FLAG_DEFAULT        0x0001
#define PRC_PRODUCT_FLAG_INTERNAL       0x0002
#define PRC_PRODUCT_FLAG_CONTAINER      0x0004
#define PRC_PRODUCT_FLAG_CONFIG         0x0008
#define PRC_PRODUCT_FLAG_VIEW           0x0010

#define PRC_TRANSFORMATION_Identity     0x00
#define PRC_TRANSFORMATION_Translate    0x01
#define PRC_TRANSFORMATION_Rotate       0x02
#define PRC_TRANSFORMATION_Mirror       0x04
#define PRC_TRANSFORMATION_Scale        0x08
#define PRC_TRANSFORMATION_NonUniformScale 0x10
#define PRC_TRANSFORMATION_NonOrtho     0x20
#define PRC_TRANSFORMATION_Homogeneous  0x40

#define PRC_FACETESSDATA_Polyface                          0x0001
#define PRC_FACETESSDATA_Triangle                          0x0002
#define PRC_FACETESSDATA_TriangleFan                       0x0004
#define PRC_FACETESSDATA_TriangleStripe                    0x0008
#define PRC_FACETESSDATA_PolyfaceOneNormal                 0x0010
#define PRC_FACETESSDATA_TriangleOneNormal                 0x0020
#define PRC_FACETESSDATA_TriangleFanOneNormal              0x0040
#define PRC_FACETESSDATA_TriangleStripeOneNormal           0x0080
#define PRC_FACETESSDATA_PolyfaceTextured                  0x0100
#define PRC_FACETESSDATA_TriangleTextured                  0x0200
#define PRC_FACETESSDATA_TriangleFanTextured               0x0400
#define PRC_FACETESSDATA_TriangleStripeTextured            0x0800
#define PRC_FACETESSDATA_PolyfaceOneNormalTextured         0x1000
#define PRC_FACETESSDATA_TriangleOneNormalTextured         0x2000
#define PRC_FACETESSDATA_TriangleFanOneNormalTextured      0x4000
#define PRC_FACETESSDATA_TriangleStripeOneNormalTextured   0x8000
#define PRC_FACETESSDATA_NORMAL_Single   		0x40000000
#define PRC_FACETESSDATA_NORMAL_Mask			0x3FFFFFFF
#define PRC_FACETESSDATA_WIRE_IsNotDrawn		0x4000		// Indicates that the edge should not be drawn (its neighbor will be drawn).
#define PRC_FACETESSDATA_WIRE_IsClosing			0x8000		// Indicates that this is the last edge of a loop.

#define PRC_3DWIRETESSDATA_IsClosing			0x10000000 // Indicates that the first point is implicitely repeated after the last one to close the wire edge.
#define PRC_3DWIRETESSDATA_IsContinuous			0x20000000 // Indicates that the last point of the preceding wire should be linked with the first point of the current one.

#define PRC_TEXTURE_MAPPING_DIFFUSE			0x0001 // Diffuse texture mapping attribute. Default value.
#define PRC_TEXTURE_MAPPING_BUMP			0x0002 // Bump texture mapping attribute.
#define PRC_TEXTURE_MAPPING_OPACITY			0x0004 // Opacity texture mapping attribute.
#define PRC_TEXTURE_MAPPING_SPHERICAL_REFLECTION	0x0008 // Spherical reflection texture mapping attribute (used for environment mapping).
#define PRC_TEXTURE_MAPPING_CUBICAL_REFLECTION		0x0010 // Cubical reflection texture mapping attribute (used for environment mapping).
#define PRC_TEXTURE_MAPPING_REFRACTION			0x0020 // Refraction texture mapping attribute.
#define PRC_TEXTURE_MAPPING_SPECULAR			0x0040 // Specular texture mapping attribute.
#define PRC_TEXTURE_MAPPING_AMBIENT			0x0080 // Ambient texture mapping attribute.
#define PRC_TEXTURE_MAPPING_EMISSION			0x0100 // Emission texture mapping attribute.

#define PRC_TEXTURE_APPLYING_MODE_NONE		0x00 // let the application choose
#define PRC_TEXTURE_APPLYING_MODE_LIGHTING	0x01 // use lighting mode
#define PRC_TEXTURE_APPLYING_MODE_ALPHATEST	0x02 // use alpha test
#define PRC_TEXTURE_APPLYING_MODE_VERTEXCOLOR	0x04 // combine a texture with one-color-per-vertex mode

#define PRC_TEXTURE_MAPPING_COMPONENTS_RED	0x0001 // Red texture mapping component.
#define PRC_TEXTURE_MAPPING_COMPONENTS_GREEN	0x0002 // Green texture mapping component.
#define PRC_TEXTURE_MAPPING_COMPONENTS_BLUE	0x0004 // Blue texture mapping component.
#define PRC_TEXTURE_MAPPING_COMPONENTS_RGB	0x0007 // RGB texture mapping component.
#define PRC_TEXTURE_MAPPING_COMPONENTS_ALPHA	0x0008 // Alpha texture mapping component.
#define PRC_TEXTURE_MAPPING_COMPONENTS_RGBA	0x000F // RGBA texture mapping component.

enum EPRCModellerAttributeType {
  KEPRCModellerAttributeTypeNull = 0,
  KEPRCModellerAttributeTypeInt = 1,
  KEPRCModellerAttributeTypeReal = 2,
  KEPRCModellerAttributeTypeTime = 3,
  KEPRCModellerAttributeTypeString = 4
};

enum EPRCPictureDataFormat {
  KEPRCPicture_PNG,
  KEPRCPicture_JPG,
  KEPRCPicture_BITMAP_RGB_BYTE,
  KEPRCPicture_BITMAP_RGBA_BYTE,
  KEPRCPicture_BITMAP_GREY_BYTE,
  KEPRCPicture_BITMAP_GREYA_BYTE
};

enum EPRCProductLoadStatus {
  KEPRCProductLoadStatus_Unknown = 0,
  KEPRCProductLoadStatus_Error,
  KEPRCProductLoadStatus_NotLoaded,
  KEPRCProductLoadStatus_NotLoadable,
  KEPRCProductLoadStatus_Loaded
};

enum EPRCExtendType { 
  KEPRCExtendTypeNone = 0,      // Discontinuous position.
  KEPRCExtendTypeExt1 = 2,      // Same as EPRCExtendTypeCInfinity.
  KEPRCExtendTypeExt2 = 4,      // Same as EPRCExtendTypeG1R for surface, and EPRCExtendTypeG1 for curve.
  KEPRCExtendTypeG1 = 6,        // Continuous in direction but not magnitude of first derivative.
  KEPRCExtendTypeG1R = 8,       // Surface extended with a ruled surface that connects with G1-continuity.
  KEPRCExtendTypeG1_G2 = 10,    // Extended by reflection, yielding a G2 continuous extension.
  KEPRCExtendTypeCInfinity = 12 // Unlimited continuity.
};

enum EPRCKnotType {			// Knot vector type
  KEPRCKnotTypeUniformKnots,		// Uniform knot vector.
  KEPRCKnotTypeUnspecified,		// Unspecified knot type.
  KEPRCKnotTypeQuasiUniformKnots,	// Quasi-uniform knot vector.
  KEPRCKnotTypePiecewiseBezierKnots	// Extrema with multiplicities of degree +1.
};					// Note : this value is currently unused and should be set to KEPRCKnotTypeUnspecified.


enum EPRCBSplineSurfaceForm {
  KEPRCBSplineSurfaceFormPlane,			// Planar surface.
  KEPRCBSplineSurfaceFormCylindrical,		// Cylindrical surface.
  KEPRCBSplineSurfaceFormConical,		// Conical surface.
  KEPRCBSplineSurfaceFormSpherical,		// Spherical surface.
  KEPRCBSplineSurfaceFormRevolution,		// Surface of revolution.
  KEPRCBSplineSurfaceFormRuled,			// Ruled surface.
  KEPRCBSplineSurfaceFormGeneralizedCone,	// Cone.
  KEPRCBSplineSurfaceFormQuadric,		// Quadric surface.
  KEPRCBSplineSurfaceFormLinearExtrusion,	// Surface of extrusion.
  KEPRCBSplineSurfaceFormUnspecified,		// Unspecified surface.
  KEPRCBSplineSurfaceFormPolynomial		// Polynomial surface.
};

enum EPRCBSplineCurveForm {		// NURBS curve form
  KEPRCBSplineCurveFormUnspecified,	// Unspecified curve form.
  KEPRCBSplineCurveFormPolyline,	// Polygon.
  KEPRCBSplineCurveFormCircularArc,	// Circle arc.
  KEPRCBSplineCurveFormEllipticArc,	// Elliptical arc.
  KEPRCBSplineCurveFormParabolicArc,	// Parabolic arc.
  KEPRCBSplineCurveFormHyperbolicArc	// Hyperbolic arc.
};					// Note : this value is currently unused and should be set to KEPRCBSplineCurveFormUnspecified.

enum EPRCTextureMappingType {			// Defines how to retrieve mapping coordinates.
  KEPRCTextureMappingType_Unknown,		// Let the application choose.
  KEPRCTextureMappingType_Stored,		// Use the mapping coordinates that are stored on a 3D tessellation object
  KEPRCTextureMappingType_Parametric,		// Retrieve the UV coordinates on the surface as mapping coordinates
  KEPRCTextureMappingType_Operator		// Use the defined Texture mapping operator to calculate mapping coordinates
};

enum EPRCTextureFunction {			// Defines how to paint a texture on the surface being rendered.
  KEPRCTextureFunction_Unknown,			// Let the application choose.
  KEPRCTextureFunction_Modulate,		// Combine lighting with texturing. This is the default value.
  KEPRCTextureFunction_Replace,			// Replace the object color with texture color data.
  KEPRCTextureFunction_Blend,			// Reserved for future use.
  KEPRCTextureFunction_Decal			// Reserved for future use.
};

enum EPRCTextureMappingOperator {		// The operator to use when computing mapping coordinates.
  KEPRCTextureMappingOperator_Unknown,		// Default value
  KEPRCTextureMappingOperator_Planar,		// Reserved for future use
  KEPRCTextureMappingOperator_Cylindrical,	// Reserved for future use
  KEPRCTextureMappingOperator_Spherical, 	// Reserved for future use
  KEPRCTextureMappingOperator_Cubical 		// Reserved for future use
};

enum EPRCTextureBlendParameter {		// Reserved for future use. Defines how to apply blending.
  KEPRCTextureBlendParameter_Unknown,		// Default value.
  KEPRCTextureBlendParameter_Zero,		// Reserved for future use.
  KEPRCTextureBlendParameter_One,		// Reserved for future use.
  KEPRCTextureBlendParameter_SrcColor,		// Reserved for future use.
  KEPRCTextureBlendParameter_OneMinusSrcColor,	// Reserved for future use.
  KEPRCTextureBlendParameter_DstColor,		// Reserved for future use.
  KEPRCTextureBlendParameter_OneMinusDstColor,	// Reserved for future use.
  KEPRCTextureBlendParameter_SrcAlpha,		// Reserved for future use.
  KEPRCTextureBlendParameter_OneMinusSrcAlpha,	// Reserved for future use.
  KEPRCTextureBlendParameter_DstAlpha,		// Reserved for future use.
  KEPRCTextureBlendParameter_OneMinusDstAlpha,	// Reserved for future use.
  KEPRCTextureBlendParameter_SrcAlphaSaturate	// Reserved for future use.
};

enum EPRCTextureWrappingMode {			// Defines repeating and clamping texture modes.
  KEPRCTextureWrappingMode_Unknown,		// Let the application choose.
  KEPRCTextureWrappingMode_Repeat,		// Display the repeated texture on the surface.
  KEPRCTextureWrappingMode_ClampToBorder,	// Clamp the texture to the border. Display the surface color along the texture limits.
  KEPRCTextureWrappingMode_Clamp,		// Reserved for future use.
  KEPRCTextureWrappingMode_ClampToEdge,		// Reserved for future use.
  KEPRCTextureWrappingMode_MirroredRepeat	// Reserved for future use.
};

enum EPRCTextureAlphaTest {			// Reserved for future use. Defines how to use a texture alpha test.
  KEPRCTextureAlphaTest_Unknown,		// Default value.
  KEPRCTextureAlphaTest_Never,			// Reserved for future use.
  KEPRCTextureAlphaTest_Less,			// Reserved for future use.
  KEPRCTextureAlphaTest_Equal,			// Reserved for future use.
  KEPRCTextureAlphaTest_Lequal,			// Reserved for future use.
  KEPRCTextureAlphaTest_Greater,		// Reserved for future use.
  KEPRCTextureAlphaTest_Notequal,		// Reserved for future use.
  KEPRCTextureAlphaTest_Gequal,			// Reserved for future use.
  KEPRCTextureAlphaTest_Always			// Reserved for future use.
};


// Bit field for graphics behavior
#define PRC_GRAPHICS_Show			0x0001 // The entity is shown.
#define PRC_GRAPHICS_SonHeritShow		0x0002 // Shown entity son inheritance.
#define PRC_GRAPHICS_FatherHeritShow		0x0004 // Shown entity father inheritance.
#define PRC_GRAPHICS_SonHeritColor		0x0008 // Color/material son inheritance.
#define PRC_GRAPHICS_FatherHeritColor		0x0010 // Color/material father inheritance.
#define PRC_GRAPHICS_SonHeritLayer		0x0020 // Layer son inheritance.
#define PRC_GRAPHICS_FatherHeritLayer		0x0040 // Layer father inheritance.
#define PRC_GRAPHICS_SonHeritTransparency	0x0080 // Transparency son inheritance.
#define PRC_GRAPHICS_FatherHeritTransparency	0x0100 // Transparency father inheritance.
#define PRC_GRAPHICS_SonHeritLinePattern	0x0200 // Line pattern son inheritance.
#define PRC_GRAPHICS_FatherHeritLinePattern	0x0400 // Line pattern father inheritance.
#define PRC_GRAPHICS_SonHeritLineWidth		0x0800 // Line width son inheritance.
#define PRC_GRAPHICS_FatherHeritLineWidth	0x1000 // Line width father inheritance.
#define PRC_GRAPHICS_Removed			0x2000 // The entity has been removed and no longer appears in the tree.

enum EPRCMarkupType {
  KEPRCMarkupType_Unknown = 0,
  KEPRCMarkupType_Text,
  KEPRCMarkupType_Dimension,
  KEPRCMarkupType_Arrow,
  KEPRCMarkupType_Balloon,
  KEPRCMarkupType_CircleCenter,
  KEPRCMarkupType_Coordinate,
  KEPRCMarkupType_Datum,
  KEPRCMarkupType_Fastener,
  KEPRCMarkupType_Gdt,
  KEPRCMarkupType_Locator,
  KEPRCMarkupType_MeasurementPoint,
  KEPRCMarkupType_Roughness,
  KEPRCMarkupType_Welding,
  KEPRCMarkupType_Table,
  KEPRCMarkupType_Other 
};

enum EPRCMarkupSubType {
  KEPRCMarkupSubType_Unknown = 0,
  KEPRCMarkupSubType_EnumMax,

  KEPRCMarkupSubType_Datum_Ident = 1 ,
  KEPRCMarkupSubType_Datum_EnumMax, 

  KEPRCMarkupSubType_Dimension_Distance = 1,
  KEPRCMarkupSubType_Dimension_Radius_Tangent,
  KEPRCMarkupSubType_Dimension_Radius_Cylinder,
  KEPRCMarkupSubType_Dimension_Radius_Edge,
  KEPRCMarkupSubType_Dimension_Diameter,
  KEPRCMarkupSubType_Dimension_Diameter_Tangent,
  KEPRCMarkupSubType_Dimension_Diameter_Cylinder,
  KEPRCMarkupSubType_Dimension_Diameter_Edge, 
  KEPRCMarkupSubType_Dimension_Diameter_Cone,
  KEPRCMarkupSubType_Dimension_Length,
  KEPRCMarkupSubType_Dimension_Length_Curvilinear,
  KEPRCMarkupSubType_Dimension_Length_Circular,
  KEPRCMarkupSubType_Dimension_Angle,
  KEPRCMarkupSubType_Dimension_EnumMax,

  KEPRCMarkupSubType_Gdt_Fcf = 1,
  KEPRCMarkupSubType_Gdt_EnumMax,

  KEPRCMarkupSubType_Welding_Line = 1,
  KEPRCMarkupSubType_Welding_EnumMax,

  KEPRCMarkupSubType_Other_Symbol_User = 1,
  KEPRCMarkupSubType_Other_EnumMax 
};

#define PRC_MARKUP_IsHidden		0x01	// The tessellation is hidden.
#define PRC_MARKUP_HasFrame		0x02	// The tessellation has a frame.
#define PRC_MARKUP_IsNotModifiable	0x04	// The tessellation is given and should not be modified.
#define PRC_MARKUP_IsZoomable		0x08	// The tessellation has zoom capability.
#define PRC_MARKUP_IsOnTop		0x10	// The tessellation is on top of the geometry.
#define PRC_MARKUP_IsFlipable		0x20	// The text tessellation can be flipped to always be readable on screen. This value is currently unused.

#define PRC_RENDERING_PARAMETER_SPECIAL_CULLING	0x0001 // special culling strategy to apply
#define PRC_RENDERING_PARAMETER_FRONT_CULLING	0x0002 // apply front culling (ignored if no special culling strategy)
#define PRC_RENDERING_PARAMETER_BACK_CULLING	0x0004 // apply back culling (ignored if no special culling strategy)
#define PRC_RENDERING_PARAMETER_NO_LIGHT	0x0008 // if set, no light will apply on corresponding object

#define PRC_MARKUP_IsMatrix		0x08000000 // Bit to denote that the current entity is a matrix.
#define PRC_MARKUP_IsExtraData		0x04000000 // Bit to denote that the current entity is extra data (it is neither a matrix nor a polyline).
#define PRC_MARKUP_IntegerMask		0xFFFFF    // Integer mask to retrieve sizes.
#define PRC_MARKUP_ExtraDataType	0x3E00000  // Mask to retrieve the integer type of the entity.

#define PRC_MARKUP_ExtraDataType_Pattern	(( 0<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Picture	(( 1<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Triangles	(( 2<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Quads		(( 3<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_FaceViewMode	(( 6<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_FrameDrawMode	(( 7<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_FixedSizeMode	(( 8<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Symbol		(( 9<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Cylinder	((10<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Color		((11<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_LineStipple	((12<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Font		((13<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Text		((14<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Points		((15<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_Polygon	((16<<21)|PRC_MARKUP_IsExtraData)
#define PRC_MARKUP_ExtraDataType_LineWidth	((17<<21)|PRC_MARKUP_IsExtraData)

enum EPRCCharSet
{
  KEPRCCharsetUnknown = -1,
  KEPRCCharsetRoman = 0,
  KEPRCCharsetJapanese,
  KEPRCCharsetTraditionalChinese,
  KEPRCCharsetKorean,
  KEPRCCharsetArabic,
  KEPRCCharsetHebrew,
  KEPRCCharsetGreek,
  KEPRCCharsetCyrillic,
  KEPRCCharsetRightLeft,
  KEPRCCharsetDevanagari,
  KEPRCCharsetGurmukhi,
  KEPRCCharsetGujarati,
  KEPRCCharsetOriya,
  KEPRCCharsetBengali,
  KEPRCCharsetTamil,
  KEPRCCharsetTelugu,
  KEPRCCharsetKannada,
  KEPRCCharsetMalayalam,
  KEPRCCharsetSinhalese,
  KEPRCCharsetBurmese,
  KEPRCCharsetKhmer,
  KEPRCCharsetThai,
  KEPRCCharsetLaotian,
  KEPRCCharsetGeorgian,
  KEPRCCharsetArmenian,
  KEPRCCharsetSimplifiedChinese,
  KEPRCCharsetTibetan,
  KEPRCCharsetMongolian,
  KEPRCCharsetGeez,
  KEPRCCharsetEastEuropeanRoman,
  KEPRCCharsetVietnamese,
  KEPRCCharsetExtendedArabic
};

#define PRC_Font_Bold         0x02    /*!< Bold. */
#define PRC_Font_Italic       0x04    /*!< Italic. */
#define PRC_Font_Underlined   0x08    /*!< Underlined. */
#define PRC_Font_StrikedOut   0x10    /*!< Striked-out. */
#define PRC_Font_Overlined    0x20    /*!< Overlined. */
#define PRC_Font_Streched     0x40    /*!< Streched. In case the font used is not the original font, it indicates that the text needs to be stretched to fit its bounding box. */
#define PRC_Font_Wired        0x80    /*!< Wired. Indicates that the original font is a wirefame font. */
#define PRC_Font_FixedWidth   0x100   /*!< Fixed width. Indicates that the original font is not proportional (each glyph has the same width). */

#define PRC_CONTEXT_OuterLoopFirst 0x0001 // Outer loops are always first loops (specific to PRC_TYPE_TOPO_BrepData). 
#define PRC_CONTEXT_NoClamp        0x0002 // UV curves are clamped on the surface (specific to PRC_TYPE_TOPO_BrepData). 
#define PRC_CONTEXT_NoSplit        0x0004 // Faces are split (specific to PRC_TYPE_TOPO_BrepData). 

#define PRC_BODY_BBOX_Evaluation 0x0001 // Bounding box based on geometry. 
#define PRC_BODY_BBOX_Precise    0x0002 // Bounding box based on tessellation. 
#define PRC_BODY_BBOX_CADData    0x0003 // Bounding box given by a CAD data file. 

#endif // __PRC_H
