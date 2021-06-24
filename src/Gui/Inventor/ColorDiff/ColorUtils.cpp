//
//  ColorUtils.cpp
//  RGB-ColorDifference
//
//  Created by Mohamed Shahawy on 4/22/16.
//  Copyright © 2016 Mohamed Shahawy. All rights reserved.
//

#include "ColorUtils.hpp"
#include <cmath>

ColorUtils::xyzColor ColorUtils::rgbToXyz(ColorUtils::rgbColor c)
{
    float x, y, z, r, g, b;
    
    r = c.r / 255.0; g = c.g / 255.0; b = c.b / 255.0;
    
    if (r > 0.04045)
        r = powf(( (r + 0.055) / 1.055 ), 2.4);
    else r /= 12.92;
    
    if (g > 0.04045)
        g = powf(( (g + 0.055) / 1.055 ), 2.4);
    else g /= 12.92;
    
    if (b > 0.04045)
        b = powf(( (b + 0.055) / 1.055 ), 2.4);
    else b /= 12.92;
    
    r *= 100; g *= 100; b *= 100;
    
    // Calibration for observer @2° with illumination = D65
    x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    z = r * 0.0193 + g * 0.1192 + b * 0.9505;
    
    return xyzColor(x, y, z);
}

ColorUtils::CIELABColorSpace ColorUtils::xyzToCIELAB(ColorUtils::xyzColor c)
{
    float x, y, z, l, a, b;
    const float refX = 95.047, refY = 100.0, refZ = 108.883;
    
    // References set at calibration for observer @2° with illumination = D65
    x = c.x / refX; y = c.y / refY; z = c.z / refZ;
    
    if (x > 0.008856)
        x = powf(x, 1 / 3.0);
    else x = (7.787 * x) + (16.0 / 116.0);
    
    if (y > 0.008856)
        y = powf(y, 1 / 3.0);
    else y = (7.787 * y) + (16.0 / 116.0);
    
    if (z > 0.008856)
        z = powf(z, 1 / 3.0);
    else z = (7.787 * z) + (16.0 / 116.0);
    
    l = 116 * y - 16;
    a = 500 * (x - y);
    b = 200 * (y - z);
    
    return CIELABColorSpace(l, a, b);
}

float ColorUtils::getColorDeltaE(ColorUtils::rgbColor c1, ColorUtils::rgbColor c2)
{
    xyzColor xyzC1 = rgbToXyz(c1), xyzC2 = rgbToXyz(c2);
    CIELABColorSpace labC1 = xyzToCIELAB(xyzC1), labC2 = xyzToCIELAB(xyzC2);
    
    float deltaE;
    
    // Euclidian Distance between two points in 3D matrices
    deltaE = sqrtf( powf(labC1.l - labC2.l, 2) + powf(labC1.a - labC2.a, 2) + powf(labC1.b - labC2.b, 2) );
    
    return deltaE;
}


