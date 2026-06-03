// SPDX-FileCopyrightText: © 2026 Dustin Hartlyn <https://github.com/dustinhartlyn>
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef PLANEGCS_HP_SPARSEJACOBIAN_H
#define PLANEGCS_HP_SPARSEJACOBIAN_H

#include <vector>
#include <string>
#include <tuple>
#include <utility>
#include <memory>
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseCholesky>

namespace PlaneGCS
{

struct SystemNode
{
    int i;
    int j;
    double x;
    double y;
};

struct ConstraintEdge
{
    int edge_id;
    std::pair<int, int> node_A;
    std::pair<int, int> node_B;
    double target_distance;
};

class HighPerformanceSparseJacobian
{
public:
    explicit HighPerformanceSparseJacobian(std::pair<int, int> grid_size = {5, 5});
    ~HighPerformanceSparseJacobian() = default;

    void initializeHexagonalGrid(double spacing = 1.0);
    void loadEdgesFromJson(const std::string& filepath);

    std::vector<double> getConfigurationVector() const;
    void setConfigurationVector(const std::vector<double>& config);

    std::vector<double> evaluateConstraints(const std::vector<double>& config);
    std::vector<double> evaluateConstraints();

    void computeJacobian();

    void applyTransformation(
        double u_x,
        double u_y,
        double t_x,
        double t_y,
        const std::vector<std::pair<int, int>>& node_indices = {}
    );

    void applyTransformationChain(
        const std::vector<std::tuple<double, double, double, double>>& transformations,
        const std::vector<std::pair<int, int>>& node_indices = {}
    );

    struct SolveResult
    {
        bool converged;
        int iterations;
        double final_residual;
    };

    SolveResult solveNewtonRaphson(
        int max_iterations = 50,
        double tolerance = 1e-10,
        double damping = 1.0
    );

    // Getters for performance testing and internal verification
    int getRows() const
    {
        return m_rows;
    }
    int getCols() const
    {
        return m_cols;
    }
    int getNumNodes() const
    {
        return m_num_nodes;
    }
    int getNumVariables() const
    {
        return m_num_variables;
    }
    int getNumConstraints() const
    {
        return m_num_constraints;
    }

    const std::vector<double>& getJacobianData() const
    {
        return m_csr_data;
    }
    const std::vector<int>& getJacobianIndices() const
    {
        return m_csr_indices;
    }
    const std::vector<int>& getJacobianIndPtr() const
    {
        return m_csr_indptr;
    }

private:
    void prepareVectorizedEvaluation();
    void evaluateConstraintsDirect(std::vector<double>& out_res);

    int m_rows;
    int m_cols;
    int m_num_nodes;
    int m_num_variables;
    int m_num_constraints;

    std::vector<SystemNode> m_nodes;
    std::vector<ConstraintEdge> m_edges;

    // Vectorized index caching
    std::vector<int> m_node_A_indices;
    std::vector<int> m_node_B_indices;
    std::vector<double> m_target_distances_sq;
    bool m_vectorized_ready = false;

    // CSR representation for Jacobian
    std::vector<double> m_csr_data;
    std::vector<int> m_csr_indices;
    std::vector<int> m_csr_indptr;
    bool m_cache_valid = false;

    // Pre-allocated buffers for solving inside the hot loops (eliminates allocations)
    std::vector<double> m_residual_buffer;
    Eigen::VectorXd m_F_eigen;
    Eigen::VectorXd m_delta_x_eigen;
};

}  // namespace PlaneGCS

#endif  // PLANEGCS_HP_SPARSEJACOBIAN_H
