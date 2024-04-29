#ifndef __stock_object_h__
#define __stock_object_h__
#include "SimShapes.h"
#include "linmath.h"

namespace MillSim {

    class StockObject
    {
    public:
        /// <summary>
        /// Create a stock primitive
        /// </summary>
        /// <param name="x">Stock's corner x location</param>
        /// <param name="y">Stock's corner y location</param>
        /// <param name="z">Stock's corner z location</param>
        /// <param name="l">Stock's length (along x)</param>
        /// <param name="w">Stock's width (along y)</param>
        /// <param name="h">Stock's height (along z)</param>
        StockObject();
        virtual ~StockObject();


        /// Calls the display list.
        virtual void render();
        Shape mShape;
        void SetPosition(vec3 position);
        void GenerateBoxStock(float x, float y, float z, float l, float w, float h);
        vec3 mCenter = {};
        vec3 mSize = {};

    private:
        float mProfile[8] = {};
        mat4x4 modelMat;

    };
}

#endif