#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <map>
#include <cstdio>
namespace sjtu {
	template <class Key, class Value, class Compare = std::less<Key> >
	class BTree {
	private:
		class BlockHead {
		public:
			bool blockT = false;
			int _size = 0;
			int _pos = 0;
			int _parent = 0;
			int _prev = 0;
			int _next = 0;
		};
		struct DataNode {
			int _child = 0;
			Key _key;
		};

		class FileHead {
		public:
			int block_cnt = 1;
			int root_pos = 0;
			int data_blockHead = 0;
			int data_block_rear = 0;
			int _size = 0;
		};
		const static int BLOCK_SIZE = 4096;
		const static int INIT_SIZE = sizeof(BlockHead);
		const static int KEY_SIZE = sizeof(Key);
		const static int VALUE_SIZE = sizeof(Value);
		const static int BLOCK_KEY_NUM = (BLOCK_SIZE - INIT_SIZE) / sizeof(DataNode) - 1;
		const static int BLOCK_PAIR_NUM = (BLOCK_SIZE - INIT_SIZE) / (KEY_SIZE + VALUE_SIZE) - 1;
		const char BPTREE_ADDRESS[12] = "sjtu";
		class NormalData {
		public:
			DataNode val[BLOCK_KEY_NUM];
		};
		class LeafData {
		public:
			pair<Key, Value> val[BLOCK_PAIR_NUM];
		};
		FileHead contentOfTree;
		static FILE* pointerOfFile;

		template <class MEM_TYPE>
		static void mem_read(MEM_TYPE buff, int buff_size, int pos) {
			fseek(pointerOfFile, long(buff_size * pos), SEEK_SET);
			fread(buff, buff_size, 1, pointerOfFile);
		}

		template <class MEM_TYPE>
		static void mem_write(MEM_TYPE buff, int buff_size, int pos) {
			fseek(pointerOfFile, long(buff_size * pos), SEEK_SET);
			fwrite(buff, buff_size, 1, pointerOfFile);
			fflush(pointerOfFile);
		}

		void write_contentOfTree() {
			fseek(pointerOfFile, 0, SEEK_SET);
			char buff[BLOCK_SIZE] = { 0 };
			memcpy(buff, &contentOfTree, sizeof(contentOfTree));
			mem_write(buff, BLOCK_SIZE, 0);
		}

		int memory_get() {
			++contentOfTree.block_cnt;
			write_contentOfTree();
			char buff[BLOCK_SIZE] = { 0 };
			mem_write(buff, BLOCK_SIZE, contentOfTree.block_cnt - 1);
			return contentOfTree.block_cnt - 1;
		}

		int create_normal_node(int _parent) {
			auto node_pos = memory_get();
			BlockHead temp;
			NormalData normalData;
			temp.blockT = false;
			temp._parent = _parent;
			temp._pos = node_pos;
			temp._size = 0;
			write_block(&temp, &normalData, node_pos);
			return node_pos;
		}

		int create_leaf_node(int _parent, int _prev, int _next) {
			auto node_pos = memory_get();
			BlockHead temp;
			LeafData leafData;
			temp.blockT = true;
			temp._parent = _parent;
			temp._pos = node_pos;
			temp._prev = _prev;
			temp._next = _next;
			temp._size = 0;
			write_block(&temp, &leafData, node_pos);
			return node_pos;
		}

		void insert_new_index(BlockHead & parent_info, NormalData & parent_data,
			int origin, int new_pos, const Key & new_index) {
			++parent_info._size;
			auto p = parent_info._size - 2;
			for (; parent_data.val[p]._child != origin; --p) {
				parent_data.val[p + 1] = parent_data.val[p];
			}
			parent_data.val[p + 1]._key = parent_data.val[p]._key;
			parent_data.val[p]._key = new_index;
			parent_data.val[p + 1]._child = new_pos;
		}

		template <class DATA_TYPE>
		static void write_block(BlockHead * _info, DATA_TYPE * _data, int _pos) {
			char buff[BLOCK_SIZE] = { 0 };
			memcpy(buff, _info, sizeof(BlockHead));
			memcpy(buff + INIT_SIZE, _data, sizeof(DATA_TYPE));
			mem_write(buff, BLOCK_SIZE, _pos);
		}

		template <class DATA_TYPE>
		static void read_block(BlockHead * _info, DATA_TYPE * _data, int _pos) {
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, _pos);
			memcpy(_info, buff, sizeof(BlockHead));
			memcpy(_data, buff + INIT_SIZE, sizeof(DATA_TYPE));
		}

		Key split_leaf_node(int pos, BlockHead & origin_info, LeafData & origin_data) {
			int parent_pos;
			BlockHead parent_info;
			NormalData parent_data;

			if (pos == contentOfTree.root_pos) {
				auto root_pos = create_normal_node(0);
				contentOfTree.root_pos = root_pos;
				write_contentOfTree();
				read_block(&parent_info, &parent_data, root_pos);
				origin_info._parent = root_pos;
				++parent_info._size;
				parent_data.val[0]._child = pos;
				parent_pos = root_pos;
			}
			else {
				read_block(&parent_info, &parent_data, origin_info._parent);
				parent_pos = parent_info._pos;
			}
			if (check_parent(origin_info)) {
				parent_pos = origin_info._parent;
				read_block(&parent_info, &parent_data, parent_pos);
			}
			auto new_pos = create_leaf_node(parent_pos, pos, origin_info._next);

			auto temp_pos = origin_info._next;
			BlockHead temp_info;
			LeafData temp_data;
			read_block(&temp_info, &temp_data, temp_pos);
			temp_info._prev = new_pos;
			write_block(&temp_info, &temp_data, temp_pos);
			origin_info._next = new_pos;

			BlockHead new_info;
			LeafData new_data;
			read_block(&new_info, &new_data, new_pos);

			int mid_pos = origin_info._size >> 1;
			for (int p = mid_pos, i = 0; p < origin_info._size; ++p, ++i) {
				new_data.val[i].first = origin_data.val[p].first;
				new_data.val[i].second = origin_data.val[p].second;
				++new_info._size;
			}
			origin_info._size = mid_pos;
			insert_new_index(parent_info, parent_data, pos, new_pos, origin_data.val[mid_pos].first);

			write_block(&origin_info, &origin_data, pos);
			write_block(&new_info, &new_data, new_pos);
			write_block(&parent_info, &parent_data, parent_pos);

			return new_data.val[0].first;
		}

		bool check_parent(BlockHead & child_info) {
			int parent_pos, origin_pos = child_info._parent;
			BlockHead parent_info, origin_info;
			NormalData parent_data, origin_data;
			read_block(&origin_info, &origin_data, origin_pos);
			if (origin_info._size < BLOCK_KEY_NUM)
				return false;

			if (origin_pos == contentOfTree.root_pos) {
				auto root_pos = create_normal_node(0);
				contentOfTree.root_pos = root_pos;
				write_contentOfTree();
				read_block(&parent_info, &parent_data, root_pos);
				origin_info._parent = root_pos;
				++parent_info._size;
				parent_data.val[0]._child = origin_pos;
				parent_pos = root_pos;
			}
			else {
				read_block(&parent_info, &parent_data, origin_info._parent);
				parent_pos = parent_info._pos;
			}
			if (check_parent(origin_info)) {
				parent_pos = origin_info._parent;
				read_block(&parent_info, &parent_data, parent_pos);
			}
			auto new_pos = create_normal_node(parent_pos);
			BlockHead new_info;
			NormalData new_data;
			read_block(&new_info, &new_data, new_pos);

			int mid_pos = origin_info._size >> 1;
			for (int p = mid_pos + 1, i = 0; p < origin_info._size; ++p, ++i) {
				if (origin_data.val[p]._child == child_info._pos) {
					child_info._parent = new_pos;
				}
				std::swap(new_data.val[i], origin_data.val[p]);
				++new_info._size;
			}
			origin_info._size = mid_pos + 1;
			insert_new_index(parent_info, parent_data, origin_pos, new_pos, origin_data.val[mid_pos]._key);

			write_block(&origin_info, &origin_data, origin_pos);
			write_block(&new_info, &new_data, new_pos);
			write_block(&parent_info, &parent_data, parent_pos);
			return true;
		}

		void change_index(int l_parent, int l_child, const Key & new_key) {
			BlockHead parent_info;
			NormalData parent_data;
			read_block(&parent_info, &parent_data, l_parent);
			if (parent_data.val[parent_info._size - 1]._child == l_child) {
				change_index(parent_info._parent, l_parent, new_key);
				return;
			}
			for (int cur_pos = parent_info._size - 2;; --cur_pos) {
				if (parent_data.val[cur_pos]._child == l_child) {
					parent_data.val[cur_pos]._key = new_key;
					break;
				}
			}
			write_block(&parent_info, &parent_data, l_parent);
		}

		void merge_normal(BlockHead & l_info, NormalData & l_data, BlockHead & r_info, NormalData & r_data) {
			for (int p = l_info._size, i = 0; i < r_info._size; ++p, ++i) {
				l_data.val[p] = r_data.val[i];
			}
			l_data.val[l_info._size - 1]._key = adjust_normal(r_info._parent, r_info._pos);
			l_info._size += r_info._size;
			write_block(&l_info, &l_data, l_info._pos);
		}

		void balance_normal(BlockHead & info, NormalData & normalData) {
			if (info._size >= BLOCK_KEY_NUM / 2) {
				write_block(&info, &normalData, info._pos);
				return;
			}
			if (info._pos == contentOfTree.root_pos && info._size <= 1) {
				contentOfTree.root_pos = normalData.val[0]._child;
				write_contentOfTree();
				return;
			}
			else if (info._pos == contentOfTree.root_pos) {
				write_block(&info, &normalData, info._pos);
				return;
			}

			BlockHead parent_info, brother_info;
			NormalData parent_data, brother_data;
			read_block(&parent_info, &parent_data, info._parent);
			int value_pos;
			for (value_pos = 0; parent_data.val[value_pos]._child != info._pos; ++value_pos);
			if (value_pos > 0) {
				read_block(&brother_info, &brother_data, parent_data.val[value_pos - 1]._child);
				brother_info._parent = info._parent;
				if (brother_info._size > BLOCK_KEY_NUM / 2) {
					for (int p = info._size; p > 0; --p) {
						normalData.val[p] = normalData.val[p - 1];
					}
					normalData.val[0]._child = brother_data.val[brother_info._size - 1]._child;
					normalData.val[0]._key = parent_data.val[value_pos - 1]._key;
					parent_data.val[value_pos - 1]._key = brother_data.val[brother_info._size - 2]._key;
					--brother_info._size;
					++info._size;
					write_block(&brother_info, &brother_data, brother_info._pos);
					write_block(&info, &normalData, info._pos);
					write_block(&parent_info, &parent_data, parent_info._pos);
					return;
				}
				else {
					merge_normal(brother_info, brother_data, info, normalData);
					return;
				}
			}
			if (value_pos < parent_info._size - 1) {
				read_block(&brother_info, &brother_data, parent_data.val[value_pos + 1]._child);
				brother_info._parent = info._parent;
				if (brother_info._size > BLOCK_KEY_NUM / 2) {
					normalData.val[info._size]._child = brother_data.val[0]._child;
					normalData.val[info._size - 1]._key = parent_data.val[value_pos]._key;
					parent_data.val[value_pos]._key = brother_data.val[0]._key;
					for (int p = 1; p < brother_info._size; ++p) {
						brother_data.val[p - 1] = brother_data.val[p];
					}
					--brother_info._size;
					++info._size;
					write_block(&brother_info, &brother_data, brother_info._pos);
					write_block(&info, &normalData, info._pos);
					write_block(&parent_info, &parent_data, parent_info._pos);
					return;
				}
				else {
					merge_normal(info, normalData, brother_info, brother_data);
					return;
				}
			}
		}

		Key adjust_normal(int pos, int removed_child) {
			BlockHead info;
			NormalData normalData;
			read_block(&info, &normalData, pos);
			int cur_pos;
			for (cur_pos = 0; normalData.val[cur_pos]._child != removed_child; ++cur_pos);
			Key ans = normalData.val[cur_pos - 1]._key;
			normalData.val[cur_pos - 1]._key = normalData.val[cur_pos]._key;
			for (; cur_pos < info._size - 1; ++cur_pos) {
				normalData.val[cur_pos] = normalData.val[cur_pos + 1];
			}
			--info._size;
			balance_normal(info, normalData);
			return ans;
		}

		void merge_leaf(BlockHead & l_info, LeafData & l_data, BlockHead & r_info, LeafData & r_data) {
			for (int p = l_info._size, i = 0; i < r_info._size; ++p, ++i) {
				l_data.val[p].first = r_data.val[i].first;
				l_data.val[p].second = r_data.val[i].second;
			}
			l_info._size += r_info._size;
			adjust_normal(r_info._parent, r_info._pos);
			l_info._next = r_info._next;
			BlockHead temp_info;
			LeafData temp_data;
			read_block(&temp_info, &temp_data, r_info._next);
			temp_info._prev = l_info._pos;
			write_block(&temp_info, &temp_data, temp_info._pos);
			write_block(&l_info, &l_data, l_info._pos);
		}

		void balance_leaf(BlockHead & leaf_info, LeafData & leafData) {
			if (leaf_info._size >= BLOCK_PAIR_NUM / 2) {
				write_block(&leaf_info, &leafData, leaf_info._pos);
				return;
			}
			else if (leaf_info._pos == contentOfTree.root_pos) {
				if (leaf_info._size == 0) {
					BlockHead temp_info;
					LeafData temp_data;
					read_block(&temp_info, &temp_data, contentOfTree.data_blockHead);
					temp_info._next = contentOfTree.data_block_rear;
					write_block(&temp_info, &temp_data, contentOfTree.data_blockHead);
					read_block(&temp_info, &temp_data, contentOfTree.data_block_rear);
					temp_info._prev = contentOfTree.data_blockHead;
					write_block(&temp_info, &temp_data, contentOfTree.data_block_rear);
					return;
				}
				write_block(&leaf_info, &leafData, leaf_info._pos);
				return;
			}

			BlockHead brother_info, parent_info;
			LeafData brother_data;
			NormalData parent_data;

			read_block(&parent_info, &parent_data, leaf_info._parent);
			int node_pos = 0;
			for (; node_pos < parent_info._size; ++node_pos) {
				if (parent_data.val[node_pos]._child == leaf_info._pos)
					break;
			}

			if (node_pos > 0) {
				read_block(&brother_info, &brother_data, leaf_info._prev);
				brother_info._parent = leaf_info._parent;
				if (brother_info._size > BLOCK_PAIR_NUM / 2) {
					for (int p = leaf_info._size; p > 0; --p) {
						leafData.val[p].first = leafData.val[p - 1].first;
						leafData.val[p].second = leafData.val[p - 1].second;
					}
					leafData.val[0].first = brother_data.val[brother_info._size - 1].first;
					leafData.val[0].second = brother_data.val[brother_info._size - 1].second;
					--brother_info._size;
					++leaf_info._size;
					change_index(brother_info._parent, brother_info._pos, leafData.val[0].first);
					write_block(&brother_info, &brother_data, brother_info._pos);
					write_block(&leaf_info, &leafData, leaf_info._pos);
					return;
				}
				else {
					merge_leaf(brother_info, brother_data, leaf_info, leafData);
					return;
				}
			}

			if (node_pos < parent_info._size - 1) {
				read_block(&brother_info, &brother_data, leaf_info._next);
				brother_info._parent = leaf_info._parent;
				if (brother_info._size > BLOCK_PAIR_NUM / 2) {
					leafData.val[leaf_info._size].first = brother_data.val[0].first;
					leafData.val[leaf_info._size].second = brother_data.val[0].second;
					for (int p = 1; p < brother_info._size; ++p) {
						brother_data.val[p - 1].first = brother_data.val[p].first;
						brother_data.val[p - 1].second = brother_data.val[p].second;
					}
					++leaf_info._size;
					--brother_info._size;
					change_index(leaf_info._parent, leaf_info._pos, brother_data.val[0].first);
					write_block(&leaf_info, &leafData, leaf_info._pos);
					write_block(&brother_info, &brother_data, brother_info._pos);
					return;
				}
				else {
					merge_leaf(leaf_info, leafData, brother_info, brother_data);

					return;
				}
			}
		}

		void check_file() {
			if (!pointerOfFile) {
				pointerOfFile = fopen(BPTREE_ADDRESS, "wb+");
				write_contentOfTree();

				auto node_head = contentOfTree.block_cnt,
					node_rear = contentOfTree.block_cnt + 1;

				contentOfTree.data_blockHead = node_head;
				contentOfTree.data_block_rear = node_rear;

				create_leaf_node(0, 0, node_rear);
				create_leaf_node(0, node_head, 0);

				return;
			}
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, 0);
			memcpy(&contentOfTree, buff, sizeof(contentOfTree));
		}
	public:
		typedef pair<const Key, Value> value_type;

		class const_iterator;
		class iterator {
			friend class sjtu::BTree<Key, Value, Compare>::const_iterator;
			friend iterator sjtu::BTree<Key, Value, Compare>::begin();
			friend iterator sjtu::BTree<Key, Value, Compare>::end();
			friend iterator sjtu::BTree<Key, Value, Compare>::find(const Key&);
			friend pair<iterator, OperationResult> sjtu::BTree<Key, Value, Compare>::insert(const Key&, const Value&);
		private:
			BTree* cur_bptree = nullptr;
			BlockHead block_info;
			int cur_pos = 0;

		public:
			bool modify(const Value& value) {
				BlockHead info;
				LeafData leafData;
				read_block(&info, &leafData, block_info._pos);
				leafData.val[cur_pos].second = value;
				write_block(&info, &leafData, block_info._pos);
				return true;
			}
			iterator() {
		
			}
			iterator(const iterator& other) {
		
				cur_bptree = other.cur_bptree;
				block_info = other.block_info;
				cur_pos = other.cur_pos;
			}
	
			iterator operator++(int) {
	
				auto temp = *this;
				++cur_pos;
				if (cur_pos >= block_info._size) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._next);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return temp;
			}
			iterator& operator++() {
			
				++cur_pos;
				if (cur_pos >= block_info._size) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._next);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return *this;
			}
			iterator operator--(int) {
			
				auto temp = *this;
				if (cur_pos == 0) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._prev);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info._size - 1;
				}
				else
					--cur_pos;
				return temp;
			}
			iterator& operator--() {
	
				if (cur_pos == 0) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._prev);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info._size - 1;
				}
				else
					--cur_pos;

				return *this;
			}
	
			value_type operator*() const {

				if (cur_pos >= block_info._size)
					throw invalid_iterator();
				char buff[BLOCK_SIZE] = { 0 };
				mem_read(buff, BLOCK_SIZE, block_info._pos);
				LeafData leafData;
				memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
				value_type result(leafData.val[cur_pos].first, leafData.val[cur_pos].second);
				return result;
			}
			bool operator==(const iterator & rhs) const {

				return cur_bptree == rhs.cur_bptree
					&& block_info._pos == rhs.block_info._pos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator==(const const_iterator & rhs) const {
	
				return block_info._pos == rhs.block_info._pos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator!=(const iterator & rhs) const {

				return cur_bptree != rhs.cur_bptree|| block_info._pos != rhs.block_info._pos|| cur_pos != rhs.cur_pos;
			}
			bool operator!=(const const_iterator & rhs) const {
			
				return block_info._pos != rhs.block_info._pos
					|| cur_pos != rhs.cur_pos;
			}
		};
		class const_iterator {
		
			friend class sjtu::BTree<Key, Value, Compare>::iterator;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::cbegin() const;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::cend() const;
			friend const_iterator sjtu::BTree<Key, Value, Compare>::find(const Key&) const;
		private:
		
			BlockHead block_info;
			int cur_pos = 0;
		public:
			const_iterator() {
			
			}
			const_iterator(const const_iterator& other) {
		
				block_info = other.block_info;
				cur_pos = other.cur_pos;
			}
			const_iterator(const iterator& other) {
			
				block_info = other.block_info;
				cur_pos = other.cur_pos;
			}
		
			const_iterator operator++(int) {
			
				auto temp = *this;
				++cur_pos;
				if (cur_pos >= block_info._size) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._next);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return temp;
			}
			const_iterator& operator++() {
		
				++cur_pos;
				if (cur_pos >= block_info._size) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._next);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = 0;
				}
				return *this;
			}
			const_iterator operator--(int) {
			
				auto temp = *this;
				if (cur_pos == 0) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._prev);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info._size - 1;
				}
				else
					--cur_pos;
				return temp;
			}
			const_iterator& operator--() {
	
				if (cur_pos == 0) {
					char buff[BLOCK_SIZE] = { 0 };
					mem_read(buff, BLOCK_SIZE, block_info._prev);
					memcpy(&block_info, buff, sizeof(block_info));
					cur_pos = block_info._size - 1;
				}
				else
					--cur_pos;

				return *this;
			}
	
			value_type operator*() const {
		
				if (cur_pos >= block_info._size)
					throw invalid_iterator();
				char buff[BLOCK_SIZE] = { 0 };
				mem_read(buff, BLOCK_SIZE, block_info._pos);
				LeafData leafData;
				memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
				value_type result(leafData.val[cur_pos].first, leafData.val[cur_pos].second);
				return result;
			}
			bool operator==(const iterator & rhs) const {
	
				return block_info._pos == rhs.block_info._pos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator==(const const_iterator & rhs) const {

				return block_info._pos == rhs.block_info._pos
					&& cur_pos == rhs.cur_pos;
			}
			bool operator!=(const iterator & rhs) const {

				return block_info._pos != rhs.block_info._pos
					|| cur_pos != rhs.cur_pos;
			}
			bool operator!=(const const_iterator & rhs) const {
	
				return block_info._pos != rhs.block_info._pos
					|| cur_pos != rhs.cur_pos;
			}
		};
		BTree() {
			pointerOfFile = fopen(BPTREE_ADDRESS, "rb+");
			if (!pointerOfFile) {
				pointerOfFile = fopen(BPTREE_ADDRESS, "wb+");
				write_contentOfTree();

				auto node_head = contentOfTree.block_cnt,
					node_rear = contentOfTree.block_cnt + 1;

				contentOfTree.data_blockHead = node_head;
				contentOfTree.data_block_rear = node_rear;

				create_leaf_node(0, 0, node_rear);
				create_leaf_node(0, node_head, 0);

				return;
			}
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, 0);
			memcpy(&contentOfTree, buff, sizeof(contentOfTree));
		}
		BTree(const BTree& other) {
		
			pointerOfFile = fopen(BPTREE_ADDRESS, "rb+");
			contentOfTree.block_cnt = other.contentOfTree.block_cnt;
			contentOfTree.data_blockHead = other.contentOfTree.data_blockHead;
			contentOfTree.data_block_rear = other.contentOfTree.data_block_rear;
			contentOfTree.root_pos = other.contentOfTree.root_pos;
			contentOfTree._size = other.contentOfTree._size;
		}
		BTree& operator=(const BTree& other) {
		
			pointerOfFile = fopen(BPTREE_ADDRESS, "rb+");
			contentOfTree.block_cnt = other.contentOfTree.block_cnt;
			contentOfTree.data_blockHead = other.contentOfTree.data_blockHead;
			contentOfTree.data_block_rear = other.contentOfTree.data_block_rear;
			contentOfTree.root_pos = other.contentOfTree.root_pos;
			contentOfTree._size = other.contentOfTree._size;
			return *this;
		}
		~BTree() {
		
			fclose(pointerOfFile);
		}
	
		pair<iterator, OperationResult> insert(const Key& key, const Value& value) {
	
			check_file();
			if (empty()) {
				auto root_pos = create_leaf_node(0, contentOfTree.data_blockHead, contentOfTree.data_block_rear);

				BlockHead temp_info;
				LeafData temp_data;
				read_block(&temp_info, &temp_data, contentOfTree.data_blockHead);
				temp_info._next = root_pos;
				write_block(&temp_info, &temp_data, contentOfTree.data_blockHead);

				read_block(&temp_info, &temp_data, contentOfTree.data_block_rear);
				temp_info._prev = root_pos;
				write_block(&temp_info, &temp_data, contentOfTree.data_block_rear);

				read_block(&temp_info, &temp_data, root_pos);
				++temp_info._size;
				temp_data.val[0].first = key;
				temp_data.val[0].second = value;
				write_block(&temp_info, &temp_data, root_pos);

				++contentOfTree._size;
				contentOfTree.root_pos = root_pos;
				write_contentOfTree();

				pair<iterator, OperationResult> result(begin(), Success);
				return result;
			}

			char buff[BLOCK_SIZE] = { 0 };
			int cur_pos = contentOfTree.root_pos, cur_parent = 0;
			while (true) {
				mem_read(buff, BLOCK_SIZE, cur_pos);
				BlockHead temp;
				memcpy(&temp, buff, sizeof(temp));
				if (cur_parent != temp._parent) {
					temp._parent = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					mem_write(buff, BLOCK_SIZE, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				NormalData normalData;
				memcpy(&normalData, buff + INIT_SIZE, sizeof(normalData));
				int child_pos = temp._size - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1]._key > key)) {
						break;
					}
				}
				cur_parent = cur_pos;
				cur_pos = normalData.val[child_pos]._child;
			}

			BlockHead info;
			memcpy(&info, buff, sizeof(info));
			LeafData leafData;
			memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info._size && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
			
					return pair<iterator, OperationResult>(end(), Fail);
				}
				if (value_pos >= info._size || leafData.val[value_pos].first > key) {
					if (info._size >= BLOCK_PAIR_NUM) {
						auto cur_key = split_leaf_node(cur_pos, info, leafData);
						if (key > cur_key) {
							cur_pos = info._next;
							value_pos -= info._size;
							read_block(&info, &leafData, cur_pos);
						}
					}

					for (int p = info._size - 1; p >= value_pos; --p) {
						leafData.val[p + 1].first = leafData.val[p].first;
						leafData.val[p + 1].second = leafData.val[p].second;
						if (p == value_pos)
							break;
					}
					leafData.val[value_pos].first = key;
					leafData.val[value_pos].second = value;
					++info._size;
					write_block(&info, &leafData, cur_pos);
					iterator ans;
					ans.block_info = info;
					ans.cur_bptree = this;
					ans.cur_pos = value_pos;
					++contentOfTree._size;
					write_contentOfTree();
					pair<iterator, OperationResult> to_return(ans, Success);
					return to_return;
				}
			}
			return pair<iterator, OperationResult>(end(), Fail);
		}

		OperationResult erase(const Key & key) {
			check_file();

			if (empty()) {
				return Fail;
			}
			char buff[BLOCK_SIZE] = { 0 };
			int cur_pos = contentOfTree.root_pos, cur_parent = 0;
			while (true) {
				mem_read(buff, BLOCK_SIZE, cur_pos);
				BlockHead temp;
				memcpy(&temp, buff, sizeof(temp));
				if (cur_parent != temp._parent) {
					temp._parent = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					mem_write(buff, BLOCK_SIZE, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				NormalData normalData;
				memcpy(&normalData, buff + INIT_SIZE, sizeof(normalData));
				int child_pos = temp._size - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1]._key > key)) {
						break;
					}
				}
				cur_parent = cur_pos;
				cur_pos = normalData.val[child_pos]._child;
			}

			BlockHead info;
			memcpy(&info, buff, sizeof(info));
			LeafData leafData;
			memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info._size && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					--info._size;
					for (int p = value_pos; p < info._size; ++p) {
						leafData.val[p].first = leafData.val[p + 1].first;
						leafData.val[p].second = leafData.val[p + 1].second;
					}
					balance_leaf(info, leafData);
					--contentOfTree._size;
					write_contentOfTree();
					return Success;
				}
				if (value_pos >= info._size || leafData.val[value_pos].first > key) {
					return Fail;
				}
			}
			return Fail;
		}
		iterator begin() {
			check_file();
			iterator result;
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, contentOfTree.data_blockHead);
			BlockHead blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_bptree = this;
			result.cur_pos = 0;
			++result;
			return result;
		}
		const_iterator cbegin() const {
			const_iterator result;
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, contentOfTree.data_blockHead);
			BlockHead blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_pos = 0;
			++result;
			return result;
		}

		iterator end() {
			check_file();
			iterator result;
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, contentOfTree.data_block_rear);
			BlockHead blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_bptree = this;
			result.cur_pos = 0;
			return result;
		}
		const_iterator cend() const {
			const_iterator result;
			char buff[BLOCK_SIZE] = { 0 };
			mem_read(buff, BLOCK_SIZE, contentOfTree.data_block_rear);
			BlockHead blockHead;
			memcpy(&blockHead, buff, sizeof(blockHead));
			result.block_info = blockHead;
			result.cur_pos = 0;
			return result;
		}
	
		bool empty() const {
			if (!pointerOfFile)
				return true;
			return contentOfTree._size == 0;
		}
	
		int size() const {
			if (!pointerOfFile)
				return 0;
			return contentOfTree._size;
		}
	
		void clear() {
			if (!pointerOfFile)
				return;
			remove(BPTREE_ADDRESS);
			FileHead new_fileHead;
			contentOfTree = new_fileHead;
			pointerOfFile = nullptr;
		}
	
		Value at(const Key & key) {
			if (empty()) {
				throw container_is_empty();
			}

			char buff[BLOCK_SIZE] = { 0 };
			int cur_pos = contentOfTree.root_pos, cur_parent = 0;
			while (true) {
				mem_read(buff, BLOCK_SIZE, cur_pos);
				BlockHead temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp._parent) {
					temp._parent = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					mem_write(buff, BLOCK_SIZE, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				NormalData normalData;
				memcpy(&normalData, buff + INIT_SIZE, sizeof(normalData));
				int child_pos = temp._size - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1]._key > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos]._child;
			}
			BlockHead info;
			memcpy(&info, buff, sizeof(info));
			LeafData leafData;
			memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info._size && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					return leafData.val[value_pos].second;
				}
				if (value_pos >= info._size || leafData.val[value_pos].first > key) {
					throw index_out_of_bound();
				}
			}
		}
	
		int count(const Key & key) const {
			return find(key) == cend() ? 0 : 1;
		}
	
		iterator find(const Key & key) {
			if (empty()) {
				return end();
			}

			char buff[BLOCK_SIZE] = { 0 };
			int cur_pos = contentOfTree.root_pos, cur_parent = 0;
			while (true) {
				mem_read(buff, BLOCK_SIZE, cur_pos);
				BlockHead temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp._parent) {
					temp._parent = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					mem_write(buff, BLOCK_SIZE, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				NormalData normalData;
				memcpy(&normalData, buff + INIT_SIZE, sizeof(normalData));
				int child_pos = temp._size - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1]._key > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos]._child;
			}
			BlockHead info;
			memcpy(&info, buff, sizeof(info));
			sizeof(NormalData);
			LeafData leafData;
			memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info._size && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					iterator result;
					result.cur_bptree = this;
					result.block_info = info;
					result.cur_pos = value_pos;
					return result;
				}
				if (value_pos >= info._size || leafData.val[value_pos].first > key) {
					return end();
				}
			}
			return end();
		}
		const_iterator find(const Key & key) const {
			if (empty()) {
				return cend();
			}

			char buff[BLOCK_SIZE] = { 0 };
			int cur_pos = contentOfTree.root_pos, cur_parent = 0;
			while (true) {
				mem_read(buff, BLOCK_SIZE, cur_pos);
				BlockHead temp;
				memcpy(&temp, buff, sizeof(temp));

				if (cur_parent != temp._parent) {
					temp._parent = cur_parent;
					memcpy(buff, &temp, sizeof(temp));
					mem_write(buff, BLOCK_SIZE, cur_pos);
				}
				if (temp.blockT) {
					break;
				}
				NormalData normalData;
				memcpy(&normalData, buff + INIT_SIZE, sizeof(normalData));
				int child_pos = temp._size - 1;
				for (; child_pos > 0; --child_pos) {
					if (!(normalData.val[child_pos - 1]._key > key)) {
						break;
					}
				}
				cur_pos = normalData.val[child_pos]._child;
			}
			BlockHead info;
			memcpy(&info, buff, sizeof(info));
			LeafData leafData;
			memcpy(&leafData, buff + INIT_SIZE, sizeof(leafData));
			for (int value_pos = 0;; ++value_pos) {
				if (value_pos < info._size && (!(leafData.val[value_pos].first<key || leafData.val[value_pos].first>key))) {
					const_iterator result;
					result.block_info = info;
					result.cur_pos = value_pos;
					return result;
				}
				if (value_pos >= info._size || leafData.val[value_pos].first > key) {
					return cend();
				}
			}
			return cend();
		}
	};
	template <typename Key, typename Value, typename Compare> FILE* BTree<Key, Value, Compare>::pointerOfFile = nullptr;
} 
