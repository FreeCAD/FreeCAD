class TangentFaces:
    """This class provides functions to check if brep faces are tangent
    to each other. each compare_x_x function accepts two surfaces of a
    particular type, and returns a boolean value indicating tangency.
    The compare function accepts two faces and selects the correct
    compare_x_x function automatically.
    """

    @staticmethod
    def compare_plane_plane(p1: Part.Plane, p2: Part.Plane) -> bool:
        # Returns True if the two planes have similar normals and
        # the base point of the first plane is (nearly) coincident with
        # the second plane.
        return (
            SheetMetalTools.smIsParallel(p1.Axis, p2.Axis)
            and p1.Position.distanceToPlane(p2.Position, p2.Axis) < eps
        )

    @staticmethod
    def compare_plane_cylinder(p: Part.Plane, c: Part.Cylinder) -> bool:
        # Returns True if the cylinder is tangent to the plane
        # (there is 'line contact' between the surfaces).
        return (
            SheetMetalTools.smIsNormal(p.Axis, c.Axis)
            and abs(abs(c.Center.distanceToPlane(p.Position, p.Axis)) - c.Radius) < eps
        )

    @staticmethod
    def compare_cylinder_cylinder(c1: Part.Cylinder, c2: Part.Cylinder) -> bool:
        # Returns True if the two cylinders have parallel axis' and
        # those axis' are separated by a distance of
        # approximately r1 + r2.
        return (
            SheetMetalTools.smIsParallel(c1.Axis, c2.Axis)
            and (
                abs(
                    c1.Center.distanceToLine(c2.Center, c2.Axis)
                    - (c1.Radius + c2.Radius)
                )
                < eps
            )
            or (
                abs(c1.Center.distanceToLine(c2.Center, c2.Axis) < eps)
                and (abs(c1.Radius - c2.Radius) < eps)
            )
        )

    @staticmethod
    def compare_plane_torus(p: Part.Plane, t: Part.Toroid) -> bool:
        # Imagine a donut sitting flat on a table.
        # That's our tangency condition for a plane and a toroid.
        return (
            SheetMetalTools.smIsParallel(p.Axis, t.Axis)
            and abs(abs(t.Center.distanceToPlane(p.Position, p.Axis)) - t.MinorRadius)
            < eps
        )

    @staticmethod
    def compare_cylinder_torus(c: Part.Cylinder, t: Part.Toroid) -> bool:
        # If the surfaces are tangent, either we have:
        # - a donut inside a circular container, with no gap at the
        #     container perimeter;
        # - a donut shoved onto a shaft with no wiggle room;
        # - a cylinder with an axis tangent to the central circle of
        #     the donut.
        return (
            SheetMetalTools.smIsParallel(c.Axis, t.Axis)
            and c.Center.distanceToLine(t.Center, t.Axis) < eps
            and (
                abs(c.Radius - abs(t.MajorRadius - t.MinorRadius)) < eps
                or abs(c.Radius - abs(t.MajorRadius + t.MinorRadius)) < eps
            )
        ) or (
            SheetMetalTools.smIsNormal(c.Axis, t.Axis)
            and abs(abs(t.Center.distanceToLine(c.Center, c.Axis)) - t.MajorRadius)
            < eps
            and abs(c.Radius - t.MinorRadius) < eps
        )

    @staticmethod
    def compare_sphere_sphere(s1: Part.Sphere, s2: Part.Sphere) -> bool:
        # Only segments of identical spheres are tangent to each other.
        return (
            s1.Center.distanceToPoint(s2.Center) < eps
            and abs(s1.Radius - s2.Radius) < eps
        )

    @staticmethod
    def compare_plane_sphere(p: Part.Plane, s: Part.Sphere) -> bool:
        # This function will probably never actually return True,
        # because a plane and a sphere only ever share a vertex if
        # they are tangent to each other.
        return abs(abs(s.Center.distanceToPlane(p.Position, p.Axis)) - s.Radius) < eps

    @staticmethod
    def compare_torus_sphere(t: Part.Toroid, s: Part.Sphere) -> bool:
        return (
            s.Center.distanceToPoint(t.Center) < eps
            and (
                abs(t.MajorRadius - t.MinorRadius - s.Radius) < eps
                or abs(t.MajorRadius + t.MinorRadius - s.Radius) < eps
            )
        ) or (
            abs(s.Radius - t.MinorRadius) < eps
            and SheetMetalTools.smIsNormal(t.Axis, s.Center - t.Center)
            and abs(t.Center.distanceToPoint(s.Center) - t.MajorRadius) < eps
        )

    @staticmethod
    def compare_torus_torus(t1: Part.Toroid, t2: Part.Toroid) -> bool:
        return (
            t1.Center.distanceToLine(t2.Center, t2.Axis) < eps
            and SheetMetalTools.smIsParallel(t1.Axis, t2.Axis)
            and abs(
                t1.Center.distanceToPoint(t2.Center) ** 2
                + (t1.MajorRadius - t2.MajorRadius) ** 2
                - (t1.MinorRadius + t2.MinorRadius) ** 2
            )
            < eps
        )

    @staticmethod
    def compare_cylinder_sphere(c: Part.Cylinder, s: Part.Sphere) -> bool:
        # The sphere must be sized/positioned like a ball sliding down
        # a tube with no wiggle room.
        return (
            (
                s.Center.distanceToLine(c.Center, c.Axis) < eps
                and abs(s.Radius - c.Radius) < eps
            )
            # Point contact case.
            or (
                abs(s.Center.distanceToLine(c.Center, c.Axis) - s.Radius - c.Radius)
                < eps
            )
        )

    @staticmethod
    def compare_plane_cone(p: Part.Plane, cn: Part.Cone) -> bool:
        return abs(cn.Apex.distanceToPlane(p.Position, p.Axis)) < eps and (
            abs(cn.Axis.getAngle(p.Axis) - abs(cn.SemiAngle) - pi / 2) < eps_angular
            or abs(cn.Axis.getAngle(p.Axis) + abs(cn.SemiAngle) - pi / 2) < eps_angular
        )

    @staticmethod
    def compare_cone_cone(cn1: Part.Cone, cn2: Part.Cone) -> bool:
        return (
            cn1.Apex.distanceToPoint(cn2.Apex) < eps
            and abs(cn1.Axis.getAngle(cn2.Axis) - cn1.SemiAngle - cn2.SemiAngle)
            < eps_angular
        )

    @staticmethod
    def compare_sphere_cone(s: Part.Sphere, cn: Part.Cone) -> bool:
        return (
            s.Center.distanceToLine(cn.Apex, cn.Axis) < eps
            and (cn.Apex.distanceToPoint(s.Center) * sin(cn.SemiAngle) - s.Radius) < eps
        )

    @staticmethod
    def compare_cylinder_cone(c: Part.Cylinder, cn: Part.Cone) -> bool:
        return abs(cn.Apex.distanceToLine(c.Center, c.Axis) - c.Radius) < eps and (
            abs(c.Axis.getAngle(cn.Axis) - cn.SemiAngle) < eps_angular
            or abs(pi - c.Axis.getAngle(cn.Axis) - abs(cn.SemiAngle)) < eps_angular
        )

    @staticmethod
    def compare_torus_cone(t: Part.Toroid, cn: Part.Cone) -> bool:
        return (
            SheetMetalTools.smIsParallel(t.Axis, cn.Axis)
            and cn.Apex.distanceToLine(t.Center, t.Axis) < eps
            and (
                abs(
                    t.MajorRadius / tan(cn.SemiAngle)
                    - t.MinorRadius / sin(cn.SemiAngle)
                    - cn.Apex.distanceToPoint(t.Center)
                )
                < eps
                or abs(
                    t.MajorRadius / tan(cn.SemiAngle)
                    + t.MinorRadius / sin(cn.SemiAngle)
                    - cn.Apex.distanceToPoint(t.Center)
                )
                < eps
            )
        )

    @staticmethod
    def compare_plane_extrusion(p: Part.Plane, ex: Part.SurfaceOfExtrusion) -> bool:
        return False  # TODO

    @staticmethod
    def compare_cylinder_extrusion(
        c: Part.Cylinder, ex: Part.SurfaceOfExtrusion
    ) -> bool:
        return False  # TODO

    @staticmethod
    def compare_torus_extrusion(t: Part.Toroid, ex: Part.SurfaceOfExtrusion) -> bool:
        return False  # TODO

    @staticmethod
    def compare_sphere_extrusion(s: Part.Sphere, ex: Part.SurfaceOfExtrusion) -> bool:
        return False  # TODO

    @staticmethod
    def compare_extrusion_extrusion(
        ex1: Part.SurfaceOfExtrusion, ex2: Part.SurfaceOfExtrusion
    ) -> bool:
        return False  # TODO

    @staticmethod
    def compare_extrusion_cone(ex: Part.SurfaceOfExtrusion, cn: Part.Cone) -> bool:
        return False  # TODO
