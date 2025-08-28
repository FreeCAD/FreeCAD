#include "pdf3d.h"

#include <setjmp.h>
#include <hpdf.h>
#include <hpdf_conf.h>
#include <hpdf_u3d.h>
#include <hpdf_annotation.h>

//jmp_buf env;

typedef struct 
{
  double x,y,z;
} XYZ;

jmp_buf g_env;
void error_handler (HPDF_STATUS   error_no,
                    HPDF_STATUS   detail_no,
                    void         *user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no, (HPDF_UINT)detail_no);
    longjmp(g_env, 1);
}

Pdf3d::Pdf3d():
    _rcleft( 0 ), 
    _rctop( 0 ), 
    _rcwidth( 600 ), 
    _rcheight( 600 ),
    _bgr( 0.2f ), // OSG default clear color: (.2, .2, .4)
    _bgg( 0.2f ), 
    _bgb( 0.4f )
{
}

Pdf3d::~Pdf3d()
{
}

bool Pdf3d::createAdvancedPdf( const char *filepdf, const char *fileprc, const char *filejs )
{
    HPDF_Rect rect = {_rcleft, _rctop, _rcwidth, _rcheight};
  
    HPDF_Doc  pdf;
    HPDF_Page page;
    HPDF_Annotation annot;
    HPDF_U3D u3d;
  
    pdf = HPDF_New (NULL, NULL);
    if (!pdf) 
    {
        printf ("error: cannot create PdfDoc object\n");
        return false;
    }

    // set up some error handling
    if (setjmp(g_env)) 
    {
        HPDF_Free (pdf);
        return false;
    }

    page = HPDF_AddPage (pdf);
  
    HPDF_Page_SetWidth (page, _rcwidth);
    HPDF_Page_SetHeight (page, _rcheight);

    HPDF_Dict js = NULL;
    js = HPDF_DictStream_New (pdf->mmgr, pdf->xref);
    js->header.obj_class |= HPDF_OSUBCLASS_XOBJECT;
    js->filter = HPDF_STREAM_FILTER_NONE;
    js = HPDF_LoadJSFromFile  (pdf, filejs);
    u3d = HPDF_LoadU3DFromFile (pdf, fileprc);

    // add javeascript  action
    HPDF_Dict_Add (u3d, "OnInstantiate", js);

#define NS2VIEWS 7
    HPDF_Dict views[NS2VIEWS+1];
    const char *view_names[] = {"Front perspective ('1','H')",
                  "Back perspective ('2')",
                  "Right perspective ('3')",
                  "Left perspective ('4')",
                  "Bottom perspective ('5')", 
                  "Top perspective ('6')",
                  "Oblique perspective ('7')"};
    const float view_c2c[][3] = {{0., 0., 1.},
                   {0., 0., -1.},
                   {-1., 0., 0.},
                   {1., 0., 0.},
                   {0., 1., 0.},
                   {0., -1., 0.},
                   {-1., 1., -1.}};
    const float view_roll[] = {0., 180., 90., -90., 0., 0., 60.};
  
    // query camera point to rotate about - sometimes not the same as focus point
    XYZ pr;
    pr.x = 0; 
    pr.y = 0; 
    pr.z = 0;

    // camera focus point
    XYZ focus;
    focus.x = 0;
    focus.y = 0;
    focus.z = 0;

    float camrot = 5.0;

    // create views
    for (int iv = 0; iv < NS2VIEWS; iv++) 
    {
        views[iv] = HPDF_Create3DView(u3d->mmgr, view_names[iv]);
        HPDF_3DView_SetCamera(views[iv], 0., 0., 0., 
              view_c2c[iv][0], view_c2c[iv][1], view_c2c[iv][2],
              camrot, view_roll[iv]);
        HPDF_3DView_SetPerspectiveProjection(views[iv], 45.0);
        HPDF_3DView_SetBackgroundColor(views[iv], _bgr, _bgg, _bgb);
        HPDF_3DView_SetLighting(views[iv], "White");

        HPDF_U3D_Add3DView(u3d, views[iv]);
    }

    // add a psuedo-orthographic for slicing (actually perspective with point at infinity)
    views[NS2VIEWS] = HPDF_Create3DView(u3d->mmgr, "Orthgraphic slicing view");
    HPDF_3DView_SetCamera(views[NS2VIEWS], 0., 0., 0., 
            view_c2c[0][0], view_c2c[0][1], view_c2c[0][2],
            camrot*82.70f, view_roll[0]);
    HPDF_3DView_SetPerspectiveProjection(views[NS2VIEWS], 0.3333f);
    //HPDF_3DView_SetOrthogonalProjection(views[NS2VIEWS], 45.0/1000.0);
    HPDF_3DView_SetBackgroundColor(views[NS2VIEWS], _bgr,  _bgg, _bgb);
    HPDF_3DView_SetLighting(views[NS2VIEWS], "White");
    HPDF_U3D_Add3DView(u3d, views[NS2VIEWS]);


    HPDF_U3D_SetDefault3DView(u3d, "Front perspective");

    //  Create annotation
    annot = HPDF_Page_Create3DAnnot (page, rect, HPDF_TRUE, HPDF_FALSE, u3d, NULL);
  
    // make the toolbar appear by default
  
   // HPDF_Dict action = (HPDF_Dict)HPDF_Dict_GetItem (annot, "3DA", HPDF_OCLASS_DICT);
   // HPDF_Dict_AddBoolean (action, "TB", HPDF_TRUE);
  

    // save the document to a file 
    HPDF_SaveToFile (pdf, filepdf);
  
    // clean up
    HPDF_Free (pdf);

    return true;
}
