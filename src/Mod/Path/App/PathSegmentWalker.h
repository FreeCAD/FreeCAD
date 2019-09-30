#include "PreCompiled.h"
#include <Mod/Path/App/Path.h>

#include <deque>

namespace Path
{

/**
 * PathSegmentVisitor is the companion class to PathSegmentWalker. Its members are called
 * with the segmented points of each command.
 */
class PathSegmentVisitor
{
  public:
    virtual ~PathSegmentVisitor();

    virtual void setup(const Base::Vector3d &last);

    virtual void g0(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts);
    virtual void g1(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts);
    virtual void g23(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts, const Base::Vector3d &center);
    virtual void g8x(int id, const Base::Vector3d &last, const Base::Vector3d &next, const std::deque<Base::Vector3d> &pts,
                     const std::deque<Base::Vector3d> &p, const std::deque<Base::Vector3d> &q);
    virtual void g38(int id, const Base::Vector3d &last, const Base::Vector3d &next);
};

/**
 * PathSegmentWalker processes a path a splits all movement commands into straight segments and calls the
 * appropriate member of the provided PathSegmentVisitor.
 * All non-movement commands are processed accordingly if they affect the movement commands.
 */
class PathSegmentWalker
{
public:
    PathSegmentWalker(const Toolpath &tp_);


    void walk(PathSegmentVisitor &cb, const Base::Vector3d &startPosition);

private:
    const Toolpath &tp;
};


}
