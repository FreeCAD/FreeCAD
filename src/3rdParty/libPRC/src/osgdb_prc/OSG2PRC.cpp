
#include "OSG2PRC.h"

#include <osg/Transform>
#include <osg/Geode>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Geometry>

#include <iostream>


OSG2PRC::OSG2PRC()
    : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN )
{
    addDefaultMaterial();

    pushNodeAlpha();
    setNodeAlpha( 1.f );
}

#ifdef PRC_USE_ASYMPTOTE
OSG2PRC::OSG2PRC( oPRCFile* prcFile )
  : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ),
    _prcFile( prcFile )
{
    addDefaultMaterial();

    pushNodeAlpha();
    setNodeAlpha( 1.f );
}
#endif

OSG2PRC::~OSG2PRC()
{
}


void OSG2PRC::apply( osg::Node& node )
{
    //std::cout << "Found osg::Node" << std::endl;

    processNewNode( node.getName() );
    processNodeAlpha( &node );

    if( node.getStateSet() != NULL )
        apply( node.getStateSet() );

    traverse( node );

    finishNode();
}
void OSG2PRC::apply( osg::Transform& trans )
{
    //std::cout << "Found osg::Transform" << std::endl;

    osg::Matrix m;
    trans.computeLocalToWorldMatrix( m, NULL );
    processTransformNode( trans.getName(), m );
    processNodeAlpha( &trans );

    if( trans.getStateSet() != NULL )
        apply( trans.getStateSet() );

    traverse( trans );

    finishNode();
}
void OSG2PRC::apply( osg::Geode& geode )
{
    //std::cout << "Found osg::Geode" << std::endl;

    processNewNode( geode.getName() );
    processNodeAlpha( &geode );

    if( geode.getStateSet() != NULL )
        apply( geode.getStateSet() );

    for( unsigned int idx=0; idx < geode.getNumDrawables(); ++idx )
    {
        const osg::Geometry* geom( geode.getDrawable( idx )->asGeometry() );
        if( geom != NULL )
            apply( geom );
    }

    traverse( geode );

    finishNode();
}

void OSG2PRC::pushMaterial()
{
    _materialStack.resize( _materialStack.size() + 1 );
    if( _materialStack.size() > 1 )
    {
        // Copy old top of stack to current top of stack.
        _materialStack[ _materialStack.size() - 1 ] =
            _materialStack[ _materialStack.size() - 2 ];
    }
}
bool OSG2PRC::popMaterial()
{
    if( _materialStack.size() > 0 )
    {
        _materialStack.resize( _materialStack.size() - 1 );
        return( true );
    }
    return( false );
}
void OSG2PRC::setMaterial( const osg::Material* mat )
{
    if( _materialStack.size() > 0 )
        _materialStack[ _materialStack.size() - 1 ] = mat;
}
const osg::Material* OSG2PRC::getMaterial() const
{
    if( _materialStack.size() > 0 )
        return( _materialStack[ _materialStack.size() - 1 ].get() );
    else
        return( NULL );
}
void OSG2PRC::addDefaultMaterial()
{
    _materialStack.push_back( new osg::Material() );
}

void OSG2PRC::pushNodeAlpha()
{
    _nodeAlphaStack.resize( _nodeAlphaStack.size() + 1 );
    if( _nodeAlphaStack.size() > 1 )
    {
        // Copy old top of stack to current top of stack.
        _nodeAlphaStack[ _nodeAlphaStack.size() - 1 ] =
            _nodeAlphaStack[ _nodeAlphaStack.size() - 2 ];
    }
}
bool OSG2PRC::popNodeAlpha()
{
    if( _nodeAlphaStack.size() > 0 )
    {
        _nodeAlphaStack.resize( _nodeAlphaStack.size() - 1 );
        return( true );
    }
    return( false );
}
void OSG2PRC::setNodeAlpha( const float alpha )
{
    if( _nodeAlphaStack.size() > 0 )
        _nodeAlphaStack[ _nodeAlphaStack.size() - 1 ] = alpha;
}
float OSG2PRC::getNodeAlpha() const
{
    if( _nodeAlphaStack.size() > 0 )
        return( _nodeAlphaStack[ _nodeAlphaStack.size() - 1 ] );
    else
        return( 1.f );
}
bool OSG2PRC::checkNodeAlpha( float& alpha, const osg::Node* node )
{
    const std::string targetString( "nodeAlpha=" );
    for( unsigned int idx=0; idx < node->getNumDescriptions(); ++idx )
    {
        const std::string desc( node->getDescription( idx ) );
        if( desc.substr( 0, targetString.size() ) == targetString )
        {
            alpha = 
                atof( desc.substr( targetString.size() ).c_str() );
            return( true );
        }
    }
    return( false );
}

uint32_t OSG2PRC::getStyle( const osg::Material* mat, const float alpha )
{
    StyleAlphaKey key( mat, alpha );
    StyleAlphaMap::iterator it( _styleAlphaMap.find( key ) );
    if( it != _styleAlphaMap.end() )
        return( it->second );

    const osg::Material::Face face( osg::Material::FRONT );
    PRCmaterial m( colorToPRC( mat->getAmbient( face ) ),
        colorToPRC( mat->getDiffuse( face ) ),
        colorToPRC( mat->getEmission( face ) ),
        colorToPRC( mat->getSpecular( face ) ),
        alpha,
        (double)( mat->getShininess( face ) ) );

    const uint32_t style = _prcFile->addMaterial( m );
    _styleAlphaMap[ key ] = style;
    return( style );
}



void OSG2PRC::apply( const osg::StateSet* stateSet )
{
    //std::cout << "Found osg::StateSet" << std::endl;

    const osg::StateAttribute* sa( stateSet->getAttribute( osg::StateAttribute::MATERIAL ) );
    if( sa != NULL )
    {
        const osg::Material* mat( static_cast< const osg::Material* >( sa ) );
        setMaterial( mat );
    }
}
void OSG2PRC::apply( const osg::Geometry* geom )
{
    //std::cout << "Found osg::Geometry" << std::endl;

    if( geom->getStateSet() != NULL )
        apply( geom->getStateSet() );


    PRC3DTess *tess = createTess( geom );
    if( tess == NULL )
    {
        std::cerr << "Failed to create 3D Tess object." << std::endl;
        return;
    }
    const bool hasTexCoords( tess->texture_coordinate.size() > 0 );

    PRCTessFace *tessFace = new PRCTessFace();
    tessFace->number_of_texture_coordinate_indexes = hasTexCoords ? 1 : 0;
    tessFace->start_triangulated = 0;

    unsigned int tris( 0 ), strips( 0 ), fans( 0 );
    for( unsigned int idx=0; idx < geom->getNumPrimitiveSets(); ++idx )
    {
        const osg::PrimitiveSet* ps( geom->getPrimitiveSet( idx ) );
        switch( ps->getMode() ) {
        case GL_TRIANGLES:
        case GL_QUADS:
            ++tris;
            break;
        case GL_TRIANGLE_STRIP:
            ++strips;
            break;
        case GL_TRIANGLE_FAN:
            ++fans;
            break;
        }
    }

    // Triangle primitives first
    if( tris > 0 )
    {
        tessFace->used_entities_flag |= hasTexCoords ? PRC_FACETESSDATA_TriangleTextured : PRC_FACETESSDATA_Triangle;
        tessFace->sizes_triangulated.push_back( 0 );
        for( unsigned int idx=0; idx < geom->getNumPrimitiveSets(); ++idx )
        {
            const osg::PrimitiveSet* ps( geom->getPrimitiveSet( idx ) );
            if( ( ps->getMode() == GL_TRIANGLES ) ||
                    ( ps->getMode() == GL_QUADS ) )
                apply( ps, tess, tessFace, geom );
        }
    }
    // Triangle strips second
    if( strips > 0 )
    {
        tessFace->used_entities_flag |= hasTexCoords ? PRC_FACETESSDATA_TriangleStripeTextured : PRC_FACETESSDATA_TriangleStripe;
        tessFace->sizes_triangulated.push_back( strips );
        for( unsigned int idx=0; idx < geom->getNumPrimitiveSets(); ++idx )
        {
            const osg::PrimitiveSet* ps( geom->getPrimitiveSet( idx ) );
            if( ps->getMode() == GL_TRIANGLE_STRIP )
                apply( ps, tess, tessFace, geom );
        }
    }
    // Triangle fans third
    if( fans > 0 )
    {
        tessFace->used_entities_flag |= hasTexCoords ? PRC_FACETESSDATA_TriangleFanTextured : PRC_FACETESSDATA_TriangleFan;
        tessFace->sizes_triangulated.push_back( fans );
        for( unsigned int idx=0; idx < geom->getNumPrimitiveSets(); ++idx )
        {
            const osg::PrimitiveSet* ps( geom->getPrimitiveSet( idx ) );
            if( ps->getMode() == GL_TRIANGLE_FAN )
                apply( ps, tess, tessFace, geom );
        }
    }

    // Set is_rgba=true if we have a color array and it's a Vec4Array.
    tessFace->is_rgba = ( ( geom->getColorArray() != NULL ) &&
        ( geom->getColorArray()->getType() == osg::Array::Vec4ArrayType ) );

    // Add the tessface. Note this zeros the tessFace pointer.
    tess->addTessFace( tessFace );
    tess->has_faces = true;

    // add the tess mesh then use it
    const uint32_t tess_index = _prcFile->add3DTess( tess );
    uint32_t styleIndex( getStyle( getMaterial(), getNodeAlpha() ) );
    _prcFile->useMesh( tess_index, styleIndex );
}
void OSG2PRC::apply( const osg::PrimitiveSet* ps, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom )
{
    switch( ps->getType() ) {
    case osg::PrimitiveSet::DrawArraysPrimitiveType:
    {
        processDrawArrays( static_cast< const osg::DrawArrays* >( ps ), tess, tessFace, geom );
        break;
    }
    case osg::PrimitiveSet::DrawArrayLengthsPrimitiveType:
    {
        processDrawArrayLengths( static_cast< const osg::DrawArrayLengths* >( ps ), tess, tessFace, geom );
        break;
    }
    case osg::PrimitiveSet::DrawElementsUBytePrimitiveType:
    case osg::PrimitiveSet::DrawElementsUShortPrimitiveType:
    case osg::PrimitiveSet::DrawElementsUIntPrimitiveType:
    {
        processDrawElements( static_cast< const osg::DrawElements* >( ps ), tess, tessFace, geom );
        break;
    }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////////
PRC3DTess* OSG2PRC::createTess( const osg::Geometry* geom )
{
    const osg::Array* array( geom->getVertexArray() );
    if( array == NULL )
    {
        std::cerr << "osg::Geometry has zero vertices." << std::endl;
        return( NULL );
    }
    if( array->getType() != osg::Array::Vec3ArrayType )
    {
        std::cerr << "Vertex: Unsupported array type." << std::endl;
        return( NULL );
    }
    const osg::Vec3Array* vertices( static_cast< const osg::Vec3Array* >( array ) );
    //std::cout << "Adding vertex array to PRC, size " << array->getNumElements() << std::endl;

    PRC3DTess *tess = new PRC3DTess();
    tess->coordinates.reserve( vertices->size()*3 );
    for( uint32_t i=0; i<vertices->size(); i++ )
    {
        const osg::Vec3& v( vertices->at( i ) );
        tess->coordinates.push_back(v.x());
        tess->coordinates.push_back(v.y());
        tess->coordinates.push_back(v.z());
    }

    array = geom->getNormalArray();
    if( array != NULL )
    {
        if( array->getType() != osg::Array::Vec3ArrayType )
        {
            std::cerr << "Normal: Unsupported array type." << std::endl;
            delete tess;
            return NULL;
        }
        const osg::Vec3Array* normals( static_cast< const osg::Vec3Array* >( array ) );
        //std::cout << "Adding normals array to PRC, size " << array->getNumElements() << std::endl;

        tess->normal_coordinate.reserve( vertices->size()*3 );
        if( normals->size() == vertices->size() )
        {
            for( uint32_t i=0; i<normals->size(); i++ )
            {
                const osg::Vec3& v( normals->at( i ) );
                tess->normal_coordinate.push_back(v.x());
                tess->normal_coordinate.push_back(v.y());
                tess->normal_coordinate.push_back(v.z());
            }
        }
        else
        {
            // Normals array is a different size, BIND_OVERALL is likely cause.
            // Let's just fake it by repeating the first normal.
            const osg::Vec3& v( normals->at( 0 ) );
            for( uint32_t i=0; i<vertices->size(); i++ )
            {
                tess->normal_coordinate.push_back(v.x());
                tess->normal_coordinate.push_back(v.y());
                tess->normal_coordinate.push_back(v.z());
            }
        }
    }
    else
    {
        // For future aupport. According to spec, crease_angle is
        // used somehow when you set the flag to recalaulate normals.
        // Our exporter currently doesn't do this.
        tess->crease_angle = 25.8419;  // arccos(0.9), default found in Acrobat output; 
    }

    // TODO: support texture coordinates
    /*
    if( textured )
    {
        tess->texture_coordinate.reserve(2*nT);
        for(uint32_t i=0; i<nT; i++)
        {
            tess->texture_coordinate.push_back(T[i][0]);
            tess->texture_coordinate.push_back(T[i][1]);
        }
    }
    */
    return tess;
}

void OSG2PRC::processDrawArrays( const osg::DrawArrays* da, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom )
{
    const bool hasNormals( tess->normal_coordinate.size() > 0 );
    const bool hasTexCoords( tess->texture_coordinate.size() > 0 );

    // Add indices to the tess.
    uint32_t triCount = 0;
    const unsigned int first( da->getFirst() );
    const unsigned int lastPlusOne( da->getFirst() + da->getCount() );
    switch( da->getMode() )
    {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
    {
        for( unsigned int idx = first; idx < lastPlusOne; ++idx )
        {
            processIndex( idx, tess, tessFace, hasNormals, hasTexCoords, geom );
        }

        if( da->getMode() == GL_TRIANGLES )
            triCount = da->getCount() / 3;
        else // strip or fan
            triCount = ( da->getCount() >= 3 ) ? ( da->getCount() - 2 ) : 0;
        
        break;
    }
    case GL_QUADS:
    {
        for( unsigned int idx = first; idx+3 < lastPlusOne; idx += 4 )
        {
            const unsigned int curIdx0( idx );
            const unsigned int curIdx1( idx+1 );
            const unsigned int curIdx2( idx+2 );
            const unsigned int curIdx3( idx+3 );

            // Two triangles, A and B
            //   Triangle A, verts 0, 1, 2
            processIndex( curIdx0, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx1, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx2, tess, tessFace, hasNormals, hasTexCoords, geom );

            //   Triangle B, verts 0, 2, 3
            processIndex( curIdx0, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx2, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx3, tess, tessFace, hasNormals, hasTexCoords, geom );

            triCount += 2;
        }
        break;
    }
    default:
        break;
    }

    // update our face object
    switch( da->getMode() )
    {
    case GL_TRIANGLES:
    case GL_QUADS:
        tessFace->sizes_triangulated[ tessFace->sizes_triangulated.size()-1 ] += triCount;
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
        tessFace->sizes_triangulated.push_back( da->getCount() );
        break;
    }
}
void OSG2PRC::processDrawArrayLengths( const osg::DrawArrayLengths* dal, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom )
{
    std::cerr << "Warning: DrawArrayLengths not fully supported." << std::endl;

    osg::ref_ptr< osg::DrawArrays > da( new osg::DrawArrays( dal->getMode() ) );
    da->setFirst( dal->getFirst() );
    da->setNumInstances( dal->getNumInstances() );
    for( unsigned int idx=0; idx < dal->size(); ++idx )
    {
        da->setCount( (*dal)[ idx ] );
        processDrawArrays( da.get(), tess, tessFace, geom );
        da->setFirst( da->getFirst() + (*dal)[ idx ] );
    }
}
void OSG2PRC::processDrawElements( const osg::DrawElements* de, PRC3DTess* tess, PRCTessFace *tessFace, const osg::Geometry* geom )
{
    //std::cout << "Processing DrawElements." << std::endl;

    const bool hasNormals( tess->normal_coordinate.size() > 0 );
    const bool hasTexCoords( tess->texture_coordinate.size() > 0 );

    // Add indices to the tess.
    uint32_t triCount = 0;
    switch( de->getMode() )
    {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
    {
        for( unsigned int idx = 0; idx < de->getNumIndices(); ++idx )
        {
            const unsigned int curIdx( de->index( idx ) );
            processIndex( curIdx, tess, tessFace, hasNormals, hasTexCoords, geom );
        }

        if( de->getMode() == GL_TRIANGLES )
            triCount = de->getNumIndices() / 3;
        else // strip or fan
            triCount = ( de->getNumIndices() >= 3 ) ? ( de->getNumIndices() - 2 ) : 0;
        break;
    }
    case GL_QUADS:
    {
        for( unsigned int idx = 0; idx+3 < de->getNumIndices(); idx += 4 )
        {
            const unsigned int curIdx0( de->index( idx ) );
            const unsigned int curIdx1( de->index( idx+1 ) );
            const unsigned int curIdx2( de->index( idx+2 ) );
            const unsigned int curIdx3( de->index( idx+3 ) );

            // Two triangles, A and B
            //   Triangle A, verts 0, 1, 2
            processIndex( curIdx0, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx1, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx2, tess, tessFace, hasNormals, hasTexCoords, geom );

            //   Triangle B, verts 0, 2, 3
            processIndex( curIdx0, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx2, tess, tessFace, hasNormals, hasTexCoords, geom );
            processIndex( curIdx3, tess, tessFace, hasNormals, hasTexCoords, geom );

            triCount += 2;
        }
        break;
    }
    default:
        break;
    }

    // update our face object
    switch( de->getMode() )
    {
    case GL_TRIANGLES:
    case GL_QUADS:
        tessFace->sizes_triangulated[ tessFace->sizes_triangulated.size()-1 ] += triCount;
        break;
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLE_FAN:
        tessFace->sizes_triangulated.push_back( de->getNumIndices() );
        break;
    }
}

void OSG2PRC::processIndex( const unsigned int index, PRC3DTess* tess, PRCTessFace *tessFace,
                           const bool hasNormals, const bool hasTexCoords, const osg::Geometry* geom )
{
    if( hasNormals )
        tess->triangulated_index.push_back( index * 3 );
    if( hasTexCoords )
        tess->triangulated_index.push_back( index * 2 );
    tess->triangulated_index.push_back( index * 3 );

    const osg::Array* array( geom->getColorArray() );
    if( array != NULL )
    {
        const unsigned int useIndex( ( array->getNumElements() > index ) ? index : 0 );
        if( array->getType() == osg::Array::Vec3ArrayType )
        {
            const osg::Vec3Array* rgb( static_cast< const osg::Vec3Array* >( array ) );
            const osg::Vec3& c( (*rgb)[ useIndex ] );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.x() * 255. ) );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.y() * 255. ) );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.z() * 255. ) );
        }
        {
            const osg::Vec4Array* rgba( static_cast< const osg::Vec4Array* >( array ) );
            const osg::Vec4& c( (*rgba)[ useIndex ] );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.x() * 255. ) );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.y() * 255. ) );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.z() * 255. ) );
            tessFace->rgba_vertices.push_back( (uint8_t)( c.w() * 255. ) );
        }
    }
}



void OSG2PRC::processNewNode( const std::string& name )
{
    //std::cout << "Adding new node (with name " << name << ") to PRC" << std::endl;

    _prcFile->begingroup( name.c_str() );

    pushMaterial();
}
void OSG2PRC::processTransformNode( const std::string& name, const osg::Matrix& matrix )
{
    const double* d( matrix.ptr() );
    const osg::Matrix transpose(
        d[0], d[4], d[8], d[12],
        d[1], d[5], d[9], d[13],
        d[2], d[6], d[10], d[14],
        d[3], d[7], d[11], d[15] );

    _prcFile->begingroup( name.c_str(), NULL, transpose.ptr());

    pushMaterial();
}
void OSG2PRC::processNodeAlpha( const osg::Node* node )
{
    pushNodeAlpha();
    float alpha;
    if( checkNodeAlpha( alpha, node ) )
        setNodeAlpha( alpha );
}
void OSG2PRC::finishNode()
{
    _prcFile->endgroup();

    popMaterial();
    popNodeAlpha();
}
