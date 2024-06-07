#pragma once
#include <iostream>
#include<map>
#include<vector>
#include<mutex>
#include<thread>
#include<list>
#include<memory>
#include<shared_mutex>
using namespace std;
template<typename Key,typename Value,typename Hash=std::hash<Key>>
class threadsafe_lookup_table
{
	class bucket_type
	{
		friend threadsafe_lookup_table;
		//存储元素
	private:
		typedef std::pair<Key, Value> bucket_value;
		//由链表组成存储结构
		typedef std::list<bucket_value> bucket_data;
		//链表的迭代器
		typedef typename bucket_data::iterator bucket_iterator;
		//链表数据
		bucket_data data;
		mutable std::shared_mutex mutex;


		bucket_iterator find_entry_for(const Key& key)
		{
			return std::find_if(data.begin(), data.end(), [&](bucket_type const& item)
				{
					return item.first == key;
				});
		}
	public:
		//查找key值，找到则返回对应的value，没找到就返回提供的默认值
		//可以并发地查找，不用加锁
		Value value_for(Key const& key, Value const& default_value)
		{
			std::shared_lock<std::mutex> lk(mutex);
			auto found_entry = find_entry_for(key);
			return(found_entry == data.end()) ? default_value : found_entry->second;
		}
		//添加Key和Value，找到则更新，没找到则插入
		void add_or_updata_mapping(Key const& key, Value const& value)
		{
			unique_lock<shared_mutex> lock(mutex);
			auto found_entry = find_entry_for(key);
			if (found_entry == data.end())//如果没有找到
			{
				data.push_back(bucket_value(key, value));
			}
			else
			{
				found_entry->second = value;
			}
		}
		void remove_mapping(Key const& key)
		{
			unique_lock<shared_mutex> lock(mutex);
			auto found_entry = find_entry_for(key);
			if (found_entry != data.end())//如果找到了
			{
				data.erase(found_entry);//把该迭代器删除
			}
		}
	};

private:
	//用vector存储桶的类型
	vector<unique_ptr<bucket_type>> buckets;
	//用一个HASH<key>哈希表 根据key来构造散列
	Hash hasher;

	bucket_type& get_bucket(Key const& key) const
	{
		std::size_t const bucket_index = hasher(key) % buckets.size();
		//利用哈希函数生成值
		return *(buckets[bucket_index]);//返回一个bucket_type对象
	}

public:
	threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher_ = Hash()):buckets(num_buckets), hasher(hasher_) {
		for (unsigned i = 0; i < num_buckets; i++) 
		{
			buckets[i].reset(new bucket_type);
		}
	};
	threadsafe_lookup_table(threadsafe_lookup_table const&other) = delete;
	threadsafe_lookup_table& operator=(threadsafe_lookup_table const& other) = delete;
	Value value_for(Key const& key,Value const& default_value=Value()) 
	{
		return get_bucket(key).value_for(key, default_value);
	}
	void add_or_update_mapping(Key const& key, Value const& value) 
	{
		return get_bucket(key).add_or_updata_mapping(key, value);
	}

	void remove_mapping(Key const& key) 
	{
		return get_bucket(key).remove_mapping(key);
	}

	//把所有的锁都加上
	std::map<Key, Value> get_map() 
	{
		vector<unique_lock<shared_mutex>> locks;
		for (unsigned i = 0; i < buckets.size(); i++) 
		{
			locks.push_back(unique_lock<shared_mutex>(buckets[i]->mutex));
			//统一创建右值
			//在将所有的锁全部锁上
		}

		map<Key, Value> res;
		for (unsigned i = 0; i < buckets.size(); i++) 
		{
			//typename bucket_type::bucket_iterator it = buckets[i]->data.begin();
			auto it = buckets[i]->data.begin();

			//两行意义相同


			for (; it != buckets[i]->data.end(); it++) 
			{
				res.insert(*it);
			}
		}

		return res;
	}

};