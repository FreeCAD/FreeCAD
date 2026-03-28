
#ifndef __OSG_2_PRC_H__
#define __OSG_2_PRC_H__ 1

#include <osg/NodeVisitor>

#ifdef PRC_USE_ASYMPTOTE
#  include <oPRCFile.h>
#endif

#include <vector>
#include <map>


namespace osg {
    class MatrixTransform;
    class Geode;
    class Geometry;
    class StateSet;
    class Material;
    class DrawArrays;
    class DrawArrayLengths;
    class DrawElementsUByte;
    class DrawElementsUShort;
    class DrawElementsUInt;
}


/** \class OSG2PRC OSG2PRC.h
\brief OSG NodeVisitor that interfaces with libPRC.
\details OSG2PRC is invoked by ReaderWriterPRC to export an OSG
scene graph to the PRC file format.

Here are the OSG scene graph features currently supported, how they are
stored in the PRC file, and how they affect rendering with current
Adobe Reader.

\li Node names. OSG2PRC preseves the scene graph hierarchical structure with node names.
\li Transforms. Both osg::MatrixTransform and osg::PositionAttitudeTransform are supporeted.
\li Materials. OSG2PRC stores osg::Material values as a PRC material (http://livedocs.adobe.com/acrobat_sdk/9/Acrobat9_HTMLHelp/API_References/PRCReference/PRC_Format_Specification/group___tf_material_generic_____serialize2.html)
The PRC mesh objects created from osg::Geometry data reference the current
PRC material corresponding to the current osg::Material. Note that the osg::StateAttribute
OVERRIDE and PROTECTED modes are not currently supported.
\li Geometry. Vertex and normal, and color data are stored as PRC mesh and tess
objects. Only triangle, triangle strip, triangle fan, and quad primitives are supported.
\li Geometry colors. If the Geometry object has a color array and the binding
is OVERALL or PER_VERTEX, then the colors are stored in the PRC tess object.
This causes Adobe Reader to disable lighting and ignore all material colors.
The geometry is rendered as unlit and Gouraud-shaded. To render a model
with lighting and shading effects, the application must remove Geometry
colors or set the binding to PER_PRIMITIVE.
\li Transparency. OSG2PRC supports per-node alpha. This is done with
a plugin-specific description string. If a node contains a description string
of the form: \c "nodeAlpha=<a>", then \c "<a>" is converted to a floating point
number and stored (along with osg::Material values) in the PRC material
object. Note that this alpha value affects transparency regardless of
the presence of Geometry colors. Note also that, while alpha values stored in
Geometry colors are stored in the PRC file, currently Adobe Reader appears
to ignore them, instead taking its alpha value from the PRC material
associated with the PRC mesh.

There are several currently unsupported OSG features. Here are a
couple that are noteworthy:

\li Texture mapping. Texture objects and texture coordinate date are currently ignored.
\li Quad strips.
\li Line and point primitives.
\li Polygon mode.
\li osg::Depth.
*/
class OSG2PRC : public osg::NodeVisitor
{
public:
    OSG2PRC();
#ifdef PRC_USE_ASYMPTOTE
    OSG2PRC( oPRCFile* prcFile );
#endif
    virtual ~OSG2PRC();

    virtual void apply( osg::Node& node );
    virtual void apply( osg::Transform& trans );
    virtual void apply( osg::Geode& geode );

protected:
    void apply( const osg::StateSet* stateSet );

    void processNewNode( const std::string& name );
    void processTransformNode( const std::string& name, const osg::Matrix& matrix );
    void finishNode();

    void apply( const osg::Geometry* geom );
    static void apply( const osg::PrimitiveSet* ps, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom );

    static void processDrawArrays( const osg::DrawArrays* da, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom );
    static void processDrawArrayLengths( const osg::DrawArrayLengths* dal, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom );
    static void processDrawElements( const osg::DrawElements* de, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom );

    static void processIndex( const unsigned int index, PRC3DTess* tess, PRCTessFace *tessFace,
                           const bool hasNormals, const bool hasTexCoords, const osg::Geometry* geom );

    typedef std::vector< osg::ref_ptr< const osg::Material > > MaterialStack;
    MaterialStack _materialStack;
    void pushMaterial();
    bool popMaterial();
    void setMaterial( const osg::Material* mat );
    const osg::Material* getMaterial() const;
    void addDefaultMaterial();

    typedef std::vector< float > AlphaStack;
    AlphaStack _nodeAlphaStack;
    void pushNodeAlpha();
    bool popNodeAlpha();
    void setNodeAlpha( const float alpha );
    float getNodeAlpha() const;
    static bool checkNodeAlpha( float& alpha, const osg::Node* node );

    typedef std::pair< const osg::Material*, float > StyleAlphaKey;
    typedef std::map< StyleAlphaKey, uint32_t > StyleAlphaMap;
    StyleAlphaMap _styleAlphaMap;
    uint32_t getStyle( const osg::Material* mat, const float alpha );

    void processNodeAlpha( const osg::Node* node );


    PRC3DTess* createTess( const osg::Geometry* geom );

protected:
#ifdef PRC_USE_ASYMPTOTE

    static RGBAColour colorToPRC( const osg::Vec4& osgColor )
    {
        return( RGBAColour( osgColor[ 0 ],
            osgColor[ 1 ],osgColor[ 2 ],osgColor[ 3 ] ) );
    }

    oPRCFile* _prcFile;

#endif

};


// __OSG_2_PRC_H__
#endif
