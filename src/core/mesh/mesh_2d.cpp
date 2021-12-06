#include "mesh_2d.hpp"
#include "../element/finite_element_2d.hpp"
#include "../material/material.hpp"
#include "../problem_type/problem_type.hpp"
#include "../solver/global_stiffness_matrix_2d.hpp"
#include "Eigen/Sparse"
#include <memory>
#include <vector>
#include <cmath>

using std::underlying_type;

Mesh2D::Mesh2D(std::vector<std::shared_ptr<Node>> nodes, std::vector<std::shared_ptr<FiniteElement2D>> elements)
    : nodes(nodes),
      elements(elements) {
}

GlobalStiffnessMatrix2D Mesh2D::GlobalKMatrix(Material material, ProblemType problem_type) {
    std::vector<int> node_indexes;
    for (std::shared_ptr<Node> node : nodes) {
        node_indexes.push_back(node->Index());
    }

    GlobalStiffnessMatrix2D global_k_matrix(nodes);

    for (std::shared_ptr<FiniteElement2D> element : elements) {
        Eigen::MatrixXd element_k_matrix = element->KMatrix(material, problem_type);

        for (int i = 0; i < element->NumNode(); i++) {
            for (int j = 0; j < element->NumNode(); j++) {
                std::shared_ptr<Node> node_i = element->GetNode(i);
                std::shared_ptr<Node> node_j = element->GetNode(j);

                for (auto axis_i : Axis2D()) {
                    for (auto axis_j : Axis2D()) {
                        int d_i = underlying_type<Axis2D>::type(axis_i);
                        int d_j = underlying_type<Axis2D>::type(axis_j);

                        double value = element_k_matrix(NumDimension * i + d_i, NumDimension * j + d_j);

                        global_k_matrix.add(node_i, axis_i, node_j, axis_j, value);
                    }
                }
            }
        }
    }

    global_k_matrix.makeMatrix();

    return global_k_matrix;
}

std::vector<std::shared_ptr<Node>> Mesh2D::GetNodes() {
    return nodes;
}

std::shared_ptr<Node> Mesh2D::GetNodeClosestTo(Point &point) {
    double x0 = point.X();
    double y0 = point.Y();

    std::shared_ptr<Node> closest_node = nodes[0];

    double x = nodes[0]->X();
    double y = nodes[0]->Y();

    double min = pow(x - x0, 2) + pow(y - y0, 2);

    for (auto node : nodes) {
        x = node->X();
        y = node->Y();

        double d = pow(x - x0, 2) + pow(y - y0, 2);

        if (d > min) {
            continue;
        }

        min = d;

        closest_node = node;
    }

    return closest_node;
}

int Mesh2D::NumNodes() {
    return nodes.size();
}

Mesh2D::~Mesh2D() {
}

const int Mesh2D::NumDimension = 2;