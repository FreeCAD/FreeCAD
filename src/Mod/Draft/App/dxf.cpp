// dxf.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "dxf.h"

using namespace std;
static const double Pi = 3.14159265358979323846264338327950288419716939937511;

CDxfWrite::CDxfWrite(const char* filepath)
{
    // start the file
    m_fail = false;
#ifdef __WXMSW__
    m_ofs = new ofstream(filepath, ios::out);
#else
    m_ofs = new ofstream(filepath, ios::out);
#endif
    if(!(*m_ofs)){
        m_fail = true;
        return;
    }
    m_ofs->imbue(std::locale("C"));

    // start
    (*m_ofs) << 0          << endl;
    (*m_ofs) << "SECTION"  << endl;
    (*m_ofs) << 2          << endl;
    (*m_ofs) << "ENTITIES" << endl;
}

CDxfWrite::~CDxfWrite()
{
    // end
    (*m_ofs) << 0          << endl;
    (*m_ofs) << "ENDSEC"   << endl;
    (*m_ofs) << 0          << endl;
    (*m_ofs) << "EOF";

    delete m_ofs;
}

void CDxfWrite::WriteLine(const double* s, const double* e, const char* layer_name)
{
    (*m_ofs) << 0           << endl;
    (*m_ofs) << "LINE"      << endl;
    (*m_ofs) << 8           << endl;    // Group code for layer name
    (*m_ofs) << layer_name  << endl;    // Layer number
    (*m_ofs) << 10          << endl;    // Start point of line
    (*m_ofs) << s[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 20          << endl;
    (*m_ofs) << s[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 30          << endl;
    (*m_ofs) << s[2]        << endl;    // Z in WCS coordinates
    (*m_ofs) << 11          << endl;    // End point of line
    (*m_ofs) << e[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 21          << endl;
    (*m_ofs) << e[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 31          << endl;
    (*m_ofs) << e[2]        << endl;    // Z in WCS coordinates
}

void CDxfWrite::WritePoint(const double* s, const char* layer_name)
{
    (*m_ofs) << 0           << endl;
    (*m_ofs) << "POINT"     << endl;
    (*m_ofs) << 8           << endl;    // Group code for layer name
    (*m_ofs) << layer_name  << endl;    // Layer number
    (*m_ofs) << 10          << endl;    // Start point of line
    (*m_ofs) << s[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 20          << endl;
    (*m_ofs) << s[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 30          << endl;
    (*m_ofs) << s[2]        << endl;    // Z in WCS coordinates
}

void CDxfWrite::WriteArc(const double* s, const double* e, const double* c, bool dir, const char* layer_name)
{
    double ax = s[0] - c[0];
    double ay = s[1] - c[1];
    double bx = e[0] - c[0];
    double by = e[1] - c[1];

    double start_angle = atan2(ay, ax) * 180/Pi;
    double end_angle = atan2(by, bx) * 180/Pi;
    double radius = sqrt(ax*ax + ay*ay);
    if(!dir){
        double temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    (*m_ofs) << 0           << endl;
    (*m_ofs) << "ARC"       << endl;
    (*m_ofs) << 8           << endl;    // Group code for layer name
    (*m_ofs) << layer_name  << endl;    // Layer number
    (*m_ofs) << 10          << endl;    // Centre X
    (*m_ofs) << c[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 20          << endl;
    (*m_ofs) << c[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 30          << endl;
    (*m_ofs) << c[2]        << endl;    // Z in WCS coordinates
    (*m_ofs) << 40          << endl;    //
    (*m_ofs) << radius      << endl;    // Radius
    (*m_ofs) << 50          << endl;
    (*m_ofs) << start_angle << endl;    // Start angle
    (*m_ofs) << 51          << endl;
    (*m_ofs) << end_angle   << endl;    // End angle
}

void CDxfWrite::WriteCircle(const double* c, double radius, const char* layer_name)
{
    (*m_ofs) << 0           << endl;
    (*m_ofs) << "CIRCLE"        << endl;
    (*m_ofs) << 8           << endl;    // Group code for layer name
    (*m_ofs) << layer_name  << endl;    // Layer number
    (*m_ofs) << 10          << endl;    // Centre X
    (*m_ofs) << c[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 20          << endl;
    (*m_ofs) << c[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 30          << endl;
    (*m_ofs) << c[2]        << endl;    // Z in WCS coordinates
    (*m_ofs) << 40          << endl;    //
    (*m_ofs) << radius      << endl;    // Radius
}

void CDxfWrite::WriteEllipse(const double* c, double major_radius, double minor_radius, double rotation, double start_angle, double end_angle, bool dir, const char* layer_name )
{
    double m[3];
    m[2]=0;
    m[0] = major_radius * sin(rotation);
    m[1] = major_radius * cos(rotation);

    double ratio = minor_radius/major_radius;

    if(!dir){
        double temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }
    (*m_ofs) << 0           << endl;
    (*m_ofs) << "ELLIPSE"       << endl;
    (*m_ofs) << 8           << endl;    // Group code for layer name
    (*m_ofs) << layer_name  << endl;    // Layer number
    (*m_ofs) << 10          << endl;    // Centre X
    (*m_ofs) << c[0]        << endl;    // X in WCS coordinates
    (*m_ofs) << 20          << endl;
    (*m_ofs) << c[1]        << endl;    // Y in WCS coordinates
    (*m_ofs) << 30          << endl;
    (*m_ofs) << c[2]        << endl;    // Z in WCS coordinates
    (*m_ofs) << 40          << endl;    //
    (*m_ofs) << ratio       << endl;    // Ratio
    (*m_ofs) << 11          << endl;    //
    (*m_ofs) << m[0]        << endl;    // Major X
    (*m_ofs) << 21          << endl;
    (*m_ofs) << m[1]        << endl;    // Major Y
    (*m_ofs) << 31          << endl;
    (*m_ofs) << m[2]        << endl;    // Major Z
    (*m_ofs) << 41      << endl;
    (*m_ofs) << start_angle << endl;    // Start angle
    (*m_ofs) << 42      << endl;
    (*m_ofs) << end_angle   << endl;    // End angle
}

CDxfRead::CDxfRead(const char* filepath)
{
    // start the file
    memset( m_unused_line, '\0', sizeof(m_unused_line) );
    m_fail = false;
    m_aci = 0;
    m_eUnits = eMillimeters;
    m_measurement_inch = false;
    strcpy(m_layer_name, "0");  // Default layer name
    m_ignore_errors = true;

    m_ifs = new ifstream(filepath);
    if(!(*m_ifs)){
        m_fail = true;
        printf("DXF file didn't load\n");
        return;
    }
    m_ifs->imbue(std::locale("C"));

}

CDxfRead::~CDxfRead()
{
    delete m_ifs;
}

double CDxfRead::mm( double value ) const
{
    if(m_measurement_inch)
    {
        value *= 0.0393700787401575;
    }

    switch(m_eUnits)
    {
        case eUnspecified:  return(value * 1.0);    // We don't know any better.
        case eInches:       return(value * 25.4);
        case eFeet:     return(value * 25.4 * 12);
        case eMiles:        return(value *  1609344.0);
        case eMillimeters:  return(value * 1.0);
        case eCentimeters:  return(value * 10.0);
        case eMeters:       return(value * 1000.0);
        case eKilometers:   return(value * 1000000.0);
        case eMicroinches:  return(value * 25.4 / 1000.0);
        case eMils:     return(value * 25.4 / 1000.0);
        case eYards:        return(value * 3 * 12 * 25.4);
        case eAngstroms:    return(value * 0.0000001);
        case eNanometers:   return(value * 0.000001);
        case eMicrons:      return(value * 0.001);
        case eDecimeters:   return(value * 100.0);
        case eDekameters:   return(value * 10000.0);
        case eHectometers:  return(value * 100000.0);
        case eGigameters:   return(value * 1000000000000.0);
        case eAstronomicalUnits: return(value * 149597870690000.0);
        case eLightYears:   return(value * 9454254955500000000.0);
        case eParsecs:      return(value * 30856774879000000000.0);
        default:        return(value * 1.0);    // We don't know any better.
    } // End switch
} // End mm() method


bool CDxfRead::ReadLine()
{
    double s[3] = {0, 0, 0};
    double e[3] = {0, 0, 0};
    bool hidden = false;

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;

        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadLine() Failed to read integer from '%s'\n", m_str );
            return false;
        }

        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with line
                    DerefACI();
                    OnReadLine(s, e, hidden);
                    hidden = false;
                return true;

            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 6: // line style name follows
                get_line();
                if(m_str[0] == 'h' || m_str[0] == 'H')hidden = true;
                break;

            case 10:
                // start x
                get_line();
                ss.str(m_str); ss >> s[0]; s[0] = mm(s[0]); if(ss.fail()) return false;
                break;
            case 20:
                // start y
                get_line();
                ss.str(m_str); ss >> s[1]; s[1] = mm(s[1]); if(ss.fail()) return false;
                break;
            case 30:
                // start z
                get_line();
                ss.str(m_str); ss >> s[2]; s[2] = mm(s[2]); if(ss.fail()) return false;
                break;
            case 11:
                // end x
                get_line();
                ss.str(m_str); ss >> e[0]; e[0] = mm(e[0]); if(ss.fail()) return false;
                break;
            case 21:
                // end y
                get_line();
                ss.str(m_str); ss >> e[1]; e[1] = mm(e[1]); if(ss.fail()) return false;
                break;
            case 31:
                // end z
                get_line();
                ss.str(m_str); ss >> e[2]; e[2] = mm(e[2]); if(ss.fail()) return false;
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;

            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }

    try {
        DerefACI();
        OnReadLine(s, e, false);
    }
    catch(...)
    {
        if (! IgnoreErrors()) throw;    // Re-throw the exception.
    }

    return false;
}

bool CDxfRead::ReadPoint()
{
    double s[3] = {0, 0, 0};

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;

        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadPoint() Failed to read integer from '%s'\n", m_str );
            return false;
        }

        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with line
                    DerefACI();
                OnReadPoint(s);
                return true;

            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 10:
                // start x
                get_line();
                ss.str(m_str); ss >> s[0]; s[0] = mm(s[0]); if(ss.fail()) return false;
                break;
            case 20:
                // start y
                get_line();
                ss.str(m_str); ss >> s[1]; s[1] = mm(s[1]); if(ss.fail()) return false;
                break;
            case 30:
                // start z
                get_line();
                ss.str(m_str); ss >> s[2]; s[2] = mm(s[2]); if(ss.fail()) return false;
                break;

                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;

            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }

    }

    try {
        DerefACI();
        OnReadPoint(s);
    }
    catch(...)
    {
        if (! IgnoreErrors()) throw;    // Re-throw the exception.
    }

    return false;
}

bool CDxfRead::ReadArc()
{
    double start_angle = 0.0;// in degrees
    double end_angle = 0.0;
    double radius = 0.0;
    double c[3]; // centre
    double z_extrusion_dir = 1.0;
    bool hidden = false;
    
    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadArc() Failed to read integer from '%s'\n", m_str);
            return false;
        }

        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with arc
                    DerefACI();
                    OnReadArc(start_angle, end_angle, radius, c,z_extrusion_dir, hidden);
                    hidden = false;
                    return true;

            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 6: // line style name follows
                get_line();
                if(m_str[0] == 'h' || m_str[0] == 'H')hidden = true;
                break;

            case 10:
                // centre x
                get_line();
                ss.str(m_str); ss >> c[0]; c[0] = mm(c[0]); if(ss.fail()) return false;
                break;
            case 20:
                // centre y
                get_line();
                ss.str(m_str); ss >> c[1]; c[1] = mm(c[1]); if(ss.fail()) return false;
                break;
            case 30:
                // centre z
                get_line();
                ss.str(m_str); ss >> c[2]; c[2] = mm(c[2]); if(ss.fail()) return false;
                break;
            case 40:
                // radius
                get_line();
                ss.str(m_str); ss >> radius; radius = mm(radius); if(ss.fail()) return false;
                break;
            case 50:
                // start angle
                get_line();
                ss.str(m_str); ss >> start_angle; if(ss.fail()) return false;
                break;
            case 51:
                // end angle
                get_line();
                ss.str(m_str); ss >> end_angle; if(ss.fail()) return false;
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;



            case 100:
            case 39:
            case 210:
            case 220:
                // skip the next line
                get_line();
                break;
            case 230:
                //Z extrusion direction for arc 
                get_line();
                ss.str(m_str); ss >> z_extrusion_dir; if(ss.fail()) return false;                                
                break;

            default:
                // skip the next line
                get_line();
                break;
        }
    }
    DerefACI();
    OnReadArc(start_angle, end_angle, radius, c, z_extrusion_dir, false);
    return false;
}

bool CDxfRead::ReadSpline()
{
    struct SplineData sd;
    sd.norm[0] = 0;
    sd.norm[1] = 0;
    sd.norm[2] = 1;
    sd.degree = 0;
    sd.knots = 0;
    sd.flag = 0;
    sd.control_points = 0;
    sd.fit_points = 0;

    double temp_double;

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadSpline() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with Spline
                    DerefACI();
                OnReadSpline(sd);
                return true;
            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            case 210:
                // normal x
                get_line();
                ss.str(m_str); ss >> sd.norm[0]; if(ss.fail()) return false;
                break;
            case 220:
                // normal y
                get_line();
                ss.str(m_str); ss >> sd.norm[1]; if(ss.fail()) return false;
                break;
            case 230:
                // normal z
                get_line();
                ss.str(m_str); ss >> sd.norm[2]; if(ss.fail()) return false;
                break;
            case 70:
                // flag
                get_line();
                ss.str(m_str); ss >> sd.flag; if(ss.fail()) return false;
                break;
            case 71:
                // degree
                get_line();
                ss.str(m_str); ss >> sd.degree; if(ss.fail()) return false;
                break;
            case 72:
                // knots
                get_line();
                ss.str(m_str); ss >> sd.knots; if(ss.fail()) return false;
                break;
            case 73:
                // control points
                get_line();
                ss.str(m_str); ss >> sd.control_points; if(ss.fail()) return false;
                break;
            case 74:
                // fit points
                get_line();
                ss.str(m_str); ss >> sd.fit_points; if(ss.fail()) return false;
                break;
            case 12:
                // starttan x
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.starttanx.push_back(temp_double);
                break;
            case 22:
                // starttan y
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.starttany.push_back(temp_double);
                break;
            case 32:
                // starttan z
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.starttanz.push_back(temp_double);
                break;
            case 13:
                // endtan x
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.endtanx.push_back(temp_double);
                break;
            case 23:
                // endtan y
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.endtany.push_back(temp_double);
                break;
            case 33:
                // endtan z
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.endtanz.push_back(temp_double);
                break;
            case 40:
                // knot
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.knot.push_back(temp_double);
                break;
            case 41:
                // weight
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.weight.push_back(temp_double);
                break;
            case 10:
                // control x
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.controlx.push_back(temp_double);
                break;
            case 20:
                // control y
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.controly.push_back(temp_double);
                break;
            case 30:
                // control z
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.controlz.push_back(temp_double);
                break;
            case 11:
                // fit x
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.fitx.push_back(temp_double);
                break;
            case 21:
                // fit y
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.fity.push_back(temp_double);
                break;
            case 31:
                // fit z
                get_line();
                ss.str(m_str); ss >> temp_double; temp_double = mm(temp_double); if(ss.fail()) return false;
                sd.fitz.push_back(temp_double);
                break;
            case 42:
            case 43:
            case 44:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    DerefACI();
    OnReadSpline(sd);
    return false;
}


bool CDxfRead::ReadCircle()
{
    double radius = 0.0;
    double c[3] = {0,0,0}; // centre
    bool hidden = false;

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadCircle() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with Circle
                DerefACI();
                OnReadCircle(c, radius, hidden);
                hidden = false;
                return true;

            case 6: // line style name follows
                get_line();
                if(m_str[0] == 'h' || m_str[0] == 'H')hidden = true;
                break;

            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 10:
                // centre x
                get_line();
                ss.str(m_str); ss >> c[0]; c[0] = mm(c[0]); if(ss.fail()) return false;
                break;
            case 20:
                // centre y
                get_line();
                ss.str(m_str); ss >> c[1]; c[1] = mm(c[1]); if(ss.fail()) return false;
                break;
            case 30:
                // centre z
                get_line();
                ss.str(m_str); ss >> c[2]; c[2] = mm(c[2]); if(ss.fail()) return false;
                break;
            case 40:
                // radius
                get_line();
                ss.str(m_str); ss >> radius; radius = mm(radius); if(ss.fail()) return false;
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;

            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    DerefACI();
    OnReadCircle(c, radius, false);
    return false;
}


bool CDxfRead::ReadText()
{
    double c[3]; // coordinate
    double height = 0.03082;

    memset( c, 0, sizeof(c) );

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadText() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                return false;
            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 10:
                // centre x
                get_line();
                ss.str(m_str); ss >> c[0]; c[0] = mm(c[0]); if(ss.fail()) return false;
                break;
            case 20:
                // centre y
                get_line();
                ss.str(m_str); ss >> c[1]; c[1] = mm(c[1]); if(ss.fail()) return false;
                break;
            case 30:
                // centre z
                get_line();
                ss.str(m_str); ss >> c[2]; c[2] = mm(c[2]); if(ss.fail()) return false;
                break;
            case 40:
                // text height
                get_line();
                ss.str(m_str); ss >> height; height = mm(height); if(ss.fail()) return false;
                break;
            case 1:
                // text
                get_line();
                DerefACI();
                OnReadText(c, height * 25.4 / 72.0, m_str);
                return(true);

            case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;

            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }

    return false;
}


bool CDxfRead::ReadEllipse()
{
    double c[3] = {0,0,0}; // centre
    double m[3] = {0,0,0}; //major axis point
    double ratio=0; //ratio of major to minor axis
    double start=0; //start of arc
    double end=0;  // end of arc

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadEllipse() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found, so finish with Ellipse
                    DerefACI();
                OnReadEllipse(c, m, ratio, start, end);
                return true;
            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;

            case 10:
                // centre x
                get_line();
                ss.str(m_str); ss >> c[0]; c[0] = mm(c[0]); if(ss.fail()) return false;
                break;
            case 20:
                // centre y
                get_line();
                ss.str(m_str); ss >> c[1]; c[1] = mm(c[1]); if(ss.fail()) return false;
                break;
            case 30:
                // centre z
                get_line();
                ss.str(m_str); ss >> c[2]; c[2] = mm(c[2]); if(ss.fail()) return false;
                break;
            case 11:
                // major x
                get_line();
                ss.str(m_str); ss >> m[0]; m[0] = mm(m[0]); if(ss.fail()) return false;
                break;
            case 21:
                // major y
                get_line();
                ss.str(m_str); ss >> m[1]; m[1] = mm(m[1]); if(ss.fail()) return false;
                break;
            case 31:
                // major z
                get_line();
                ss.str(m_str); ss >> m[2]; m[2] = mm(m[2]); if(ss.fail()) return false;
                break;
            case 40:
                // ratio
                get_line();
                ss.str(m_str); ss >> ratio; if(ss.fail()) return false;
                break;
            case 41:
                // start
                get_line();
                ss.str(m_str); ss >> start; if(ss.fail()) return false;
                break;
            case 42:
                // end
                get_line();
                ss.str(m_str); ss >> end; if(ss.fail()) return false;
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            case 100:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    DerefACI();
    OnReadEllipse(c, m, ratio, start, end);
    return false;
}


static bool poly_prev_found = false;
static double poly_prev_x;
static double poly_prev_y;
static double poly_prev_z;
static double poly_prev_bulge_found;
static double poly_prev_bulge;
static bool poly_first_found = false;
static double poly_first_x;
static double poly_first_y;
static double poly_first_z;

static void AddPolyLinePoint(CDxfRead* dxf_read, double x, double y, double z, bool bulge_found, double bulge)
{

    try {
        if(poly_prev_found)
        {
            bool arc_done = false;
            if(poly_prev_bulge_found)
            {
                double cot = 0.5 * ((1.0 / poly_prev_bulge) - poly_prev_bulge);
                double cx = ((poly_prev_x + x) - ((y - poly_prev_y) * cot)) / 2.0;
                double cy = ((poly_prev_y + y) + ((x - poly_prev_x) * cot)) / 2.0;
                double ps[3] = {poly_prev_x, poly_prev_y, poly_prev_z};
                double pe[3] = {x, y, z};
                double pc[3] = {cx, cy, (poly_prev_z + z)/2.0};
                dxf_read->OnReadArc(ps, pe, pc, poly_prev_bulge >= 0, false);
                arc_done = true;
            }

            if(!arc_done)
            {
                double s[3] = {poly_prev_x, poly_prev_y, poly_prev_z};
                double e[3] = {x, y, z};
                dxf_read->OnReadLine(s, e, false);
            }
        }

        poly_prev_found = true;
        poly_prev_x = x;
        poly_prev_y = y;
        poly_prev_z = z;
        if(!poly_first_found)
        {
            poly_first_x = x;
            poly_first_y = y;
            poly_first_z = z;
            poly_first_found = true;
        }
        poly_prev_bulge_found = bulge_found;
        poly_prev_bulge = bulge;
    }
    catch(...)
    {
        if (! dxf_read->IgnoreErrors()) throw;  // Re-throw it.
    }
}

static void PolyLineStart()
{
    poly_prev_found = false;
    poly_first_found = false;
}

bool CDxfRead::ReadLwPolyLine()
{
    PolyLineStart();

    bool x_found = false;
    bool y_found = false;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    bool bulge_found = false;
    double bulge = 0.0;
    bool closed = false;
    int flags;
    bool next_item_found = false;

    while(!((*m_ifs).eof()) && !next_item_found)
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadLwPolyLine() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found

                    DerefACI();
                    if(x_found && y_found){
                    // add point
                    AddPolyLinePoint(this, x, y, z, bulge_found, bulge);
                    bulge_found = false;
                    x_found = false;
                    y_found = false;
                }
                next_item_found = true;
                break;
            case 8: // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;
            case 10:
                // x
                get_line();
                if(x_found && y_found){
                    // add point
                    AddPolyLinePoint(this, x, y, z, bulge_found, bulge);
                    bulge_found = false;
                    x_found = false;
                    y_found = false;
                }
                ss.str(m_str); ss >> x; x = mm(x); if(ss.fail()) return false;
                x_found = true;
                break;
            case 20:
                // y
                get_line();
                ss.str(m_str); ss >> y; y = mm(y); if(ss.fail()) return false;
                y_found = true;
                break;
            case 38: 
                // elevation
                get_line();
                ss.str(m_str); ss >> z; z = mm(z); if(ss.fail()) return false;
                break;
            case 42:
                // bulge
                get_line();
                ss.str(m_str); ss >> bulge; if(ss.fail()) return false;
                bulge_found = true;
                break;
            case 70:
                // flags
                get_line();
                if(sscanf(m_str, "%d", &flags) != 1)return false;
                closed = ((flags & 1) != 0);
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }

    if(next_item_found)
    {
        if(closed && poly_first_found)
        {
            // repeat the first point
                DerefACI();
            AddPolyLinePoint(this, poly_first_x, poly_first_y, poly_first_z, false, 0.0);
        }
        return true;
    }

    return false;
}


bool CDxfRead::ReadVertex(double *pVertex, bool *bulge_found, double *bulge)
{
    bool x_found = false;
    bool y_found = false;

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    *bulge = 0.0;
    *bulge_found = false;

    pVertex[0] = 0.0;
    pVertex[1] = 0.0;
    pVertex[2] = 0.0;

    while(!(*m_ifs).eof()) {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1) {
            printf("CDxfRead::ReadVertex() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
        case 0:
        DerefACI();
            put_line(m_str);    // read one line too many.  put it back.
            return(x_found && y_found);
            break;

        case 8: // Layer name follows
            get_line();
            strcpy(m_layer_name, m_str);
            break;

        case 10:
            // x
            get_line();
            ss.str(m_str); ss >> x; pVertex[0] = mm(x); if(ss.fail()) return false;
            x_found = true;
            break;
        case 20:
            // y
            get_line();
            ss.str(m_str); ss >> y; pVertex[1] = mm(y); if(ss.fail()) return false;
            y_found = true;
            break;
        case 30:
            // z
            get_line();
            ss.str(m_str); ss >> z; pVertex[2] = mm(z); if(ss.fail()) return false;
            break;

        case 42:
            get_line();
            *bulge_found = true;
            ss.str(m_str); ss >> *bulge; if(ss.fail()) return false;
            break;
    case 62:
        // color index
        get_line();
        ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
        break;

        default:
            // skip the next line
            get_line();
            break;
        }
    }

    return false;
}



bool CDxfRead::ReadPolyLine()
{
    PolyLineStart();

    bool closed = false;
    int flags;
    bool first_vertex_section_found = false;
    double first_vertex[3] = {0,0,0};
    bool bulge_found;
    double bulge;

    while(!(*m_ifs).eof())
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadPolyLine() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0:
                // next item found
                    DerefACI();
                get_line();
                if (! strcmp(m_str,"VERTEX"))
                {
                    double vertex[3];
                    if (CDxfRead::ReadVertex(vertex, &bulge_found, &bulge))
                    {
                        if(!first_vertex_section_found) {
                            first_vertex_section_found = true;
                            memcpy(first_vertex, vertex, 3*sizeof(double));
                        }
                        AddPolyLinePoint(this, vertex[0], vertex[1], vertex[2], bulge_found, bulge);
                        break;
                    }
                }
                if (! strcmp(m_str,"SEQEND"))
                {
                    if(closed && first_vertex_section_found) {
                        AddPolyLinePoint(this, first_vertex[0], first_vertex[1], first_vertex[2], 0, 0);
                    }
                    first_vertex_section_found = false;
                    PolyLineStart();
                    return(true);
                }
                break;
            case 70:
                // flags
                get_line();
                if(sscanf(m_str, "%d", &flags) != 1)return false;
                closed = ((flags & 1) != 0);
                break;
                case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }

    return false;
}

void CDxfRead::OnReadArc(double start_angle, double end_angle, double radius, const double* c, double z_extrusion_dir, bool hidden){
    double s[3], e[3], temp[3] ;
    if (z_extrusion_dir==1.0)
  {
    temp[0] =c[0];
    temp[1] =c[1];
    temp[2] =c[2];
    s[0] = c[0] + radius * cos(start_angle * Pi/180);
    s[1] = c[1] + radius * sin(start_angle * Pi/180);
    s[2] = c[2];
    e[0] = c[0] + radius * cos(end_angle * Pi/180);
    e[1] = c[1] + radius * sin(end_angle * Pi/180);
    e[2] = c[2];
   }
    else
    {
    temp[0] =-c[0];
    temp[1] =c[1];
    temp[2] =c[2];
    
    e[0] = -(c[0] + radius * cos(start_angle * Pi/180));
    e[1] = (c[1] + radius * sin(start_angle * Pi/180));
    e[2] = c[2];
    s[0] = -(c[0] + radius * cos(end_angle * Pi/180));
    s[1] = (c[1] + radius * sin(end_angle * Pi/180));
    s[2] = c[2];

    }
    OnReadArc(s, e, temp, true, hidden);
}

void CDxfRead::OnReadCircle(const double* c, double radius, bool hidden){
    double s[3];
    double start_angle = 0;
    s[0] = c[0] + radius * cos(start_angle * Pi/180);
    s[1] = c[1] + radius * sin(start_angle * Pi/180);
    s[2] = c[2];

    OnReadCircle(s, c, false, hidden); //false to change direction because otherwise the arc length is zero
}

void CDxfRead::OnReadEllipse(const double* c, const double* m, double ratio, double start_angle, double end_angle){
    double major_radius = sqrt(m[0]*m[0] + m[1]*m[1] + m[2]*m[2]);
    double minor_radius = major_radius * ratio;

    //Since we only support 2d stuff, we can calculate the rotation from the major axis x and y value only,
    //since z is zero, major_radius is the vector length

    double rotation = atan2(m[1]/major_radius,m[0]/major_radius);


    OnReadEllipse(c, major_radius, minor_radius, rotation, start_angle, end_angle, true);
}


bool CDxfRead::ReadInsert()
{
    double c[3]; // coordinate
    double s[3]; // scale
    double rot = 0.0; // rotation
    char name[1024];
    s[0] = 1.0;
    s[1] = 1.0;
    s[2] = 1.0;

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadInsert() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0: 
                // next item found
                DerefACI();
                OnReadInsert(c, s, name, rot * Pi/180);
                return(true);
            case 8: 
                // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;
            case 10:
                // coord x
                get_line();
                ss.str(m_str); ss >> c[0]; c[0] = mm(c[0]); if(ss.fail()) return false;
                break;
            case 20:
                // coord y
                get_line();
                ss.str(m_str); ss >> c[1]; c[1] = mm(c[1]); if(ss.fail()) return false;
                break;
            case 30:
                // coord z
                get_line();
                ss.str(m_str); ss >> c[2]; c[2] = mm(c[2]); if(ss.fail()) return false;
                break;
            case 41:
                // scale x
                get_line();
                ss.str(m_str); ss >> s[0]; if(ss.fail()) return false;
                break;
            case 42:
                // scale y
                get_line();
                ss.str(m_str); ss >> s[1]; if(ss.fail()) return false;
                break;
            case 43:
                // scale z
                get_line();
                ss.str(m_str); ss >> s[2]; if(ss.fail()) return false;
                break;
            case 50:
                // rotation
                get_line();
                ss.str(m_str); ss >> rot; if(ss.fail()) return false;
                break;
            case 2:
                // block name
                get_line();
                strcpy(name, m_str);
                break;
            case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    return false;
}


bool CDxfRead::ReadDimension()
{
    double s[3]; // startpoint
    double e[3]; // endpoint
    double p[3]; // dimpoint
    double rot = -1.0; // rotation

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadInsert() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0: 
                // next item found
                DerefACI();
                OnReadDimension(s, e, p, rot * Pi/180);
                return(true);
            case 8: 
                // Layer name follows
                get_line();
                strcpy(m_layer_name, m_str);
                break;
            case 13:
                // start x
                get_line();
                ss.str(m_str); ss >> s[0]; s[0] = mm(s[0]); if(ss.fail()) return false;
                break;
            case 23:
                // start y
                get_line();
                ss.str(m_str); ss >> s[1]; s[1] = mm(s[1]); if(ss.fail()) return false;
                break;
            case 33:
                // start z
                get_line();
                ss.str(m_str); ss >> s[2]; s[2] = mm(s[2]); if(ss.fail()) return false;
                break;
            case 14:
                // end x
                get_line();
                ss.str(m_str); ss >> e[0]; e[0] = mm(e[0]); if(ss.fail()) return false;
                break;
            case 24:
                // end y
                get_line();
                ss.str(m_str); ss >> e[1]; e[1] = mm(e[1]); if(ss.fail()) return false;
                break;
            case 34:
                // end z
                get_line();
                ss.str(m_str); ss >> e[2]; e[2] = mm(e[2]); if(ss.fail()) return false;
                break;
            case 10:
                // dimline x
                get_line();
                ss.str(m_str); ss >> p[0]; p[0] = mm(p[0]); if(ss.fail()) return false;
                break;
            case 20:
                // dimline y
                get_line();
                ss.str(m_str); ss >> p[1]; p[1] = mm(p[1]); if(ss.fail()) return false;
                break;
            case 30:
                // dimline z
                get_line();
                ss.str(m_str); ss >> p[2]; p[2] = mm(p[2]); if(ss.fail()) return false;
                break;
            case 50:
                // rotation
                get_line();
                ss.str(m_str); ss >> rot; if(ss.fail()) return false;
                break;
            case 62:
                // color index
                get_line();
                ss.str(m_str); ss >> m_aci; if(ss.fail()) return false;
                break;
            case 100:
            case 39:
            case 210:
            case 220:
            case 230:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    return false;
}


bool CDxfRead::ReadBlockInfo()
{
    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;
        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadBlockInfo() Failed to read integer from '%s'\n", m_str);
            return false;
        }
        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 2:
                // block name
                get_line();
                strcpy(m_block_name, m_str);
                return true;
            case 3:
                // block name too???
                get_line();
                strcpy(m_block_name, m_str);
                return true;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    return false;
}


void CDxfRead::get_line()
{
    if (m_unused_line[0] != '\0')
    {
        strcpy(m_str, m_unused_line);
        memset( m_unused_line, '\0', sizeof(m_unused_line));
        return;
    }

    m_ifs->getline(m_str, 1024);

    char str[1024];
    size_t len = strlen(m_str);
    int j = 0;
    bool non_white_found = false;
    for(size_t i = 0; i<len; i++){
        if(non_white_found || (m_str[i] != ' ' && m_str[i] != '\t')){
            if(m_str[i] != '\r')
            {
                str[j] = m_str[i]; j++;
            }
            non_white_found = true;
        }
    }
    str[j] = 0;
    strcpy(m_str, str);
}

void CDxfRead::put_line(const char *value)
{
    strcpy( m_unused_line, value );
}


bool CDxfRead::ReadUnits()
{
    get_line(); // Skip to next line.
    get_line(); // Skip to next line.
    int n = 0;
    if(sscanf(m_str, "%d", &n) == 1)
    {
        m_eUnits = eDxfUnits_t( n );
        return(true);
    } // End if - then
    else
    {
        printf("CDxfRead::ReadUnits() Failed to get integer from '%s'\n", m_str);
        return(false);
    }
}


bool CDxfRead::ReadLayer()
{
    std::string layername;
    int aci = -1;

    while(!((*m_ifs).eof()))
    {
        get_line();
        int n;

        if(sscanf(m_str, "%d", &n) != 1)
        {
            printf("CDxfRead::ReadLayer() Failed to read integer from '%s'\n", m_str );
            return false;
        }

        std::istringstream ss;
        ss.imbue(std::locale("C"));
        switch(n){
            case 0: // next item found, so finish with line
                    if (layername.empty())
                {
                    printf("CDxfRead::ReadLayer() - no layer name\n");
                    return false;
                }
                    m_layer_aci[layername] = aci;
                return true;

            case 2: // Layer name follows
                get_line();
                layername = m_str;
                break;

            case 62:
                // layer color ; if negative, layer is off
                get_line();
                if(sscanf(m_str, "%d", &aci) != 1)return false;
                break;

            case 6: // linetype name
            case 70: // layer flags
            case 100:
            case 290:
            case 370:
            case 390:
                // skip the next line
                get_line();
                break;
            default:
                // skip the next line
                get_line();
                break;
        }
    }
    return false;
}

void CDxfRead::DoRead(const bool ignore_errors /* = false */ )
{
    m_ignore_errors = ignore_errors;
    if(m_fail)return;

    get_line();

    while(!((*m_ifs).eof()))
    {
        if (!strcmp( m_str, "$INSUNITS" )){
            if (!ReadUnits())return;
            continue;
        } // End if - then

        if (!strcmp( m_str, "$MEASUREMENT" )){
            get_line();
            get_line();
            int n = 1;
            if(sscanf(m_str, "%d", &n) == 1)
            {
                if(n == 0)m_measurement_inch = true;
            }
            continue;
        } // End if - then
        
        else if(!strcmp(m_str, "0"))
        {
            get_line();
            if (!strcmp( m_str, "SECTION" )){
              get_line();
              get_line();
              if (strcmp( m_str, "ENTITIES" ))
                strcpy(m_section_name, m_str);
              strcpy(m_block_name, "");

        } // End if - then
        else if (!strcmp( m_str, "TABLE" )){
              get_line();
              get_line();
        }

        else if (!strcmp( m_str, "LAYER" )){
              get_line();
              get_line();
              if(!ReadLayer())
                {
                  printf("CDxfRead::DoRead() Failed to read layer\n");
                  //return; Some objects or tables can have "LAYER" as name...
                }
              continue;     
        }
        
        else if (!strcmp( m_str, "BLOCK" )) {
            if(!ReadBlockInfo())
            {
                printf("CDxfRead::DoRead() Failed to read block info\n");
                return;
            }
            continue;
        } // End if - then

        else if (!strcmp( m_str, "ENDSEC" )){
                    strcpy(m_section_name, "");
                    strcpy(m_block_name, "");
                } // End if - then
        else if(!strcmp(m_str, "LINE")){
                if(!ReadLine())
                {
                    printf("CDxfRead::DoRead() Failed to read line\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "ARC")){
                if(!ReadArc())
                {
                    printf("CDxfRead::DoRead() Failed to read arc\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "CIRCLE")){
                if(!ReadCircle())
                {
                    printf("CDxfRead::DoRead() Failed to read circle\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "MTEXT")){
                if(!ReadText())
                {
                    printf("CDxfRead::DoRead() Failed to read text\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "TEXT")){
                if(!ReadText())
                {
                    printf("CDxfRead::DoRead() Failed to read text\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "ELLIPSE")){
                if(!ReadEllipse())
                {
                    printf("CDxfRead::DoRead() Failed to read ellipse\n");
                    return;
                }
                continue;
            }
            else if(!strcmp(m_str, "SPLINE")){
                if(!ReadSpline())
                {
                    printf("CDxfRead::DoRead() Failed to read spline\n");
                    return;
                }
                continue;
            }
            else if (!strcmp(m_str, "LWPOLYLINE")) {
                if(!ReadLwPolyLine())
                {
                    printf("CDxfRead::DoRead() Failed to read LW Polyline\n");
                    return;
                }
                continue;
            }
            else if (!strcmp(m_str, "POLYLINE")) {
                if(!ReadPolyLine())
                {
                    printf("CDxfRead::DoRead() Failed to read Polyline\n");
                    return;
                }
                continue;
            }
            else if (!strcmp(m_str, "POINT")) {
                if(!ReadPoint())
                {
                    printf("CDxfRead::DoRead() Failed to read Point\n");
                    return;
                }
                continue;
            }
            else if (!strcmp(m_str, "INSERT")) {
                if(!ReadInsert())
                {
                    printf("CDxfRead::DoRead() Failed to read Insert\n");
                    return;
                }
                continue;
            }
            else if (!strcmp(m_str, "DIMENSION")) {
                if(!ReadDimension())
                {
                    printf("CDxfRead::DoRead() Failed to read Dimension\n");
                    return;
                }
                continue;
            }
        }

        get_line();
    }
    AddGraphics();
}


void  CDxfRead::DerefACI()
{

    if (m_aci == 256) // if color = layer color, replace by color from layer
    {
         m_aci = m_layer_aci[std::string(m_layer_name)];
    }
}

std::string CDxfRead::LayerName() const
{
    std::string result;

    if (strlen(m_section_name) > 0)
    {
        result.append(m_section_name);
        result.append(" ");
    }

    if (strlen(m_block_name) > 0)
    {
        result.append(m_block_name);
        result.append(" ");
    }

    if (strlen(m_layer_name) > 0)
    {
        result.append(m_layer_name);
    }

    return(result);
}
