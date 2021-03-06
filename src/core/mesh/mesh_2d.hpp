#pragma once

#include "../element/finite_element_2d.hpp"
#include "../node/node.hpp"
#include "../solver/global_stiffness_matrix_2d.hpp"
#include "../solver/index_holder.hpp"
#include "../util/iterator.hpp"
#include "mesh.hpp"
#include <memory>
#include <vector>

class Mesh2D : public Mesh {
  public:
    static const int NumDimension;

    Mesh2D(std::vector<std::shared_ptr<FiniteElement2D>> elements);

    GlobalStiffnessMatrix2D GlobalKMatrix(unsigned int node_size, IndexHolder index_holder);

    fem::Iterator<std::shared_ptr<FiniteElement2D>> Iterator();

    unsigned int NumElements();

    void Erase(std::shared_ptr<FiniteElement2D> element);

  private:
    std::vector<std::shared_ptr<FiniteElement2D>> elements;
};
