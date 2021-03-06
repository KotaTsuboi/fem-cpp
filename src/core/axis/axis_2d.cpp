#include "axis_2d.hpp"
#include <iostream>

using std::ostream;
using std::underlying_type;

Axis2D begin(Axis2D) {
    return Axis2D::X;
}
Axis2D end(Axis2D) {
    return Axis2D::END;
}
Axis2D operator*(Axis2D axis) {
    return axis;
}
Axis2D operator++(Axis2D &axis) {
    return axis = Axis2D(underlying_type<Axis2D>::type(axis) + 1);
}

ostream &operator<<(ostream &os, Axis2D axis) {
    switch (axis) {
    case Axis2D::X:
        return os << "X";
    case Axis2D::Y:
        return os << "Y";
    default:
        return os;
    }
}
