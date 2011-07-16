# -*- coding: utf-8 -
#
# This file is part of the restpose python module, released under the MIT
# license.  See the COPYING file for more information.

"""
Queries in RestPose.

"""

import copy

def _query_struct(query):
    """Get a structure to be sent to the server, from a Query.

    """
    if isinstance(query, Query):
        return copy.deepcopy(query.query)
    elif hasattr(query, 'items'):
        return dict([(k, copy.deepcopy(v)) for (k, v) in query.items()])
    elif query is None:
        return None
    raise TypeError("Query must either be a restpose.Query object, or have an 'items' method")

def _target_from_queries(queries):
    """Get a target from some queries.

    If the queries contain differing targets (other than some containing None),
    raises an exception.

    Returns None if none of the queries have a target set.

    """
    target = None
    for query in queries:
        if query.target is not None:
            if target is not None and target is not query.target:
                raise ValueError("Queries have inconsistent targets.")
            target = query.target
    return target


class Searchable(object):
    """An object which can be sliced or iterated to perform a query.

    """
    # Number of results to get in each request, if size is not explicitly set.
    _page_size = 20

    def __init__(self, target):
        """Create a new Searchable.

        target is the object that the search will be performed on.  For
        example, a restpose.Collection or restpose.DocumentType object.

        """
        self._target = target
        self._results = None

    def _build_search(self, offset=None, size=None, check_at_least=None):
        """Build a search structure to send to the server.

        When supplied, the optional parameters should modify the corresponding
        parts of the search structure.

        """
        raise NotImplementedError

    def _search(self):
        """Convert the Searchable to a Search.

        Must return a Search object which may be modified without affecting
        the Searchable.

        """
        raise NotImplementedError

    @property
    def results(self):
        """Get the results for this search.

        """
        if self._results is None:
            if self._target is None:
                raise ValueError("Target of search not set")
            self._results = self._target.search(self)
        return self._results

    def __len__(self):
        """Get the exact number of matching documents.

        Note that searches often don't involve calculating the exact number of
        matching documents, so this will often force a search to be performed
        with a check_at_least value of -1, which is something to be avoided
        unless it is strictly neccessary to know the exact number of matching
        documents.

        """
        if self._results is not None and self._results.estimate_is_exact:
            return self._results.matches_estimated
        FIXME
        self._results

    def __getitem__(self, key):
        """Get an item, or set the slice of results to return.

        """
        if not isinstance(key, (slice, int, long)):
            raise TypeError("keys must be slice objects, or integers")

        if isinstance(key, slice):
            result = self._search()
            if key.step is not None and int(key.step) != 1:
                raise TypeError("step values != 1 are not currently supported")

            # Get start from the slice, or 0 if not specified.
            if key.start is not None:
                start = int(key.start)
                if start < 0:
                    raise IndexError("Negative indexing is not supported")
            else:
                start = 0

            # Get stop from the slice, or None if not specified.
            if key.stop is not None:
                stop = int(key.stop)
                if stop < 0:
                    raise IndexError("Negative indexing is not supported")
            else:
                stop = None

            # Set the starting offset.
            oldstart = result._offset
            result._offset = start + oldstart

            # Update the size.
            oldsize = result._size
            if stop is None:
                if oldsize is None:
                    newstop = None
                else:
                    newstop = oldstart + oldsize
            else:
                newsize = max(stop - start, 0)
                newstop = start + oldstart + newsize
                if oldsize is not None:
                    oldstop = oldstart + oldsize
                    newstop = min(oldstop, newstop)

            if newstop is not None:
                result._size = max(newstop - result._offset, 0)

            return result

        else:
            key = int(key)
            if key < 0:
                raise IndexError("Negative indexing is not supported")
            start = result._body.get('from', 0)
            size = result._body.get('size', None)
            FIXME
            if key >= size:
                raise IndexError("")

            return result.results[start]

    def check_at_least(self, check_at_least):
        """Set the check_at_least value.

        This is the minimum number of documents to try and check when running
        the search - useful mainly when you want reasonably accurate counts of
        matching documents, but don't want to retrieve all matches.

        Returns a new Search, with the check_at_least value to use when
        performing the search set to the specified value.

        """
        result = self._search()
        result._body['check_at_least'] = int(check_at_least)
        return result

    def calc_occur(self, prefix, doc_limit=None, result_limit=None,
                   get_termfreqs=False, stopwords=[]):
        """Get occurrence counts of terms in the matching documents.

        Warning - fairly slow.

        Causes the search results to contain counts for each term seen, in
        decreasing order of occurrence.  The count entries are of the form:
        [suffix, occurrence count] or [suffix, occurrence count, termfreq] if
        get_termfreqs was true.

        @param prefix: prefix of terms to check occurrence for
        @param doc_limit: number of matching documents to stop checking after.  None=unlimited.  Integer or None.  Default=None
        @param result_limit: number of terms to return results for.  None=unlimited.  Integer or None. Default=None
        @param get_termfreqs: set to true to also get frequencies of terms in the db.  Boolean.  Default=False
        @param stopwords: list of stopwords - term suffixes to ignore.  Array of strings.  Default=[]

        """
        result = self._search()
        info = result._body.setdefault('info', [])
        info.append({'occur': dict(prefix=prefix,
                                   doc_limit=doc_limit,
                                   result_limit=result_limit,
                                   get_termfreqs=get_termfreqs,
                                   stopwords=stopwords,
                                  )})
        return result

    def calc_cooccur(self, prefix, doc_limit=None, result_limit=None,
                     get_termfreqs=False, stopwords=[]):
        """Get cooccurrence counts of terms in the matching documents.

        Warning - fairly slow (and O(L*L), where L is the average document
        length).

        Causes the search results to contain counts for each pair of terms
        seen, in decreasing order of cooccurrence.  The count entries are of
        the form: [suffix1, suffix2, co-occurrence count] or [suffix1, suffix2,
        co-occurrence count, termfreq of suffix1, termfreq of suffix2] if
        get_termfreqs was true.

        """
        result = self._search()
        info = result._body.setdefault('info', [])
        info.append({'cooccur': dict(prefix=prefix,
                                     doc_limit=doc_limit,
                                     result_limit=result_limit,
                                     get_termfreqs=get_termfreqs,
                                     stopwords=stopwords,
                                    )})
        return result


class Query(Searchable):
    """Base class of all queries.

    All query subclasses should have a property called "query", containing the
    query as a structure ready to be converted to JSON and sent to the server.

    """
    def __init__(self, target=None):
        super(Query, self).__init__(target)

    def _build_search(self, offset=None, size=None, check_at_least=None):
        """Build a search structure to send to the server.

        """
        body = dict(query=self.query)
        if offset is not None:
            body['from'] = offset
        if size is not None:
            body['size'] = size
        if check_at_least is not None:
            body['check_at_least'] = check_at_least
        return body

    def _search(self):
        """Get a Search object from this query.

        """
        return Search(self._target, dict(query=self.query))

    def __mul__(self, mult):
        """Return a query with the weights scaled by a multiplier.

        This can be used to build a query in which the weights of some
        subqueries are increased or decreased relative to the other subqueries.

        """
        return QueryMultWeight(self, mult, target=self._target)

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
        return QueryAnd((self, other), target=self._target)

    def __or__(self, other):
        return QueryOr((self, other), target=self._target)

    def __xor__(self, other):
        return QueryXor((self, other), target=self._target)

    def __sub__(self, other):
        return QueryNot((self, other), target=self._target)

    def filter(self, other):
        """Return the results of this query filtered by another query.

        This returns only documents which match both the original and the
        filter query, but uses only the weights from the original query.

        """
        return QueryAnd((self, other * 0), target=self._target)

    def and_maybe(self, other):
        """Return the results of this query, with additional weights from
        another query.

        This returns exactly the documents which match the original query, but
        adds the weight from corresponding matches to the other query.

        """
        return QueryAndMaybe((self, other), target=self._target)


class QueryField(Query):
    """A query in a particular field.

    """
    def __init__(self, fieldname, querytype, value, target=None):
        super(QueryField, self).__init__(target=target)
        self.query = dict(field=[fieldname, querytype, value])


class QueryMeta(Query):
    """A query for meta information (about field presence, errors, etc).

    """
    def __init__(self, querytype, value, target=None):
        super(QueryMeta, self).__init__(target=target)
        self.query = dict(meta=[querytype, value])


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
        super(QueryNone, self).__init__(target=target)
QueryNothing = QueryNone


class QueryAnd(Query):
    """A query which matches only the documents matched by all subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(QueryAnd, self).__init__(target=target)
        self.query = {"and": list(_query_struct(query) for query in queries)}


class QueryOr(Query):
    """A query which matches the documents matched by any subquery.

    The weights are the sum of the weights in the subqueries which match.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(QueryOr, self).__init__(target=target)
        self.query = {"or": list(_query_struct(query) for query in queries)}


class QueryXor(Query):
    """A query which matches the documents matched by an odd number of
    subqueries.

    The weights are the sum of the weights in the subqueries which match.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(QueryXor, self).__init__(target=target)
        self.query = {"xor": list(_query_struct(query) for query in queries)}


class QueryNot(Query):
    """A query which matches the documents matched by the first subquery, but
    not any of the other subqueries.

    The weights returned are the weights in the first subquery.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(QueryNot, self).__init__(target=target)
        self.query = {"not": list(_query_struct(query) for query in queries)}


class QueryAndMaybe(Query):
    """A query which matches the documents matched by the first subquery, but
    adds additional weights from the other subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(QueryNot, self).__init__(target=target)
        self.query = {"and_maybe": list(_query_struct(query) for query in queries)}


class QueryMultWeight(Query):
    """A query which matches all the documents matched by another query, but
    with the weights multiplied by a factor.

    """
    def __init__(self, query, factor, target=None):
        """Build a query in which the weights are multiplied by a factor.

        """
        if target is None:
            target = query.target
        super(QueryMultWeight, self).__init__(target=target)
        self.query = dict(scale=dict(query=_query_struct(query), factor=factor))


class Search(Searchable):
    """A search: a query, together with how to perform it.

    """
    def __init__(self, target, body):
        super(Search, self).__init__(target)
        self._body = body

    @property
    def _offset(self):
        """The rank offset that this search will retrieve results from.

        """
        return self._body.get('from', 0)
    @_offset.setter
    def _offset(self, offset):
        self._body['from'] = int(offset)

    @property
    def _size(self):
        """The number of results that this search will retrieve.

        Returns None if undefined.

        """
        return self._body.get('size', None)
    @_size.setter
    def _size(self, size):
        self._body['size'] = int(size)

    def _build_search(self, offset=None, size=None, check_at_least=None):
        """Build the search into the structure to send to the server.

        This returns the search stored in this object, modified by the
        parameters passed to this call, if any.

        """
        body = self._body
        copied = False
        if offset is not None:
            body = copy.copy(body)
            copied = True
            body['from'] = offset
        if size is not None:
            if not copied:
                body = copy.copy(body)
                copied = True
            body['size'] = size
        if check_at_least is not None:
            if not copied:
                body = copy.copy(body)
                copied = True
            body['check_at_least'] = check_at_least
        return body

    def _search(self):
        """Get a clone of this Search object.

        Returns a Search whose configuration may be independently modified.

        """
        body = {}
        for k, v in self._body.iteritems():
            if k == 'query':
                # No need to copy the query structure - it should be immutable,
                # anyway.
                body[k] = v
            else:
                body[k] = copy.deepcopy(v)
        return Search(self._target, body)


class SearchResult(object):
    def __init__(self, rank, data):
        self.rank = rank
        self.data = data

    def __str__(self):
        return 'SearchResult(rank=%d, data=%s)' % (
            self.rank, self.data,
        )


class InfoItem(object):
    def __init__(self, raw):
        self._raw = raw


class SearchResults(object):
    def __init__(self, raw):
        self._raw = raw
        self._items = None
        self._infos = None

    @property
    def offset(self):
        """The offset of the first result item."""
        return self._raw.get('from', 0)

    @property
    def size_requested(self):
        """The requested size."""
        return self._raw.get('size_requested', 0)

    @property
    def check_at_least(self):
        """The requested check_at_least value."""
        return self._raw.get('check_at_least', 0)

    @property
    def matches_lower_bound(self):
        """A lower bound on the number of matches."""
        return self._raw.get('matches_lower_bound', 0)

    @property
    def matches_estimated(self):
        """An estimate of the number of matches."""
        return self._raw.get('matches_estimated', 0)

    @property
    def matches_upper_bound(self):
        """An upper bound on the number of matches."""
        return self._raw.get('matches_upper_bound', 0)

    @property
    def estimate_is_exact(self):
        """Return True if the value returned by matches_estimated is exact,
        False if it isn't (or at least, isn't guaranteed to be).

        """
        return self.matches_lower_bound == self.matches_upper_bound

    @property
    def items(self):
        """The matching result items."""
        if self._items is None:
            self._items = [SearchResult(rank, data) for (rank, data) in
                enumerate(self._raw.get('items', []), self.offset)]
        return self._items

    @property
    def info(self):
        return self._raw.get('info', {})

    def at_rank(self, rank):
        """Get the result at a given rank.

        The rank is the position in the entire result set, starting at 0.

        Raises IndexError if the rank is out of the range in the result set.

        """
        index = rank - self.offset
        if index < 0:
            raise IndexError
        if self._items is None:
            return SearchResult(rank, self._raw.get('items', [])[index])
        return self.items[index]

    def __iter__(self):
        """Get an iterator over all items in this result set.

        The iterator produces SearchResult items.

        """
        return iter(self.items)

    def __len__(self):
        """Get the number of items in this result set.

        This is the number of items produced by iterating over the result set.
        It is not (usually) the number of items matching the search.

        """
        return len(self.items)

    def __str__(self):
        result = u'SearchResults(offset=%d, size_requested=%d, ' \
                 u'check_at_least=%d, ' \
                 u'matches_lower_bound=%d, ' \
                 u'matches_estimated=%d, ' \
                 u'matches_upper_bound=%d, ' \
                 u'items=[%s]' % (
            self.offset, self.size_requested, self.check_at_least,
            self.matches_lower_bound,
            self.matches_estimated,
            self.matches_upper_bound,
            u', '.join(unicode(item) for item in self.items),
        )
        if self.info:
            result += u', info=%s' % unicode(self.info)
        return result + u')'
