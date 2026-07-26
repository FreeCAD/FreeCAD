#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include "eclass.h"
#include "part.h"

namespace py = pybind11;
using namespace pspy;

std::string face_repr(const Face& f) {
	std::string message = "";
	switch (f.function) {
	case SurfaceFunction::PLANE:
		message += "PLANE(";
		break;
	case SurfaceFunction::CYLINDER:
		message += "CYLINDER(";
		break;
	case SurfaceFunction::CONE:
		message += "CONE(";
		break;
	case SurfaceFunction::SPHERE:
		message += "SPHERE(";
		break;
	case SurfaceFunction::TORUS:
		message += "TORUS(";
		break;
	case SurfaceFunction::SPUN:
		message += "SPUN(";
		break;
	case SurfaceFunction::BSURF:
		message += "BSURF(";
		break;
	case SurfaceFunction::OFFSET:
		message += "OFFSET(";
		break;
	case SurfaceFunction::SWEPT:
		message += "SWEPT(";
		break;
	case SurfaceFunction::BLENDSF:
		message += "BLENDSF(";
		break;
	case SurfaceFunction::MESH:
		message += "MESH(";
		break;
	case SurfaceFunction::FSURF:
		message += "FSURF";
		break;
	case SurfaceFunction::SURFACEOFEXTRUSION:
		message += "SURFACEOFEXTRUSION";
		break;
	case SurfaceFunction::OTHERSURFACE:
		message += "OTHERSURFACE";
		break;
	}
	for (int i = 0; i < f.parameters.size(); ++i) {
		if (i > 0) {
			message += ",";
		}
		message += std::to_string(f.parameters[i]);
	}
	message += ")";
	return message;
}

std::string loop_repr(const Loop& l) {
	std::string message = "";
	switch (l._type) {
	case LoopType::OUTER:
		message = "<Outer Loop>";
		break;
	case LoopType::LIKELY_OUTER:
		message = "<Likely Outer Loop>";
		break;
	case LoopType::INNER:
		message = "<Inner Loop>";
		break;
	case LoopType::LIKELY_INNER:
		message = "<Likely Inner Loop>";
		break;
	case LoopType::INNER_SING:
		message = "<Inner Singular Loop>";
		break;
	case LoopType::VERTEX:
		message = "<Vertex Loop>";
		break;
	case LoopType::UNCLEAR:
		message = "<Unclear Loop>";
		break;
	case LoopType::WINDING:
		message = "<Winding Loop>";
		break;
	case LoopType::WIRE:
		message = "<Wire Loop>";
		break;
	case LoopType::ERROR:
		message = "<Error Loop>";
		break;
	}
	return message;
}

std::string edge_repr(const Edge& e) {
	std::string message = "";

	switch (e.function) {
	case CurveFunction::LINE:
		message += "LINE(";
		break;
	case CurveFunction::CIRCLE:
		message += "CIRCLE(";
		break;
	case CurveFunction::ELLIPSE:
		message += "ELLIPSE(";
		break;
	case CurveFunction::BCURVE:
		message += "BCURVE(";
		break;
	case CurveFunction::ICURVE:
		message += "ICURVE(";
		break;
	case CurveFunction::FCURVE:
		message += "FCURVE(";
		break;
	case CurveFunction::SPCURVE:
		message += "SPCURVE(";
		break;
	case CurveFunction::TRCURVE:
		message += "TRCURVE(";
		break;
	case CurveFunction::CPCURVE:
		message += "CPCURVE(";
		break;
	case CurveFunction::PLINE:
		message += "PLINE(";
		break;
	case CurveFunction::HYPERBOLA:
		message += "HYPERBOLA(";
		break;
	case CurveFunction::PARABOLA:
		message += "PARABOLA(";
		break;
	case CurveFunction::OFFSETCURVE:
		message += "OFFSETCURVE(";
		break;
	case CurveFunction::OTHERCURVE:
		message += "OTHERCURVE(";
		break;
	}

	for (int i = 0; i < e.parameters.size(); ++i) {
		if (i > 0) {
			message += ",";
		}
		message += std::to_string(e.parameters[i]);
	}
	message += ")";
	return message;
}

PYBIND11_MODULE(automate_cpp, m) {
	// part.h

	py::class_<PartOptions>(m, "PartOptions")
		.def(py::init<>())
		.def_readwrite("just_bb", &PartOptions::just_bb)
		.def_readwrite("normalize", &PartOptions::normalize)
		.def_readwrite("transform", &PartOptions::transform)
		.def_readwrite("transform_matrix", &PartOptions::transform_matrix)
		.def_readwrite("num_uv_samples", &PartOptions::num_uv_samples)
		.def_readwrite("num_random_samples", &PartOptions::num_random_samples)
		.def_readwrite("num_sdf_samples", &PartOptions::num_sdf_samples)
		.def_readwrite("sdf_sample_quality", &PartOptions::sdf_sample_quality)
		.def_readwrite("sample_normals", &PartOptions::num_uv_samples)
		.def_readwrite("sample_tangents", &PartOptions::sample_tangents)
		.def_readwrite("tesselate", &PartOptions::tesselate)
		.def_readwrite("default_mcfs", &PartOptions::default_mcfs)
		.def_readwrite("default_mcfs_only_face_axes", &PartOptions::default_mcfs_only_face_axes)
		.def_readwrite("onshape_style", &PartOptions::onshape_style)
		.def_readwrite("collect_inferences", &PartOptions::collect_inferences)
		.def_readwrite("set_quality", &PartOptions::set_quality)
		.def_readwrite("quality", &PartOptions::quality);

	py::class_<Part>(m, "Part")
		.def(py::init<const std::string&>())
		.def(py::init<const std::string&, PartOptions>())
		.def_readonly("mesh", &Part::mesh)
		.def_readonly("mesh_topology", &Part::mesh_topology)
		.def_readonly("brep", &Part::brep)
		.def_readonly("samples", &Part::samples)
		.def_readonly("random_samples", &Part::random_samples)
		.def_readonly("mask_sdf", &Part::mask_sdf)
		.def_readonly("summary", &Part::summary)
		.def_readonly("inferences", &Part::inferences)
		.def_readonly("default_mcfs", &Part::default_mcfs)
		.def_readonly("is_valid", &Part::_is_valid);

	py::class_<Mesh>(m, "Mesh")
		.def_readonly("V", &Mesh::V)
		.def_readonly("F", &Mesh::F);

	py::class_<MeshTopology>(m, "MeshTopology")
		.def_readonly("face_to_topology", &MeshTopology::face_to_topology)
		.def_readonly("edge_to_topology", &MeshTopology::edge_to_topology)
		.def_readonly("point_to_topology", &MeshTopology::point_to_topology);

	py::class_<InferenceReference>(m, "InferenceReference")
		.def_readonly("reference_index", &InferenceReference::reference_index)
		.def_readonly("reference_type", &InferenceReference::reference_type)
		.def_readonly("inference_type", &InferenceReference::inference_type);

	py::class_<PartInference>(m, "PartInference")
		.def_readonly("origin", &PartInference::origin)
		.def_readonly("axis", &PartInference::axis)
		.def_readonly("onshape_inference", &PartInference::onshape_inference)
		.def_readonly("flipped_in_onshape", &PartInference::flipped_in_onshape)
		.def_readonly("reference", &PartInference::reference);

	py::class_<PartFace>(m, "PartFace")
		.def_readonly("index", &PartFace::index)
		.def_readonly("function", &PartFace::function)
		.def_readonly("parameters", &PartFace::parameters)
		.def_readonly("orientation", &PartFace::orientation)
		.def_readonly("bounding_box", &PartFace::bounding_box)
		.def_readonly("na_bounding_box", &PartFace::na_bounding_box)
		.def_readonly("surface_area", &PartFace::surface_area)
		.def_readonly("circumference", &PartFace::circumference)
		.def_readonly("center_of_gravity", &PartFace::center_of_gravity)
		.def_readonly("moment_of_inertia", &PartFace::moment_of_inertia)
		.def_readonly("loop_neighbors", &PartFace::loop_neighbors)
		.def_readonly("edge_neighbors", &PartFace::edge_neighbors)
		.def_readonly("vertex_neighbors", &PartFace::vertex_neighbors)
		.def_readonly("inferences", &PartFace::inferences)
		.def_readonly("export_id", &PartFace::export_id);

	py::class_<PartLoop>(m, "PartLoop")
		.def_readonly("index", &PartLoop::index)
		.def_readonly("type", &PartLoop::type)
		.def_readonly("length", &PartLoop::length)
		.def_readonly("center_of_gravity", &PartLoop::center_of_gravity)
		.def_readonly("moment_of_inertia", &PartLoop::moment_of_inertia)
		.def_readonly("na_bounding_box", &PartLoop::na_bounding_box)
		.def_readonly("edge_neighbors", &PartLoop::edge_neighbors)
		.def_readonly("vertex_neighbors", &PartLoop::vertex_neighbors)
		.def_readonly("inferences", &PartLoop::inferences)
		.def_readonly("export_id", &PartLoop::export_id);

	py::class_<PartEdge>(m, "PartEdge")
		.def_readonly("index", &PartEdge::index)
		.def_readonly("function", &PartEdge::function)
		.def_readonly("parameters", &PartEdge::parameters)
		.def_readonly("orientation", &PartEdge::orientation)
		.def_readonly("t_range", &PartEdge::t_range)
		.def_readonly("start", &PartEdge::start)
		.def_readonly("end", &PartEdge::end)
		.def_readonly("is_periodic", &PartEdge::is_periodic)
		.def_readonly("mid_point", &PartEdge::mid_point)
		.def_readonly("length", &PartEdge::length)
		.def_readonly("center_of_gravity", &PartEdge::center_of_gravity)
		.def_readonly("moment_of_inertia", &PartEdge::moment_of_inertia)
		.def_readonly("bounding_box", &PartEdge::bounding_box)
		.def_readonly("na_bounding_box", &PartEdge::na_bounding_box)
		.def_readonly("vertex_neighbors", &PartEdge::vertex_neighbors)
		.def_readonly("inferences", &PartEdge::inferences)
		.def_readonly("export_id", &PartEdge::export_id);

	py::class_<PartVertex>(m, "PartVertex")
		.def_readonly("index", &PartVertex::index)
		.def_readonly("position", &PartVertex::position)
		.def_readonly("inferences", &PartVertex::inferences)
		.def_readonly("export_id", &PartVertex::export_id);

	py::class_<PartTopologyNodes>(m, "PartTopologyNodes")
		.def_readonly("faces", &PartTopologyNodes::faces)
		.def_readonly("loops", &PartTopologyNodes::loops)
		.def_readonly("edges", &PartTopologyNodes::edges)
		.def_readonly("vertices", &PartTopologyNodes::vertices);
	
	py::class_<PartTopologyRelations>(m, "PartTopologyRelations")
		.def_readonly("face_to_loop", &PartTopologyRelations::face_to_loop)
		.def_readonly("loop_to_edge", &PartTopologyRelations::loop_to_edge)
		.def_readonly("edge_to_vertex", &PartTopologyRelations::edge_to_vertex)
		.def_readonly("face_to_face", &PartTopologyRelations::face_to_face);

	py::class_<PartTopology>(m, "PartTopology")
		.def_readonly("nodes", &PartTopology::nodes)
		.def_readonly("relations", &PartTopology::relations);

	py::class_<PartSamples>(m, "PartSamples")
		.def_readonly("face_samples", &PartSamples::face_samples)
		.def_readonly("edge_samples", &PartSamples::edge_samples);

	py::class_<PartRandomSamples>(m, "PartRandomSamples")
		.def_readonly("samples", &PartRandomSamples::samples)
		.def_readonly("coords", &PartRandomSamples::coords)
		.def_readonly("uv_box", &PartRandomSamples::uv_box);

	py::class_<PartMaskSDF>(m, "PartMaskSDF")
		.def_readonly("coords", &PartMaskSDF::coords)
		.def_readonly("sdf", &PartMaskSDF::sdf)
		.def_readonly("uv_box", &PartMaskSDF::uv_box);

	py::class_<PartSummary>(m, "PartSummary")
		.def_readonly("bounding_box", &PartSummary::bounding_box)
		.def_readonly("volume", &PartSummary::volume)
		.def_readonly("mass", &PartSummary::mass)
		.def_readonly("center_of_gravity", &PartSummary::center_of_gravity)
		.def_readonly("moment_of_inertia", &PartSummary::moment_of_inertia)
		.def_readonly("surface_area", &PartSummary::surface_area)
		.def_readonly("topo_type_counts", &PartSummary::topo_type_counts)
		.def_readonly("surface_type_counts", &PartSummary::surface_type_counts)
		.def_readonly("curve_type_counts", &PartSummary::curve_type_counts)
		.def_readonly("loop_type_counts", &PartSummary::loop_type_counts)
		.def_readonly("fingerprint", &PartSummary::fingerprint);

	py::class_<PartUniqueInferences>(m, "PartUniqueInferences")
		.def_readonly("origins", &PartUniqueInferences::origins)
		.def_readonly("axes", &PartUniqueInferences::axes)
		.def_readonly("frames", &PartUniqueInferences::frames)
		.def_readonly("origin_references", &PartUniqueInferences::origin_references)
		.def_readonly("axes_references", &PartUniqueInferences::axes_references)
		.def_readonly("frame_references", &PartUniqueInferences::frame_references);

	py::class_<MCFReference>(m, "MCFReference")
		.def_readonly("origin_ref", &MCFReference::origin_ref)
		.def_readonly("axis_ref", &MCFReference::axis_ref);

	py::class_<MCF>(m, "MCF")
		.def_readonly("origin", &MCF::origin)
		.def_readonly("axis", &MCF::axis)
		.def_readonly("ref", &MCF::ref);

	// eclass.h
	m.def("find_equivalence_classes", &find_equivalence_classes);

	
	// body.h
	py::class_<Body>(m, "Body")
		.def("__repr__",
			[](const Body& p) {
				return "<Body>";
			});

#ifdef PARASOLID
	py::class_<PSBody>(m, "PSBody")
		.def("GetTopology", &PSBody::GetTopology)
		.def("GetMassProperties", &PSBody::GetMassProperties)
		.def("GetBoundingBox", &PSBody::GetBoundingBox)
		.def("Transform", &PSBody::Transform)
		.def("Tesselate", &PSBody::Tesselate)
		.def("__repr__",
			[](const PSBody& p) {
				return "<Parasolid Body>";
			});
#endif

	py::class_<OCCTBody>(m, "OCCTBody")
		.def("GetTopology", &OCCTBody::GetTopology)
		.def("GetMassProperties", &OCCTBody::GetMassProperties)
		.def("GetBoundingBox", &OCCTBody::GetBoundingBox)
		.def("Transform", &OCCTBody::Transform)
		.def("Tesselate", &OCCTBody::Tesselate)
		.def("__repr__",
			[](const OCCTBody& p) {
				return "<Open CASCADE Body>";
			});

	m.def("read_file", &read_file);
#ifdef PARASOLID
	m.def("read_xt", &read_xt);
#endif
	m.def("read_step", &read_step);

	

	// types.h

	py::enum_<TopologyType>(m, "TopologyType")
		.value("FACE", TopologyType::FACE)
		.value("EDGE", TopologyType::EDGE)
		.value("VERTEX", TopologyType::VERTEX)
		.value("LOOP", TopologyType::LOOP)
		.value("OTHER", TopologyType::OTHER);

	py::enum_<SurfaceFunction>(m, "SurfaceFunction")
		.value("PLANE", SurfaceFunction::PLANE)
		.value("CYLINDER", SurfaceFunction::CYLINDER)
		.value("CONE", SurfaceFunction::CONE)
		.value("SPHERE", SurfaceFunction::SPHERE)
		.value("TORUS", SurfaceFunction::TORUS)
		.value("SPUN", SurfaceFunction::SPUN)
		.value("BSURF", SurfaceFunction::BSURF)
		.value("OFFSET", SurfaceFunction::OFFSET)
		.value("SWEPT", SurfaceFunction::SWEPT)
		.value("BLENDSF", SurfaceFunction::BLENDSF)
		.value("MESH", SurfaceFunction::MESH)
		.value("FSURF", SurfaceFunction::FSURF)
		.value("SURFACEOFEXTRUSION", SurfaceFunction::SURFACEOFEXTRUSION)
		.value("OTHERSURFACE", SurfaceFunction::OTHERSURFACE)
		.value("NONE", SurfaceFunction::NONE);

	py::enum_<CurveFunction>(m, "CurveFunction")
		.value("LINE", CurveFunction::LINE)
		.value("CIRCLE", CurveFunction::CIRCLE)
		.value("ELLIPSE", CurveFunction::ELLIPSE)
		.value("BCURVE", CurveFunction::BCURVE)
		.value("ICURVE", CurveFunction::ICURVE)
		.value("FCURVE", CurveFunction::FCURVE)
		.value("SPCURVE", CurveFunction::SPCURVE)
		.value("TRCURVE", CurveFunction::TRCURVE)
		.value("CPCURVE", CurveFunction::CPCURVE)
		.value("PLINE", CurveFunction::PLINE)
		.value("HYPERBOLA", CurveFunction::HYPERBOLA)
		.value("PARABOLA", CurveFunction::PARABOLA)
		.value("OFFSETCURVE", CurveFunction::OFFSETCURVE)
		.value("OTHERCURVE", CurveFunction::OTHERCURVE)
		.value("NONE", CurveFunction::NONE);

	py::enum_<LoopType>(m, "LoopType")
		.value("VERTEX", LoopType::VERTEX)
		.value("WIRE", LoopType::WIRE)
		.value("OUTER", LoopType::OUTER)
		.value("INNER", LoopType::INNER)
		.value("WINDING", LoopType::WINDING)
		.value("INNER_SING", LoopType::INNER_SING)
		.value("LIKELY_OUTER", LoopType::LIKELY_OUTER)
		.value("LIKELY_INNER", LoopType::LIKELY_INNER)
		.value("UNCLEAR", LoopType::UNCLEAR)
		.value("ERROR", LoopType::ERROR);

	py::enum_<InferenceType>(m, "InferenceType")
		.value("CENTER", InferenceType::CENTER)
		.value("CENTROID", InferenceType::CENTROID)
		.value("MID_POINT", InferenceType::MID_POINT)
		.value("POINT", InferenceType::POINT)
		.value("TOP_AXIS_POINT", InferenceType::TOP_AXIS_POINT)
		.value("BOTTOM_AXIS_POINT", InferenceType::BOTTOM_AXIS_POINT)
		.value("MID_AXIS_POINT", InferenceType::MID_AXIS_POINT)
		.value("LOOP_CENTER", InferenceType::LOOP_CENTER);

	py::class_<MassProperties>(m, "MassProperties")
		.def_readonly("amount", &MassProperties::amount)
		.def_readonly("mass", &MassProperties::mass)
		.def_readonly("c_of_g", &MassProperties::c_of_g)
		.def_readonly("m_of_i", &MassProperties::m_of_i)
		.def_readonly("periphery", &MassProperties::periphery)
		.def("__repr__",
			[](const MassProperties& m) {
				return "MassProperties(amount=" + std::to_string(m.amount) + ")";
			});

	py::class_<Inference>(m, "Inference")
		.def_readonly("z_axis", &Inference::z_axis)
		.def_readonly("origin", &Inference::origin)
		.def_readonly("inference_type", &Inference::inference_type)
		.def_readonly("onshape_inference", &Inference::onshape_inference)
		.def_readonly("flipped_in_onshape", &Inference::flipped_in_onshape)
		.def("__repr__",
			[](const Inference& i) {
				return "<Inference>";
				/*
				return "Inference(origin=" + 
					"(" + std::to_string(i.origin(0)) + "," + std::to_string(i.origin(1)) + "," + std::to_string(i.origin(2)) + ")" +
					" , z_axis=" + 
					"(" + std::to_string(i.origin(0)) + "," + std::to_string(i.origin(1)) + "," + std::to_string(i.origin(2)) + ")" +
					")";
				*/
			});

	

	// topology.h
	py::enum_<TopoRelationSense>(m, "TopoRelationSense")
		.value("None", TopoRelationSense::None)
		.value("Negative", TopoRelationSense::Negative)
		.value("Positive", TopoRelationSense::Positive);

	py::class_<TopoRelation>(m, "TopoRelation")
		.def_readonly("_parent", &TopoRelation::_parent)
		.def_readonly("_child", &TopoRelation::_child)
		.def_readonly("_sense", &TopoRelation::_sense)
		.def("__repr__",
			[](const TopoRelation& tr) {
				return "TopoRelation(" + 
					std::to_string(tr._parent) + "," + 
					std::to_string(tr._child) + ")";
			});

	py::class_<BREPTopology>(m, "BREPTopology")
		.def_readonly("faces", &BREPTopology::faces)
		.def_readonly("loops", &BREPTopology::loops)
		.def_readonly("edges", &BREPTopology::edges)
		.def_readonly("vertices", &BREPTopology::vertices)
		.def_readonly("face_to_loop", &BREPTopology::face_to_loop)
		.def_readonly("loop_to_edge", &BREPTopology::loop_to_edge)
		.def_readonly("edge_to_vertex", &BREPTopology::edge_to_vertex)
		.def_readonly("loop_to_vertex", &BREPTopology::loop_to_vertex)
		.def_readonly("face_to_face", &BREPTopology::face_to_face)
		.def_readonly("face_loop", &BREPTopology::face_loop)
		.def_readonly("face_edge", &BREPTopology::face_edge)
		.def_readonly("face_vertex", &BREPTopology::face_vertex)
		.def_readonly("loop_edge", &BREPTopology::loop_edge)
		.def_readonly("loop_vertex", &BREPTopology::loop_vertex)
		.def_readonly("edge_vertex", &BREPTopology::edge_vertex)
		.def_readonly("pk_to_idx", &BREPTopology::pk_to_idx)
		.def_readonly("pk_to_class", &BREPTopology::pk_to_class)
		.def("__repr__",
			[](const BREPTopology& t) {
				return "<BREPTopology(" +
					std::to_string(t.faces.size()) + " faces, " +
					std::to_string(t.loops.size()) + " loops, " +
					std::to_string(t.edges.size()) + " edges, " +
					std::to_string(t.vertices.size()) + " vertices>";
			});


	// face.h
	py::class_<Face>(m, "Face")
		.def_readonly("function", &Face::function)
		.def_readonly("parameters", &Face::parameters)
		.def_readonly("orientation", &Face::orientation)
		.def_readonly("bounding_box", &Face::bounding_box)
		.def_readonly("na_bb_center", &Face::na_bb_center)
		.def_readonly("na_bb_x", &Face::na_bb_x)
		.def_readonly("na_bb_z", &Face::na_bb_z)
		.def_readonly("na_bounding_box", &Face::na_bounding_box)
		.def_readonly("surface_area", &Face::surface_area)
		.def_readonly("circumference", &Face::circumference)
		.def_readonly("center_of_gravity", &Face::center_of_gravity)
		.def_readonly("moment_of_inertia", &Face::moment_of_inertia)
		.def("__repr__", face_repr);

#ifdef PARASOLID
	py::class_<PSFace>(m, "PSFace")
		.def("get_inferences", &PSFace::get_inferences)
		.def("sample_points", &PSFace::sample_points)
		.def_readonly("function", &PSFace::function)
		.def_readonly("parameters", &PSFace::parameters)
		.def_readonly("orientation", &PSFace::orientation)
		.def_readonly("bounding_box", &PSFace::bounding_box)
		.def_readonly("na_bb_center", &PSFace::na_bb_center)
		.def_readonly("na_bb_x", &PSFace::na_bb_x)
		.def_readonly("na_bb_z", &PSFace::na_bb_z)
		.def_readonly("na_bounding_box", &PSFace::na_bounding_box)
		.def_readonly("surface_area", &PSFace::surface_area)
		.def_readonly("circumference", &PSFace::circumference)
		.def_readonly("center_of_gravity", &PSFace::center_of_gravity)
		.def_readonly("moment_of_inertia", &PSFace::moment_of_inertia)
		.def("__repr__", face_repr);
#endif

	py::class_<OCCTFace>(m, "OCCTFace")
		.def("get_inferences", &OCCTFace::get_inferences)
		.def("sample_points", &OCCTFace::sample_points)
		.def_readonly("function", &OCCTFace::function)
		.def_readonly("parameters", &OCCTFace::parameters)
		.def_readonly("orientation", &OCCTFace::orientation)
		.def_readonly("bounding_box", &OCCTFace::bounding_box)
		.def_readonly("na_bb_center", &OCCTFace::na_bb_center)
		.def_readonly("na_bb_x", &OCCTFace::na_bb_x)
		.def_readonly("na_bb_z", &OCCTFace::na_bb_z)
		.def_readonly("na_bounding_box", &OCCTFace::na_bounding_box)
		.def_readonly("surface_area", &OCCTFace::surface_area)
		.def_readonly("circumference", &OCCTFace::circumference)
		.def_readonly("center_of_gravity", &OCCTFace::center_of_gravity)
		.def_readonly("moment_of_inertia", &OCCTFace::moment_of_inertia)
		.def("__repr__", face_repr);


	// loop.h
	py::class_<Loop>(m, "Loop")
		.def_readonly("_type", &Loop::_type)
		.def_readonly("_is_circle", &Loop::_is_circle)
		.def_readonly("length", &Loop::length)
		.def_readonly("center_of_gravity", &Loop::center_of_gravity)
		.def_readonly("moment_of_inertia", &Loop::moment_of_inertia)
		.def_readonly("na_bb_center", &Loop::na_bb_center)
		.def_readonly("na_bb_x", &Loop::na_bb_x)
		.def_readonly("na_bb_z", &Loop::na_bb_z)
		.def_readonly("na_bounding_box", &Loop::na_bounding_box)
		.def("__repr__", loop_repr);

#ifdef PARASOLID
	py::class_<PSLoop>(m, "PSLoop")
		.def("get_inferences", &PSLoop::get_inferences)
		.def_readonly("_type", &PSLoop::_type)
		.def_readonly("_is_circle", &PSLoop::_is_circle)
		.def_readonly("length", &PSLoop::length)
		.def_readonly("center_of_gravity", &PSLoop::center_of_gravity)
		.def_readonly("moment_of_inertia", &PSLoop::moment_of_inertia)
		.def_readonly("na_bb_center", &PSLoop::na_bb_center)
		.def_readonly("na_bb_x", &PSLoop::na_bb_x)
		.def_readonly("na_bb_z", &PSLoop::na_bb_z)
		.def_readonly("na_bounding_box", &PSLoop::na_bounding_box)
		.def("__repr__", loop_repr);
#endif

	py::class_<OCCTLoop>(m, "OCCTLoop")
		.def("get_inferences", &OCCTLoop::get_inferences)
		.def_readonly("_type", &OCCTLoop::_type)
		.def_readonly("_is_circle", &OCCTLoop::_is_circle)
		.def_readonly("length", &OCCTLoop::length)
		.def_readonly("center_of_gravity", &OCCTLoop::center_of_gravity)
		.def_readonly("moment_of_inertia", &OCCTLoop::moment_of_inertia)
		.def_readonly("na_bb_center", &OCCTLoop::na_bb_center)
		.def_readonly("na_bb_x", &OCCTLoop::na_bb_x)
		.def_readonly("na_bb_z", &OCCTLoop::na_bb_z)
		.def_readonly("na_bounding_box", &OCCTLoop::na_bounding_box)
		.def("__repr__", loop_repr);

	// edge.h
	py::class_<Edge>(m, "Edge")
		.def_readonly("function", &Edge::function)
		.def_readonly("parameters", &Edge::parameters)
		.def_readonly("t_start", &Edge::t_start)
		.def_readonly("t_end", &Edge::t_end)
		.def_readonly("start", &Edge::start)
		.def_readonly("end", &Edge::end)
		.def_readonly("has_curve", &Edge::_has_curve)
		.def_readonly("_is_reversed", &Edge::_is_reversed)
		.def_readonly("is_periodic", &Edge::is_periodic)
		.def_readonly("mid_point", &Edge::mid_point)
		.def_readonly("length", &Edge::length)
		.def_readonly("center_of_gravity", &Edge::center_of_gravity)
		.def_readonly("moment_of_inertia", &Edge::moment_of_inertia)
		.def_readonly("bounding_box", &Edge::bounding_box)
		.def_readonly("na_bb_center", &Edge::na_bb_center)
		.def_readonly("na_bb_x", &Edge::na_bb_x)
		.def_readonly("na_bb_z", &Edge::na_bb_z)
		.def_readonly("nn_bounding_box", &Edge::na_bounding_box)
		.def("__repr__", edge_repr);

#ifdef PARASOLID
	py::class_<PSEdge>(m, "PSEdge")
		.def("get_inferences", &PSEdge::get_inferences)
		.def("sample_points", &PSEdge::sample_points)
		.def_readonly("function", &PSEdge::function)
		.def_readonly("parameters", &PSEdge::parameters)
		.def_readonly("t_start", &PSEdge::t_start)
		.def_readonly("t_end", &PSEdge::t_end)
		.def_readonly("start", &PSEdge::start)
		.def_readonly("end", &PSEdge::end)
		.def_readonly("has_curve", &PSEdge::_has_curve)
		.def_readonly("_is_reversed", &PSEdge::_is_reversed)
		.def_readonly("is_periodic", &PSEdge::is_periodic)
		.def_readonly("mid_point", &PSEdge::mid_point)
		.def_readonly("length", &PSEdge::length)
		.def_readonly("center_of_gravity", &PSEdge::center_of_gravity)
		.def_readonly("moment_of_inertia", &PSEdge::moment_of_inertia)
		.def_readonly("bounding_box", &PSEdge::bounding_box)
		.def_readonly("na_bb_center", &PSEdge::na_bb_center)
		.def_readonly("na_bb_x", &PSEdge::na_bb_x)
		.def_readonly("na_bb_z", &PSEdge::na_bb_z)
		.def_readonly("nn_bounding_box", &PSEdge::na_bounding_box)
		.def("__repr__", edge_repr);
#endif

	py::class_<OCCTEdge>(m, "OCCTEdge")
		.def("get_inferences", &OCCTEdge::get_inferences)
		.def("sample_points", &OCCTEdge::sample_points)
		.def_readonly("function", &OCCTEdge::function)
		.def_readonly("parameters", &OCCTEdge::parameters)
		.def_readonly("t_start", &OCCTEdge::t_start)
		.def_readonly("t_end", &OCCTEdge::t_end)
		.def_readonly("start", &OCCTEdge::start)
		.def_readonly("end", &OCCTEdge::end)
		.def_readonly("has_curve", &OCCTEdge::_has_curve)
		.def_readonly("_is_reversed", &OCCTEdge::_is_reversed)
		.def_readonly("is_periodic", &OCCTEdge::is_periodic)
		.def_readonly("mid_point", &OCCTEdge::mid_point)
		.def_readonly("length", &OCCTEdge::length)
		.def_readonly("center_of_gravity", &OCCTEdge::center_of_gravity)
		.def_readonly("moment_of_inertia", &OCCTEdge::moment_of_inertia)
		.def_readonly("bounding_box", &OCCTEdge::bounding_box)
		.def_readonly("na_bb_center", &OCCTEdge::na_bb_center)
		.def_readonly("na_bb_x", &OCCTEdge::na_bb_x)
		.def_readonly("na_bb_z", &OCCTEdge::na_bb_z)
		.def_readonly("nn_bounding_box", &OCCTEdge::na_bounding_box)
		.def("__repr__", edge_repr);
		

	// vertex.h
	py::class_<Vertex>(m, "Vertex")
		.def_readonly("position", &Vertex::position)
		.def("__repr__",
			[](const Vertex& v) {
				return "Vertex(" + std::to_string(v.position(0)) + "," + std::to_string(v.position(1)) + "," + std::to_string(v.position(2)) + ")";
			});

#ifdef PARASOLID
	py::class_<PSVertex>(m, "PSVertex")
		.def("get_inferences", &PSVertex::get_inferences)
		.def_readonly("position", &PSVertex::position)
		.def("__repr__",
			[](const PSVertex& v) {
				return "Vertex(" + std::to_string(v.position(0)) + "," + std::to_string(v.position(1)) + "," + std::to_string(v.position(2)) + ")";
			});
#endif


	py::class_<OCCTVertex>(m, "OCCTVertex")
		.def("get_inferences", &OCCTVertex::get_inferences)
		.def_readonly("position", &OCCTVertex::position)
		.def("__repr__",
			[](const OCCTVertex& v) {
				return "Vertex(" + std::to_string(v.position(0)) + "," + std::to_string(v.position(1)) + "," + std::to_string(v.position(2)) + ")";
			});

}