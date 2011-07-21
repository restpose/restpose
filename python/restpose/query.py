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
        return copy.deepcopy(query._query)
    elif hasattr(query, 'items'):
        return dict([(k, copy.deepcopy(v)) for (k, v) in query.items()])
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
    #: Number of results to get in each request, if size is not explicitly set.
    page_size = 20

    _query = None
    _offset = 0
    _size = None
    _check_at_least = 0
    _info = None
    _order_by = None
    _results = None

    def __init__(self, target):
        """Create a new Searchable.

        target is the object that the search will be performed on.  For
        example, a restpose.Collection or restpose.DocumentType object.

        """
        self._target = target

    def _build_search(self, offset=None, size=None, check_at_least=None):
        """Build the search structure to send to the server.

        """
        body = dict(query=self._query)

        if offset is None:
            offset = self._offset
        if offset:
            body['from'] = offset

        if size is None:
            size = self._size
        if size is not None:
            body['size'] = size

        if check_at_least is None:
            check_at_least = self._check_at_least
        if check_at_least:
            body['check_at_least'] = check_at_least

        if self._info is not None:
            body['info'] = self._info

        if self._order_by is not None:
            body['order_by'] = self._order_by

        return body

    def _ensure_results(self, offset, size, check_at_least):
        """Ensure that the results contain items from offset to size, with
        check_at_least being at least the value set.

        """
        if self._target is None:
            raise ValueError("Target of search not set")

        if size is None:
            size = self.page_size

        if self._results is not None:
            need_recalc = False
            if offset < self._results.offset:
                need_recalc = True
            elif (offset + size >
                  self._results.offset + self._results.size_requested):
                need_recalc = True
            elif check_at_least == -1:
                if self._results.total_docs > self._results.check_at_least:
                    need_recalc = True
            elif check_at_least is not None and check_at_least >= 0:
                if check_at_least > self._results.check_at_least:
                    need_recalc = True
            if not need_recalc:
                return

        s = self._build_search(offset, size, check_at_least)
        #print "search: ", s
        self._results = self._target.search(s)
        #print "raw results: ", self._results._raw

    def _ensure_results_stats(self):
        """Ensure that the results contain stats.

        """
        # If any results have been calculated, we have stats.
        if self._results is None:
            self._ensure_results(self._offset, self._size,
                                 self._check_at_least)

    def _ensure_results_contain(self, rank):
        """Ensure that the results contain the given rank.

        If no size is set, and the offset is after the starting offset, will
        fetch the necessary page of results (according to the page_size
        setting).

        If size is set, and the offset is within the valid range, will fetch
        the results.

        Otherwise, will raise IndexError.

        """
        if rank < self._offset:
            raise IndexError("Rank requested is outsize slice range.")
        if self._size is not None and self._offset + self._size <= rank:
            raise IndexError("Rank requested is outsize slice range.")
        if self._results is not None:
            # Check if the requested range for the results includes the
            # specified rank.
            if self._results.offset <= rank and \
               self._results.offset + self._results.size_requested > rank:
                return

        if self._size is None:
            # Fetch a page of results.
            page_num = int((rank - self._offset) / self.page_size)
            self._ensure_results(self._offset + page_num * self.page_size,
                                 self.page_size,
                                 self._check_at_least)
        else:
            # Fetch the specified results.
            self._ensure_results(self._offset, self._size,
                                 self._check_at_least)

    @property
    def total_docs(self):
        """Get the total number of documents searched.

        """
        self._ensure_results_stats()
        return self._results.total_docs

    @property
    def matches_lower_bound(self):
        """A lower bound on the number of matches.

        """
        self._ensure_results_stats()
        return self._results.matches_lower_bound

    @property
    def matches_estimated(self):
        """An estimate of the number of matches.

        """
        self._ensure_results_stats()
        return self._results.matches_estimated

    @property
    def matches_upper_bound(self):
        """An upper bound on the number of matches.

        """
        self._ensure_results_stats()
        return self._results.matches_upper_bound

    @property
    def estimate_is_exact(self):
        """True if the value returned by matches_estimated is exact,
        False if it isn't (or at least, isn't guaranteed to be).

        """
        return self.matches_lower_bound == self.matches_upper_bound

    def __len__(self):
        """Get the exact number of matching documents for this Query.

        Note that searches often don't involve calculating the exact number of
        matching documents, so this will often force a search to be performed
        with a check_at_least value of -1, which is something to be avoided
        unless it is strictly neccessary to know the exact number of matching
        documents.  If you just need a rough idea of the number of matching
        documents, see `matches_estimated`, and the associated
        `matches_lower_bound`, `matches_upper_bound` and `estimate_is_exact`
        properties.

        Also, note that if this is a TerminalQuery which has been sliced, this
        will return the number of results in the sliced region.

        """
        if self._results is not None and self._results.estimate_is_exact:
            total = self._results.matches_estimated
        else:
            # Need to run a search with check_at_least = -1 to ensure exact
            # estimate.
            self._ensure_results(self._offset, self._size, -1)
            assert self._results.estimate_is_exact
            total = self._results.matches_estimated

        # Note - the following code could be shrunk using min and max, but
        # please leave it as is until test branch coverage for it is at 100%.
        if total < self._offset:
            return 0
        total = total - self._offset
        if self._size is not None and total > self._size:
            return self._size
        return total

    def __iter__(self):
        """Get an iterator over the results in this Query.

        """
        return QueryIterator(self)

    def __getitem__(self, key):
        """Get an item, or a slice of results.

        """
        if not isinstance(key, (slice, int, long)):
            raise TypeError("keys must be slice objects, or integers")

        if isinstance(key, slice):
            result = TerminalQuery(self)
            result._apply_slice(key)
            return result

        key = int(key)
        if key < 0:
            raise IndexError("Negative indexing is not supported")
        size = self._size
        if size is not None and key >= size:
            raise IndexError("Index out of range of slice of results")
        rank = self._offset + key
        self._ensure_results_contain(rank)
        return self._results.at_rank(rank)

    def check_at_least(self, check_at_least):
        """Set the check_at_least value.

        This is the minimum number of documents to try and check when running
        the search - useful mainly when you want reasonably accurate counts of
        matching documents, but don't want to retrieve all matches.

        Returns a new Search, with the check_at_least value to use when
        performing the search set to the specified value.

        """
        result = TerminalQuery(self)
        result._check_at_least = int(check_at_least)
        return result

    RELEVANCE = object()
    def order_by(self, field, ascending=None):
        """Set the sort order.

        """
        order_item = {}
        if field is self.RELEVANCE:
            order_item['score'] = 'weight'
        else:
            # assert that field is a string, because we may well want to allow
            # more complex types to be specified in future, and don't want to
            # risk breaking existing applications at that point.
            assert isinstance(field, basestring)
            order_item['field'] = field
        
        if ascending is not None:
            order_item['ascending'] = ascending
        
        result = TerminalQuery(self)
        result._order_by = [order_item]
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
        result = TerminalQuery(self)
        if result._info is None:
            result._info = []
        result._info.append({'occur': dict(prefix=prefix,
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
        result = TerminalQuery(self)
        if result._info is None:
            result._info = []
        result._info.append({'cooccur': dict(prefix=prefix,
                                             doc_limit=doc_limit,
                                             result_limit=result_limit,
                                             get_termfreqs=get_termfreqs,
                                             stopwords=stopwords,
                                            )})
        return result


class QueryIterator(object):
    """Iterate over the results of a query.

    """
    def __init__(self, query):
        self.query = query
        self.index = 0

    def __iter__(self):
        return self

    def next(self):
        try:
            result = self.query[self.index]
        except IndexError:
            raise StopIteration
        self.index += 1
        return result


class Query(Searchable):
    """Base class of all queries.

    All query subclasses should have a property called "query", containing the
    query as a structure ready to be converted to JSON and sent to the server.

    """

    def __init__(self, target=None):
        super(Query, self).__init__(target)

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
        self._query = dict(field=[fieldname, querytype, value])


class QueryMeta(Query):
    """A query for meta information (about field presence, errors, etc).

    """
    def __init__(self, querytype, value, target=None):
        super(QueryMeta, self).__init__(target=target)
        self._query = dict(meta=[querytype, value])


class QueryAll(Query):
    """A query which matches all documents.

    """
    _query = {"matchall": True}

    def __init__(self, target=None):
        super(QueryAll, self).__init__(target=target)


class QueryNone(Query):
    """A query which matches no documents.

    """
    _query = {"matchnothing": True}

    def __init__(self, target=None):
        super(QueryNone, self).__init__(target=target)
QueryNothing = QueryNone


class CombinedQuery(Query):
    """Base class of Queries which are combinations of a sequence of queries.

    Subclasses must define self._op, the operator to use to combine queries.

    """
    def __init__(self, queries, target=None):
        if target is None:
            queries = tuple(queries) # Handle queries being an iterator.
            target = _target_from_queries(queries)
        super(CombinedQuery, self).__init__(target=target)
        self._query = {self._op: list(_query_struct(query)
                                      for query in queries)}

class QueryAnd(CombinedQuery):
    """A query which matches only the documents matched by all subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    _op = "and"


class QueryOr(CombinedQuery):
    """A query which matches the documents matched by any subquery.

    The weights are the sum of the weights in the subqueries which match.

    """
    _op = "or"


class QueryXor(CombinedQuery):
    """A query which matches the documents matched by an odd number of
    subqueries.

    The weights are the sum of the weights in the subqueries which match.

    """
    _op = "xor"


class QueryNot(CombinedQuery):
    """A query which matches the documents matched by the first subquery, but
    not any of the other subqueries.

    The weights returned are the weights in the first subquery.

    """
    _op = "not"


class QueryAndMaybe(CombinedQuery):
    """A query which matches the documents matched by the first subquery, but
    adds additional weights from the other subqueries.

    The weights are the sum of the weights in the subqueries.

    """
    _op = "and_maybe"


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
        self._query = dict(scale=dict(query=_query_struct(query), factor=factor))


class TerminalQuery(Searchable):
    """A Query which has had offsets or additional search options set.

    This is produced from a Query when additional search options are set.  It
    can't be combined with other Query objects, since the semantics of doing so
    would be confusing.

    """
    def __init__(self, orig, slice=None):
        super(TerminalQuery, self).__init__(orig._target)
        self._query = orig._query
        self._offset = orig._offset
        self._size = orig._size
        self._check_at_least = orig._check_at_least
        self._info = copy.copy(orig._info)
        self._order_by = copy.copy(orig._order_by)
        if slice is not None:
            self._apply_slice(slice)

    def _apply_slice(self, slice):
        """Restrict the range of documents to return from the query.

        """
        if slice.step is not None and int(slice.step) != 1:
            raise IndexError("step values != 1 are not supported")

        # Get start from the slice, or 0 if not specified.
        if slice.start is not None:
            start = int(slice.start)
            if start < 0:
                raise IndexError("Negative indexing is not supported")
        else:
            start = 0

        # Get stop from the slice, or None if not specified.
        if slice.stop is not None:
            stop = int(slice.stop)
            if stop < 0:
                raise IndexError("Negative indexing is not supported")
        else:
            stop = None

        # Set the starting offset.
        oldstart = self._offset
        self._offset = start + oldstart

        # Update the size.
        oldsize = self._size
        if stop is None:
            if oldsize is not None:
                self._size = max(oldsize - start, 0)
        else:
            if oldsize is not None:
                newstop = min(oldsize, stop)
            else:
                newstop = stop
            self._size = max(newstop - start, 0)


class SearchResult(object):
    def __init__(self, rank, data):
        self.rank = rank
        self.data = data

    def __str__(self):
        return 'SearchResult(rank=%d, data=%s)' % (
            self.rank, self.data,
        )


class SearchResults(object):
    """The results returned from the server when performing a search.

    """
    def __init__(self, raw):
        #: The raw results returned from the server.
        self._raw = raw

        #: The matching documents.
        self._items = None

        #: Information associated with the search results (eg, term
        #: occurrence, facets).
        self._infos = None

        #: The total number of documents searched.
        self.total_docs = raw.get('total_docs', 0)

        #: The offset of the first result item.
        self.offset = raw.get('from', 0)

        #: The requested size.
        self.size_requested = raw.get('size_requested', 0)

        #: The requested check_at_least value.
        self.check_at_least = raw.get('check_at_least', 0)

        #: A lower bound on the number of matches.
        self.matches_lower_bound = raw.get('matches_lower_bound', 0)

        #: An estimate of the number of matches.
        self.matches_estimated = raw.get('matches_estimated', 0)

        #: An upper bound on the number of matches.
        self.matches_upper_bound = raw.get('matches_upper_bound', 0)

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
        return self._raw.get('info', [])

    def at_rank(self, rank):
        """Get the result at a given rank.

        The rank is the position in the entire result set, starting at 0.

        Raises IndexError if the rank is out of the range in the result set.

        """
        index = rank - self.offset
        if index < 0:
            raise IndexError
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

    def __getitem__(self, key):
        return self.items.__getitem__(key)

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
