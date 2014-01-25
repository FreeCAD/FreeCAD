/*
    openDCM, dimensional constraint manager
    Copyright (C) 2013  Stefan Troeger <stefantroeger@gmx.net>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along
    with this library; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef DCM_CORE_MULTIMAP_H
#define DCM_CORE_MULTIMAP_H

#include <Eigen/Dense>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/fusion/include/at_c.hpp>

namespace dcm {
namespace details {
template<typename Derived>
class MultiMap;
}
}

namespace Eigen {
namespace internal {

template<typename PlainObjectType>
struct traits<dcm::details::MultiMap<PlainObjectType> >
        : public traits<PlainObjectType> {
    typedef traits<PlainObjectType> TraitsBase;
    typedef typename PlainObjectType::Index Index;
    typedef typename PlainObjectType::Scalar Scalar;
    enum {
        InnerStrideAtCompileTime = int(PlainObjectType::InnerStrideAtCompileTime),
        OuterStrideAtCompileTime = int(PlainObjectType::OuterStrideAtCompileTime),
        HasNoInnerStride = InnerStrideAtCompileTime == 1,
        HasNoOuterStride = OuterStrideAtCompileTime == 1,
        HasNoStride = HasNoInnerStride && HasNoOuterStride,
        IsAligned = false,//bool(EIGEN_ALIGN),
        IsDynamicSize = PlainObjectType::SizeAtCompileTime == Dynamic,
        KeepsPacketAccess = bool(HasNoInnerStride)
        && (bool(IsDynamicSize)
        || HasNoOuterStride
        || (OuterStrideAtCompileTime != Dynamic
        && ((static_cast<int>(sizeof(Scalar)) * OuterStrideAtCompileTime) % 16) == 0)),
        Flags0 = TraitsBase::Flags & (~NestByRefBit),
        Flags1 = IsAligned ? (int(Flags0) | AlignedBit) : (int(Flags0) & ~AlignedBit),
        Flags2 = (bool(HasNoStride) || bool(PlainObjectType::IsVectorAtCompileTime))
        ? int(Flags1) : int(Flags1 & ~LinearAccessBit),
        Flags3 = is_lvalue<PlainObjectType>::value ? int(Flags2) : (int(Flags2) & ~LvalueBit),
        Flags = KeepsPacketAccess ? int(Flags3) : (int(Flags3) & ~PacketAccessBit)
    };
private:
    enum { Options }; // Expressions don't have Options
};
} //internal
} //Eigen

namespace dcm {
namespace details {

using namespace Eigen; //needed for macros
namespace fusion = boost::fusion;

template<typename Derived>
class MultiMapBase : public internal::dense_xpr_base<Derived>::type {

public:

    typedef typename internal::dense_xpr_base<Derived>::type Base;

    enum {
        RowsAtCompileTime = internal::traits<Derived>::RowsAtCompileTime,
        ColsAtCompileTime = internal::traits<Derived>::ColsAtCompileTime,
        SizeAtCompileTime = Base::SizeAtCompileTime
    };

    typedef typename internal::traits<Derived>::StorageKind StorageKind;
    typedef typename internal::traits<Derived>::Index Index;
    typedef typename internal::traits<Derived>::Scalar Scalar;
    typedef typename internal::packet_traits<Scalar>::type PacketScalar;
    typedef typename NumTraits<Scalar>::Real RealScalar;
    typedef typename internal::conditional <
    bool(internal::is_lvalue<Derived>::value),
         Scalar*,
         const Scalar* >::type
         PointerType;

    using Base::derived;
    using Base::MaxRowsAtCompileTime;
    using Base::MaxColsAtCompileTime;
    using Base::MaxSizeAtCompileTime;
    using Base::IsVectorAtCompileTime;
    using Base::Flags;
    using Base::IsRowMajor;

    using Base::size;
    using Base::coeff;
    using Base::coeffRef;
    using Base::lazyAssign;
    using Base::eval;

    // bug 217 - compile error on ICC 11.1
    using Base::operator=;

    typedef typename Base::CoeffReturnType CoeffReturnType;

    typedef Stride<Dynamic, Dynamic> DynStride;
    typedef fusion::vector6<PointerType, int, int, int, int, DynStride> MapData;


    inline Index rows() const {
        return m_rows.value();
    }
    inline Index cols() const {
        return m_cols.value();
    }

    inline const Scalar* data() const {
        assert(false);
        return m_data;
    }

    inline const Scalar& coeff(Index row, Index col) const {

        typedef typename std::vector<MapData>::const_iterator iter;

        for(iter i = m_data.begin(); i != m_data.end(); i++) {
            const MapData& data = *i;

            if((fusion::at_c<1>(data) + fusion::at_c<3>(data)) > row && fusion::at_c<1>(data) <= row
                    && (fusion::at_c<2>(data) + fusion::at_c<4>(data)) > col && fusion::at_c<2>(data) <= col)
                return fusion::at_c<0>(data)[(col - fusion::at_c<2>(data)) * fusion::at_c<5>(data).outer() + (row - fusion::at_c<1>(data)) * fusion::at_c<5>(data).inner()];
        };

        return default_value;
    }

    inline const Scalar& coeff(Index index) const {
        EIGEN_STATIC_ASSERT_INDEX_BASED_ACCESS(Derived)

        typedef typename std::vector<MapData>::const_iterator iter;

        for(iter i = m_data.begin(); i != m_data.end(); i++) {
            const MapData& data = *i;

            if((fusion::at_c<1>(data) + fusion::at_c<3>(data)) > index)
                return fusion::at_c<0>(data)[(index - fusion::at_c<1>(data)) * fusion::at_c<5>(data).inner()];
        };

        return default_value;
    }

    inline const Scalar& coeffRef(Index row, Index col) const {
        return coeff(row, col);
    }

    inline const Scalar& coeffRef(Index index) const {
        EIGEN_STATIC_ASSERT_INDEX_BASED_ACCESS(Derived)
        return coeff(index);
    }

    template<int LoadMode>
    inline PacketScalar packet(Index row, Index col) const {

        typedef typename std::vector<MapData>::const_iterator iter;

        for(iter i = m_data.begin(); i != m_data.end(); i++) {
            const MapData& data = *i;

            if((fusion::at_c<1>(data) + fusion::at_c<3>(data)) > row && fusion::at_c<1>(data) <= row
                    && (fusion::at_c<2>(data) + fusion::at_c<4>(data)) > col && fusion::at_c<2>(data) <= col)
                return internal::ploadt<PacketScalar, LoadMode>
                       (fusion::at_c<0>(data) + (col - fusion::at_c<2>(data)) * fusion::at_c<5>(data).outer() + (row - fusion::at_c<1>(data)) * fusion::at_c<5>(data).inner());
        };
	return internal::ploadt<PacketScalar, LoadMode>(&default_value);
    }

    template<int LoadMode>
    inline PacketScalar packet(Index index) const {
        EIGEN_STATIC_ASSERT_INDEX_BASED_ACCESS(Derived)
        typedef typename std::vector<MapData>::const_iterator iter;

        for(iter i = m_data.begin(); i != m_data.end(); i++) {
            const MapData& data = *i;

            if((fusion::at_c<1>(data) + fusion::at_c<3>(data)) > index)
                return internal::ploadt<PacketScalar, LoadMode>(fusion::at_c<0>(data) + (index - fusion::at_c<1>(data)) * fusion::at_c<5>(data).inner());
        };
	return internal::ploadt<PacketScalar, LoadMode>(&default_value);
    }


    // constructor for datapointer which resembles the fixed size derived type
    // this works only if this is the only maped data!
    inline MultiMapBase(PointerType data)
        : default_value(0), m_rows(RowsAtCompileTime), m_cols(ColsAtCompileTime) {

        EIGEN_STATIC_ASSERT_FIXED_SIZE(Derived)
        m_data.push_back(fusion::make_vector(data, 0, 0, m_rows.value(), m_cols.value(), DynStride(Base::outerStride(), Base::innerStride())));
        checkSanity();
    }

    // constructor for datapointer which resembles the derived dynamic size vector
    inline MultiMapBase(PointerType data, Index size)
        : default_value(0),
          m_rows(RowsAtCompileTime == Dynamic ? size : Index(RowsAtCompileTime)),
          m_cols(ColsAtCompileTime == Dynamic ? 1 : Index(ColsAtCompileTime))  {

        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
        eigen_assert(size >= 0);
        eigen_assert(data == 0 || SizeAtCompileTime == Dynamic || SizeAtCompileTime == size);

        m_data.push_back(fusion::make_vector(data, 0, 0, size, 1, DynStride(Base::outerStride(), Base::innerStride())));
        checkSanity();
    }

    // constructor for datapointer which resembles the derived dynamic size matrix
    // this works only if this matrix is the only mapped data!
    inline MultiMapBase(PointerType data, Index rows, Index cols)
        : default_value(0), m_rows(rows), m_cols(cols) {

        eigen_assert((data == 0)
                     || (rows >= 0 && (RowsAtCompileTime == Dynamic || RowsAtCompileTime == rows)
                         && cols >= 0 && (ColsAtCompileTime == Dynamic || ColsAtCompileTime == cols)));

        m_data.push_back(fusion::make_vector(data, 0, 0, m_rows.value(), m_cols.value(), DynStride(Base::outerStride(), Base::innerStride())));
        checkSanity();
    }

    //constructor for a data pointer with stride. Data in combination with stride needs to resemble
    //the fixed size derived type. This only works if this is the only data
    template<typename Stride>
    inline MultiMapBase(PointerType data, const Stride& stride)
        : default_value(0), m_rows(RowsAtCompileTime), m_cols(ColsAtCompileTime) {

        EIGEN_STATIC_ASSERT_FIXED_SIZE(Derived)
        m_data.push_back(fusion::make_vector(data, 0, 0, m_rows.value(), m_cols.value(), DynStride(stride.outer(), stride.inner())));
        checkSanity();
    }

    //constructor for a data pointer with stride. Data in combination with stride needs to resemble
    //a part or all of the derived vector type.
    template<typename Stride>
    inline MultiMapBase(PointerType data, Index size, const Stride& stride)
        : default_value(0),
          m_rows(RowsAtCompileTime == Dynamic ? size : Index(RowsAtCompileTime)),
          m_cols(ColsAtCompileTime == Dynamic ? size : Index(ColsAtCompileTime)) {

        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
        eigen_assert(size >= 0);
        eigen_assert(data == 0 || SizeAtCompileTime == Dynamic || SizeAtCompileTime == size);
        m_data.push_back(fusion::make_vector(data, 0, 0, m_rows.value(), m_cols.value(), DynStride(stride.outer(), stride.inner())));
        checkSanity();
    }

    //constructor for a data pointer with stride. Data in combination with stride needs to resemble
    //a part or all of the derived matrix type
    template<typename Stride>
    inline MultiMapBase(PointerType data, Index rows, Index cols, const Stride& stride)
        : default_value(0), m_rows(rows), m_cols(cols) {

        eigen_assert((data == 0)
                     || (rows >= 0 && (RowsAtCompileTime == Dynamic || RowsAtCompileTime == rows)
                         && cols >= 0 && (ColsAtCompileTime == Dynamic || ColsAtCompileTime == cols)));
        m_data.push_back(fusion::make_vector(data, 0, 0, m_rows.value(), m_cols.value(), DynStride(Base::outerStride(), Base::innerStride())));
        checkSanity();
    }

    //constructor for arbitrary dense matrix types. The matrix must resemble a part or all of the
    //derived type.
    template<typename DerivedType>
    inline MultiMapBase(MatrixBase<DerivedType>& matrix)
        : default_value(0),
          m_rows(RowsAtCompileTime == Dynamic ? matrix.rows() : Index(RowsAtCompileTime)),
          m_cols(ColsAtCompileTime == Dynamic ? matrix.cols() : Index(ColsAtCompileTime)) {

        m_data.push_back(fusion::make_vector(&matrix(0, 0), 0, 0, matrix.rows(), matrix.cols(), DynStride(matrix.outerStride(), matrix.innerStride())));
        checkSanity();
    };


    //map extensions
    //**************

    //extend with data vector in derived type form
    inline void extend(PointerType data, Index size) {

        extend(data, size, DynStride(Base::outerStride(), Base::innerStride()));
    };

    //extend with data vector in arbitrary form
    template<typename Stride>
    inline void extend(PointerType data, Index size, const Stride& stride) {

        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
        eigen_assert(size >= 0);
        eigen_assert(data == 0 || SizeAtCompileTime == Dynamic || SizeAtCompileTime == size);

        m_data.push_back(fusion::make_vector(data, m_rows.value(), 0, size, 1, DynStride(stride.outer(), stride.inner())));
        m_rows.setValue(m_rows.value() + size);
        checkSanity();
    };

    //extend with eigen vector
    template<typename DerivedType>
    inline void extend(MatrixBase<DerivedType>& matrix) {

        //this only works for vectors, as we would not know where to add a matrix
        EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived)
        eigen_assert(matrix.cols() == 1);
        extend(matrix.derived().data(), matrix.rows(), DynStride(matrix.outerStride(), matrix.innerStride()));
    };

    //extend with general pointer data
    template<typename Stride>
    inline void extend(PointerType matrix, Index row, Index col, Index rows, Index cols, const Stride& stride) {

        m_data.push_back(fusion::make_vector(matrix, row, col, rows, cols, DynStride(stride.outer(), stride.inner())));

        if((row + rows) > m_rows.value())
            m_rows.setValue(row + rows);

        if((col + cols) > m_cols.value())
            m_cols.setValue(col + cols);

        checkSanity();
    };
    
    //extend with pointer data in derived type form
    inline void extend(PointerType matrix, Index row, Index col, Index rows, Index cols) {

        extend(matrix.derived().data(), row, col, rows, cols, DynStride(Base::outerStride(), Base::innerStride()));
    };
    
    //extend with eigen matrix
    template<typename DerivedType>
    inline void extend(MatrixBase<DerivedType>& matrix, Index row, Index col) {

        extend(matrix.derived().data(), row, col, matrix.rows(), matrix.cols(), DynStride(matrix.outerStride(), matrix.innerStride()));
    };

protected:

    void checkSanity() const {
        EIGEN_STATIC_ASSERT(EIGEN_IMPLIES(internal::traits<Derived>::Flags & PacketAccessBit,
                                          internal::inner_stride_at_compile_time<Derived>::ret == 1),
                            PACKET_ACCESS_REQUIRES_TO_HAVE_INNER_STRIDE_FIXED_TO_1);
        eigen_assert(EIGEN_IMPLIES(internal::traits<Derived>::Flags & AlignedBit, (size_t(fusion::at_c<0>(m_data.back())) % 16) == 0)
                     && "data is not aligned");
    }

    std::vector<MapData> m_data;
    Scalar default_value;
    internal::variable_if_dynamic<Index, RowsAtCompileTime> m_rows;
    internal::variable_if_dynamic<Index, ColsAtCompileTime> m_cols;
};

//this indirection is needed so that the map base coeff functions are called
template<typename Derived>
class MultiMap : public MultiMapBase< MultiMap<Derived> > {

public:
    typedef MultiMapBase< MultiMap<Derived> > Base;
    EIGEN_DENSE_PUBLIC_INTERFACE(MultiMap)

    inline Index innerStride() const {
        return 1;
    }

    inline Index outerStride() const {
        if(Flags & RowMajorBit)
            return Base::m_cols.value();
        else // column-major
            return Base::m_rows.value();
    }

    inline MultiMap(typename Base::PointerType matrix) : Base(matrix) {};
    inline MultiMap(typename Base::PointerType matrix, typename Base::Index size) : Base(matrix, size) {};
    inline MultiMap(typename Base::PointerType matrix,
                    typename Base::Index row,
                    typename Base::Index col) : Base(matrix, row, col) {};


    template<typename DerivedType>
    inline MultiMap(MatrixBase<DerivedType>& matrix) : Base(matrix) {};
};

} // details
} // dcm

#endif // EIGEN_MAPBASE_H
