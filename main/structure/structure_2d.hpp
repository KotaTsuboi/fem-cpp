#include "Eigen/Core"
#include "Eigen/IterativeLinearSolvers"
#include "Eigen/Sparse"
#include "../constraint/constraint_2d.hpp"
#include "../constraint/constraint_collection_2d.hpp"
#include "../solver/force_vector_2d.hpp"
#include "../solver/global_stiffness_matrix_2d.hpp"
#include "../load/load_collection_2d.hpp"
#include "../material/material.hpp"
#include "../mesh/mesh_2d.hpp"
#include "../problem_type/problem_type.hpp"

class Structure2D {
  public:
    static const int NumDimension;

    Structure2D(Mesh2D mesh, Material material, ProblemType problem_type)
        : mesh(mesh),
          material(material),
          problem_type(problem_type),
          loads(),
          constraints() {
    }

    void SetLoads(LoadCollection2D loads) {
        this->loads = loads;
    }

    void SetConstraint(ConstraintCollection2D constraints) {
        this->constraints = constraints;
    }

    map<Node2D, map<Axis2D, double>> Analize() {
        GlobalStiffnessMatrix2D global_k_matrix = mesh.GlobalKMatrix(material, problem_type);
        ForceVector2D f = loads.ForceVector(mesh.GetNodes());

        cout << "Global K matrix:" << endl;
        global_k_matrix.print();

        cout << "Force vector:" << endl;
        cout << f.getEigen() << endl;

        Eigen::SparseMatrix<double> k_sub = constraints.ContractSparseMatrix(global_k_matrix);
        Eigen::VectorXd f_sub = constraints.ContractVector(f);

        cout << "Contracted global K matrix:" << endl;
        cout << k_sub << endl;

        cout << "Contracted force vector:" << endl;
        cout << f_sub << endl;

        Eigen::ConjugateGradient<Eigen::SparseMatrix<double>, Eigen::Upper> solver;
        solver.compute(k_sub);
        Eigen::VectorXd d = solver.solve(f_sub);

        cout << "Displacement:" << endl;
        cout << d << endl;

        map<Node2D, map<Axis2D, double>> displacement = constraints.Displacement();

        int count = 0;

        for (auto node : mesh.GetNodes()) {
            for (auto axis : Axis2D()) {
                if (displacement[node].count(axis) > 0) {
                    cout << "n" << node.Index() + 1
                    << axis
                    << " found" << endl;
                    continue;
                }
                displacement[node][axis] = d[count];
                count++;
            }
        }

        return displacement;
    }

  private:
    Mesh2D mesh;
    Material material;
    ProblemType problem_type;
    LoadCollection2D loads;
    ConstraintCollection2D constraints;
};

const int Structure2D::NumDimension = 2;