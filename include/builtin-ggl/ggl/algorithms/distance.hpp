// Generic Geometry Library
//
// Copyright Barend Gehrels 1995-2009, Geodan Holding B.V. Amsterdam, the Netherlands.
// Copyright Bruno Lalande 2008, 2009
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GGL_ALGORITHMS_DISTANCE_HPP
#define GGL_ALGORITHMS_DISTANCE_HPP

#include <boost/mpl/if.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>
#include <boost/static_assert.hpp>

#include <ggl/core/cs.hpp>
#include <ggl/core/is_multi.hpp>
#include <ggl/core/reverse_dispatch.hpp>
#include <ggl/geometries/segment.hpp>
#include <ggl/strategies/distance_result.hpp>
#include <ggl/strategies/strategies.hpp>

/*!
\defgroup distance distance calculation
The distance algorithm returns the distance between two geometries.
\par Coordinate systems and strategies:
With help of strategies the distance function returns the appropriate distance.
If the input is in cartesian coordinates, the Euclidian distance (Pythagoras) is calculated.
If the input is in spherical coordinates (either degree or radian), the distance over the sphere is returned.
If the input is in geographic coordinates, distance is calculated over the globe and returned in meters.

\par Distance result:
Depending on calculation type the distance result is either a structure, convertable
to a double, or a double value. In case of Pythagoras it makes sense to not draw the square root in the
strategy itself. Taking a square root is relative expensive and is not necessary when comparing distances.

\par Geometries:
Currently implemented, for both cartesian and spherical/geographic:
- POINT - POINT
- POINT - SEGMENT and v.v.
- POINT - LINESTRING and v.v.

Not yet implemented:
- POINT - RING etc, note that it will return a zero if the point is anywhere within the ring

\par Example:
Example showing distance calculation of two points, in xy and in latlong coordinates
\dontinclude doxygen_examples.cpp
\skip example_distance_point_point
\line {
\until }
*/

namespace ggl
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance {

template <typename P1, typename P2, typename Strategy>
struct point_to_point
{
    static inline typename Strategy::return_type apply(P1 const& p1,
                P2 const& p2, Strategy const& strategy)
    {
        return strategy(p1, p2);
    }
};


template<typename Point, typename Segment, typename Strategy>
struct point_to_segment
{
    static inline typename Strategy::return_type apply(Point const& point,
                Segment const& segment, Strategy const& strategy)
    {
        typename strategy_distance_segment
            <
            typename cs_tag<Point>::type,
            typename cs_tag<Segment>::type,
            Point,
            Segment
            >::type segment_strategy;

        return segment_strategy(point, segment);
    }
};


template<typename P, typename L, typename PPStrategy, typename PSStrategy>
struct point_to_linestring
{
    typedef typename PPStrategy::return_type return_type;

    static inline return_type apply(P const& point, L const& linestring,
            PPStrategy const& pp_strategy, PSStrategy const& ps_strategy)
    {
        typedef segment<const typename point_type<L>::type> segment_type;

        if (boost::begin(linestring) == boost::end(linestring))
        {
            return return_type(0);
        }

        // line of one point: return point square_distance
        typedef typename boost::range_const_iterator<L>::type iterator_type;
        iterator_type it = boost::begin(linestring);
        iterator_type prev = it++;
        if (it == boost::end(linestring))
        {
            return pp_strategy(point, *boost::begin(linestring));
        }


        // start with first segment distance
        return_type d = ps_strategy(point, segment_type(*prev, *it));

        // check if other segments are closer
        prev = it++;
        while(it != boost::end(linestring))
        {
            return_type ds = ps_strategy(point, segment_type(*prev, *it));
            if (ggl::close_to_zero(ds))
            {
                return return_type(0);
            }
            else if (ds < d)
            {
                d = ds;
            }
            prev = it++;
        }

        return d;
    }
};



}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag1, typename GeometryTag2,
    typename G1, typename G2,
    typename StrategyTag, typename Strategy,
    bool IsMulti1, bool IsMulti2
>
struct distance
{};


template <typename P1, typename P2, typename Strategy>
struct distance
<
    point_tag, point_tag,
    P1, P2,
    strategy_tag_distance_point_point, Strategy,
    false, false
> : detail::distance::point_to_point<P1, P2, Strategy>
{};

/// Point-line version 1, where point-point strategy is specified
template <typename Point, typename Linestring, typename Strategy>
struct distance
<
    point_tag, linestring_tag,
    Point, Linestring,
    strategy_tag_distance_point_point, Strategy,
    false, false
>
{

    static inline typename Strategy::return_type apply(Point const& point,
            Linestring const& linestring,
            Strategy const& strategy)
    {
        typedef segment<const typename point_type<Linestring>::type> segment_type;
        typedef typename ggl::strategy_distance_segment
                    <
                            typename cs_tag<Point>::type,
                            typename cs_tag<segment_type>::type,
                            Point,
                            segment_type
                    >::type ps_strategy_type;

        return detail::distance::point_to_linestring
            <
                Point, Linestring, Strategy, ps_strategy_type
            >::apply(point, linestring, strategy, ps_strategy_type());
    }
};


/// Point-line version 2, where point-segment strategy is specified
template <typename Point, typename Linestring, typename Strategy>
struct distance
<
    point_tag, linestring_tag,
    Point, Linestring,
    strategy_tag_distance_point_segment, Strategy,
    false, false
>
{
    static inline typename Strategy::return_type apply(Point const& point,
            Linestring const& linestring,
            Strategy const& strategy)
    {
        typedef typename Strategy::point_strategy_type pp_strategy_type;
        return detail::distance::point_to_linestring
            <
                Point, Linestring, pp_strategy_type, Strategy
            >::apply(point, linestring, pp_strategy_type(), strategy);
    }
};


template <typename Point, typename Segment, typename Strategy>
struct distance
<
    point_tag, segment_tag,
    Point, Segment,
    strategy_tag_distance_point_point, Strategy,
    false, false
> : detail::distance::point_to_segment<Point, Segment, Strategy>
{};


// Strictly spoken this might be in namespace <impl> again
template
<
    typename GeometryTag1, typename GeometryTag2,
    typename G1, typename G2,
    typename StrategyTag, typename Strategy,
    bool IsMulti1, bool IsMulti2
>
struct distance_reversed
{
    static inline typename Strategy::return_type apply(G1 const& g1,
                G2 const& g2, Strategy const& strategy)
    {
        return distance
            <
                GeometryTag2, GeometryTag1,
                G2, G1,
                StrategyTag, Strategy,
                IsMulti2, IsMulti1
            >::apply(g2, g1, strategy);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

/*!
    \brief Calculate distance between two geometries with a specified strategy
    \ingroup distance
    \tparam Geometry1 first geometry type
    \tparam Geometry2 second geometry type
    \tparam S point-point-distance strategy type
    \param geometry1 first geometry
    \param geometry2 second geometry
    \param strategy strategy to calculate distance between two points
    \return the distance (either a double or a distance_result, (convertable to double))
    \note The strategy can be a point-point strategy. In case of distance point-line/point-polygon
        it may also be a point-segment strategy.
    \par Example:
    Example showing distance calculation of two lat long points, using the accurate Vincenty approximation
    \dontinclude doxygen_examples.cpp
    \skip example_distance_point_point_strategy
    \line {
    \until }
 */
template <typename Geometry1, typename Geometry2, typename Strategy>
inline typename Strategy::return_type distance(Geometry1 const& geometry1,
            Geometry2 const& geometry2, Strategy const& strategy)
{
    typedef typename boost::remove_const<Geometry1>::type ncg1_type;
    typedef typename boost::remove_const<Geometry2>::type ncg2_type;

    return boost::mpl::if_c
        <
            ggl::reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::distance_reversed
                <
                    typename tag<ncg1_type>::type,
                    typename tag<ncg2_type>::type,
                    ncg1_type,
                    ncg2_type,
                    typename strategy_tag<Strategy>::type,
                    Strategy,
                    is_multi<ncg1_type>::value,
                    is_multi<ncg2_type>::value
                >,
                dispatch::distance
                <
                    typename tag<ncg1_type>::type,
                    typename tag<ncg2_type>::type,
                    ncg1_type,
                    ncg2_type,
                    typename strategy_tag<Strategy>::type,
                    Strategy,
                    is_multi<ncg1_type>::value,
                    is_multi<ncg2_type>::value
                >
        >::type::apply(geometry1, geometry2, strategy);
}


/*!
    \brief Calculate distance between two geometries
    \ingroup distance
    \details The default strategy is used, belonging to the corresponding coordinate system of the geometries
    \tparam G1 first geometry type
    \tparam G2 second geometry type
    \param geometry1 first geometry
    \param geometry2 second geometry
    \return the distance (either a double or a distance result, convertable to double)
 */
template <typename Geometry1, typename Geometry2>
inline typename distance_result<Geometry1, Geometry2>::type distance(
                Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    typedef typename point_type<Geometry1>::type point1_type;
    typedef typename point_type<Geometry2>::type point2_type;

    // Define a point-point-distance-strategy
    // for either the normal case, either the reversed case
    typedef typename boost::mpl::if_c
        <
            ggl::reverse_dispatch<Geometry1, Geometry2>::type::value,
            typename strategy_distance
                <
                    typename cs_tag<point2_type>::type,
                    typename cs_tag<point1_type>::type,
                    point2_type,
                    point1_type
                >::type,
            typename strategy_distance
                <
                    typename cs_tag<point1_type>::type,
                    typename cs_tag<point2_type>::type,
                    point1_type,
                    point2_type
                >::type
        >::type strategy;

    return distance(geometry1, geometry2, strategy());
}

} // namespace ggl

#endif // GGL_ALGORITHMS_DISTANCE_HPP
