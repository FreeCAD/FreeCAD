#include "part.h"
#include "eclass.h"
#include <tuple>
#include <set>
#include <map>

namespace pspy {

Part::Part(const std::string& path, PartOptions options)
{
	auto bodies = read_file(path);
	if (bodies.size() != 1) {
		_is_valid = false;
		return;
	}
	_is_valid = true;
	auto& body = bodies[0];
	
	auto bounding_box = body->GetBoundingBox();

	if (options.just_bb) {
		summary.bounding_box = bounding_box;
		return;
	}

	if (options.normalize) {
		options.transform = true;
		Eigen::Vector3d min_corner = bounding_box.row(0);
		Eigen::Vector3d max_corner = bounding_box.row(1);
		Eigen::Vector3d diag = max_corner - min_corner;
		double scale = diag.maxCoeff();
		Eigen::Vector3d center = (max_corner + min_corner) / 2;
		options.transform_matrix <<
			1, 0, 0, -center(0),
			0, 1, 0, -center(1),
			0, 0, 1, -center(2),
			0, 0, 0, scale;
	}
	if (options.transform) {
		int err = body->Transform(options.transform_matrix);
		if (err != 0) {
			_is_valid = false;
			return;
		}
		bounding_box = body->GetBoundingBox();
	}
	
	auto topology = body->GetTopology();
	
	auto mass_properties = body->GetMassProperties();
	
	if (options.tesselate) {
		body->Tesselate(
			mesh.V,
			mesh.F,
			mesh_topology.face_to_topology,
			mesh_topology.edge_to_topology,
			mesh_topology.point_to_topology,
			options.set_quality,
			options.quality);
		mesh_topology.renumber(topology);
	}

	brep.init(topology);
	
	
	if (options.num_uv_samples > 0) {
		samples.init(topology, options);
	}

	if (options.num_random_samples > 0) {
		random_samples.init(topology, options);
	}

	if (options.num_sdf_samples > 0) {
		mask_sdf.init(topology, options);
	}
	
	summary.init(topology, mass_properties, bounding_box);
	
	if (options.collect_inferences) {
		inferences.init(brep);
	}

	if (options.default_mcfs) {
		init_default_mcfs(options.onshape_style, options.default_mcfs_only_face_axes);
	}
	
}

void Part::init_default_mcfs(bool onshape_style, bool just_face_axes)
{
	// Gather Face Oriented MCFs
	for (auto& face : brep.nodes.faces) {
		for (auto& ax_inf : face.inferences) {
			if (ax_inf.onshape_inference || !onshape_style) {
				default_mcfs.emplace_back(ax_inf, ax_inf, onshape_style);
				// For Axial Types (Cone, Cylinder, Torus, Spun), we don't want
				// to duplicate multiple axis references since they are equivalent
				// and it does not matter which inference type is used. We will
				// arbitrarily choose the top_axis_point since they all have it
				if (ax_inf.reference.inference_type == InferenceType::BOTTOM_AXIS_POINT ||
					ax_inf.reference.inference_type == InferenceType::MID_AXIS_POINT ||
					ax_inf.reference.inference_type == InferenceType::POINT) {
					continue;
				}
				for (auto& l: face.loop_neighbors) {
					auto& loop = brep.nodes.loops[l];
					for (auto& orig_inf : loop.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

				for (auto& e : face.edge_neighbors) {
					auto& edge = brep.nodes.edges[e];
					for (auto& orig_inf : edge.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

				for (auto& v : face.vertex_neighbors) {
					auto& vert = brep.nodes.vertices[v];
					for (auto& orig_inf : vert.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

			}
		}
	}

	if (just_face_axes) {
		return;
	}

	// Gather Loop Oriented MCFs
	for (auto& loop : brep.nodes.loops) {
		for (auto& ax_inf : loop.inferences) {
			if (ax_inf.onshape_inference || !onshape_style) {
				default_mcfs.emplace_back(ax_inf, ax_inf, onshape_style);

				for (auto& e : loop.edge_neighbors) {
					auto& edge = brep.nodes.edges[e];
					for (auto& orig_inf : edge.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

				for (auto& v : loop.vertex_neighbors) {
					auto& vert = brep.nodes.vertices[v];
					for (auto& orig_inf : vert.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

			}
		}
	}

	// Gather Edge Oriented MCFs
	for (auto& edge : brep.nodes.edges) {
		for (auto& ax_inf : edge.inferences) {
			if (ax_inf.onshape_inference || !onshape_style) {
				default_mcfs.emplace_back(ax_inf, ax_inf, onshape_style);

				for (auto& v : edge.vertex_neighbors) {
					auto& vert = brep.nodes.vertices[v];
					for (auto& orig_inf : vert.inferences) {
						if (orig_inf.onshape_inference || !onshape_style) {
							default_mcfs.emplace_back(orig_inf, ax_inf, onshape_style);
						}
					}
				}

			}
		}
	}

	// Gather Vertex Oriented MCFs
	for (auto& vert : brep.nodes.vertices) {
		for (auto& ax_inf : vert.inferences) {
			if (ax_inf.onshape_inference || !onshape_style) {
				default_mcfs.emplace_back(ax_inf, ax_inf, onshape_style);
			}
		}
	}
}

void MeshTopology::renumber(BREPTopology& topology)
{
	for (int i = 0; i < face_to_topology.size(); ++i) {
		face_to_topology(i) = topology.pk_to_idx[face_to_topology(i)];
	}
	for (int i = 0; i < edge_to_topology.rows(); ++i) {
		for (int j = 0; j < edge_to_topology.cols(); ++j) {

			auto topo_pk_idx = edge_to_topology(i, j);
			if (topo_pk_idx == 0) {
				edge_to_topology(i, j) = -1;
				continue;
			}
			
			auto topo_type = topology.pk_to_class[edge_to_topology(i, j)];
			if (topo_type == TopologyType::EDGE) {
				edge_to_topology(i, j) = topology.pk_to_idx[topo_pk_idx];
			}
			else {
				edge_to_topology(i, j) = -1;
			}
		}
	}
	for (int i = 0; i < point_to_topology.size(); ++i) {
		auto topo_pk_idx = point_to_topology(i);
		if (topo_pk_idx == 0) {
			point_to_topology(i) = -1;
			continue;
		}
		auto topo_type = topology.pk_to_class[topo_pk_idx];
		if (topo_type == TopologyType::VERTEX) {
			point_to_topology(i) = topology.pk_to_idx[topo_pk_idx];
		}
		else {
			point_to_topology(i) = -1;
		}
	}
}

void PartTopology::init(BREPTopology& topology)
{
	nodes.init(topology);
	relations.init(topology);
}

void PartTopologyRelations::init(BREPTopology& topology)
{
	face_to_loop.resize(2,topology.face_to_loop.size());
	for (int i = 0; i < topology.face_to_loop.size(); ++i) {
		face_to_loop(0, i) = topology.face_to_loop[i]._parent;
		face_to_loop(1, i) = topology.face_to_loop[i]._child;
	}

	loop_to_edge.resize(2, topology.loop_to_edge.size());
	for (int i = 0; i < topology.loop_to_edge.size(); ++i) {
		loop_to_edge(0, i) = topology.loop_to_edge[i]._parent;
		loop_to_edge(1, i) = topology.loop_to_edge[i]._child;
	}

	edge_to_vertex.resize(2, topology.edge_to_vertex.size());
	for (int i = 0; i < topology.edge_to_vertex.size(); ++i) {
		edge_to_vertex(0, i) = topology.edge_to_vertex[i]._parent;
		edge_to_vertex(1, i) = topology.edge_to_vertex[i]._child;
	}

	face_to_face.resize(3, topology.face_to_face.size());
	for (int i = 0; i < topology.face_to_face.size(); ++i) {
		face_to_face(0, i) = std::get<0>(topology.face_to_face[i]);
		face_to_face(1, i) = std::get<1>(topology.face_to_face[i]);
		face_to_face(2, i) = std::get<2>(topology.face_to_face[i]);
	}
}

void PartTopologyNodes::init(BREPTopology& topology)
{
	int f = 0;
	int l = 0;
	int e = 0;
	int v = 0;
	faces.reserve(topology.faces.size());
	for (auto& face : topology.faces) {
		faces.emplace_back(face, f);
		++f;
	}
	loops.reserve(topology.loops.size());
	for (auto& loop : topology.loops) {
		loops.emplace_back(loop, l);
		++l;
	}
	edges.reserve(topology.edges.size());
	for (auto& edge : topology.edges) {
		edges.emplace_back(edge, e);
		++e;
	}
	vertices.reserve(topology.vertices.size());
	for (auto& vertex : topology.vertices) {
		vertices.emplace_back(vertex, v);
		++v;
	}

	// Add Downstream Adjacency Lists
	for (f = 0; f < faces.size(); ++f) {
		faces[f].loop_neighbors = topology.face_loop[f];
		faces[f].edge_neighbors = topology.face_edge[f];
		faces[f].vertex_neighbors = topology.face_vertex[f];
	}
	for (l = 0; l < loops.size(); ++l) {
		loops[l].edge_neighbors = topology.loop_edge[l];
		loops[l].vertex_neighbors = topology.loop_vertex[l];
	}
	for (e = 0; e < edges.size(); ++e) {
		edges[e].vertex_neighbors = topology.edge_vertex[e];
	}

}

PartFace::PartFace(std::shared_ptr<Face>& f, int i)
{
	index = i;
	function = f->function;
	parameters = f->parameters;
	orientation = f->orientation;
	bounding_box = f->bounding_box;
	na_bounding_box.resize(5, 3);
	na_bounding_box.block<1, 3>(0, 0) = f->na_bb_center;
	na_bounding_box.block<1, 3>(1, 0) = f->na_bb_x;
	na_bounding_box.block<1, 3>(2, 0) = f->na_bb_z;
	na_bounding_box.block<2, 3>(3, 0) = f->na_bounding_box;
	surface_area = f->surface_area;
	circumference = f->circumference;
	center_of_gravity = f->center_of_gravity;
	moment_of_inertia = f->moment_of_inertia;

	auto infs = f->get_inferences();
	inferences.reserve(infs.size());
	for (auto& inf : infs) {
		inferences.emplace_back(inf, TopologyType::FACE, index);
	}

	export_id = f->export_id;

}

PartLoop::PartLoop(std::shared_ptr<Loop>& l, int i)
{
	index = i;
	type = l->_type;
	length = l->length;
	center_of_gravity = l->center_of_gravity;
	moment_of_inertia = l->moment_of_inertia;

	na_bounding_box.resize(5, 3);
	na_bounding_box.block<1, 3>(0, 0) = l->na_bb_center;
	na_bounding_box.block<1, 3>(1, 0) = l->na_bb_x;
	na_bounding_box.block<1, 3>(2, 0) = l->na_bb_z;
	na_bounding_box.block<2, 3>(3, 0) = l->na_bounding_box;

	auto infs = l->get_inferences();
	inferences.reserve(infs.size());
	for (auto& inf : infs) {
		inferences.emplace_back(inf, TopologyType::LOOP, index);
	}

	export_id = l->export_id;
}

PartEdge::PartEdge(std::shared_ptr<Edge>& e, int i)
{
	index = i;
	function = e->function;
	parameters = e->parameters;
	orientation = !e->_is_reversed;
	t_range.resize(2);
	t_range(0) = e->t_start;
	t_range(1) = e->t_end;
	start = e->start;
	end = e->end;
	is_periodic = e->is_periodic;
	mid_point = e->mid_point;
	length = e->length;
	center_of_gravity = e->center_of_gravity;
	moment_of_inertia = e->moment_of_inertia;
	bounding_box = e->bounding_box;
	na_bounding_box.resize(5, 3);
	na_bounding_box.block<1, 3>(0, 0) = e->na_bb_center;
	na_bounding_box.block<1, 3>(1, 0) = e->na_bb_x;
	na_bounding_box.block<1, 3>(2, 0) = e->na_bb_z;
	na_bounding_box.block<2, 3>(3, 0) = e->na_bounding_box;

	auto infs = e->get_inferences();
	inferences.reserve(infs.size());
	for (auto& inf : infs) {
		inferences.emplace_back(inf, TopologyType::EDGE, index);
	}

	export_id = e->export_id;
}

PartVertex::PartVertex(std::shared_ptr<Vertex>& v, int i)
{
	index = i;
	position = v->position;

	auto infs = v->get_inferences();
	inferences.reserve(infs.size());
	for (auto& inf : infs) {
		inferences.emplace_back(inf, TopologyType::VERTEX, index);
	}
	
	export_id = v->export_id;
}

void PartSamples::init(BREPTopology& topology, PartOptions options)
{
	int num_points = options.num_uv_samples;
	bool sample_normals = options.sample_normals;
	bool sample_tangents = options.sample_tangents;
	Eigen::MatrixXd uv_box;
	face_samples.resize(topology.faces.size());
	for (int i = 0; i < topology.faces.size(); ++i) {
		topology.faces[i]->sample_points(num_points, sample_normals, face_samples[i], uv_box);
	}

	Eigen::Vector2d t_range;
	edge_samples.resize(topology.edges.size());
	for (int i = 0; i < topology.edges.size(); ++i) {
		topology.edges[i]->sample_points(num_points, sample_tangents, edge_samples[i], t_range);
	}
}

void PartRandomSamples::init(BREPTopology& topology, PartOptions options)
{
	const int num_points = options.num_random_samples;
	const int n_faces = topology.faces.size();
	samples.resize(n_faces);
	coords.resize(n_faces);
	uv_box.resize(n_faces);

	
	for (int i = 0; i < n_faces; ++i) {
		topology.faces[i]->random_sample_points(num_points, samples[i], coords[i], uv_box[i]);
	}
}

void PartMaskSDF::init(BREPTopology& topology, PartOptions options)
{
	const int quality = options.sdf_sample_quality;
	const int num_points = options.num_sdf_samples;
	const int n_faces = topology.faces.size();
	sdf.resize(n_faces);
	coords.resize(n_faces);
	uv_box.resize(n_faces);
	for (int i = 0; i < n_faces; ++i) {
		// TODO - commented out so it would compile - implement in the OCCT case, or remove
		//topology.faces[i].sample_mask_sdf(quality, num_points, coords[i], sdf[i], uv_box[i]);
	}
}



void PartSummary::init(BREPTopology& topology, MassProperties& mass_props, Eigen::MatrixXd& bb)
{
	bounding_box = bb;
	volume = mass_props.amount;
	mass = mass_props.mass;
	center_of_gravity = mass_props.c_of_g;
	moment_of_inertia = mass_props.m_of_i;
	surface_area = mass_props.periphery;

	topo_type_counts.resize(4);
	topo_type_counts(0) = topology.faces.size();
	topo_type_counts(1) = topology.edges.size();
	topo_type_counts(2) = topology.vertices.size();
	topo_type_counts(3) = topology.loops.size(); // Put loops last to match old fingerprint

	surface_type_counts.resize(15);
	surface_type_counts.setZero();
	for (auto& face : topology.faces) {
		int f_idx = static_cast<int>(face->function);
		surface_type_counts(f_idx) += 1;
	}

	curve_type_counts.resize(15);
	curve_type_counts.setZero();
	for (auto& edge : topology.edges) {
		int f_idx = static_cast<int>(edge->function);
		curve_type_counts(f_idx) += 1;
	}

	loop_type_counts.resize(10);
	loop_type_counts.setZero();
	for (auto& loop : topology.loops) {
		int t_idx = static_cast<int>(loop->_type);
		loop_type_counts(t_idx) += 1;
	}

	// Collect a hash data vector for part deduplication
	fingerprint.resize(14);
	fingerprint(0) = volume;
	// Take upper triangular half of moment_of_inertia matrix
	fingerprint.block<3, 1>(1, 0) = moment_of_inertia.block<1, 3>(0, 0);
	fingerprint.block<2, 1>(4, 0) = moment_of_inertia.block<1, 2>(1, 1);
	fingerprint(6) = moment_of_inertia(2, 2);
	fingerprint.block<3, 1>(7, 0) = center_of_gravity;
	fingerprint.block<4, 1>(10,0) = topo_type_counts.block<4,1>(0,0);
}

PartInference::PartInference(const Inference& inf, TopologyType ref_type, int ref_index)
{
	origin = inf.origin;
	axis = inf.z_axis;
	onshape_inference = inf.onshape_inference;
	flipped_in_onshape = inf.flipped_in_onshape;
	reference.inference_type = inf.inference_type;
	reference.reference_type = ref_type;
	reference.reference_index = ref_index;
}

void PartUniqueInferences::init(const PartTopology& topo, double tolerance)
{
	std::vector<Eigen::VectorXd> all_origins;
	std::vector<Eigen::VectorXd> all_axes;
	std::vector<Eigen::VectorXd> all_frames;
	std::vector<InferenceReference> all_refs;

	// Collect All Inferences for Deduplication
	for (auto& t : topo.nodes.faces) {
		for (auto& inf : t.inferences) {
			all_origins.push_back(inf.origin);
			all_axes.push_back(inf.axis);
			Eigen::VectorXd frame(6);
			frame.block<3, 1>(0, 0) = inf.origin;
			frame.block<3, 1>(3, 0) = inf.axis;
			all_frames.push_back(frame);
			all_refs.push_back(inf.reference);
		}
	}
	for (auto& t : topo.nodes.loops) {
		for (auto& inf : t.inferences) {
			all_origins.push_back(inf.origin);
			all_axes.push_back(inf.axis);
			Eigen::VectorXd frame(6);
			frame.block<3, 1>(0, 0) = inf.origin;
			frame.block<3, 1>(3, 0) = inf.axis;
			all_frames.push_back(frame);
			all_refs.push_back(inf.reference);
		}
	}
	for (auto& t : topo.nodes.edges) {
		for (auto& inf : t.inferences) {
			all_origins.push_back(inf.origin);
			all_axes.push_back(inf.axis);
			Eigen::VectorXd frame(6);
			frame.block<3, 1>(0, 0) = inf.origin;
			frame.block<3, 1>(3, 0) = inf.axis;
			all_frames.push_back(frame);
			all_refs.push_back(inf.reference);
		}
	}
	for (auto& t : topo.nodes.vertices) {
		for (auto& inf : t.inferences) {
			all_origins.push_back(inf.origin);
			all_axes.push_back(inf.axis);
			Eigen::VectorXd frame(6);
			frame.block<3, 1>(0, 0) = inf.origin;
			frame.block<3, 1>(3, 0) = inf.axis;
			all_frames.push_back(frame);
			all_refs.push_back(inf.reference);
		}
	}

	// Deduplicate inferences
	auto origin_eclasses = find_equivalence_classes(all_origins, tolerance);
	auto axes_eclasses = find_equivalence_classes(all_axes, tolerance);
	auto frame_eclasses = find_equivalence_classes(all_frames, tolerance);

	// Find unique equivalence class ids
	std::set<int> origin_eclass_ids(origin_eclasses.begin(), origin_eclasses.end());
	std::set<int> axes_eclass_ids(axes_eclasses.begin(), axes_eclasses.end());
	std::set<int> frame_eclass_ids(frame_eclasses.begin(), frame_eclasses.end());

	int n_origins = origin_eclass_ids.size();
	int n_axes = axes_eclass_ids.size();
	int n_frames = frame_eclass_ids.size();

	origins.resize(n_origins, 3);
	origin_references.resize(n_origins);
	axes.resize(n_axes, 3);
	axes_references.resize(n_axes);
	frames.resize(n_frames, 6);
	frame_references.resize(n_frames);

	// Renumber starting at 0
	// Also, copy the unique vectors
	int i = 0;
	std::map<int, int> origin_reindex;
	for (auto c : origin_eclass_ids) {
		origin_reindex[c] = i;
		origins.block<1, 3>(i, 0) = all_origins[c];
		++i;
	}
	

	i = 0;
	std::map<int, int> axes_reindex;
	for (auto c : axes_eclass_ids) {
		axes_reindex[c] = i;
		axes.block<1, 3>(i, 0) = all_axes[c];
		++i;
	}

	i = 0;
	std::map<int, int> frame_reindex;
	for (auto c : frame_eclass_ids) {
		frame_reindex[c] = i;
		frames.block<1, 6>(i, 0) = all_frames[c];
		++i;
	}
	
	// Finally, collect references
	for (i = 0; i < all_refs.size(); ++i) {
		origin_references[origin_reindex[origin_eclasses[i]]].push_back(all_refs[i]);
		axes_references[axes_reindex[axes_eclasses[i]]].push_back(all_refs[i]);
		frame_references[frame_reindex[frame_eclasses[i]]].push_back(all_refs[i]);
	}
}

MCF::MCF(const PartInference& origin_inf, const PartInference& axis_inf, bool onshape_style)
{
	origin = origin_inf.origin;
	axis = axis_inf.axis;
	if (onshape_style && axis_inf.flipped_in_onshape) {
		axis = -axis;
	}
	ref.origin_ref = origin_inf.reference;
	ref.axis_ref = axis_inf.reference;
}

}