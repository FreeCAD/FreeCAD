// Rots - b/f/l/k/r/t
    cl = configLine( 1 , "AB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 2 , "AC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 3 , "AD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 4 , "AE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 5 , "BA", Base::Vector3d(0,-1,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 6 , "BC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 7 , "BD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 8 , "BF", Base::Vector3d(-1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), 
                                Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 9 , "CA", Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 10 , "CB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 11 , "CE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 12 , "CF", Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0) );
    addRotItem(cl);
    cl = configLine( 13 , "DA", Base::Vector3d(0,1,0), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 14 , "DB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 15 , "DE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(1,0,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
    cl = configLine( 16 , "DF", Base::Vector3d(0,-1,0), Base::Vector3d(-1,0,0), Base::Vector3d(1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0) );
    addRotItem(cl);
    cl = configLine( 17 , "EA", Base::Vector3d(0,1,0), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,-1,0), Base::Vector3d(0,0,1), Base::Vector3d(0,1,0) );
    addRotItem(cl);
    cl = configLine( 18 , "EC", Base::Vector3d(-1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 19 , "ED", Base::Vector3d(1,0,0), Base::Vector3d(-1,0,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,0,1), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 20 , "EF", Base::Vector3d(0,-1,0), Base::Vector3d(0,-1,0), Base::Vector3d(0,0,-1), 
                                Base::Vector3d(0,1,0), Base::Vector3d(0,0,1), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 21 , "FB", Base::Vector3d(0,0,1), Base::Vector3d(0,0,1), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(0,0,-1), Base::Vector3d(0,1,0), Base::Vector3d(0,0,1) );
    addRotItem(cl);
    cl = configLine( 22 , "FC", Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0), 
                                Base::Vector3d(1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(-1,0,0) );
    addRotItem(cl);
    cl = configLine( 23 , "FD", Base::Vector3d(1,0,0), Base::Vector3d(1,0,0), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(-1,0,0), Base::Vector3d(0,1,0), Base::Vector3d(1,0,0) );
    addRotItem(cl);
    cl = configLine( 24 , "FE", Base::Vector3d(0,0,-1), Base::Vector3d(0,0,-1), Base::Vector3d(0,-1,0), 
                                Base::Vector3d(0,0,1), Base::Vector3d(0,1,0), Base::Vector3d(0,0,-1) );
    addRotItem(cl);
