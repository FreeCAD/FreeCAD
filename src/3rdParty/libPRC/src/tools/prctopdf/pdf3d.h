#ifndef __3DPDF_H__
#define __3DPDF_H__1

class Pdf3d
{
public:
    Pdf3d();
    virtual ~Pdf3d();

    bool createAdvancedPdf( const char *filepdf, const char *fileprc, const char *filejs );

protected:
    float _rcleft, _rctop, _rcwidth, _rcheight;
    float _bgr, _bgg, _bgb;

};

#endif
