%extend SoMarkerSet {
    static void addMarker(int idx, const SbVec2s & size, PyObject* string, 
                        SbBool isLSBFirst = TRUE, SbBool isUpToDown = TRUE)
    {
        short WIDTH, HEIGHT;
        size.getValue(WIDTH, HEIGHT);
        short BYTEWIDTH = (WIDTH + 7) / 2;
        const char* coin_marker;
#ifdef PY_2
        if (PyString_Check(string))
        {
            coin_marker = PyString_AsString(string);
        }
#else
        if (PyUnicode_Check(string))
        {
            coin_marker = PyUnicode_AsUTF8(string);            
        }
        else if (PyBytes_Check(string))
        {
            coin_marker = PyBytes_AsString(string);
        }
#endif
        else
        {
            return;
            // raise an attribute error: PyObject string should be of type bytes!
        }
        // https://grey.colorado.edu/coin3d/classSoMarkerSet.html
        // from addMarker example:

        int byteidx = 0;
        unsigned char* bitmapbytes = NULL;
        bitmapbytes = new unsigned char[BYTEWIDTH * HEIGHT];
        
        for (int h = 0; h < HEIGHT; h++)
        {
            unsigned char bits = 0;
            for (int w = 0; w < WIDTH; w++)
            {
                if (coin_marker[(h * WIDTH) + w] != ' ') { bits |= (0x80 >> (w % 8)); }
                if ((((w + 1) % 8) == 0) || (w == WIDTH - 1)) {
                bitmapbytes[byteidx++] = bits;
                bits = 0;
                }
            }
        }
        SoMarkerSet::addMarker(idx, size, bitmapbytes, isLSBFirst, isUpToDown);
        delete[] bitmapbytes;
        bitmapbytes = NULL;
    }
}
