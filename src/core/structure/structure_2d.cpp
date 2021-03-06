#include "structure_2d.hpp"
#include "../constraint/constraint_2d.hpp"
#include "../constraint/constraint_collection_2d.hpp"
#include "../load/load_collection_2d.hpp"
#include "../mesh/mesh_2d.hpp"
#include "../solver/force_vector_2d.hpp"
#include "../solver/global_stiffness_matrix_2d.hpp"
#include "../solver/index_holder.hpp"
#include "../util/vector_util.hpp"
#include "Eigen/Core"
#include "Eigen/IterativeLinearSolvers"
#include "Eigen/Sparse"

#include <cmath>
#include <memory>

using std::cout;
using std::endl;

Structure2D::Structure2D(std::vector<std::shared_ptr<Node>> nodes, std::shared_ptr<Mesh> mesh)
    : nodes(nodes),
      mesh(mesh),
      loads(),
      constraints() {
    fem::Iterator<std::shared_ptr<FiniteElement2D>> element_iterator = ElementIterator();

    while (element_iterator.HasNext()) {
        std::shared_ptr<FiniteElement2D> element = element_iterator.Next();

        fem::Iterator<std::shared_ptr<Node>> node_iterator = element->Iterator();

        while (node_iterator.HasNext()) {
            std::shared_ptr<Node> node = node_iterator.Next();
            link[node].push_back(element);
        }
    }
}

void Structure2D::SetLoads(LoadCollection2D loads) {
    this->loads = loads;
}

void Structure2D::SetConstraint(ConstraintCollection2D constraints) {
    this->constraints = constraints;
}

void Structure2D::Analize() {
    IndexHolder index_holder(nodes);

    /*
        for (auto node : nodes) {
            for (auto axis : Axis2D()) {
                cout << "Node, Axis, Index: " << node->Index() << ", " << axis << ", " << index_holder.IndexOf(node, axis) << endl;
            }
        }
        */

    GlobalStiffnessMatrix2D global_k_matrix = mesh->GlobalKMatrix(nodes.size(), index_holder);
    ForceVector2D f = loads.ForceVector(nodes.size(), index_holder);

    /*
        cout << "Global K matrix:" << endl;
        cout << global_k_matrix.getEigen() << endl;

        cout << "Force vector:" << endl;
        cout << f.getEigen() << endl;
        */

    std::vector<int> constraint_indexes = constraints.ConstraintIndexes(index_holder);

    Eigen::SparseMatrix<double> k_sub = global_k_matrix.Contract(constraint_indexes);
    Eigen::VectorXd f_sub = f.Contract(constraint_indexes);
    IndexHolder contracted_index_holder = index_holder.Contract(constraint_indexes);

    /*
        cout << "Contracted global K matrix:" << endl;
        cout << k_sub << endl;

        cout << "Contracted force vector:" << endl;
        cout << f_sub << endl;

        for (auto node : nodes) {
            for (auto axis : Axis2D()) {
                cout << "Node, Axis, Index: " << node->Index() << ", " << axis << ", " << contracted_index_holder.IndexOf(node, axis) << endl;
            }
        }
        */

    Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Upper> solver;
    solver.compute(k_sub);

    cout << "Solving linear equation" << endl;
    Eigen::VectorXd d = solver.solve(f_sub);
    cout << "Done" << endl;

    /*
        cout << "Displacement:" << endl;
        cout << d << endl;
        */

    displacements = constraints.Displacement();

    for (auto node : nodes) {
        for (auto axis : Axis2D()) {
            if (displacements.HasValue(node, axis)) {
                continue;
            }

            displacements.SetValue(node, axis, d[contracted_index_holder.IndexOf(node, axis)]);
        }
    }

    fem::Iterator<std::shared_ptr<FiniteElement2D>> element_iterator = ElementIterator();

    while (element_iterator.HasNext()) {
        std::shared_ptr<FiniteElement2D> element = element_iterator.Next();

        Eigen::VectorXd d_e(element->NumNode() * NumDimension);

        for (int i = 0; i < element->NumNode(); i++) {
            for (auto axis : Axis2D()) {
                int j = std::underlying_type<Axis2D>::type(axis);
                double disp = displacements.ValueOf(element->GetNode(i), axis);
                d_e(NumDimension * i + j) = disp;
            }
        }

        Eigen::VectorXd sigma = element->DMatrix() * element->BMatrix() * d_e;
        double s_x = sigma[0];
        double s_y = sigma[1];
        double t_xy = sigma[2];

        double mises_stress = sqrt(pow(s_x, 2) + pow(s_y, 2) - s_x * s_y + 3 * pow(t_xy, 2));

        stresses.SetValue(element, mises_stress);
    }
}

ElementData Structure2D::GetStresses() {
    return stresses;
}

NodeData Structure2D::GetDisplacements() {
    return displacements;
}

std::vector<std::shared_ptr<Node>> Structure2D::GetNodes() {
    return nodes;
}

std::shared_ptr<Node> Structure2D::GetNodeAt(Point &point) {
    const double Epsilon = 1e-6;

    double x0 = point.X();
    double y0 = point.Y();

    std::shared_ptr<Node> target_node = nullptr;

    for (auto node : nodes) {
        double x = node->X();
        double y = node->Y();

        if (abs(x - x0) < Epsilon && abs(y - y0) < Epsilon) {
            target_node = node;
            break;
        }
    }

    return target_node;
}

unsigned int Structure2D::NumNodes() {
    return nodes.size();
}

unsigned int Structure2D::NumElements() {
    return mesh->NumElements();
}

fem::Iterator<std::shared_ptr<Node>> Structure2D::NodeIterator() {
    fem::Iterator<std::shared_ptr<Node>> iterator(nodes);
    return iterator;
}

fem::Iterator<std::shared_ptr<FiniteElement2D>> Structure2D::ElementIterator() {
    return mesh->Iterator();
}

void Structure2D::RemoveElement(std::shared_ptr<FiniteElement2D> element) {
    fem::Iterator<std::shared_ptr<Node>> node_iterator = element->Iterator();

    while (node_iterator.HasNext()) {
        std::shared_ptr<Node> node = node_iterator.Next();
        link[node].erase(link[node].begin() + indexof<std::shared_ptr<FiniteElement2D>>(link[node], element));

        if (link[node].size() == 0) {
            link.erase(node);
            nodes.erase(nodes.begin() + indexof<std::shared_ptr<Node>>(nodes, node));
            displacements.Erase(node);
        }
    }

    stresses.Erase(element);
    mesh->Erase(element);
}

const int Structure2D::NumDimension = 2;
