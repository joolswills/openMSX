#ifndef RANGES_HH
#define RANGES_HH

#include <algorithm>
#include <iterator> // for std::begin(), std::end()
#include <numeric>

// Range based versions of the standard algorithms, these will likely become
// part of c++20. For example see this post:
//   http://ericniebler.com/2018/12/05/standard-ranges/
// In the future we can remove our implementation and instead use the standard
// version (e.g. by a namespace alias like 'namespace ranges = std::ranges').
//
// All of the range algorithms below do nothing more than delegate to the
// corresponding iterator-pair version of the algorithm.
//
// This list of algorithms is not complete. But it's easy to extend if/when we
// need more.

namespace ranges {

template<typename ForwardRange>
bool is_sorted(ForwardRange&& range)
{
	return std::is_sorted(std::begin(range), std::end(range));
}

template<typename ForwardRange, typename Compare>
bool is_sorted(ForwardRange&& range, Compare comp)
{
	return std::is_sorted(std::begin(range), std::end(range), comp);
}

template<typename RandomAccessRange>
void sort(RandomAccessRange&& range)
{
	std::sort(std::begin(range), std::end(range));
}

template<typename RandomAccessRange, typename Compare>
void sort(RandomAccessRange&& range, Compare comp)
{
	std::sort(std::begin(range), std::end(range), comp);
}

template<typename RandomAccessRange>
void stable_sort(RandomAccessRange&& range)
{
	std::stable_sort(std::begin(range), std::end(range));
}

template<typename RandomAccessRange, typename Compare>
void stable_sort(RandomAccessRange&& range, Compare comp)
{
	std::stable_sort(std::begin(range), std::end(range), comp);
}

template<typename ForwardRange, typename T>
bool binary_search(ForwardRange&& range, const T& value)
{
	return std::binary_search(std::begin(range), std::end(range), value);
}

template<typename ForwardRange, typename T, typename Compare>
bool binary_search(ForwardRange&& range, const T& value, Compare comp)
{
	return std::binary_search(std::begin(range), std::end(range), value, comp);
}

template<typename ForwardRange, typename T>
auto lower_bound(ForwardRange&& range, const T& value)
{
	return std::lower_bound(std::begin(range), std::end(range), value);
}

template<typename ForwardRange, typename T, typename Compare>
auto lower_bound(ForwardRange&& range, const T& value, Compare comp)
{
	return std::lower_bound(std::begin(range), std::end(range), value, comp);
}

template<typename ForwardRange, typename T>
auto upper_bound(ForwardRange&& range, const T& value)
{
	return std::upper_bound(std::begin(range), std::end(range), value);
}

template<typename ForwardRange, typename T, typename Compare>
auto upper_bound(ForwardRange&& range, const T& value, Compare comp)
{
	return std::upper_bound(std::begin(range), std::end(range), value, comp);
}

template<typename ForwardRange, typename T>
auto equal_range(ForwardRange&& range, const T& value)
{
	return std::equal_range(std::begin(range), std::end(range), value);
}

template<typename ForwardRange, typename T, typename Compare>
auto equal_range(ForwardRange&& range, const T& value, Compare comp)
{
	return std::equal_range(std::begin(range), std::end(range), value, comp);
}

template<typename InputRange, typename T>
auto find(InputRange&& range, const T& value)
{
	return std::find(std::begin(range), std::end(range), value);
}

template<typename InputRange, typename UnaryPredicate>
auto find_if(InputRange&& range, UnaryPredicate pred)
{
	return std::find_if(std::begin(range), std::end(range), pred);
}

template<typename InputRange, typename UnaryPredicate>
bool all_of(InputRange&& range, UnaryPredicate pred)
{
	return std::all_of(std::begin(range), std::end(range), pred);
}

template<typename InputRange, typename UnaryPredicate>
bool any_of(InputRange&& range, UnaryPredicate pred)
{
	return std::any_of(std::begin(range), std::end(range), pred);
}

template<typename InputRange, typename UnaryPredicate>
bool none_of(InputRange&& range, UnaryPredicate pred)
{
	return std::none_of(std::begin(range), std::end(range), pred);
}

template<typename ForwardRange>
auto unique(ForwardRange&& range)
{
	return std::unique(std::begin(range), std::end(range));
}

template<typename ForwardRange, typename BinaryPredicate>
auto unique(ForwardRange&& range, BinaryPredicate pred)
{
	return std::unique(std::begin(range), std::end(range), pred);
}

template<typename InputRange, typename OutputIter>
auto copy(InputRange&& range, OutputIter out)
{
	return std::copy(std::begin(range), std::end(range), out);
}

template<typename InputRange, typename OutputIter, typename UnaryPredicate>
auto copy_if(InputRange&& range, OutputIter out, UnaryPredicate pred)
{
	return std::copy_if(std::begin(range), std::end(range), out, pred);
}

template<typename InputRange, typename OutputIter, typename UnaryOperation>
auto transform(InputRange&& range, OutputIter out, UnaryOperation op)
{
	return std::transform(std::begin(range), std::end(range), out, op);
}

template<typename ForwardRange, typename T>
auto remove(ForwardRange&& range, const T& value)
{
	return std::remove(std::begin(range), std::end(range), value);
}

template<typename ForwardRange, typename UnaryPredicate>
auto remove_if(ForwardRange&& range, UnaryPredicate pred)
{
	return std::remove_if(std::begin(range), std::end(range), pred);
}

template<typename ForwardRange, typename T>
void replace(ForwardRange&& range, const T& old_value, const T& new_value)
{
	std::replace(std::begin(range), std::end(range), old_value, new_value);
}

template<typename ForwardRange, typename UnaryPredicate, typename T>
void replace_if(ForwardRange&& range, UnaryPredicate pred, const T& new_value)
{
	std::replace_if(std::begin(range), std::end(range), pred, new_value);
}

template<typename ForwardRange, typename T>
void fill(ForwardRange&& range, const T& value)
{
	std::fill(std::begin(range), std::end(range), value);
}

template<typename InputRange, typename T>
T accumulate(InputRange&& range, T init)
{
	return std::accumulate(std::begin(range), std::end(range), init);
}

template<typename InputRange, typename T, typename BinaryOperation>
T accumulate(InputRange&& range, T init, BinaryOperation op)
{
	return std::accumulate(std::begin(range), std::end(range), init, op);
}

template<typename InputRange, typename T>
auto count(InputRange&& range, const T& value)
{
	return std::count(std::begin(range), std::end(range), value);
}

template<typename InputRange, typename UnaryPredicate>
auto count_if(InputRange&& range, UnaryPredicate pred)
{
	return std::count_if(std::begin(range), std::end(range), pred);
}

template<typename InputRange1, typename InputRange2, typename OutputIter>
auto set_difference(InputRange1&& range1, InputRange2&& range2, OutputIter out)
{
	return std::set_difference(std::begin(range1), std::end(range1),
	                           std::begin(range2), std::end(range2),
	                           out);
}

} // namespace ranges

#endif
