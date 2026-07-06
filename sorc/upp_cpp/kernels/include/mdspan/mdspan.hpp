#ifndef HELM_MOCK_KOKKOS_MDSPAN_HPP
#define HELM_MOCK_KOKKOS_MDSPAN_HPP

#include <experimental/mdspan>

namespace Kokkos {
    using std::experimental::extents;
    using std::experimental::dextents;
    using std::experimental::layout_left;
    using std::experimental::layout_right;
    using std::experimental::mdspan;
    using std::experimental::dynamic_extent;
}

namespace std {
    using std::experimental::extents;
    using std::experimental::dextents;
    using std::experimental::layout_left;
    using std::experimental::layout_right;
    using std::experimental::mdspan;
}

#endif
