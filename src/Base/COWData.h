/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef BASE_COWDATA_H
#define BASE_COWDATA_H

/** @file
 * Copy-on-write storage, shared by std::shared_ptr.
 *
 * Written for the render cache in 2020 and lived under Gui/Inventor until
 * the appearance work needed the same thing one layer down: a value whose
 * storage several holders can share, where whoever writes first pays for a
 * copy and nobody else notices. Moved here so App, Part and Gui can all use
 * it; Gui/Inventor/COWData.h is a shim over this header that keeps the
 * renderer's unqualified spellings working.
 *
 * @section cow_ownership Ownership and threads
 *
 * NOT thread safe, and the reason is structural rather than incidental:
 * detaching asks std::shared_ptr::use_count() whether anyone else holds the
 * storage, and that answer is stale the moment a second thread can act on
 * it. The rule is therefore one holder per thread, and storage shared
 * across threads is read-only after publication -- a writer on one thread
 * while a reader on another holds the same data is undefined, no matter
 * that each call looks atomic.
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <ostream>
#include <set>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <vector>

namespace Base
{

// -------------------------------------------------------------
// Allocation accounting, opt-in and scoped

/** What one element type cost inside one scope */
struct COWMemUnit
{
    std::int64_t count {0};     /**< elements live now */
    std::int64_t maxcount {0};  /**< the most ever live at once */
    std::int64_t bytes {0};
};

/** One accounting scope, named by the tag type that selects it
 *
 * A scope is created by the first allocation that names it and lives for
 * the program. The numbers are diagnostic, so the mutex is the cheap
 * answer to the container's own thread rule: the containers are single
 * threaded per holder, but two holders on two threads allocating into one
 * scope would otherwise race on the map.
 */
class COWMemScope
{
public:
    explicit COWMemScope(const char *name)
        : _name(name)
    {}

    const char *name() const { return _name; }

    void note(const std::type_info &type, std::size_t count, std::size_t bytes)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _bytes += static_cast<std::int64_t>(bytes);
        if (_bytes > _maxbytes) {
            _maxbytes = _bytes;
        }
        COWMemUnit &unit = _units[std::type_index(type)];
        unit.count += static_cast<std::int64_t>(count);
        unit.bytes += static_cast<std::int64_t>(bytes);
        if (unit.count > unit.maxcount) {
            unit.maxcount = unit.count;
        }
    }

    void release(const std::type_info &type, std::size_t count, std::size_t bytes)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        _bytes -= static_cast<std::int64_t>(bytes);
        COWMemUnit &unit = _units[std::type_index(type)];
        unit.count -= static_cast<std::int64_t>(count);
        unit.bytes -= static_cast<std::int64_t>(bytes);
    }

    std::int64_t bytes() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _bytes;
    }

    std::int64_t maxBytes() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _maxbytes;
    }

    std::map<std::type_index, COWMemUnit> units() const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        return _units;
    }

    void report(std::ostream &out) const
    {
        std::lock_guard<std::mutex> guard(_mutex);
        out << "[" << _name << "] " << (_bytes / 1024) << "KB live, "
            << (_maxbytes / 1024) << "KB peak\n";
        for (const auto &unit : _units) {
            out << "    " << unit.first.name() << ": " << unit.second.count
                << " live (" << unit.second.maxcount << " peak), "
                << (unit.second.bytes / 1024) << "KB\n";
        }
    }

private:
    const char *_name;
    mutable std::mutex _mutex;
    std::int64_t _bytes {0};
    std::int64_t _maxbytes {0};
    std::map<std::type_index, COWMemUnit> _units;
};

/// Every scope some allocation has named, in creation order
inline std::vector<COWMemScope *> &cowMemScopes()
{
    static std::vector<COWMemScope *> scopes;
    return scopes;
}

/// Guards the registry itself, which is written once per scope and read by
/// a report that can come from anywhere
inline std::mutex &cowMemScopeMutex()
{
    static std::mutex mutex;
    return mutex;
}

/** The scope a tag names, created on first use
 *
 * A tag is any type with a static scopeName(); FC_COW_SCOPE declares one.
 * Keeping the scope in a template's function-local static means the
 * allocator resolves it without a lookup, and the registry below exists
 * only so a report can find every scope.
 */
template<class Tag>
COWMemScope &cowMemScope()
{
    static COWMemScope *scope = [] {
        auto *created = new COWMemScope(Tag::scopeName());
        std::lock_guard<std::mutex> guard(cowMemScopeMutex());
        cowMemScopes().push_back(created);
        return created;
    }();
    return *scope;
}

/// Write every scope's numbers. Nothing calls this on a schedule: it is
/// for a debugger, a console command or a test to ask with.
inline void reportCOWMemStats(std::ostream &out)
{
    std::lock_guard<std::mutex> guard(cowMemScopeMutex());
    for (const COWMemScope *scope : cowMemScopes()) {
        scope->report(out);
    }
}

/** Declare an accounting scope: FC_COW_SCOPE(RenderScope);
 *
 * The tag is a type, not a string, so the scope is chosen at compile time
 * and an untagged container pays nothing for the existence of tagged ones.
 */
#define FC_COW_SCOPE(Name)                                                     \
    struct Name                                                                \
    {                                                                          \
        static const char *scopeName()                                         \
        {                                                                      \
            return #Name;                                                      \
        }                                                                      \
    }

/** An allocator that counts what it hands out, per scope
 *
 * Stateless, so it costs a container nothing but the accounting itself,
 * and equal to every other instance for its scope -- which is what lets
 * two containers of the same type swap storage.
 *
 * @warning Choose it per CLASS or per local, never by switching a shared
 * alias in one translation unit: a type spelled with the tracking
 * allocator in one TU and without it in another is two different types,
 * and any class holding it in a header breaks the one-definition rule
 * silently. A build-wide macro (the renderer's FC_COW_MEM_TRACE) is safe
 * because it is the same everywhere; a per-file define is not.
 */
template<class T, class Tag>
class TrackingAllocator
{
public:
    using value_type = T;

    // Spelled out rather than inherited from std::allocator: C++17
    // deprecated these members there and C++20 removed them.
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<class U>
    struct rebind
    {
        using other = TrackingAllocator<U, Tag>;
    };

    TrackingAllocator() noexcept = default;

    // Not explicit: rebinding a container's allocator constructs one of
    // these from another instantiation, and a standard library is allowed
    // to do that by copy-initialization.
    template<class U>
    TrackingAllocator(const TrackingAllocator<U, Tag> &) noexcept  // NOLINT
    {}

    T *allocate(std::size_t count)
    {
        const std::size_t bytes = count * sizeof(T);
        // Over-aligned types need the aligned form; std::malloc, which
        // this used before, answers only up to max_align_t and hands back
        // storage a 32-byte vector type cannot legally live in.
        void *mem = nullptr;
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            mem = ::operator new(bytes, std::align_val_t(alignof(T)));
        }
        else {
            mem = ::operator new(bytes);
        }
        cowMemScope<Tag>().note(typeid(T), count, bytes);
        return static_cast<T *>(mem);
    }

    void deallocate(T *ptr, std::size_t count) noexcept
    {
        const std::size_t bytes = count * sizeof(T);
        cowMemScope<Tag>().release(typeid(T), count, bytes);
        if constexpr (alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            ::operator delete(ptr, bytes, std::align_val_t(alignof(T)));
        }
        else {
            ::operator delete(ptr, bytes);
        }
    }
};

template<class T, class U, class Tag>
bool operator==(const TrackingAllocator<T, Tag> &, const TrackingAllocator<U, Tag> &) noexcept
{
    return true;
}

template<class T, class U, class Tag>
bool operator!=(const TrackingAllocator<T, Tag> &, const TrackingAllocator<U, Tag> &) noexcept
{
    return false;
}

/** @name Containers that account into a scope
 *
 * Use these where the numbers are wanted; the plain std containers
 * everywhere else. Both are usable as the storage of the COW wrappers
 * below, which take their own allocator from whatever they are given.
 */
//@{
template<class T, class Tag>
using TrackedVector = std::vector<T, TrackingAllocator<T, Tag>>;

template<class T, class Tag>
using TrackedSet = std::set<T, std::less<T>, TrackingAllocator<T, Tag>>;

template<class KeyT, class ValueT, class Tag>
using TrackedMap = std::
    map<KeyT, ValueT, std::less<KeyT>, TrackingAllocator<std::pair<const KeyT, ValueT>, Tag>>;
//@}

/** @name The containers a COW wrapper defaults to
 *
 * Plain std containers, unless the build defines FC_COW_MEM_TRACE -- in
 * which case everything that did not name a scope of its own is counted
 * under one. That is a BUILD-WIDE switch on purpose: it decides what
 * Base::COWVector<T> means, and a per-file define would give the same
 * spelling two meanings in one program.
 */
//@{
#ifdef FC_COW_MEM_TRACE

/// Where containers that did not choose a scope are counted
FC_COW_SCOPE(COWDefaultScope);

template<class T>
using COWAllocator = TrackingAllocator<T, COWDefaultScope>;

#else  // FC_COW_MEM_TRACE

template<class T>
using COWAllocator = std::allocator<T>;

#endif  // FC_COW_MEM_TRACE

template<class T>
using FCVector = std::vector<T, COWAllocator<T>>;

template<class T>
using FCSet = std::set<T, std::less<T>, COWAllocator<T>>;

template<class KeyT, class ValueT>
using FCMap = std::map<KeyT, ValueT, std::less<KeyT>, COWAllocator<std::pair<const KeyT, ValueT>>>;
//@}

// -------------------------------------------------------------
// Copy on write

/// The allocator a COW wrapper allocates its storage OBJECT with: the
/// storage's own, rebound, so a tracked container's control block is
/// counted in the same scope as its elements, and a plain one costs
/// nothing.
template<class T, class = void>
struct COWAllocatorOf
{
    using type = std::allocator<T>;
};

template<class T>
struct COWAllocatorOf<T, std::void_t<typename T::allocator_type>>
{
    using type =
        typename std::allocator_traits<typename T::allocator_type>::template rebind_alloc<T>;
};

/** What every copy-on-write wrapper holds: one shared_ptr and the rule
 *
 * The rule is the whole of it: a const path never touches the pointer, and
 * a non-const path calls detach() first, which is a copy exactly when
 * somebody else is holding the same storage.
 */
template<class DataT>
class COWHolder
{
public:
    using AllocatorT = typename COWAllocatorOf<DataT>::type;

    /// Whether some other holder shares this storage, i.e. whether the
    /// next write pays for a copy
    bool isShared() const { return this->data.use_count() > 1; }

    bool isNull() const { return !this->data; }

    /// How many holders there are, this one included. Diagnostic; the
    /// containers below never branch on more than "> 1".
    long useCount() const { return this->data.use_count(); }

    void reset() { this->data.reset(); }

    void detach()
    {
        if (!this->data) {
            return;
        }
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<DataT>(AllocatorT(), *this->data);
        }
    }

    void copy(const DataT &other)
    {
        if (this->data.get() != &other) {
            if (!this->data || this->data.use_count() > 1) {
                this->data = std::allocate_shared<DataT>(AllocatorT(), other);
            }
            else {
                *this->data = other;
            }
        }
    }

    void move(DataT &&other)
    {
        if (this->data.get() != &other) {
            if (!this->data || this->data.use_count() > 1) {
                this->data = std::allocate_shared<DataT>(AllocatorT());
            }
            *this->data = std::move(other);
        }
    }

    /// Whether two holders are the same storage, which is the cheap half
    /// of every comparison below
    bool isSameData(const COWHolder<DataT> &other) const { return this->data == other.data; }

protected:
    /// Storage for a holder that has none yet
    DataT &create()
    {
        if (!this->data) {
            this->data = std::allocate_shared<DataT>(AllocatorT());
        }
        else if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<DataT>(AllocatorT(), *this->data);
        }
        return *this->data;
    }

    std::shared_ptr<DataT> data;
};

/** One shared value, copied on write
 *
 * The single-value flavour of the containers below, for a struct that is
 * one logical value rather than a sequence -- App::MaterialList's field
 * arrays, say, which are one appearance however many arrays it takes.
 *
 * A null holder reads as a default-constructed value, so "nothing stated"
 * costs no allocation and needs no special case in a reader.
 */
template<class T>
class COWValue: public COWHolder<T>
{
public:
    COWValue() = default;

    explicit COWValue(const T &value) { this->copy(value); }

    /// Read. Never detaches, never allocates -- a null holder answers with
    /// the default value.
    const T &get() const
    {
        static const T dummy {};
        return this->data ? *this->data : dummy;
    }

    const T &operator*() const { return get(); }

    const T *operator->() const { return &get(); }

    /// Write. Allocates if there is nothing yet, copies if the storage is
    /// shared, and hands back storage this holder alone owns.
    T &edit() { return this->create(); }

    void set(const T &value) { this->copy(value); }

    bool operator==(const COWValue<T> &other) const
    {
        if (this->data == other.data) {
            return true;
        }
        return get() == other.get();
    }

    bool operator!=(const COWValue<T> &other) const { return !operator==(other); }
};

/** The container flavour: a shared sequence or map, copied on write
 *
 * @warning operator< orders by SIZE first and only then by content. That
 * is deliberate -- it is the cheap key SoFCRenderCache::Material is sorted
 * by, where the common case is two arrays of different length -- but it is
 * not std::lexicographical_compare, so do not read it as one.
 */
template<class DataT, class ValueT>
class COWData: public COWHolder<DataT>
{
public:
    using AllocatorT = typename COWHolder<DataT>::AllocatorT;

    int getNum() const { return this->data ? static_cast<int>(this->data->size()) : 0; }

    int size() const { return getNum(); }

    bool empty() const { return getNum() == 0; }

    explicit operator bool() const { return getNum() != 0; }

    void clear()
    {
        if (!this->data) {
            return;
        }
        if (this->data.use_count() == 1) {
            this->data->clear();
        }
        else {
            this->data.reset();
        }
    }

    bool operator<(const COWData<DataT, ValueT> &other) const
    {
        if (size() < other.size()) {
            return true;
        }
        if (size() > other.size()) {
            return false;
        }
        if (this->data == other.data) {
            return false;
        }
        if (!this->data) {
            return true;
        }
        if (!other.data) {
            return false;
        }
        return *this->data < *other.data;
    }

    bool operator>(const COWData<DataT, ValueT> &other) const
    {
        if (size() < other.size()) {
            return false;
        }
        if (size() > other.size()) {
            return true;
        }
        if (this->data == other.data) {
            return false;
        }
        if (!this->data) {
            return false;
        }
        if (!other.data) {
            return true;
        }
        return *this->data > *other.data;
    }

    bool operator==(const COWData<DataT, ValueT> &other) const
    {
        if (this->data == other.data) {
            return true;
        }
        if (!this->data || !other.data) {
            return false;
        }
        return *this->data == *other.data;
    }

    bool operator!=(const COWData<DataT, ValueT> &other) const { return !operator==(other); }

    const DataT &getData() const
    {
        static const DataT dummy {};
        return this->data ? *this->data : dummy;
    }
};

// -------------------------------------------------------------
// simple copy on write map
template<class KeyT, class ValueT, class MapT = FCMap<KeyT, ValueT>>
class COWMap: public COWData<MapT, ValueT>
{
public:
    using AllocatorT = typename COWData<MapT, ValueT>::AllocatorT;

    const ValueT *get(const KeyT &key) const
    {
        if (!this->data) {
            return nullptr;
        }
        auto it = this->data->find(key);
        if (it == this->data->end()) {
            return nullptr;
        }
        return &it->second;
    }

    void erase(const KeyT &key)
    {
        if (!this->data) {
            return;
        }
        auto it = this->data->find(key);
        if (it == this->data->end()) {
            return;
        }
        if (this->data.use_count() == 1) {
            this->data->erase(it);
            return;
        }
        if (this->data->size() == 1) {
            this->data.reset();
            return;
        }
        // Shared, so the copy is rebuilt around the erased key rather than
        // copied and then erased. Note this walks the OLD map after the
        // member has been repointed, which is safe only because
        // use_count() > 1 says another holder is keeping it alive -- keep
        // that guarantee if this is ever restructured.
        auto begin = this->data->begin();
        auto end = this->data->end();
        this->data = std::allocate_shared<MapT>(AllocatorT());
        this->data->insert(begin, it);
        this->data->insert(std::next(it), end);
    }

    void set(const KeyT &key, const ValueT &value, bool overwrite = true)
    {
        if (!this->data) {
            this->data = std::allocate_shared<MapT>(AllocatorT());
        }
        else {
            auto it = this->data->find(key);
            if (it != this->data->end() && (!overwrite || it->second == value)) {
                return;
            }
            if (this->data.use_count() > 1) {
                this->data = std::allocate_shared<MapT>(AllocatorT(), *this->data);
            }
            else if (it != this->data->end()) {
                it->second = value;
                return;
            }
        }
        this->data->operator[](key) = value;
    }

    void add(const COWMap<KeyT, ValueT, MapT> &other, bool overwrite = true)
    {
        if (!other.data || other.data == this->data) {
            return;
        }
        for (auto &value : *other.data) {
            set(value.first, value.second, overwrite);
        }
    }

    void combine(const KeyT &key, const ValueT &value)
    {
        if (!this->data) {
            this->data = std::allocate_shared<MapT>(AllocatorT());
            this->data->emplace(key, value);
            return;
        }
        auto it = this->data->find(key);
        const bool found = it != this->data->end();
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<MapT>(AllocatorT(), *this->data);
        }
        else if (found) {
            // Unshared and present: combine in place, which is the only
            // branch that may use the iterator found above. It read
            // "else if (!found)" before this moved down here, i.e. it
            // dereferenced end().
            it->second.combine(value);
            return;
        }
        if (found) {
            this->data->operator[](key).combine(value);
        }
        else {
            this->data->emplace(key, value);
        }
    }

    void combine(const COWMap<KeyT, ValueT, MapT> &other)
    {
        if (!other.data || other.data == this->data) {
            return;
        }
        // Safe to walk other's storage while writing this one: the guard
        // above says they are different storage, and a detach here
        // allocates rather than touching other's.
        for (auto &value : *other.data) {
            combine(value.first, value.second);
        }
    }
};

// -------------------------------------------------------------
// simple copy on write vector
template<class ValueT, class VectorT = FCVector<ValueT>>
class COWVector: public COWData<VectorT, ValueT>
{
public:
    using AllocatorT = typename COWData<VectorT, ValueT>::AllocatorT;

    const VectorT &getData() const
    {
        static const VectorT dummy;
        return this->data ? *this->data : dummy;
    }

    typename VectorT::const_iterator begin() const { return this->getData().begin(); }

    typename VectorT::const_iterator end() const { return this->getData().end(); }

    /// Read one element. An index outside the vector answers with a
    /// default value rather than reading past the end -- the checks here
    /// are real branches rather than assertions on purpose, so that a
    /// release build and a debug build behave the same and a test can say
    /// what an out-of-range call does.
    const ValueT &get(int idx) const
    {
        static const ValueT dummy {};
        if (idx < 0 || idx >= this->size()) {
            return dummy;
        }
        return (*this->data)[idx];
    }

    /** Write access to one element
     *
     * @warning the pointer is into the storage, so anything that resizes
     * or detaches this vector afterwards invalidates it. Use it and drop
     * it.
     */
    ValueT *at(int idx)
    {
        if (idx < 0 || idx >= this->size()) {
            return nullptr;
        }
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        return &(*this->data)[idx];
    }

    const ValueT &operator[](int idx) const { return this->get(idx); }

    void set(int idx, const ValueT &value)
    {
        // Guarded, not asserted: use_count() is 0 on a null holder, so
        // the detach below would be skipped and the write would go
        // through a null pointer in a build with assertions compiled out.
        if (idx < 0 || idx >= this->size()) {
            return;
        }
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        (*this->data)[idx] = value;
    }

    bool compareAndSet(int idx, const ValueT &value)
    {
        if (idx == this->size()) {
            append(value);
            return true;
        }
        if (idx < 0 || idx > this->size()) {
            return false;
        }
        if (get(idx) != value) {
            set(idx, value);
            return true;
        }
        return false;
    }

    void erase(int idx)
    {
        if (idx < 0 || idx >= this->size()) {
            return;
        }
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        this->data->erase(this->data->begin() + idx);
    }

    void append(const ValueT &value)
    {
        this->create().push_back(value);
    }

    void append(ValueT &&value)
    {
        this->create().push_back(std::move(value));
    }

    template<class... ArgsT>
    ValueT &emplace_back(ArgsT &&...args)
    {
        VectorT &values = this->create();
        values.emplace_back(std::forward<ArgsT>(args)...);
        return values.back();
    }

    void push_back(const ValueT &value) { append(value); }

    void push_back(ValueT &&value) { append(std::move(value)); }

    /// By reference, not by value: these returned a copy per call before
    /// they moved here, which a hot loop pays for every time it peeks.
    const ValueT &back() const
    {
        static const ValueT dummy {};
        return this->empty() ? dummy : this->data->back();
    }

    const ValueT &front() const
    {
        static const ValueT dummy {};
        return this->empty() ? dummy : this->data->front();
    }

    bool reserve(int size)
    {
        if (size <= 0) {
            return false;
        }
        if (!this->data) {
            this->data = std::allocate_shared<VectorT>(AllocatorT());
        }
        else if (size <= static_cast<int>(this->data->capacity())) {
            return false;
        }
        else if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        this->data->reserve(size);
        return true;
    }

    void resize(int size)
    {
        if (size < 0) {
            return;
        }
        if (!this->data) {
            if (!size) {
                return;
            }
            this->data = std::allocate_shared<VectorT>(AllocatorT());
        }
        else if (size == static_cast<int>(this->data->size())) {
            return;
        }
        else if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        this->data->resize(size);
    }

    void append(const COWVector<ValueT, VectorT> &other)
    {
        if (!other.data) {
            return;
        }
        // Appending to nothing is the other vector, so take its storage
        // rather than copy it -- the case a caller building one array out
        // of several hits first.
        if (!this->data) {
            this->data = other.data;
            return;
        }
        if (this->data.use_count() > 1) {
            this->data = std::allocate_shared<VectorT>(AllocatorT(), *this->data);
        }
        this->data->insert(this->data->end(), other.data->begin(), other.data->end());
    }
};

}  // namespace Base

#endif  // BASE_COWDATA_H
