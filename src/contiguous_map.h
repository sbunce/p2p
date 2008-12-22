/*
This container wraps a std::map. It has functions useful for dealing with a
range of elements that arrive out of order where you need to be able to
determine what elements are contiguous.

This container has trim functions which advance the lowest possible key it
will hold. There are many uses where it is natural to do this such as when
checking hash blocks.

There is also a contiguous_set. Both templates have the same functionality.
*/

#ifndef H_CONTIGUOUS_MAP
#define H_CONTIGUOUS_MAP

//std
#include <cassert>
#include <map>

template <class T_key, class T_val>
class contiguous_map
{
public:
	/*
	start_key: lowest possible element insertable in to range
	end_key:   one past highest possible element insertable in to range
	*/
	contiguous_map(
		const T_key & start_key_in,
		const T_key & end_key_in
	):
		start_key(start_key_in),
		end_key(end_key_in)
	{}

	//normal iteration done with regular std::map iterator
	typedef typename std::map<T_key, T_val>::iterator iterator;

	//forward iterator to iterate through contiguous elements
	class contiguous_iterator
	{
		friend class contiguous_map;
	public:
		contiguous_iterator(){}
		const contiguous_iterator & operator = (const contiguous_iterator & rval)
		{
			iter = rval.iter;
			return *this;
		}

		bool operator == (const contiguous_iterator & rval)
		{
			return iter == rval.iter;
		}

		bool operator != (const contiguous_iterator & rval)
		{
			return iter != rval.iter;
		}

		//pre-increment
		const contiguous_iterator & operator ++ ()
		{
			++iter;
			return *this;
		}

		//post-increment
		contiguous_iterator operator ++ (int)
		{
			typename std::map<T_key, T_val>::iterator iter_tmp = iter;
			++iter;
			return contiguous_iterator(iter_tmp);
		}

		typename std::pair<const T_key, T_val> & operator * ()
		{
			return *iter;
		}

		//inline dereference, iterator acts like pointer
		typename std::map<T_key, T_val>::iterator & operator -> ()
		{
			return iter;
		}

	private:
		//ctor for use by contiguous class only
		contiguous_iterator(
			const typename std::map<T_key, T_val>::iterator & iter_in
		):
			iter(iter_in)
		{}

		typename std::map<T_key, T_val>::iterator iter;
	};

	//forward iterator to iterate through incontinuous elements
	class incontiguous_iterator
	{
		friend class contiguous_map;
	public:
		incontiguous_iterator(){}

		const incontiguous_iterator & operator = (const incontiguous_iterator & rval)
		{
			iter = rval.iter;
			return *this;
		}

		bool operator == (const incontiguous_iterator & rval)
		{
			return iter == rval.iter;
		}

		bool operator != (const incontiguous_iterator & rval)
		{
			return iter != rval.iter;
		}

		//pre-increment
		const incontiguous_iterator & operator ++ ()
		{
			++iter;
			return *this;
		}

		//post-increment
		incontiguous_iterator operator ++ (int)
		{
			typename std::map<T_key, T_val>::iterator iter_tmp = iter;
			++iter;
			return incontiguous_iterator(iter_tmp);
		}

		typename std::pair<const T_key, T_val> & operator * ()
		{
			return *iter;
		}

		//inline dereference, iterator acts like pointer
		typename std::map<T_key, T_val>::iterator & operator -> ()
		{
			return iter;
		}

	private:
		//ctor for use by contiguous class only
		incontiguous_iterator(
			const typename std::map<T_key, T_val>::iterator & iter_in
		):
			iter(iter_in)
		{}

		typename std::map<T_key, T_val>::iterator iter;
	};

	//iterator to iterate through all elements
	typename std::map<T_key, T_val>::iterator begin()
	{
		return range.begin();
	}

	//iterator to iterate through all elements
	typename std::map<T_key, T_val>::iterator end()
	{
		return range.end();
	}

	//returns iterator to beginning of contiguous range
	contiguous_iterator begin_contiguous()
	{
		if(!range.empty() && start_key == range.begin()->first){
			return contiguous_iterator(range.begin());
		}else{
			return contiguous_iterator(range.end());
		}
	}

	//returns iterator to end of contiguous range
	contiguous_iterator end_contiguous()
	{
		T_key HC;
		if(highest_contiguous(HC)){
			typename std::map<T_key, T_val>::iterator iter;
			//HC guaranteed to be found since highest_contiguous returned true
			iter = range.upper_bound(HC);
			return contiguous_iterator(iter);
		}else{
			return contiguous_iterator(range.end());
		}
	}

	//returns iterator to beginning of incontiguous range
	incontiguous_iterator begin_incontiguous()
	{
		T_key HC;
		if(highest_contiguous(HC)){
			typename std::map<T_key, T_val>::iterator iter;
			//HC guaranteed to be found since highest_contiguous returned true
			iter = range.upper_bound(HC);
			return incontiguous_iterator(iter);
		}else{
			return incontiguous_iterator(range.end());
		}
	}

	//returns iterator to end of incontiguous range
	incontiguous_iterator end_incontiguous()
	{
		return incontiguous_iterator(range.end());
	}

	//erase element with key TK
	void erase(const T_key & TK)
	{
		range.erase(TK);
	}

	//find element with key TK
	typename std::map<T_key, T_val>::iterator find(const T_key & TK)
	{
		return range.find(TK);
	}

	//insert element in to range
	void insert(typename std::pair<T_key, T_val> pair)
	{
		assert(!(pair.first < start_key) && pair.first < end_key);
		range.insert(pair);
	}

	/*
	Sets HC to highest contiguous key starting at start_key.
	example:
		assume start_key = 0
		0 1 2 5 6 9
		return true and set HC to 2
	example:
		assume start_key = 0
		1 2 5 6 9
		return false and don't set HC (notice key 0 is missing)
	*/
	bool highest_contiguous(T_key & HC)
	{
		T_key tmp = start_key;
		typename std::map<T_key, T_val>::iterator iter_cur, iter_end;
		iter_cur = range.begin();
		iter_end = range.end();
		while(iter_cur != iter_end){
			if(iter_cur->first == tmp){
				++tmp;
			}else{
				break;
			}
			++iter_cur;
		}

		if(tmp == start_key){
			//start key doesn't exist in range, failure
			return false;
		}else{
			//contiguous elements found
			HC = tmp - 1;
			return true;
		}
	}

	//returns the lower boundary of the range (inclusive)
	const T_key & start_range()
	{
		return start_key;
	}

	//returns upper boundary of the range (exclusive)
	const T_key & end_range()
	{
		return end_key;
	}

	/*
	Removes the contiguous part, if there is any. Sets start_key to one past the
	last contigous key if a removal is done.
	*/
	void trim_contiguous()
	{
		T_key HC;
		if(highest_contiguous(HC)){
			start_key = HC + 1;
			typename std::map<T_key, T_val>::iterator iter;
			//HC guaranteed to be found since highest_contiguous returned true
			iter = range.upper_bound(HC);
			//erase to one past HC
			range.erase(range.begin(), iter);
		}
	}

	/*
	Removes all elements from start_key up to, but not including num.
	precondition: num must be >= start_key
	*/
	void trim(const T_key & num)
	{
		assert(num >= start_key);
		start_key = num;
		typename std::map<T_key, T_val>::iterator iter = range.find(num);
		range.erase(range.begin(), iter);
	}

private:

	T_key start_key; //first key in range
	T_key end_key;   //one past the last key in range

	//stores items that arrive out of order
	std::map<T_key, T_val> range;
};
#endif
