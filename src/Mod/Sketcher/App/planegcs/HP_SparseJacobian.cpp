// SPDX-FileCopyrightText: © 2026 Dustin Hartlyn <https://github.com/dustinhartlyn>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "HP_SparseJacobian.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace PlaneGCS
{

HighPerformanceSparseJacobian::HighPerformanceSparseJacobian(std::pair<int, int> grid_size)
    : m_rows(grid_size.first)
    , m_cols(grid_size.second)
    , m_num_nodes(grid_size.first * grid_size.second)
    , m_num_variables(2 * grid_size.first * grid_size.second)
    , m_num_constraints(0)
{}

void HighPerformanceSparseJacobian::initializeHexagonalGrid(double spacing)
{
    m_nodes.clear();
    m_nodes.resize(m_num_nodes);

    double sqrt3_half = std::sqrt(3.0) / 2.0;
    double half_spacing = 0.5 * spacing;

    for (int i = 0; i < m_rows; ++i) {
        double x_offset = (i % 2 == 1) ? half_spacing : 0.0;
        double y = i * sqrt3_half * spacing;

        for (int j = 0; j < m_cols; ++j) {
            double x = j * spacing + x_offset;
            int idx = i * m_cols + j;
            m_nodes[idx] = SystemNode {i, j, x, y};
        }
    }

    m_cache_valid = false;
    m_vectorized_ready = false;
}

void HighPerformanceSparseJacobian::loadEdgesFromJson(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open JSON edge file: " + filepath);
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    m_edges.clear();

    size_t pos = 0;
    while (true) {
        size_t obj_start = content.find('{', pos);
        if (obj_start == std::string::npos) {
            break;
        }
        size_t obj_end = content.find('}', obj_start);
        if (obj_end == std::string::npos) {
            break;
        }

        std::string obj_str = content.substr(obj_start, obj_end - obj_start + 1);
        pos = obj_end + 1;

        // Extract edge_id
        size_t id_pos = obj_str.find("\"edge_id\"");
        if (id_pos == std::string::npos) {
            continue;
        }
        size_t colon_pos = obj_str.find(':', id_pos);
        if (colon_pos == std::string::npos) {
            continue;
        }

        size_t num_start = colon_pos + 1;
        while (num_start < obj_str.size()
               && (obj_str[num_start] == ' ' || obj_str[num_start] == '\t'
                   || obj_str[num_start] == '\r' || obj_str[num_start] == '\n')) {
            num_start++;
        }
        size_t num_end = num_start;
        while (num_end < obj_str.size() && std::isdigit(obj_str[num_end])) {
            num_end++;
        }
        int edge_id = std::stoi(obj_str.substr(num_start, num_end - num_start));

        // Extract node_A string
        size_t a_pos = obj_str.find("\"node_A\"");
        if (a_pos == std::string::npos) {
            continue;
        }
        size_t a_colon = obj_str.find(':', a_pos);
        size_t a_start = obj_str.find('\"', a_colon);
        size_t a_end = obj_str.find('\"', a_start + 1);
        std::string node_A_str = obj_str.substr(a_start + 1, a_end - a_start - 1);

        // Extract node_B string
        size_t b_pos = obj_str.find("\"node_B\"");
        if (b_pos == std::string::npos) {
            continue;
        }
        size_t b_colon = obj_str.find(':', b_pos);
        size_t b_start = obj_str.find('\"', b_colon);
        size_t b_end = obj_str.find('\"', b_start + 1);
        std::string node_B_str = obj_str.substr(b_start + 1, b_end - b_start - 1);

        // Parse "row_col" for A
        size_t underscore_A = node_A_str.find('_');
        int row_A = std::stoi(node_A_str.substr(0, underscore_A));
        int col_A = std::stoi(node_A_str.substr(underscore_A + 1));

        // Parse "row_col" for B
        size_t underscore_B = node_B_str.find('_');
        int row_B = std::stoi(node_B_str.substr(0, underscore_B));
        int col_B = std::stoi(node_B_str.substr(underscore_B + 1));

        int idx_A = row_A * m_cols + col_A;
        int idx_B = row_B * m_cols + col_B;

        if (idx_A > idx_B) {
            std::swap(idx_A, idx_B);
            std::swap(row_A, row_B);
            std::swap(col_A, col_B);
        }

        double dx = m_nodes[idx_A].x - m_nodes[idx_B].x;
        double dy = m_nodes[idx_A].y - m_nodes[idx_B].y;
        double distance = std::sqrt(dx * dx + dy * dy);

        ConstraintEdge edge;
        edge.edge_id = edge_id;
        edge.node_A = {row_A, col_A};
        edge.node_B = {row_B, col_B};
        edge.target_distance = distance;
        m_edges.push_back(edge);
    }

    m_num_constraints = static_cast<int>(m_edges.size());
    prepareVectorizedEvaluation();
    m_cache_valid = false;
}

void HighPerformanceSparseJacobian::prepareVectorizedEvaluation()
{
    m_node_A_indices.resize(m_num_constraints);
    m_node_B_indices.resize(m_num_constraints);
    m_target_distances_sq.resize(m_num_constraints);

    for (int k = 0; k < m_num_constraints; ++k) {
        const auto& edge = m_edges[k];
        int idx_A = edge.node_A.first * m_cols + edge.node_A.second;
        int idx_B = edge.node_B.first * m_cols + edge.node_B.second;

        m_node_A_indices[k] = idx_A;
        m_node_B_indices[k] = idx_B;
        m_target_distances_sq[k] = edge.target_distance * edge.target_distance;
    }

    // Initialize CSR layout
    m_csr_indptr.resize(m_num_constraints + 1);
    m_csr_indices.resize(4 * m_num_constraints);
    m_csr_data.resize(4 * m_num_constraints, 0.0);

    for (int k = 0; k <= m_num_constraints; ++k) {
        m_csr_indptr[k] = 4 * k;
    }

    for (int k = 0; k < m_num_constraints; ++k) {
        int idx_A = m_node_A_indices[k];
        int idx_B = m_node_B_indices[k];

        m_csr_indices[4 * k + 0] = 2 * idx_A;
        m_csr_indices[4 * k + 1] = 2 * idx_A + 1;
        m_csr_indices[4 * k + 2] = 2 * idx_B;
        m_csr_indices[4 * k + 3] = 2 * idx_B + 1;
    }

    // Pre-allocate solve buffers to prevent dynamic allocation in hot loops
    m_residual_buffer.resize(m_num_constraints);
    m_F_eigen = Eigen::VectorXd::Zero(m_num_constraints);
    m_delta_x_eigen = Eigen::VectorXd::Zero(m_num_variables);

    m_vectorized_ready = true;
}

std::vector<double> HighPerformanceSparseJacobian::getConfigurationVector() const
{
    std::vector<double> config(m_num_variables);
    for (int i = 0; i < m_num_nodes; ++i) {
        config[2 * i] = m_nodes[i].x;
        config[2 * i + 1] = m_nodes[i].y;
    }
    return config;
}

void HighPerformanceSparseJacobian::setConfigurationVector(const std::vector<double>& config)
{
    for (int i = 0; i < m_num_nodes; ++i) {
        m_nodes[i].x = config[2 * i];
        m_nodes[i].y = config[2 * i + 1];
    }
    m_cache_valid = false;
}

std::vector<double> HighPerformanceSparseJacobian::evaluateConstraints(const std::vector<double>& config)
{
    std::vector<double> res(m_num_constraints);
    for (int k = 0; k < m_num_constraints; ++k) {
        int idx_A = m_node_A_indices[k];
        int idx_B = m_node_B_indices[k];

        double x_A = config[2 * idx_A];
        double y_A = config[2 * idx_A + 1];
        double x_B = config[2 * idx_B];
        double y_B = config[2 * idx_B + 1];

        double dx = x_A - x_B;
        double dy = y_A - y_B;
        double dist_sq = dx * dx + dy * dy;
        res[k] = dist_sq - m_target_distances_sq[k];
    }
    return res;
}

std::vector<double> HighPerformanceSparseJacobian::evaluateConstraints()
{
    std::vector<double> config = getConfigurationVector();
    return evaluateConstraints(config);
}

void HighPerformanceSparseJacobian::evaluateConstraintsDirect(std::vector<double>& out_res)
{
    for (int k = 0; k < m_num_constraints; ++k) {
        int idx_A = m_node_A_indices[k];
        int idx_B = m_node_B_indices[k];

        double x_A = m_nodes[idx_A].x;
        double y_A = m_nodes[idx_A].y;
        double x_B = m_nodes[idx_B].x;
        double y_B = m_nodes[idx_B].y;

        double dx = x_A - x_B;
        double dy = y_A - y_B;
        double dist_sq = dx * dx + dy * dy;
        out_res[k] = dist_sq - m_target_distances_sq[k];
    }
}

void HighPerformanceSparseJacobian::computeJacobian()
{
    if (m_cache_valid) {
        return;
    }

    for (int k = 0; k < m_num_constraints; ++k) {
        int idx_A = m_node_A_indices[k];
        int idx_B = m_node_B_indices[k];

        double x_A = m_nodes[idx_A].x;
        double y_A = m_nodes[idx_A].y;
        double x_B = m_nodes[idx_B].x;
        double y_B = m_nodes[idx_B].y;

        // CSE: calculate coordinate deltas exactly once per row
        double dx = x_A - x_B;
        double dy = y_A - y_B;

        double two_dx = 2.0 * dx;
        double two_dy = 2.0 * dy;

        // Map to flat CSR data stream
        m_csr_data[4 * k + 0] = two_dx;
        m_csr_data[4 * k + 1] = two_dy;
        m_csr_data[4 * k + 2] = -two_dx;
        m_csr_data[4 * k + 3] = -two_dy;
    }

    m_cache_valid = true;
}

void HighPerformanceSparseJacobian::applyTransformation(
    double u_x,
    double u_y,
    double t_x,
    double t_y,
    const std::vector<std::pair<int, int>>& node_indices
)
{
    double norm = std::sqrt(u_x * u_x + u_y * u_y);
    if (norm < 1e-12) {
        norm = 1.0;
    }
    double ux = u_x / norm;
    double uy = u_y / norm;

    if (node_indices.empty()) {
        for (auto& node : m_nodes) {
            double x = node.x;
            double y = node.y;
            node.x = ux * x - uy * y + t_x;
            node.y = uy * x + ux * y + t_y;
        }
    }
    else {
        for (const auto& key : node_indices) {
            int idx = key.first * m_cols + key.second;
            if (idx >= 0 && idx < m_num_nodes) {
                double x = m_nodes[idx].x;
                double y = m_nodes[idx].y;
                m_nodes[idx].x = ux * x - uy * y + t_x;
                m_nodes[idx].y = uy * x + ux * y + t_y;
            }
        }
    }
    m_cache_valid = false;
}

void HighPerformanceSparseJacobian::applyTransformationChain(
    const std::vector<std::tuple<double, double, double, double>>& transformations,
    const std::vector<std::pair<int, int>>& node_indices
)
{
    struct Affine
    {
        double ux, uy, tx, ty;
    };
    std::vector<Affine> chain;
    chain.reserve(transformations.size());
    for (const auto& t : transformations) {
        double u_x = std::get<0>(t);
        double u_y = std::get<1>(t);
        double t_x = std::get<2>(t);
        double t_y = std::get<3>(t);
        double norm = std::sqrt(u_x * u_x + u_y * u_y);
        if (norm < 1e-12) {
            norm = 1.0;
        }
        chain.push_back({u_x / norm, u_y / norm, t_x, t_y});
    }

    if (node_indices.empty()) {
        for (auto& node : m_nodes) {
            double x = node.x;
            double y = node.y;
            for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                double nx = it->ux * x - it->uy * y + it->tx;
                double ny = it->uy * x + it->ux * y + it->ty;
                x = nx;
                y = ny;
            }
            node.x = x;
            node.y = y;
        }
    }
    else {
        for (const auto& key : node_indices) {
            int idx = key.first * m_cols + key.second;
            if (idx >= 0 && idx < m_num_nodes) {
                double x = m_nodes[idx].x;
                double y = m_nodes[idx].y;
                for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
                    double nx = it->ux * x - it->uy * y + it->tx;
                    double ny = it->uy * x + it->ux * y + it->ty;
                    x = nx;
                    y = ny;
                }
                m_nodes[idx].x = x;
                m_nodes[idx].y = y;
            }
        }
    }
    m_cache_valid = false;
}

HighPerformanceSparseJacobian::SolveResult HighPerformanceSparseJacobian::solveNewtonRaphson(
    int max_iterations,
    double tolerance,
    double damping
)
{
    if (!m_vectorized_ready) {
        throw std::runtime_error("Vectorized evaluation not ready. Call loadEdgesFromJson first.");
    }

    int iteration = 0;
    double residual_norm = 0.0;
    bool converged = false;

    Eigen::SparseMatrix<double> A;
    Eigen::VectorXd rhs(m_num_variables);
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver;
    bool solver_initialized = false;

    for (iteration = 0; iteration < max_iterations; ++iteration) {
        evaluateConstraintsDirect(m_residual_buffer);

        double sum_sq = 0.0;
        for (double val : m_residual_buffer) {
            sum_sq += val * val;
        }
        residual_norm = std::sqrt(sum_sq);

        if (residual_norm < tolerance) {
            converged = true;
            break;
        }

        computeJacobian();

        // 1. Construct a zero-copy sparse view of m_csr_data, m_csr_indices, and m_csr_indptr
        Eigen::Map<Eigen::SparseMatrix<double, Eigen::RowMajor>> J_sparse(
            m_num_constraints,
            m_num_variables,
            static_cast<int>(m_csr_data.size()),
            m_csr_indptr.data(),
            m_csr_indices.data(),
            m_csr_data.data()
        );

        // 2. Compute the sparse normal equations system matrix
        A = J_sparse.transpose() * J_sparse;

        // 3. Apply regularizing damping parameter of 1e-6 along the matrix diagonal
        for (int i = 0; i < A.rows(); ++i) {
            A.coeffRef(i, i) += 1e-6;
        }

        // Map residual buffer to Eigen Vector for zero-copy operation
        Eigen::Map<const Eigen::VectorXd> F_eigen_map(m_residual_buffer.data(), m_num_constraints);

        // Compute right hand side of normal equations: J^T * (-F)
        rhs.noalias() = J_sparse.transpose() * (-F_eigen_map);

        // 4. Compute the final factorization and resolve the step vector using SimplicialLDLT
        if (!solver_initialized) {
            solver.analyzePattern(A);
            solver_initialized = true;
        }

        solver.factorize(A);
        if (solver.info() != Eigen::Success) {
            break;
        }

        m_delta_x_eigen = solver.solve(rhs);
        if (solver.info() != Eigen::Success) {
            break;
        }

        for (int i = 0; i < m_num_nodes; ++i) {
            m_nodes[i].x += damping * m_delta_x_eigen(2 * i);
            m_nodes[i].y += damping * m_delta_x_eigen(2 * i + 1);
        }

        m_cache_valid = false;
    }

    return SolveResult {converged, converged ? (iteration + 1) : max_iterations, residual_norm};
}

}  // namespace PlaneGCS
