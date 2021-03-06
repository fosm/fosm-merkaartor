// Generic Geometry Library
//
// Copyright Barend Gehrels 1995-2009, Geodan Holding B.V. Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GGL_MULTI_ALGORITHMS_TRANSFORM_HPP
#define GGL_MULTI_ALGORITHMS_TRANSFORM_HPP

#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>

#include <ggl/algorithms/transform.hpp>

#include <ggl/multi/core/tags.hpp>

namespace ggl
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace transform {

/*!
    \brief Is able to transform any multi-geometry, calling the single-version as policy
*/
template <typename Multi1, typename Multi2, typename Policy>
struct transform_multi
{
    template <typename S>
    static inline bool apply(Multi1 const& multi1, Multi2& multi2, S const& strategy)
    {
        multi2.resize(boost::size(multi1));

        typename boost::range_const_iterator<Multi1>::type it1 
                = boost::begin(multi1);
        typename boost::range_iterator<Multi2>::type it2 
                = boost::begin(multi2);

        for (; it1 != boost::end(multi1); ++it1, ++it2)
        {
            if (! Policy::apply(*it1, *it2, strategy))
            {
                return false;
            }
        }

        return true;
    }
};




}} // namespace detail::transform
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Multi1, typename Multi2>
struct transform<multi_polygon_tag, multi_polygon_tag, Multi1, Multi2>
    : detail::transform::transform_multi
        <
            Multi1,
            Multi2,
            detail::transform::transform_polygon
                <
                    typename boost::range_value<Multi1>::type,
                    typename boost::range_value<Multi2>::type
                >
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


} // namespace ggl

#endif // GGL_MULTI_ALGORITHMS_TRANSFORM_HPP
