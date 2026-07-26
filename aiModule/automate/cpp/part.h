#ifndef PART_H_INCLUDED
#define PART_H_INCLUDED

#include <string>
#include <Eigen/Core>
#include <vector>
#include "body.h"

namespace pspy {
struct PartOptions {
	bool just_bb = false;
	bool normalize = false;
	bool transform = false;
	Eigen::Matrix<double, 4, 4> transform_matrix;
	int num_uv_samples = 10;
	int num_random_samples = 0;
	int num_sdf_samples = 0;
	int sdf_sample_quality = 5000;
	bool sample_normals = true;
	bool sample_tangents = true;
	bool tesselate = true;
	bool default_mcfs = true;
	bool default_mcfs_only_face_axes = false;
	bool onshape_style = true;
	bool collect_inferences = false;
	bool set_quality = false;
	double quality = 0.01;
};

struct Mesh {
	Eigen::MatrixXd V;
	Eigen::MatrixXi F;
};

struct MeshTopology {
	Eigen::VectorXi face_to_topology;
	Eigen::MatrixXi edge_to_topology;
	Eigen::VectorXi point_to_topology;

	void renumber(BREPTopology& topology);
};

struct InferenceReference {
	int reference_index;
	TopologyType reference_type;
	InferenceType inference_type;
};

struct PartInference {
	PartInference(const Inference& inf, TopologyType ref_type, int ref_index);
	Eigen::Vector3d origin;
	Eigen::Vector3d axis;
	
	bool onshape_inference;
	bool flipped_in_onshape;

	InferenceReference reference;
};

struct PartFace {
	PartFace(std::shared_ptr<Face>& f, int i);
	int index;
	SurfaceFunction function;
	std::vector<double> parameters;
	bool orientation; // True is face normal matches surface normal
	Eigen::MatrixXd bounding_box;
	Eigen::MatrixXd na_bounding_box;
	double surface_area;
	double circumference;
	Eigen::Vector3d center_of_gravity;
	Eigen::MatrixXd moment_of_inertia;

	std::vector<int> loop_neighbors;
	std::vector<int> edge_neighbors;
	std::vector<int> vertex_neighbors;

	std::vector<PartInference> inferences;

	std::string export_id;
};

struct PartLoop {
	PartLoop(std::shared_ptr<Loop>& l, int i);
	int index;
	LoopType type;
	double length;
	Eigen::Vector3d center_of_gravity;
	Eigen::MatrixXd moment_of_inertia;
	Eigen::MatrixXd na_bounding_box;

	std::vector<int> edge_neighbors;
	std::vector<int> vertex_neighbors;

	std::vector<PartInference> inferences;

	std::string export_id;
};

struct PartEdge {
	PartEdge(std::shared_ptr<Edge>& e, int i);
	int index;
	CurveFunction function;
	std::vector<double> parameters;
	bool orientation; // true is edge direction matches curve direction
	Eigen::VectorXd t_range;
	Eigen::Vector3d start;
	Eigen::Vector3d end;
	bool is_periodic;
	Eigen::Vector3d mid_point;
	double length;
	Eigen::Vector3d center_of_gravity;
	Eigen::MatrixXd moment_of_inertia;
	Eigen::MatrixXd bounding_box;
	Eigen::MatrixXd na_bounding_box;

	std::vector<int> vertex_neighbors;

	std::vector<PartInference> inferences;

	std::string export_id;
};

struct PartVertex {
	PartVertex(std::shared_ptr<Vertex>& v, int i);
	int index;
	Eigen::Vector3d position;

	std::vector<PartInference> inferences;

	std::string export_id;
};

struct PartTopologyNodes {
	void init(BREPTopology& topology);
	std::vector<PartFace> faces;
	std::vector<PartLoop> loops;
	std::vector<PartEdge> edges;
	std::vector<PartVertex> vertices;
};

struct PartTopologyRelations {
	void init(BREPTopology& topology);
	Eigen::MatrixXi face_to_loop;
	Eigen::MatrixXi loop_to_edge;
	Eigen::MatrixXi edge_to_vertex;
	Eigen::MatrixXi face_to_face; // mx3 (third is edge ref)
};

struct PartTopology {
	void init(BREPTopology& topology);
	PartTopologyNodes nodes;
	PartTopologyRelations relations;
};

struct PartSamples {
	void init(BREPTopology& topology, PartOptions options);
	std::vector< std::vector< Eigen::MatrixXd> > face_samples;
	std::vector< std::vector< Eigen::VectorXd> > edge_samples;
};

struct PartRandomSamples {
	void init(BREPTopology& topology, PartOptions options);
	std::vector<Eigen::MatrixXd> samples;
	std::vector<Eigen::MatrixXd> coords;
	std::vector<Eigen::MatrixXd> uv_box;
};

struct PartMaskSDF {
	void init(BREPTopology& topology, PartOptions options);
	std::vector<Eigen::MatrixXd> coords;
	std::vector<Eigen::VectorXd> sdf;
	std::vector<Eigen::MatrixXd> uv_box;
};

struct PartSummary {
	void init(BREPTopology& topology, MassProperties& mass_props, Eigen::MatrixXd& bounding_box);
	Eigen::MatrixXd bounding_box;
	double volume;
	double mass;
	Eigen::Vector3d center_of_gravity;
	Eigen::MatrixXd moment_of_inertia;
	double surface_area;
	Eigen::VectorXd topo_type_counts; // faces, edges, vertices, loops
	Eigen::VectorXd surface_type_counts;
	Eigen::VectorXd curve_type_counts;
	Eigen::VectorXd loop_type_counts;
	Eigen::VectorXd fingerprint;
	
};

struct PartUniqueInferences {
	void init(const PartTopology& topo, double tolerance = 1e-8);
	Eigen::MatrixXd origins;
	Eigen::MatrixXd axes;
	Eigen::MatrixXd frames;
	std::vector<std::vector<InferenceReference> > origin_references;
	std::vector<std::vector<InferenceReference> > axes_references;
	std::vector<std::vector<InferenceReference> > frame_references;
};

struct MCFReference {
	InferenceReference origin_ref;
	InferenceReference axis_ref;
};

struct MCF {
	MCF(const PartInference& origin_inf, 
		const PartInference& axis_inf, 
		bool onshape_style = true // flip axes where onshape would
	);
	Eigen::Vector3d origin;
	Eigen::Vector3d axis;
	MCFReference ref;
};

struct Part {
	Part(const std::string& path, PartOptions options = PartOptions());
	void init_default_mcfs(
		bool onshape_style, // only use onshape specific MCFs
		bool just_face_axes // limit to MCFs with face-based axes
	);
	Mesh mesh;
	MeshTopology mesh_topology;
	PartTopology brep;
	PartSamples samples;
	PartRandomSamples random_samples;
	PartMaskSDF mask_sdf;
	PartSummary summary;
	PartUniqueInferences inferences;
	std::vector<MCF> default_mcfs;
	bool _is_valid;
};

}

#endif // !PART_H_INCLUDED
