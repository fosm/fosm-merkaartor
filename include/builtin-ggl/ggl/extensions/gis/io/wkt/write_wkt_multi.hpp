// Generic Geometry Library
//
// Copyright Barend Gehrels 2008-2009, Geodan, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef GGL_EXTENSIONS_GIS_IO_WKT_WRITE_WKT_MULTI_HPP
#define GGL_EXTENSIONS_GIS_IO_WKT_WRITE_WKT_MULTI_HPP



#include <ggl/multi/core/tags.hpp>

#include <ggl/extensions/gis/io/wkt/write_wkt.hpp>
#include <ggl/extensions/gis/io/wkt/detail/wkt_multi.hpp>


namespace ggl
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace wkt {


template <typename Multi, typename StreamPolicy, typename PrefixPolicy>
struct wkt_multi
{
    template <typename Char, typename Traits>
    static inline void apply(std::basic_ostream<Char, Traits>& os,
                Multi const& geometry)
    {
        os << PrefixPolicy::apply();
        // TODO: check EMPTY here
        os << "(";

        for (typename boost::range_const_iterator<Multi>::type
                    it = boost::begin(geometry);
            it != boost::end(geometry);
            ++it)
        {
            if (it != boost::begin(geometry))
            {
                os << ",";
            }
            StreamPolicy::apply(os, *it);
        }

        os << ")";
    }
};

}} // namespace wkt::impl
#endif


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch {


template <typename Multi>
struct wkt<multi_point_tag, Multi>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_point
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipoint
        >
{};


template <typename Multi>
struct wkt<multi_linestring_tag, Multi>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_sequence
                <
                    typename boost::range_value<Multi>::type
                >,
            detail::wkt::prefix_multilinestring
        >
{};


template <typename Multi>
struct wkt<multi_polygon_tag, Multi>
    : detail::wkt::wkt_multi
        <
            Multi,
            detail::wkt::wkt_poly
                <
                    typename boost::range_value<Multi>::type,
                    detail::wkt::prefix_null
                >,
            detail::wkt::prefix_multipolygon
        >
{};

} // namespace dispatch
#endif

}

#endif // GGL_EXTENSIONS_GIS_IO_WKT_WRITE_WKT_MULTI_HPP
