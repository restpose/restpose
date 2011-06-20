# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Queries in RestPose.

"""

import copy

def query_struct(query):
    if isinstance(query, Query):
        return copy.deepcopy(query.query)
    elif hasattr(query, 'items'):
        return dict([(k, copy.deepcopy(v)) for (k, v) in query.items()])
    raise TypeError("Query must either be a restpose.Query object, or have an 'items' method")

class Query(object):
    """Base class of all queries.

    All query subclasses should have a property called "query", containing the
    query as a structure ready to be converted to JSON and sent to the server.

    """

    def __init__(self, target=None):
        self.target = target

    def __mul__(self, mult):
        """Return a query with the weights scaled by a multiplier.

        This can be used to build a query in which the weights of some
        subqueries are increased or decreased relative to the other subqueries.

        """
        return QueryMultWeight(self, mult, target=self.target)

    def __rmul__(self, lhs):
        """Return a query with the weights scaled by a multiplier.

        This can be used to build a query in which the weights of some
        subqueries are increased or decreased relative to the other subqueries.

        """
        return self.__mul__(lhs)

    def __div__(self, rhs):
        """Return a query with the weight divided by a number.

        """
        return self.__mul__(1.0 / rhs)

    def __truediv__(self, rhs):
        """Return a query with the weight divided by a number.

        """
        return self.__mul__(1.0 / rhs)

    def __and__(self, other):
        return QueryAnd((self, other), target=self.target)

    def __or__(self, other):
        return QueryOr((self, other), target=self.target)

    def __xor__(self, other):
        return QueryXor((self, other), target=self.target)

    def __sub__(self, other):
        return QueryNot((self, other), target=self.target)

    def filter(self, other):
        """Return the results of this query filtered by another query.

        This returns only documents which match both the original and the
        filter query, but uses only the weights from the original query.

        """
        return QueryAnd((self, other * 0), target=self.target)

    def and_maybe(self, other):
        """Return the results of this query, with additional weights from
        another query.

        This returns exactly the documents which match the original query, but
        adds the weight from corresponding matches to the other query.

        """
        return QueryAndMaybe((self, other), target=self.target)

    def search(self, **kwargs):
        """Make a search using this query.

        """
        if self.target is not None:
            if kwargs.get('target', None) is None:
                kwargs['target'] = self.target
        return Search(query=self, **kwargs)

class QueryField(Query):
    """A query in a particular field.

    """
    def __init__(self, fieldname, querytype, value, target=None):
        super(QueryField, self).__init__(target=target)
        self.query = dict(field=[fieldname, querytype, value])


class QueryAll(Query):
    """A query which matches all documents.

    """
    query = {"matchall": True}

    def __init__(self, target=None):
        super(QueryAll, self).__init__(target=target)


class QueryNone(Query):
    """A query which matches no documents.

    """
    query = {"matchnothing": True}

    def __init__(self, target=None):
        super(QueryNone, self).__init__(targettarget)
QueryNothing = QueryNone


class QueryAnd(Query):
    """A query which matches only the documents matched by all subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    def __init__(self, queries, target=None):
        super(QueryAnd, self).__init__(target=target)
        self.query = {"and": list(query_struct(query) for query in queries)}


class QueryOr(Query):
    """A query which matches the documents matched by any subquery.

    The weights are the sum of the weights in the subqueries which match.

    """
    def __init__(self, queries, target=None):
        super(QueryOr, self).__init__(target=target)
        self.query = {"or": list(query_struct(query) for query in queries)}


class QueryXor(Query):
    """A query which matches the documents matched by an odd number of
    subqueries.

    The weights are the sum of the weights in the subqueries which match.

    """
    def __init__(self, queries, target=None):
        super(QueryXor, self).__init__(target=target)
        self.query = {"xor": list(query_struct(query) for query in queries)}


class QueryNot(Query):
    """A query which matches the documents matched by the first subquery, but
    not any of the other subqueries.

    The weights returned are the weights in the first subquery.

    """
    def __init__(self, queries, target=None):
        super(QueryNot, self).__init__(target=target)
        self.query = {"not": list(query_struct(query) for query in queries)}


class QueryAndMaybe(Query):
    """A query which matches the documents matched by the first subquery, but
    adds additional weights from the other subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    def __init__(self, queries, target=None):
        super(QueryAndMaybe, self).__init__(target=target)
        self.query = {"and_maybe": list(query_struct(query)
                                        for query in queries)}


class QueryMultWeight(Query):
    """A query which matches all the documents matched by another query, but
    with the weights multiplied by a factor.

    """
    def __init__(self, query, factor, target=None):
        """Build a query in which the weights are multiplied by a factor.

        """
        super(QueryMultWeight, self).__init__(target=target)
        self.query = dict(scale=dict(query=query_struct(query), factor=factor))


class Search(object):
    """A search: a query, together with how to perform it.

    """
    def __init__(self, target=None, query=None,
                 offset=0, size=10, checkatleast=0,
                 info=None, display=None):
        self.target = target
        self.body = { 'query': query_struct(query),
                      'from': offset,
                      'size': size,
                      'checkatleast': checkatleast,
                    }
        if info:
            self.body['info'] = info
        if display:
            self.body['display'] = display

    def do(self):
        return self.target.search(self)

class SearchResult(object):
    def __init__(self, rank, fields):
        self.rank = rank
        self.fields = fields

    def __str__(self):
        return 'SearchResult(rank=%d, fields=%s)' % (
            self.rank, self.fields,
        )

class SearchResults(object):
    def __init__(self, result):
        #: The offset of the first result item.
        self.offset = result.get('from', 0)

        #: The requested size.
        self.size = result.get('size', 0)

        #: The requested checkatleast value.
        self.checkatleast = result.get('checkatleast', 0)

        #: A lower bound on the number of matches.
        self.matches_lower_bound = result.get('matches_lower_bound', 0)

        #: An estimate of the number of matches.
        self.matches_estimated = result.get('matches_estimated', 0)

        #: An upper bound on the number of matches.
        self.matches_upper_bound = result.get('matches_upper_bound', 0)

        #: The matching result items.
        self.items = [SearchResult(rank, fields) for (rank, fields) in
                      enumerate(result.get('items', []), self.offset)]

    def __iter__(self):
        return iter(self.items)

    def __len__(self):
        return len(self.items)

    def __str__(self):
        return 'SearchResults(offset=%d, size=%d, checkatleast=%d, ' \
               'matches_lower_bound=%d, ' \
               'matches_estimated=%d, ' \
               'matches_upper_bound=%d, ' \
               'items=[%s])' % (
            self.offset, self.size, self.checkatleast,
            self.matches_lower_bound,
            self.matches_estimated,
            self.matches_upper_bound,
            ', '.join(str(item) for item in self.items),
        )
