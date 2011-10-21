/** @file latlong_posting_source.cc
 * @brief LatLongPostingSource implementation.
 */
/* Copyright 2008 Lemur Consulting Ltd
 * Copyright 2010,2011 Richard Boulton
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <config.h>

#include "xapian/geospatial.h"

#include "xapian/document.h"
#include "xapian/error.h"
#include "xapian/registry.h"

#include "serialise.h"
#include "serialise-double.h"
#include "str.h"

#include <cmath>

using namespace Xapian;
using namespace std;

static double
weight_from_distance(double dist, double k1, double k2)
{
    return k1 * pow(dist + k1, -k2);
}

void
LatLongDistancePostingSource::calc_distance()
{
    dist = (*metric)(center, *value_it);
}

/// Validate the parameters supplied to LatLongDistancePostingSource.
static void
validate_postingsource_params(double k1, double k2) {
    if (k1 <= 0) {
	string msg("k1 parameter to LatLongDistancePostingSource must be "
		   "greater than 0; was ");
	msg += str(k1);
	throw InvalidArgumentError(msg);
    }
    if (k2 <= 0) {
	string msg("k2 parameter to LatLongDistancePostingSource must be "
		   "greater than 0; was ");
	msg += str(k2);
	throw InvalidArgumentError(msg);
    }
}

LatLongDistancePostingSource::LatLongDistancePostingSource(
	valueno slot_,
	const LatLongCoords & center_,
	const LatLongMetric * metric_,
	double max_range_,
	double k1_,
	double k2_)
	: ValuePostingSource(slot_),
	  center(center_),
	  metric(metric_),
	  max_range(max_range_),
	  k1(k1_),
	  k2(k2_)
{
    validate_postingsource_params(k1, k2);
    set_maxweight(weight_from_distance(0, k1, k2));
}

LatLongDistancePostingSource::LatLongDistancePostingSource(
	valueno slot_,
	const LatLongCoords & center_,
	const LatLongMetric & metric_,
	double max_range_,
	double k1_,
	double k2_)
	: ValuePostingSource(slot_),
	  center(center_),
	  metric(metric_.clone()),
	  max_range(max_range_),
	  k1(k1_),
	  k2(k2_)
{
    validate_postingsource_params(k1, k2);
    set_maxweight(weight_from_distance(0, k1, k2));
}

LatLongDistancePostingSource::~LatLongDistancePostingSource()
{
    delete metric;
}

void
LatLongDistancePostingSource::next(weight min_wt)
{
    ValuePostingSource::next(min_wt);

    while (value_it != db.valuestream_end(slot)) {
	calc_distance();
	if (max_range == 0 || dist <= max_range)
	    break;
	++value_it;
    }
}

void
LatLongDistancePostingSource::skip_to(docid min_docid,
				      weight min_wt)
{
    ValuePostingSource::skip_to(min_docid, min_wt);

    while (value_it != db.valuestream_end(slot)) {
	calc_distance();
	if (max_range == 0 || dist <= max_range)
	    break;
	++value_it;
    }
}

bool
LatLongDistancePostingSource::check(docid min_docid,
				    weight min_wt)
{
    if (!ValuePostingSource::check(min_docid, min_wt)) {
	// check returned false, so we know the document is not in the source.
	return false;
    }
    if (value_it == db.valuestream_end(slot)) {
	// return true, since we're definitely at the end of the list.
	return true;
    }

    calc_distance();
    if (max_range > 0 && dist > max_range) {
	return false;
    }
    return true;
}

weight
LatLongDistancePostingSource::get_weight() const
{
    return weight_from_distance(dist, k1, k2);
}

LatLongDistancePostingSource *
LatLongDistancePostingSource::clone() const
{
    return new LatLongDistancePostingSource(slot, center,
					    metric->clone(),
					    max_range, k1, k2);
}

string
LatLongDistancePostingSource::name() const
{
    return "Xapian::LatLongDistancePostingSource";
}

string
LatLongDistancePostingSource::serialise() const
{
    string serialised_center = center.serialise();
    string metric_name = metric->name();
    string serialised_metric = metric->serialise();

    string result = encode_length(slot);
    result += encode_length(serialised_center.size());
    result += serialised_center;
    result += encode_length(metric_name.size());
    result += metric_name;
    result += encode_length(serialised_metric.size());
    result += serialised_metric;
    result += serialise_double(max_range);
    result += serialise_double(k1);
    result += serialise_double(k2);
    return result;
}

LatLongDistancePostingSource *
LatLongDistancePostingSource::unserialise_with_registry(const string &s,
					     const Registry & registry) const
{
    const char * p = s.data();
    const char * end = p + s.size();

    valueno new_slot = decode_length(&p, end, false);
    size_t len = decode_length(&p, end, true);
    string new_serialised_center(p, len);
    p += len;
    len = decode_length(&p, end, true);
    string new_metric_name(p, len);
    p += len;
    len = decode_length(&p, end, true);
    string new_serialised_metric(p, len);
    p += len;
    double new_max_range = unserialise_double(&p, end);
    double new_k1 = unserialise_double(&p, end);
    double new_k2 = unserialise_double(&p, end);
    if (p != end) {
	throw NetworkError("Bad serialised LatLongDistancePostingSource - junk at end");
    }

    LatLongCoords new_center;
    new_center.unserialise(new_serialised_center);

#if 0 // Requires a Xapian with geospatial support, in which case we wouldn't be here anyway.
    const Xapian::LatLongMetric * metric_type =
	    registry.get_lat_long_metric(new_metric_name);
#else
    (void) registry;
    const Xapian::LatLongMetric * metric_type = NULL;
    Xapian::GreatCircleMetric great_circle_metric;
    if (new_metric_name == "Xapian::GreatCircleMetric") {
	metric_type = &great_circle_metric;
    }
#endif
    if (metric_type == NULL) {
	string msg("LatLongMetric ");
	msg += new_metric_name;
	msg += " not registered";
	throw InvalidArgumentError(msg);
    }
    LatLongMetric * new_metric =
	    metric_type->unserialise(new_serialised_metric);

    return new LatLongDistancePostingSource(new_slot, new_center,
					    new_metric,
					    new_max_range, new_k1, new_k2);
}

void
LatLongDistancePostingSource::init(const Database & db_)
{
    ValuePostingSource::init(db_);
    if (max_range > 0.0) {
	// Possible that no documents are in range.
	termfreq_min = 0;
	// Note - would be good to improve termfreq_est here, too, but
	// I can't think of anything we can do with the information
	// available.
    }
}

string
LatLongDistancePostingSource::get_description() const
{
    string result("Xapian::LatLongDistancePostingSource(slot=");
    result += str(slot);
    result += ")";
    return result;
}
