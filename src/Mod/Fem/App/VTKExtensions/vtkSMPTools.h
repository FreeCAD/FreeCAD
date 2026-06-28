/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPTools
 * @brief   A set of parallel (multi-threaded) utility functions.
 *
 * vtkSMPTools provides a set of utility functions that can
 * be used to parallelize parts of VTK code using multiple threads.
 * There are several back-end implementations of parallel functionality
 * (currently Sequential, TBB, OpenMP and STDThread) that actual execution is
 * delegated to.
 *
 * @sa
 * vtkSMPThreadLocal
 * vtkSMPThreadLocalObject
 */

#pragma once

#include "vtkCommonCoreModule.h"  // For export macro
#include "vtkObject.h"

#include "SMP/Common/vtkSMPToolsAPI.h"
#include "vtkSMPThreadLocal.h"  // For Initialized

#include <functional>   // For std::function
#include <iterator>     // For std::iterator
#include <type_traits>  // For std:::enable_if

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace vtk
{
namespace detail
{
namespace smp
{
template<typename T>
class vtkSMPTools_Has_Initialize
{
    typedef char (&no_type)[1];
    typedef char (&yes_type)[2];
    template<typename U, void (U::*)()>
    struct V
    {
    };
    template<typename U>
    static yes_type check(V<U, &U::Initialize>*);
    template<typename U>
    static no_type check(...);

public:
    static bool const value = sizeof(check<T>(nullptr)) == sizeof(yes_type);
};

template<typename T>
class vtkSMPTools_Has_Initialize_const
{
    typedef char (&no_type)[1];
    typedef char (&yes_type)[2];
    template<typename U, void (U::*)() const>
    struct V
    {
    };
    template<typename U>
    static yes_type check(V<U, &U::Initialize>*);
    template<typename U>
    static no_type check(...);

public:
    static bool const value = sizeof(check<T>(0)) == sizeof(yes_type);
};

template<typename Functor, bool Init>
struct vtkSMPTools_FunctorInternal;

template<typename Functor>
struct vtkSMPTools_FunctorInternal<Functor, false>
{
    Functor& F;
    vtkSMPTools_FunctorInternal(Functor& f)
        : F(f)
    {}
    void Execute(vtkIdType first, vtkIdType last)
    {
        this->F(first, last);
    }
    void For(vtkIdType first, vtkIdType last, vtkIdType grain)
    {
        auto& SMPToolsAPI = vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.For(first, last, grain, *this);
    }
    vtkSMPTools_FunctorInternal<Functor, false>& operator=(
        const vtkSMPTools_FunctorInternal<Functor, false>&
    );
    vtkSMPTools_FunctorInternal(const vtkSMPTools_FunctorInternal<Functor, false>&);
};

template<typename Functor>
struct vtkSMPTools_FunctorInternal<Functor, true>
{
    Functor& F;
    vtkSMPThreadLocal<unsigned char> Initialized;
    vtkSMPTools_FunctorInternal(Functor& f)
        : F(f)
        , Initialized(0)
    {}
    void Execute(vtkIdType first, vtkIdType last)
    {
        unsigned char& inited = this->Initialized.Local();
        if (!inited) {
            this->F.Initialize();
            inited = 1;
        }
        this->F(first, last);
    }
    void For(vtkIdType first, vtkIdType last, vtkIdType grain)
    {
        auto& SMPToolsAPI = vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.For(first, last, grain, *this);
        this->F.Reduce();
    }
    vtkSMPTools_FunctorInternal<Functor, true>& operator=(
        const vtkSMPTools_FunctorInternal<Functor, true>&
    );
    vtkSMPTools_FunctorInternal(const vtkSMPTools_FunctorInternal<Functor, true>&);
};

template<typename Functor>
class vtkSMPTools_Lookup_For
{
    static bool const init = vtkSMPTools_Has_Initialize<Functor>::value;

public:
    typedef vtkSMPTools_FunctorInternal<Functor, init> type;
};

template<typename Functor>
class vtkSMPTools_Lookup_For<Functor const>
{
    static bool const init = vtkSMPTools_Has_Initialize_const<Functor>::value;

public:
    typedef vtkSMPTools_FunctorInternal<Functor const, init> type;
};

template<typename Iterator, typename Functor, bool Init>
struct vtkSMPTools_RangeFunctor;

template<typename Iterator, typename Functor>
struct vtkSMPTools_RangeFunctor<Iterator, Functor, false>
{
    Functor& F;
    Iterator& Begin;
    vtkSMPTools_RangeFunctor(Iterator& begin, Functor& f)
        : F(f)
        , Begin(begin)
    {}
    void operator()(vtkIdType first, vtkIdType last)
    {
        Iterator itFirst(Begin);
        std::advance(itFirst, first);
        Iterator itLast(itFirst);
        std::advance(itLast, last - first);
        this->F(itFirst, itLast);
    }
};

template<typename Iterator, typename Functor>
struct vtkSMPTools_RangeFunctor<Iterator, Functor, true>
{
    Functor& F;
    Iterator& Begin;
    vtkSMPTools_RangeFunctor(Iterator& begin, Functor& f)
        : F(f)
        , Begin(begin)
    {}
    void Initialize()
    {
        this->F.Initialize();
    }
    void operator()(vtkIdType first, vtkIdType last)
    {
        Iterator itFirst(Begin);
        std::advance(itFirst, first);
        Iterator itLast(itFirst);
        std::advance(itLast, last - first);
        this->F(itFirst, itLast);
    }
    void Reduce()
    {
        this->F.Reduce();
    }
};

template<typename Iterator, typename Functor>
class vtkSMPTools_Lookup_RangeFor
{
    static bool const init = vtkSMPTools_Has_Initialize<Functor>::value;

public:
    typedef vtkSMPTools_RangeFunctor<Iterator, Functor, init> type;
};

template<typename Iterator, typename Functor>
class vtkSMPTools_Lookup_RangeFor<Iterator, Functor const>
{
    static bool const init = vtkSMPTools_Has_Initialize_const<Functor>::value;

public:
    typedef vtkSMPTools_RangeFunctor<Iterator, Functor const, init> type;
};

template<typename T>
using resolvedNotInt = typename std::enable_if<!std::is_integral<T>::value, void>::type;
}  // namespace smp
}  // namespace detail
}  // namespace vtk
#endif  // DOXYGEN_SHOULD_SKIP_THIS

class VTKCOMMONCORE_EXPORT vtkSMPTools
{
public:
    ///@{
    /**
     * Execute a for operation in parallel. First and last
     * define the range over which to operate (which is defined
     * by the operator). The operation executed is defined by
     * operator() of the functor object. The grain gives the parallel
     * engine a hint about the coarseness over which to parallelize
     * the function (as defined by last-first of each execution of
     * operator() ).
     */
    template<typename Functor>
    static void For(vtkIdType first, vtkIdType last, vtkIdType grain, Functor& f)
    {
        typename vtk::detail::smp::vtkSMPTools_Lookup_For<Functor>::type fi(f);
        fi.For(first, last, grain);
    }

    template<typename Functor>
    static void For(vtkIdType first, vtkIdType last, vtkIdType grain, Functor const& f)
    {
        typename vtk::detail::smp::vtkSMPTools_Lookup_For<Functor const>::type fi(f);
        fi.For(first, last, grain);
    }
    ///@}

    ///@{
    /**
     * Execute a for operation in parallel. First and last
     * define the range over which to operate (which is defined
     * by the operator). The operation executed is defined by
     * operator() of the functor object. The grain gives the parallel
     * engine a hint about the coarseness over which to parallelize
     * the function (as defined by last-first of each execution of
     * operator() ). Uses a default value for the grain.
     */
    template<typename Functor>
    static void For(vtkIdType first, vtkIdType last, Functor& f)
    {
        vtkSMPTools::For(first, last, 0, f);
    }

    template<typename Functor>
    static void For(vtkIdType first, vtkIdType last, Functor const& f)
    {
        vtkSMPTools::For(first, last, 0, f);
    }
    ///@}

    ///@{
    /**
     * Execute a for operation in parallel. Begin and end iterators
     * define the range over which to operate (which is defined
     * by the operator). The operation executed is defined by
     * operator() of the functor object. The grain gives the parallel
     * engine a hint about the coarseness over which to parallelize
     * the function (as defined by last-first of each execution of
     * operator() ).
     *
     * Usage example:
     * \code
     * template<class IteratorT>
     * class ExampleFunctor
     * {
     *   void operator()(IteratorT begin, IteratorT end)
     *   {
     *     for (IteratorT it = begin; it != end; ++it)
     *     {
     *       // Do stuff
     *     }
     *   }
     * };
     * ExampleFunctor<std::set<int>::iterator> worker;
     * vtkSMPTools::For(container.begin(), container.end(), 5, worker);
     * \endcode
     *
     * Lambda are also supported through Functor const&
     * function overload:
     * \code
     * vtkSMPTools::For(container.begin(), container.end(), 5,
     *    [](std::set<int>::iterator begin, std::set<int>::iterator end) {
     *      // Do stuff
     *    });
     * \endcode
     */
    template<typename Iter, typename Functor>
    static vtk::detail::smp::resolvedNotInt<Iter> For(Iter begin, Iter end, vtkIdType grain, Functor& f)
    {
        vtkIdType size = std::distance(begin, end);
        typename vtk::detail::smp::vtkSMPTools_Lookup_RangeFor<Iter, Functor>::type fi(begin, f);
        vtkSMPTools::For(0, size, grain, fi);
    }

    template<typename Iter, typename Functor>
    static vtk::detail::smp::resolvedNotInt<Iter> For(
        Iter begin,
        Iter end,
        vtkIdType grain,
        Functor const& f
    )
    {
        vtkIdType size = std::distance(begin, end);
        typename vtk::detail::smp::vtkSMPTools_Lookup_RangeFor<Iter, Functor const>::type fi(begin, f);
        vtkSMPTools::For(0, size, grain, fi);
    }
    ///@}

    ///@{
    /**
     * Execute a for operation in parallel. Begin and end iterators
     * define the range over which to operate (which is defined
     * by the operator). The operation executed is defined by
     * operator() of the functor object. Uses a default value
     * for the grain.
     *
     * Usage example:
     * \code
     * template<class IteratorT>
     * class ExampleFunctor
     * {
     *   void operator()(IteratorT begin, IteratorT end)
     *   {
     *     for (IteratorT it = begin; it != end; ++it)
     *     {
     *       // Do stuff
     *     }
     *   }
     * };
     * ExampleFunctor<std::set<int>::iterator> worker;
     * vtkSMPTools::For(container.begin(), container.end(), worker);
     * \endcode
     *
     * Lambda are also supported through Functor const&
     * function overload:
     * \code
     * vtkSMPTools::For(container.begin(), container.end(),
     *    [](std::set<int>::iterator begin, std::set<int>::iterator end) {
     *      // Do stuff
     *    });
     * \endcode
     */
    template<typename Iter, typename Functor>
    static vtk::detail::smp::resolvedNotInt<Iter> For(Iter begin, Iter end, Functor& f)
    {
        vtkSMPTools::For(begin, end, 0, f);
    }

    template<typename Iter, typename Functor>
    static vtk::detail::smp::resolvedNotInt<Iter> For(Iter begin, Iter end, Functor const& f)
    {
        vtkSMPTools::For(begin, end, 0, f);
    }
    ///@}

    /**
     * Get the backend in use.
     */
    static const char* GetBackend();

    /**
     * /!\ This method is not thread safe.
     * Change the backend in use.
     * The options can be: "Sequential", "STDThread", "TBB" or "OpenMP"
     *
     * VTK_SMP_BACKEND_IN_USE env variable can also be used to set the default SMPTools
     * backend, in that case SetBackend() doesn't need to be called.
     * The backend selected with SetBackend() have the priority over VTK_SMP_BACKEND_IN_USE.
     *
     * SetBackend() will return true if the backend was found and available.
     */
    static bool SetBackend(const char* backend);

    /**
     * /!\ This method is not thread safe.
     * Initialize the underlying libraries for execution. This is
     * not required as it is automatically defined by the libraries.
     * However, it can be used to control the maximum number of thread used.
     * Make sure to call it before the parallel operation.
     *
     * If Initialize is called without argument it will reset
     * to the maximum number of threads or use the VTK_SMP_MAX_THREADS
     * env variable if it is defined.
     *
     * Note: If VTK_SMP_MAX_THREADS env variable is defined the SMPTools will try
     * to use it to set the maximum number of threads. Initialize() doesn't
     * need to be called.
     */
    static void Initialize(int numThreads = 0);

    /**
     * Get the estimated number of threads being used by the backend.
     * This should be used as just an estimate since the number of threads may
     * vary dynamically and a particular task may not be executed on all the
     * available threads.
     */
    static int GetEstimatedNumberOfThreads();

    /**
     * /!\ This method is not thread safe.
     * If true enable nested parallelism for underlying backends.
     * When enabled the comportement is different for each backend:
     *    - TBB support nested parallelism by default.
     *    - For OpenMP, we set `omp_set_nested` to true so that it is supported.
     *    - STDThread support nested parallelism by creating new threads pools.
     *    - For Sequential nothing changes.
     *
     * Default to true
     */
    static void SetNestedParallelism(bool isNested);

    /**
     * Get true if the nested parallelism is enabled.
     */
    static bool GetNestedParallelism();

    /**
     * Return true if it is called from a parallel scope.
     */
    static bool IsParallelScope();

    /**
     * Structure used to specify configuration for LocalScope() method.
     * Several parameters can be configured:
     *    - MaxNumberOfThreads set the maximum number of threads.
     *    - Backend set a specific SMPTools backend.
     *    - NestedParallelism, if true enable nested parallelism.
     */
    struct Config
    {
        int MaxNumberOfThreads = 0;
        std::string Backend = vtk::detail::smp::vtkSMPToolsAPI::GetInstance().GetBackend();
        bool NestedParallelism = true;

        Config()
        {}
        Config(int maxNumberOfThreads)
            : MaxNumberOfThreads(maxNumberOfThreads)
        {}
        Config(std::string backend)
            : Backend(backend)
        {}
        Config(bool nestedParallelism)
            : NestedParallelism(nestedParallelism)
        {}
        Config(int maxNumberOfThreads, std::string backend, bool nestedParallelism)
            : MaxNumberOfThreads(maxNumberOfThreads)
            , Backend(backend)
            , NestedParallelism(nestedParallelism)
        {}
#ifndef DOXYGEN_SHOULD_SKIP_THIS
        Config(vtk::detail::smp::vtkSMPToolsAPI& API)
            : MaxNumberOfThreads(API.GetInternalDesiredNumberOfThread())
            , Backend(API.GetBackend())
            , NestedParallelism(API.GetNestedParallelism())
        {}
#endif  // DOXYGEN_SHOULD_SKIP_THIS
    };

    /**
     * /!\ This method is not thread safe.
     * Change the number of threads locally within this scope and call a functor which
     * should contains a vtkSMPTools method.
     *
     * Usage example:
     * \code
     * vtkSMPTools::LocalScope(
     *   vtkSMPTools::Config{ 4, "OpenMP", false }, [&]() { vtkSMPTools::For(0, size, worker); });
     * \endcode
     */
    template<typename T>
    static void LocalScope(Config const& config, T&& lambda)
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.LocalScope<vtkSMPTools::Config>(config, lambda);
    }

    /**
     * A convenience method for transforming data. It is a drop in replacement for
     * std::transform(), it does a unary operation on the input ranges. The data array must have the
     * same length. The performed transformation is defined by operator() of the functor object.
     *
     * Usage example with vtkDataArray:
     * \code
     * const auto range0 = vtk::DataArrayValueRange<1>(array0);
     * auto range1 = vtk::DataArrayValueRange<1>(array1);
     * vtkSMPTools::Transform(
     *   range0.cbegin(), range0.cend(), range1.begin(), [](double x) { return x - 1; });
     * \endcode
     *
     * Please visit vtkDataArrayRange.h documentation for more information and optimisation.
     */
    template<typename InputIt, typename OutputIt, typename Functor>
    static void Transform(InputIt inBegin, InputIt inEnd, OutputIt outBegin, Functor transform)
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.Transform(inBegin, inEnd, outBegin, transform);
    }

    /**
     * A convenience method for transforming data. It is a drop in replacement for
     * std::transform(), it does a binary operation on the input ranges. The data array must have
     * the same length. The performed transformation is defined by operator() of the functor object.
     *
     * Usage example with vtkDataArray:
     * \code
     * const auto range0 = vtk::DataArrayValueRange<1>(array0);
     * auto range1 = vtk::DataArrayValueRange<1>(array1);
     * vtkSMPTools::Transform(
     *   range0.cbegin(), range0.cend(), range1.cbegin(), range1.begin(),
     *   [](double x, double y) { return x * y; });
     * \endcode
     *
     * Please visit vtkDataArrayRange.h documentation for more information and optimisation.
     */
    template<typename InputIt1, typename InputIt2, typename OutputIt, typename Functor>
    static void Transform(
        InputIt1 inBegin1,
        InputIt1 inEnd,
        InputIt2 inBegin2,
        OutputIt outBegin,
        Functor transform
    )
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.Transform(inBegin1, inEnd, inBegin2, outBegin, transform);
    }

    /**
     * A convenience method for filling data. It is a drop in replacement for std::fill(),
     * it assign the given value to the element in ranges.
     *
     * Usage example with vtkDataArray:
     * \code
     * // Fill range with its first tuple value
     * auto range = vtk::DataArrayTupleRange<1>(array);
     * const auto value = *range.begin();
     * vtkSMPTools::Fill(range.begin(), range.end(), value);
     * \endcode
     *
     * Please visit vtkDataArrayRange.h documentation for more information and optimisation.
     */
    template<typename Iterator, typename T>
    static void Fill(Iterator begin, Iterator end, const T& value)
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.Fill(begin, end, value);
    }

    /**
     * A convenience method for sorting data. It is a drop in replacement for
     * std::sort(). Under the hood different methods are used. For example,
     * tbb::parallel_sort is used in TBB.
     */
    template<typename RandomAccessIterator>
    static void Sort(RandomAccessIterator begin, RandomAccessIterator end)
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.Sort(begin, end);
    }

    /**
     * A convenience method for sorting data. It is a drop in replacement for
     * std::sort(). Under the hood different methods are used. For example,
     * tbb::parallel_sort is used in TBB. This version of Sort() takes a
     * comparison class.
     */
    template<typename RandomAccessIterator, typename Compare>
    static void Sort(RandomAccessIterator begin, RandomAccessIterator end, Compare comp)
    {
        auto& SMPToolsAPI = vtk::detail::smp::vtkSMPToolsAPI::GetInstance();
        SMPToolsAPI.Sort(begin, end, comp);
    }
};

// VTK-HeaderTest-Exclude: vtkSMPTools.h
