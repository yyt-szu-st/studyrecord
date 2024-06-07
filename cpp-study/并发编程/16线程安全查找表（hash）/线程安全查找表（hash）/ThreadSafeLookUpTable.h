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
		//�洢Ԫ��
	private:
		typedef std::pair<Key, Value> bucket_value;
		//��������ɴ洢�ṹ
		typedef std::list<bucket_value> bucket_data;
		//����ĵ�����
		typedef typename bucket_data::iterator bucket_iterator;
		//��������
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
		//����keyֵ���ҵ��򷵻ض�Ӧ��value��û�ҵ��ͷ����ṩ��Ĭ��ֵ
		//���Բ����ز��ң����ü���
		Value value_for(Key const& key, Value const& default_value)
		{
			std::shared_lock<std::mutex> lk(mutex);
			auto found_entry = find_entry_for(key);
			return(found_entry == data.end()) ? default_value : found_entry->second;
		}
		//���Key��Value���ҵ�����£�û�ҵ������
		void add_or_updata_mapping(Key const& key, Value const& value)
		{
			unique_lock<shared_mutex> lock(mutex);
			auto found_entry = find_entry_for(key);
			if (found_entry == data.end())//���û���ҵ�
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
			if (found_entry != data.end())//����ҵ���
			{
				data.erase(found_entry);//�Ѹõ�����ɾ��
			}
		}
	};

private:
	//��vector�洢Ͱ������
	vector<unique_ptr<bucket_type>> buckets;
	//��һ��HASH<key>��ϣ�� ����key������ɢ��
	Hash hasher;

	bucket_type& get_bucket(Key const& key) const
	{
		std::size_t const bucket_index = hasher(key) % buckets.size();
		//���ù�ϣ��������ֵ
		return *(buckets[bucket_index]);//����һ��bucket_type����
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

	//�����е���������
	std::map<Key, Value> get_map() 
	{
		vector<unique_lock<shared_mutex>> locks;
		for (unsigned i = 0; i < buckets.size(); i++) 
		{
			locks.push_back(unique_lock<shared_mutex>(buckets[i]->mutex));
			//ͳһ������ֵ
			//�ڽ����е���ȫ������
		}

		map<Key, Value> res;
		for (unsigned i = 0; i < buckets.size(); i++) 
		{
			//typename bucket_type::bucket_iterator it = buckets[i]->data.begin();
			auto it = buckets[i]->data.begin();

			//����������ͬ


			for (; it != buckets[i]->data.end(); it++) 
			{
				res.insert(*it);
			}
		}

		return res;
	}

};